//! Independent installed-player probe. It observes real LSL and decoded-video
//! statistics; Qt stays offscreen and only the child created here is stopped.
use labstream::{Buffer, Inlet, Post, Query};
use std::error::Error;
use std::fs::{self, File};
use std::io::{Read, Write};
use std::net::{TcpListener, TcpStream};
use std::path::{Path, PathBuf};
use std::process::{Child, Command, Stdio};
use std::thread;
use std::time::{Duration, Instant};

type Result<T> = std::result::Result<T, Box<dyn Error>>;

struct Player(Child);
impl Drop for Player {
    fn drop(&mut self) {
        if matches!(self.0.try_wait(), Ok(None)) {
            let _ = self.0.kill();
            let _ = self.0.wait();
        }
    }
}

fn command(socket: &mut TcpStream, value: &str) -> Result<()> {
    socket.write_all(value.as_bytes())?;
    socket.write_all(b"\n")?;
    socket.flush()?;
    Ok(())
}

fn csv_files(folder: &Path) -> Result<Vec<PathBuf>> {
    let mut files = fs::read_dir(folder)?
        .filter_map(|entry| entry.ok().map(|entry| entry.path()))
        .filter(|path| path.extension().is_some_and(|extension| extension == "csv"))
        .collect::<Vec<_>>();
    files.sort();
    Ok(files)
}

fn closed_csv(path: &Path) -> Result<Vec<u8>> {
    let started = Instant::now();
    loop {
        let mut options = fs::OpenOptions::new();
        options.read(true);
        #[cfg(windows)]
        {
            use std::os::windows::fs::OpenOptionsExt;
            options.share_mode(0);
        }
        match options.open(path) {
            Ok(mut file) => {
                let mut bytes = Vec::new();
                file.read_to_end(&mut bytes)?;
                return Ok(bytes);
            }
            Err(error)
                if matches!(error.raw_os_error(), Some(32 | 33))
                    && started.elapsed() < Duration::from_secs(3) =>
            {
                thread::sleep(Duration::from_millis(10));
            }
            Err(error) => return Err(error.into()),
        }
    }
}

