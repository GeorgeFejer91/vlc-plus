//! C ABI between VLC's Qt GUI thread and Flubber's independent LSL and CSV clocks.
//! All caller strings are copied before return; no VLC, Qt, or HWND pointer is retained.

use labstream::{Channel, Format, Outlet, StreamInfo};
use std::cell::Cell;
use std::ffi::{c_char, c_int, c_void, CStr};
use std::fs::{File, OpenOptions};
use std::io::{BufWriter, Write};
use std::path::{Path, PathBuf};
use std::sync::{
    atomic::{AtomicU64, AtomicU8, Ordering},
    mpsc, Arc, Mutex,
};
use std::thread::{self, JoinHandle};
use std::time::{Duration, Instant, SystemTime, UNIX_EPOCH};
use windows_sys::Win32::Foundation::{CloseHandle, HANDLE, WAIT_OBJECT_0};
use windows_sys::Win32::Media::{timeBeginPeriod, timeEndPeriod};
use windows_sys::Win32::System::Threading::{
    CreateWaitableTimerExW, SetWaitableTimer, WaitForSingleObject,
    CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS,
};

const AFFECT_HZ: u32 = 480;
const AFFECT_PERIOD: Duration = Duration::from_nanos(1_000_000_000 / AFFECT_HZ as u64);
const RECORD_PERIOD: Duration = Duration::from_nanos(33_333_333);
const ACTION_QUEUE_CAPACITY: usize = 1024;

struct Action {
    stamp: f64,
    json: String,
}

struct Markers {
    outlet: Outlet,
    latest: Cell<f64>,
}

impl Markers {
    fn push_text_at(&self, text: &str, stamp: f64) -> Result<(), String> {
        // Lifecycle markers can be backdated to video onset. Keep the shared
        // stream ordered when an interaction has already been published.
        let stamp = stamp.max(self.latest.get());
        self.outlet
            .push_text_at(text, stamp)
            .map_err(|error| error.to_string())?;
        self.latest.set(stamp);
        Ok(())
    }

    fn action(&self, action: Action) -> Result<(), String> {
        self.push_text_at(&format!("VLC_Action {}", action.json), action.stamp)
    }
}

#[derive(Default)]
struct AffectPair(AtomicU64);

impl AffectPair {
    fn store(&self, x: f32, y: f32) {
        let x = x.clamp(-1., 1.).to_bits();
        let y = y.clamp(-1., 1.).to_bits();
        self.0
            .store((u64::from(x) << 32) | u64::from(y), Ordering::Relaxed);
    }

    fn load(&self) -> [f32; 2] {
        let bits = self.0.load(Ordering::Relaxed);
        [
            f32::from_bits((bits >> 32) as u32),
            f32::from_bits(bits as u32),
        ]
    }
}

fn stream_source(process_id: u32, session: Option<&str>) -> Result<String, String> {
    let base = format!("vlc-flubber-{process_id}");
    match session {
        None => Ok(base),
        Some(token)
            if !token.is_empty()
                && token.len() <= 64
                && token
                    .bytes()
                    .all(|byte| byte.is_ascii_alphanumeric() || byte == b'-' || byte == b'_') =>
        {
            Ok(format!("{base}-{token}"))
        }
        Some(_) => Err("Invalid VLC_FLUBBER_SESSION_ID.".into()),
    }
}

#[derive(Clone)]
struct Snapshot {
    state: i32,
    media_ms: i64,
    name: String,
    path: Option<PathBuf>,
    interrupts: u64,
}
impl Default for Snapshot {
    fn default() -> Self {
        Self {
            state: 0,
            media_ms: 0,
            name: String::new(),
            path: None,
            interrupts: 0,
        }
    }
}