fn main() -> Result<()> {
    let args = std::env::args_os().skip(1).collect::<Vec<_>>();
    if args.len() != 3 {
        return Err("usage: verify-player <installed vlc.exe> <synthetic validation.mp4> <evidence directory>".into());
    }
    let exe = fs::canonicalize(&args[0])?;
    let media = fs::canonicalize(&args[1])?;
    let evidence = fs::canonicalize(&args[2])?;
    if evidence.join("player.log").exists() {
        return Err("use a fresh evidence directory; prior logs are retained".into());
    }
    let media = PathBuf::from(media.to_string_lossy().trim_start_matches(r"\\?\"));
    if media.file_name().and_then(|value| value.to_str()) != Some("validation.mp4") {
        return Err("probe requires its own synthetic validation.mp4".into());
    }
    let folder = media.parent().ok_or("media has no folder")?;
    if !csv_files(folder)?.is_empty() {
        return Err("probe media folder already has CSV files; use a fresh run directory".into());
    }
    let port = TcpListener::bind("127.0.0.1:0")?.local_addr()?.port();
    let log = File::create(evidence.join("player.log"))?;
    let mut launch = Command::new(&exe);
    launch
        .args([
            "--intf=qt",
            "--extraintf=rc",
            "--rc-quiet",
            "--no-one-instance",
            "--ignore-config",
            "--no-qt-privacy-ask",
            "--no-qt-error-dialogs",
            "--no-plugins-cache",
            "--avcodec-hw=none",
            "--vout=dummy",
            "--aout=dummy",
            "--language=en",
            "--verbose=2",
        ])
        .arg(format!("--rc-host=127.0.0.1:{port}"))
        .env("QT_QPA_PLATFORM", "offscreen")
        .env("QT_DEBUG_PLUGINS", "1")
        .current_dir(exe.parent().ok_or("player has no directory")?)
        .stdin(Stdio::null())
        .stdout(Stdio::from(log.try_clone()?))
        .stderr(Stdio::from(log));
    for key in [
        "QT_PLUGIN_PATH",
        "QT_QPA_PLATFORM_PLUGIN_PATH",
        "QML2_IMPORT_PATH",
        "QML_IMPORT_PATH",
        "VLC_PLUGIN_PATH",
        "VLC_DATA_PATH",
        "VLC_FLUBBER_PANEL_PERCENT",
        "VLC_FLUBBER_STEP_PERCENT",
        "VLC_FLUBBER_CSV_PATH",
    ] {
        launch.env_remove(key);
    }
    #[cfg(windows)]
    {
        use std::os::windows::process::CommandExt;
        launch.creation_flags(0x0800_0000);
        let windows = PathBuf::from(std::env::var_os("SystemRoot").ok_or("SystemRoot missing")?);
        launch.env(
            "PATH",
            std::env::join_paths([
                exe.parent().unwrap().to_path_buf(),
                windows.join("System32"),
                windows,
            ])?,
        );
    }
    let mut player = Player(launch.spawn()?);
    let startup = Instant::now();
    let mut socket = loop {
        if let Some(status) = player.0.try_wait()? {
            return Err(format!("player exited during startup: {status}; see player.log").into());
        }
        if let Ok(socket) = TcpStream::connect(("127.0.0.1", port)) {
            break socket;
        }
        if startup.elapsed() > Duration::from_secs(20) {
            return Err("player RC startup exceeded 20 seconds".into());
        }
        thread::sleep(Duration::from_millis(50));
    };
    socket.set_read_timeout(Some(Duration::from_millis(1)))?;
    let timeout = Duration::from_secs(8);
    let source = format!("vlc-flubber-{}", player.0.id());
    let affect_info =
        labstream::resolve_first(&Query::source_id(&format!("{source}-affect")), timeout)?
            .ok_or("installed player did not publish its affect outlet")?
            .fetch(timeout)?;
    let marker_info =
        labstream::resolve_first(&Query::source_id(&format!("{source}-markers")), timeout)?
            .ok_or("installed player did not publish its marker outlet")?
            .fetch(timeout)?;
    let mut affect = Inlet::builder(&affect_info)
        .buffer(Buffer::Samples(4096))
        .recover(false)
        .postprocess(Post::NONE)
        .open(timeout)?;
    let mut markers = Inlet::builder(&marker_info)
        .buffer(Buffer::Samples(128))
        .recover(false)
        .postprocess(Post::NONE)
        .open(timeout)?;
    for value in [
        "flubber primary grid",
        "flubber visible flubber on",
        "flubber visible grid on",
        "flubber visible face on",
        "flubber input arrows",
    ] {
        command(&mut socket, value)?;
    }
    command(&mut socket, &format!("add {}", media.display()))?;
    let mut labels = Vec::new();
    let mut marker_log = File::create(evidence.join("observed-markers.txt"))?;
    let mut rc_log = File::create(evidence.join("rc-output.txt"))?;
    let mut stamps = Vec::new();
    let mut rc_output = String::new();
    let began = Instant::now();
    let mut first_start = None;
    let mut second_start = None;
    let mut paused = false;
    let mut resumed = false;
    let mut stats = false;
    let mut stopped_second = false;
    let mut replayed = false;
    let mut first_csv = None;
    while began.elapsed() < Duration::from_secs(25) {
        if let Some((stamp, values)) = affect.pull::<f32>(Duration::from_millis(8))? {
            if values.len() != 2
                || values
                    .iter()
                    .any(|value| !value.is_finite() || !(-1.0..=1.0).contains(value))
            {
                return Err("invalid affect sample".into());
            }
            if stamps.last().is_some_and(|previous| stamp <= *previous) {
                return Err("LSL affect timestamps are not strictly increasing".into());
            }
            stamps.push(stamp);
        }
        while let Some((_stamp, values)) = markers.pull_text(Duration::ZERO)? {
            if values.len() != 1 {
                return Err("unexpected marker channel count".into());
            }
            let label = values[0].clone();
            writeln!(marker_log, "{label}")?;
            if label == "validation.mp4_Start" {
                if first_start.is_none() {
                    first_start = Some(Instant::now());
                } else {
                    second_start = Some(Instant::now());
                }
            }
            labels.push(label);
        }
        let mut buffer = [0; 4096];
        match socket.read(&mut buffer) {
            Ok(length) => {
                rc_log.write_all(&buffer[..length])?;
                rc_output.push_str(&String::from_utf8_lossy(&buffer[..length]));
            }
            Err(error)
                if matches!(
                    error.kind(),
                    std::io::ErrorKind::WouldBlock | std::io::ErrorKind::TimedOut
                ) => {}
            Err(error) => return Err(error.into()),
        }
        if let Some(start) = first_start {
            if !paused && start.elapsed() > Duration::from_secs(1) {
                command(&mut socket, "pause")?;
                paused = true;
            }
            if !resumed && start.elapsed() > Duration::from_secs(2) {
                command(&mut socket, "pause")?;
                resumed = true;
            }
            if !stats && start.elapsed() > Duration::from_secs(3) {
                command(&mut socket, "stats")?;
                stats = true;
            }
        }
        let stop_count = labels
            .iter()
            .filter(|label| *label == "validation.mp4_Stop")
            .count();
        if stop_count == 1 && !replayed {
            let files = csv_files(folder)?;
            if files.len() != 1 {
                return Err("first run did not create exactly one CSV".into());
            }
            first_csv = Some((files[0].clone(), closed_csv(&files[0])?));
            command(&mut socket, &format!("add {}", media.display()))?;
            replayed = true;
        }
        if second_start.is_some_and(|start| start.elapsed() > Duration::from_secs(1))
            && !stopped_second
        {
            command(&mut socket, "stop")?;
            stopped_second = true;
        }
        if stop_count == 2 {
            break;
        }
        if player.0.try_wait()?.is_some() {
            return Err("player exited before both runs finished".into());
        }
    }
    let expected = ["Start", "Pause", "Resume", "End", "Stop", "Start", "Stop"]
        .map(|suffix| format!("validation.mp4_{suffix}"));
    if labels != expected {
        return Err(format!("marker chronology mismatch: {labels:?}").into());
    }
    let decoded = rc_output
        .lines()
        .filter(|line| line.contains("video decoded"))
        .filter_map(|line| line.split(':').nth(1)?.trim().parse::<u64>().ok())
        .max()
        .unwrap_or(0);
    if decoded == 0 {
        return Err("VLC statistics did not prove actual video decoding".into());
    }
    command(&mut socket, "quit")?;
    let quitting = Instant::now();
    loop {
        if let Some(status) = player.0.try_wait()? {
            if !status.success() {
                return Err(format!("player exit status: {status}").into());
            }
            break;
        }
        if quitting.elapsed() > Duration::from_secs(10) {
            return Err("player shutdown exceeded 10 seconds".into());
        }
        thread::sleep(Duration::from_millis(25));
    }
    let (first_path, original) = first_csv.ok_or("missing first CSV")?;
    if fs::read(&first_path)? != original {
        return Err("replay overwrote the first CSV".into());
    }
    let files = csv_files(folder)?;
    if files.len() != 2 {
        return Err("replay did not create a second CSV".into());
    }
    let mut rows = Vec::new();
    let mut csv_seconds = Vec::new();
    for path in files {
        let filename = path
            .file_stem()
            .and_then(|name| name.to_str())
            .ok_or("invalid CSV filename")?;
        let parts = filename
            .strip_prefix("validation-flubber-")
            .ok_or("unexpected CSV name")?
            .split('-')
            .collect::<Vec<_>>();
        if parts.len() != 3
            || parts[0].parse::<u128>()? == 0
            || parts[1].parse::<u32>()? != player.0.id()
            || parts[2].parse::<u32>()? >= 100
        {
            return Err(
                "CSV filename does not match the player-owned unique naming contract".into(),
            );
        }
        let natural = path == first_path;
        let text = fs::read_to_string(&path)?;
        let mut lines = text.lines();
        if lines.next() != Some("time_s,valence,arousal") {
            return Err("invalid CSV header".into());
        }
        let mut previous = -1.0;
        let mut count = 0;
        let mut first_time = None;
        for line in lines {
            let values = line
                .split(',')
                .map(str::parse::<f64>)
                .collect::<std::result::Result<Vec<_>, _>>()?;
            if values.len() != 3
                || values.iter().any(|value| !value.is_finite())
                || values[0] <= previous
                || values[1..]
                    .iter()
                    .any(|value| !(-1.0..=1.0).contains(value))
            {
                return Err("invalid CSV values or chronology".into());
            }
            previous = values[0];
            first_time.get_or_insert(values[0]);
            count += 1;
        }
        let first_time = first_time.ok_or("CSV has no samples")?;
        let csv_rate = (count - 1) as f64 / (previous - first_time);
        let duration_ok = if natural {
            (5.5..=6.4).contains(&previous) && count >= 150
        } else {
            (0.6..=1.8).contains(&previous) && count >= 20
        };
        if first_time > 0.25 || !duration_ok || !(28.0..=32.0).contains(&csv_rate) {
            return Err(format!("CSV duration/cadence mismatch: natural={natural}, rows={count}, first={first_time:.3}, last={previous:.3}, Hz={csv_rate:.2}").into());
        }
        rows.push(count);
        csv_seconds.push(previous);
    }
    if stamps.len() < 100 {
        return Err("too few independent affect samples".into());
    }
    let rate = (stamps.len() - 1) as f64 / (stamps.last().unwrap() - stamps[0]);
    if !(28.0..=32.0).contains(&rate) {
        return Err(format!("observed LSL rate {rate:.2} Hz").into());
    }
    let receipt = format!(
        "{{\n  \"schema\": \"vlc-plus-installed-probe/v1\",\n  \"result\": \"VERIFIED\",\n  \"offscreen\": true,\n  \"physicalInputTested\": false,\n  \"decodedVideoFrames\": {decoded},\n  \"affectSamples\": {},\n  \"observedLslHz\": {rate:.4},\n  \"csvRows\": {rows:?},\n  \"csvActiveSeconds\": {csv_seconds:?},\n  \"markers\": {labels:?},\n  \"shutdownMs\": {}\n}}\n",
        stamps.len(), quitting.elapsed().as_millis()
    );
    fs::write(evidence.join("installed-probe.json"), &receipt)?;
    println!("{receipt}");
    Ok(())
}