struct WorkerTimer(HANDLE);
impl WorkerTimer {
    fn new() -> Result<Self, &'static str> {
        // SAFETY: An unnamed, non-inheritable timer belongs only to this worker.
        let timer = unsafe {
            CreateWaitableTimerExW(
                std::ptr::null(),
                std::ptr::null(),
                CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
                TIMER_ALL_ACCESS,
            )
        };
        if timer.is_null() {
            Err("high-resolution timer creation failed")
        } else {
            Ok(Self(timer))
        }
    }
    fn wait_until(&self, target: Instant) -> Result<(), &'static str> {
        let remaining = target.saturating_duration_since(Instant::now());
        if remaining.is_zero() {
            return Ok(());
        }
        let due = -((remaining.as_nanos() / 100).max(1).min(i64::MAX as u128) as i64);
        // SAFETY: This worker owns the live timer and supplies a relative 100 ns due time.
        if unsafe { SetWaitableTimer(self.0, &due, 0, None, std::ptr::null(), 0) } == 0
            || unsafe { WaitForSingleObject(self.0, u32::MAX) } != WAIT_OBJECT_0
        {
            return Err("high-resolution timer wait failed");
        }
        Ok(())
    }
}
impl Drop for WorkerTimer {
    fn drop(&mut self) {
        // SAFETY: Drop occurs after this worker's final wait.
        unsafe { CloseHandle(self.0) };
    }
}

struct TimerPeriod;
impl TimerPeriod {
    fn new() -> Result<Self, &'static str> {
        // SAFETY: Paired by Drop and kept alive throughout the worker loop.
        if unsafe { timeBeginPeriod(1) } == 0 {
            Ok(Self)
        } else {
            Err("1 ms timer period unavailable")
        }
    }
}
impl Drop for TimerPeriod {
    fn drop(&mut self) {
        // SAFETY: Pairs exactly one successful timeBeginPeriod.
        unsafe { timeEndPeriod(1) };
    }
}

struct Session {
    name: String,
    path: Option<PathBuf>,
    origin: f64,
    paused_at: Option<f64>,
    last_state: i32,
}
impl Session {
    fn start(
        snapshot: &Snapshot,
        stamp: f64,
        markers: &Markers,
        csv: &mpsc::SyncSender<CsvCommand>,
    ) -> Result<Self, String> {
        let name = if snapshot.name.is_empty() {
            "video"
        } else {
            &snapshot.name
        };
        let origin = stamp - snapshot.media_ms as f64 / 1000.;
        if let Some(path) = &snapshot.path {
            csv.try_send(CsvCommand::Open(path.clone()))
                .map_err(|e| e.to_string())?;
        }
        markers.push_text_at(&format!("{name}_Start"), origin)?;
        Ok(Self {
            name: name.into(),
            path: snapshot.path.clone(),
            origin,
            paused_at: None,
            last_state: snapshot.state,
        })
    }
    fn marker(&self, markers: &Markers, suffix: &str, stamp: f64) -> Result<(), String> {
        markers.push_text_at(&format!("{}_{}", self.name, suffix), stamp)
    }
    fn sample(
        &mut self,
        snapshot: &Snapshot,
        pair: [f32; 2],
        stamp: f64,
        markers: &Markers,
        csv: &mpsc::SyncSender<CsvCommand>,
    ) -> Result<(), String> {
        if snapshot.state != self.last_state {
            match snapshot.state {
                4 => self.marker(markers, "Pause", stamp)?,
                3 if self.last_state == 4 => self.marker(markers, "Resume", stamp)?,
                2 => self.marker(markers, "BufferingStart", stamp)?,
                3 if self.last_state == 2 => self.marker(markers, "BufferingEnd", stamp)?,
                _ => {}
            }
            self.last_state = snapshot.state;
        }
        if snapshot.state != 3 {
            self.paused_at.get_or_insert(stamp);
            return Ok(());
        }
        if let Some(started) = self.paused_at.take() {
            self.origin += stamp - started;
        }
        if self.path.is_some() {
            csv.try_send(CsvCommand::Row(
                (stamp - self.origin).max(0.),
                pair[0],
                pair[1],
            ))
            .map_err(|e| e.to_string())?;
        }
        Ok(())
    }
    fn stop(
        self,
        markers: &Markers,
        stamp: f64,
        suffix: Option<&str>,
        csv: &mpsc::SyncSender<CsvCommand>,
    ) -> Result<(), String> {
        if let Some(suffix) = suffix {
            self.marker(markers, suffix, stamp)?;
        }
        self.marker(markers, "Stop", stamp)?;
        if self.path.is_some() {
            csv.try_send(CsvCommand::Close).map_err(|e| e.to_string())?;
        }
        Ok(())
    }
}

enum CsvCommand {
    Open(PathBuf),
    Row(f64, f32, f32),
    Close,
}
fn csv_writer(commands: mpsc::Receiver<CsvCommand>) -> Result<(), String> {
    let mut recording: Option<BufWriter<File>> = None;
    let mut pending = 0;
    let mut selected_path = std::env::var_os("VLC_FLUBBER_CSV_PATH").map(PathBuf::from);
    for command in commands {
        match command {
            CsvCommand::Open(path) => {
                if recording.is_some() {
                    return Err("CSV recording already open".into());
                }
                recording = Some(if let Some(selected) = selected_path.take() {
                    open_selected_csv(&path, &selected)?
                } else {
                    open_csv(&path)?
                });
                pending = 0;
            }
            CsvCommand::Row(time, x, y) => {
                if let Some(file) = &mut recording {
                    writeln!(file, "{time:.6},{x:.6},{y:.6}").map_err(|e| e.to_string())?;
                    pending += 1;
                    if pending >= 30 {
                        file.flush().map_err(|e| e.to_string())?;
                        pending = 0;
                    }
                }
            }
            CsvCommand::Close => {
                if let Some(mut file) = recording.take() {
                    file.flush().map_err(|e| e.to_string())?;
                }
                pending = 0;
            }
        }
    }
    if let Some(mut file) = recording {
        file.flush().map_err(|e| e.to_string())?;
    }
    Ok(())
}

fn open_selected_csv(media: &Path, selected: &Path) -> Result<BufWriter<File>, String> {
    if !selected.is_absolute() || selected.extension().is_none_or(|ext| ext != "csv") {
        return Err("VLC_FLUBBER_CSV_PATH must be an absolute .csv path".into());
    }
    let media_parent = media
        .parent()
        .ok_or("media path has no parent")?
        .canonicalize()
        .map_err(|e| e.to_string())?;
    let selected_parent = selected
        .parent()
        .ok_or("CSV path has no parent")?
        .canonicalize()
        .map_err(|e| e.to_string())?;
    if media_parent != selected_parent {
        return Err("VLC_FLUBBER_CSV_PATH must be beside the media file".into());
    }
    let file = OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(selected)
        .map_err(|e| e.to_string())?;
    let mut csv = BufWriter::new(file);
    csv.write_all(b"time_s,valence,arousal\n")
        .and_then(|_| csv.flush())
        .map_err(|e| e.to_string())?;
    Ok(csv)
}
fn open_csv(path: &Path) -> Result<BufWriter<File>, String> {
    let parent = path.parent().ok_or("media path has no parent")?;
    let stem = path.file_stem().and_then(|s| s.to_str()).unwrap_or("video");
    let stamp = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .map_err(|e| e.to_string())?
        .as_millis();
    for retry in 0..100_u32 {
        let filename = parent.join(format!(
            "{stem}-flubber-{stamp}-{}-{retry}.csv",
            std::process::id()
        ));
        match OpenOptions::new()
            .write(true)
            .create_new(true)
            .open(filename)
        {
            Ok(file) => {
                let mut csv = BufWriter::new(file);
                csv.write_all(b"time_s,valence,arousal\n")
                    .map_err(|e| e.to_string())?;
                csv.flush().map_err(|e| e.to_string())?;
                return Ok(csv);
            }
            Err(error) if error.kind() == std::io::ErrorKind::AlreadyExists => continue,
            Err(error) => return Err(error.to_string()),
        }
    }
    Err("no unique affect CSV name available".into())
}

fn worker(
    shared: Arc<Mutex<Snapshot>>,
    pair: Arc<AffectPair>,
    stop: mpsc::Receiver<()>,
    actions: mpsc::Receiver<Action>,
    dropped_actions: Arc<AtomicU64>,
    status: Arc<AtomicU8>,
) -> Result<(), String> {
    let session = match std::env::var("VLC_FLUBBER_SESSION_ID") {
        Ok(value) => Some(value),
        Err(std::env::VarError::NotPresent) => None,
        Err(_) => return Err("Invalid VLC_FLUBBER_SESSION_ID.".into()),
    };
    let source = stream_source(std::process::id(), session.as_deref())?;
    let result: Result<(Outlet, Outlet, WorkerTimer, TimerPeriod), String> = (|| {
        let info = StreamInfo::builder("VLC_Flubber_Affect", "Affect", Format::Float32)
            .rate(f64::from(AFFECT_HZ))
            .source_id(&format!("{source}-affect"))
            .channels([
                Channel::new("valence").unit("normalized").kind("Affect"),
                Channel::new("arousal").unit("normalized").kind("Affect"),
            ])
            .build()
            .map_err(|e| e.to_string())?;
        let marker_info = StreamInfo::builder("VLC_Flubber_Markers", "Markers", Format::String)
            .irregular()
            .source_id(&format!("{source}-markers"))
            .channels([Channel::new("marker").kind("Markers")])
            .build()
            .map_err(|e| e.to_string())?;
        let affect = Outlet::new(info).map_err(|e| e.to_string())?;
        let markers = Outlet::new(marker_info).map_err(|e| e.to_string())?;
        let period = TimerPeriod::new().map_err(str::to_owned)?;
        let timer = WorkerTimer::new().map_err(str::to_owned)?;
        Ok((affect, markers, timer, period))
    })();
    let (affect, marker_outlet, timer, _period) = match result {
        Ok(value) => {
            status.store(1, Ordering::Release);
            value
        }
        Err(error) => {
            status.store(2, Ordering::Release);
            return Err(error);
        }
    };
    let markers = Markers {
        outlet: marker_outlet,
        latest: Cell::new(0.),
    };
    let (csv_tx, csv_rx) = mpsc::sync_channel(256);
    let csv_failure = Arc::new(Mutex::new(None::<String>));
    let writer_failure = Arc::clone(&csv_failure);
    let csv_thread = thread::spawn(move || {
        let result = csv_writer(csv_rx);
        if let Err(message) = &result {
            if let Ok(mut slot) = writer_failure.lock() {
                *slot = Some(message.clone());
            }
        }
        result
    });
    let run = (|| -> Result<(), String> {
        let mut session: Option<Session> = None;
        let mut last_interrupts = 0;
        let mut next_affect = Instant::now();
        let mut next_record = next_affect;
        loop {
            if stop.try_recv().is_ok() {
                break;
            }
            let now = Instant::now();
            let coordinates = pair.load();
            let stamp = labstream::clock();
            for action in actions.try_iter().take(16) {
                markers.action(action)?;
            }
            if now >= next_record {
                let dropped = dropped_actions.swap(0, Ordering::Relaxed);
                if dropped != 0 {
                    markers.push_text_at(
                        &format!(r#"VLC_Action {{"event":"events-dropped","count":{dropped}}}"#),
                        stamp,
                    )?;
                }
                if let Ok(failure) = csv_failure.lock() {
                    if let Some(message) = failure.as_ref() {
                        return Err(format!("CSV writer: {message}"));
                    }
                }
                let snapshot = shared.lock().map_err(|_| "playback lock poisoned")?.clone();
                if snapshot.interrupts != last_interrupts {
                    if let Some(active) = &session {
                        active.marker(&markers, "Interrupt", stamp)?;
                    }
                    last_interrupts = snapshot.interrupts;
                }
                if let Some(active) = &session {
                    if matches!(snapshot.state, 5..=7)
                        || snapshot.name != active.name
                        || snapshot.path != active.path
                    {
                        let active = session.take().unwrap();
                        let suffix = match snapshot.state {
                            6 => Some("End"),
                            7 => Some("Error"),
                            _ => None,
                        };
                        active.stop(&markers, stamp, suffix, &csv_tx)?;
                    }
                }
                if session.is_none() && snapshot.state == 3 && snapshot.media_ms > 0 {
                    session = Some(Session::start(&snapshot, stamp, &markers, &csv_tx)?);
                }
                if let Some(active) = &mut session {
                    active.sample(&snapshot, coordinates, stamp, &markers, &csv_tx)?;
                }
                next_record += RECORD_PERIOD;
                if next_record <= now {
                    next_record = now + RECORD_PERIOD;
                }
            }
            affect
                .push_at(&coordinates, stamp)
                .map_err(|e| e.to_string())?;
            next_affect += AFFECT_PERIOD;
            let now = Instant::now();
            if next_affect <= now {
                // Keep the next deadline on the same grid after a scheduling
                // stall. Skip expired slots and never publish catch-up bursts.
                let period_ns = AFFECT_PERIOD.as_nanos();
                let late_ns = now.duration_since(next_affect).as_nanos();
                next_affect = now + Duration::from_nanos((period_ns - late_ns % period_ns) as u64);
            }
            timer.wait_until(next_affect).map_err(str::to_owned)?;
        }
        for action in actions.try_iter() {
            markers.action(action)?;
        }
        if let Some(active) = session {
            active.stop(&markers, labstream::clock(), None, &csv_tx)?;
        }
        Ok(())
    })();
    drop(csv_tx);
    let writer = csv_thread
        .join()
        .unwrap_or_else(|_| Err("CSV writer panicked".into()));
    run.and(writer)
}

pub struct Bridge {
    shared: Arc<Mutex<Snapshot>>,
    pair: Arc<AffectPair>,
    stop: mpsc::Sender<()>,
    actions: mpsc::SyncSender<Action>,
    dropped_actions: Arc<AtomicU64>,
    worker: Option<JoinHandle<Result<(), String>>>,
    status: Arc<AtomicU8>,
    error: Arc<Mutex<String>>,
}

/// Returns an owned handle immediately; poll status for asynchronous startup.
#[no_mangle]
pub extern "C" fn flubber_bridge_new() -> *mut c_void {
    std::panic::catch_unwind(|| {
        let shared = Arc::new(Mutex::new(Snapshot::default()));
        let pair = Arc::new(AffectPair::default());
        let (stop_tx, stop_rx) = mpsc::channel();
        let (action_tx, action_rx) = mpsc::sync_channel(ACTION_QUEUE_CAPACITY);
        let dropped_actions = Arc::new(AtomicU64::new(0));
        let status = Arc::new(AtomicU8::new(0));
        let error = Arc::new(Mutex::new(String::new()));
        let worker_shared = Arc::clone(&shared);
        let worker_pair = Arc::clone(&pair);
        let worker_dropped_actions = Arc::clone(&dropped_actions);
        let worker_status = Arc::clone(&status);
        let worker_error = Arc::clone(&error);
        let thread = thread::spawn(move || {
            let result = worker(
                worker_shared,
                worker_pair,
                stop_rx,
                action_rx,
                worker_dropped_actions,
                Arc::clone(&worker_status),
            );
            if let Err(message) = &result {
                if let Ok(mut error) = worker_error.lock() {
                    *error = message.clone();
                }
                worker_status.store(2, Ordering::Release);
            }
            result
        });
        Box::into_raw(Box::new(Bridge {
            shared,
            pair,
            stop: stop_tx,
            actions: action_tx,
            dropped_actions,
            worker: Some(thread),
            status,
            error,
        }))
        .cast()
    })
    .unwrap_or(std::ptr::null_mut())
}

/// Queue a small action payload without blocking the GUI or the affect producer.
///
/// # Safety
/// `bridge` must be live and `json` must point to a NUL-terminated UTF-8 string.
#[no_mangle]
pub unsafe extern "C" fn flubber_bridge_action(bridge: *mut c_void, json: *const c_char) -> c_int {
    if bridge.is_null() || json.is_null() {
        return -1;
    }
    let bridge = unsafe { &*(bridge as *const Bridge) };
    let Ok(json) = unsafe { CStr::from_ptr(json) }.to_str() else {
        return -1;
    };
    if json.is_empty() || json.len() > 1024 {
        return -1;
    }
    let action = Action {
        stamp: labstream::clock(),
        json: json.to_owned(),
    };
    match bridge.actions.try_send(action) {
        Ok(()) => 0,
        Err(mpsc::TrySendError::Full(_)) => {
            bridge.dropped_actions.fetch_add(1, Ordering::Relaxed);
            -2
        }
        Err(mpsc::TrySendError::Disconnected(_)) => -4,
    }
}

/// Publish both axes together from the Qt input handler without a playback lock.
///
/// # Safety
/// `bridge` must be a live handle owned by the calling GUI thread.
#[no_mangle]
pub unsafe extern "C" fn flubber_bridge_set_affect(bridge: *mut c_void, x: f32, y: f32) -> c_int {
    if bridge.is_null() || !x.is_finite() || !y.is_finite() {
        return -1;
    }
    let bridge = unsafe { &*(bridge as *const Bridge) };
    if bridge.status.load(Ordering::Acquire) == 2 {
        return -4;
    }
    bridge.pair.store(x, y);
    0
}

/// All caller-owned UTF-8 buffers are copied during this call.
#[no_mangle]
pub unsafe extern "C" fn flubber_bridge_update(
    bridge: *mut c_void,
    x: f32,
    y: f32,
    state: c_int,
    media_ms: i64,
    name: *const c_char,
    path: *const c_char,
) -> c_int {
    std::panic::catch_unwind(|| {
        if bridge.is_null() || name.is_null() || path.is_null() || !x.is_finite() || !y.is_finite()
        {
            return -1;
        }
        // SAFETY: Qt owns the handle and passes live zero-terminated buffers.
        let bridge = unsafe { &*(bridge as *const Bridge) };
        if bridge.status.load(Ordering::Acquire) == 2 {
            return -4;
        }
        let name = unsafe { CStr::from_ptr(name) }
            .to_string_lossy()
            .into_owned();
        let path = unsafe { CStr::from_ptr(path) }
            .to_string_lossy()
            .into_owned();
        if let Ok(mut shared) = bridge.shared.lock() {
            let interrupts = shared.interrupts;
            bridge.pair.store(x, y);
            *shared = Snapshot {
                state,
                media_ms,
                name,
                path: if path.is_empty() {
                    None
                } else {
                    Some(PathBuf::from(path))
                },
                interrupts,
            };
            0
        } else {
            -2
        }
    })
    .unwrap_or(-3)
}

/// Queue one focus-loss marker for the live video session.
#[no_mangle]
pub unsafe extern "C" fn flubber_bridge_interrupt(bridge: *mut c_void) -> c_int {
    std::panic::catch_unwind(|| {
        if bridge.is_null() {
            return -1;
        }
        // SAFETY: Qt owns this handle until its timer has stopped.
        let bridge = unsafe { &*(bridge as *const Bridge) };
        if bridge.status.load(Ordering::Acquire) == 2 {
            return -4;
        }
        if let Ok(mut shared) = bridge.shared.lock() {
            shared.interrupts = shared.interrupts.wrapping_add(1);
            0
        } else {
            -2
        }
    })
    .unwrap_or(-3)
}

/// 0 starting, 1 outlets live, -1 worker failed.
#[no_mangle]
pub unsafe extern "C" fn flubber_bridge_status(bridge: *mut c_void) -> c_int {
    std::panic::catch_unwind(|| {
        if bridge.is_null() {
            return -1;
        }
        // SAFETY: Qt owns this handle until its timer has stopped.
        let bridge = unsafe { &*(bridge as *const Bridge) };
        match bridge.status.load(Ordering::Acquire) {
            0 => 0,
            1 => 1,
            _ => -1,
        }
    })
    .unwrap_or(-1)
}

/// Copy a terminal error into caller storage, including a zero terminator.
#[no_mangle]
pub unsafe extern "C" fn flubber_bridge_error(
    bridge: *mut c_void,
    out: *mut c_char,
    capacity: usize,
) -> usize {
    std::panic::catch_unwind(|| {
        if bridge.is_null() || out.is_null() || capacity == 0 {
            return 0;
        }
        // SAFETY: Qt owns the handle and supplies a writable buffer of capacity bytes.
        let bridge = unsafe { &*(bridge as *const Bridge) };
        let Ok(error) = bridge.error.lock() else {
            return 0;
        };
        let length = error.len().min(capacity - 1);
        // SAFETY: length is within the caller capacity and the buffers do not overlap.
        unsafe {
            std::ptr::copy_nonoverlapping(error.as_ptr(), out.cast(), length);
            *out.add(length) = 0;
        }
        length
    })
    .unwrap_or(0)
}

/// Qt stops its update timer before freeing; join closes outlets and CSV before DLL unload.
#[no_mangle]
pub unsafe extern "C" fn flubber_bridge_free(bridge: *mut c_void) {
    if bridge.is_null() {
        return;
    }
    let _ = std::panic::catch_unwind(|| {
        // SAFETY: Caller frees its unique owned handle exactly once.
        let mut bridge = unsafe { Box::from_raw(bridge as *mut Bridge) };
        let _ = bridge.stop.send(());
        if let Some(worker) = bridge.worker.take() {
            if let Err(error) = worker.join().unwrap_or_else(|_| Err("worker panic".into())) {
                eprintln!("Flubber bridge worker stopped: {error}");
            }
        }
    });
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::ffi::CString;

    #[test]
    fn recorder_session_identity_is_unique_and_default_ids_stay_compatible() {
        assert_eq!(stream_source(42, None).unwrap(), "vlc-flubber-42");
        assert_eq!(
            stream_source(42, Some("session_A-1")).unwrap(),
            "vlc-flubber-42-session_A-1"
        );
        assert_ne!(
            stream_source(42, Some("session_A")).unwrap(),
            stream_source(42, Some("session_B")).unwrap()
        );
        for token in ["", "bad token", "bad'query", "bad\nquery", &"x".repeat(65)] {
            assert!(stream_source(42, Some(token)).is_err());
        }
    }

    #[test]
    fn csv_name_is_unique_and_header_is_exact() {
        let root = std::env::temp_dir().join(format!("flubber-bridge-test-{}", std::process::id()));
        std::fs::create_dir_all(&root).unwrap();
        let path = root.join("source.mp4");
        let _first = open_csv(&path).unwrap();
        let _second = open_csv(&path).unwrap();
        let files: Vec<_> = std::fs::read_dir(&root).unwrap().collect();
        assert_eq!(files.len(), 2);
        for file in files {
            let file = file.unwrap().path();
            assert_eq!(
                std::fs::read_to_string(&file).unwrap(),
                "time_s,valence,arousal\n"
            );
            std::fs::remove_file(file).unwrap();
        }
        std::fs::remove_dir(root).unwrap();
    }

    #[test]
    fn selected_csv_is_same_folder_and_never_overwrites() {
        let root = std::env::temp_dir().join(format!("flubber-selected-{}", std::process::id()));
        std::fs::create_dir_all(&root).unwrap();
        let media = root.join("source.mp4");
        let selected = root.join("session.csv");
        let _csv = open_selected_csv(&media, &selected).unwrap();
        assert!(open_selected_csv(&media, &selected).is_err());
        assert!(open_selected_csv(&media, &std::env::temp_dir().join("elsewhere.csv")).is_err());
        std::fs::remove_file(selected).unwrap();
        std::fs::remove_dir(root).unwrap();
    }

    #[test]
    fn live_bridge_emits_lsl_markers_and_video_csv() {
        use labstream::{Buffer, Inlet, Post, Query};
        let bridge = flubber_bridge_new();
        assert!(!bridge.is_null());
        let startup = Instant::now();
        while unsafe { flubber_bridge_status(bridge) } == 0
            && startup.elapsed() < Duration::from_secs(5)
        {
            thread::sleep(Duration::from_millis(10));
        }
        assert_eq!(unsafe { flubber_bridge_status(bridge) }, 1);
        let timeout = Duration::from_secs(3);
        let source = format!("vlc-flubber-{}", std::process::id());
        let affect_info =
            labstream::resolve_first(&Query::source_id(&format!("{source}-affect")), timeout)
                .unwrap()
                .unwrap()
                .fetch(timeout)
                .unwrap();
        let marker_info =
            labstream::resolve_first(&Query::source_id(&format!("{source}-markers")), timeout)
                .unwrap()
                .unwrap()
                .fetch(timeout)
                .unwrap();
        let mut affect = Inlet::builder(&affect_info)
            .buffer(Buffer::Seconds(2.))
            .recover(false)
            .postprocess(Post::NONE)
            .open(timeout)
            .unwrap();
        let mut markers = Inlet::builder(&marker_info)
            .buffer(Buffer::Samples(16))
            .recover(false)
            .postprocess(Post::NONE)
            .open(timeout)
            .unwrap();
        for json in [
            c"{\"event\":\"seek\",\"position\":0.5}",
            c"{\"event\":\"fullscreen\",\"enabled\":true}",
        ] {
            assert_eq!(unsafe { flubber_bridge_action(bridge, json.as_ptr()) }, 0);
        }
        let (seek_at, seek) = markers.pull_text(timeout).unwrap().unwrap();
        let (fullscreen_at, fullscreen) = markers.pull_text(timeout).unwrap().unwrap();
        assert_eq!(seek, [r#"VLC_Action {"event":"seek","position":0.5}"#]);
        assert_eq!(
            fullscreen,
            [r#"VLC_Action {"event":"fullscreen","enabled":true}"#]
        );
        assert!(seek_at <= fullscreen_at);
        assert!(affect.pull::<f32>(timeout).unwrap().is_some());
        let mut response_ms = Vec::new();
        for index in 0..48 {
            let x = if index % 2 == 0 { -0.25 } else { 0.25 };
            let sent = Instant::now();
            assert_eq!(unsafe { flubber_bridge_set_affect(bridge, x, 0.5) }, 0);
            loop {
                let (_, sample) = affect.pull::<f32>(timeout).unwrap().unwrap();
                if sample == [x, 0.5] {
                    response_ms.push(sent.elapsed().as_secs_f64() * 1000.);
                    break;
                }
                assert!(sent.elapsed() < timeout, "updated pair never arrived");
            }
        }
        response_ms.sort_by(f64::total_cmp);
        let p95 = response_ms[45];
        eprintln!(
            "Affect update to loopback receipt: p50 {:.2} ms, p95 {p95:.2} ms, max {:.2} ms (n={})",
            response_ms[23],
            response_ms[47],
            response_ms.len()
        );
        assert!(p95 < 15., "loopback p95 was {p95:.2} ms");
        let folder = std::env::temp_dir().join(format!("flubber-live-{}", std::process::id()));
        std::fs::create_dir_all(&folder).unwrap();
        let video = folder.join("clip.mp4");
        let path = CString::new(video.to_str().unwrap()).unwrap();
        let name = CString::new("clip.mp4").unwrap();
        let mut stamps = Vec::new();
        let start = Instant::now();
        let mut interrupted = false;
        while start.elapsed() < Duration::from_secs(2) {
            let media_ms = 40 + start.elapsed().as_millis() as i64;
            assert_eq!(
                unsafe {
                    flubber_bridge_update(
                        bridge,
                        0.25,
                        -0.5,
                        3,
                        media_ms,
                        name.as_ptr(),
                        path.as_ptr(),
                    )
                },
                0
            );
            if !interrupted && start.elapsed() >= Duration::from_secs(1) {
                assert_eq!(unsafe { flubber_bridge_interrupt(bridge) }, 0);
                interrupted = true;
            }
            if let Some((stamp, sample)) = affect.pull::<f32>(Duration::from_millis(40)).unwrap() {
                if sample == vec![0.25, -0.5] {
                    stamps.push(stamp);
                }
            }
        }
        assert_eq!(affect_info.rate(), f64::from(AFFECT_HZ));
        assert!(
            stamps.len() >= AFFECT_HZ as usize,
            "only {} affect samples",
            stamps.len()
        );
        let span = stamps.last().unwrap() - stamps.first().unwrap();
        let rate = (stamps.len() - 1) as f64 / span;
        assert!(
            (0.95..1.05).contains(&(rate / f64::from(AFFECT_HZ))),
            "LSL sampling rate {rate:.2} Hz"
        );
        assert_eq!(
            unsafe {
                flubber_bridge_update(
                    bridge,
                    0.25,
                    -0.5,
                    0,
                    0,
                    name.as_ptr(),
                    CString::new("").unwrap().as_ptr(),
                )
            },
            0
        );
        let deadline = Instant::now() + Duration::from_secs(2);
        let mut labels = Vec::new();
        while Instant::now() < deadline && labels.len() < 3 {
            if let Some((_stamp, label)) = markers.pull_text(Duration::from_millis(100)).unwrap() {
                labels.push(label[0].clone());
            }
        }
        assert_eq!(
            labels,
            ["clip.mp4_Start", "clip.mp4_Interrupt", "clip.mp4_Stop"]
        );
        assert_eq!(unsafe { flubber_bridge_status(bridge) }, 1);
        unsafe { flubber_bridge_free(bridge) };
        let csv = std::fs::read_dir(&folder)
            .unwrap()
            .next()
            .unwrap()
            .unwrap()
            .path();
        let data = std::fs::read_to_string(&csv).unwrap();
        assert!(data.starts_with("time_s,valence,arousal\n"));
        let csv_times: Vec<f64> = data
            .lines()
            .skip(1)
            .map(|line| line.split(',').next().unwrap().parse().unwrap())
            .collect();
        assert!(csv_times.len() >= 50);
        let csv_hz = (csv_times.len() - 1) as f64 / (csv_times.last().unwrap() - csv_times[0]);
        assert!(
            (28.0..32.0).contains(&csv_hz),
            "CSV sampling rate {csv_hz:.2} Hz"
        );
        eprintln!("Measured cadence: LSL {rate:.2} Hz, CSV {csv_hz:.2} Hz");
        std::fs::remove_file(csv).unwrap();
        std::fs::remove_dir(folder).unwrap();
    }
}
