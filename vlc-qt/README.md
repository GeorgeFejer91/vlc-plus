# Stock VLC 3.0.20 Flubber Qt extension

This source modifies the official VLC 3.0.20 Qt interface. It keeps VLC's
existing Media, Playback, Audio, Video, Subtitle, Tools, View, and Help menus,
playlist, transport and video child window. The patch adds a black bottom
Flubber panel, makes the idle central background black, and adds a top-level
**Flubber → Settings…** menu. Its dialog holds appearance JSON import, size,
and position. It is separate from the standard Experiment Runner.

## Reproducible inputs and outputs

- Upstream source: official `vlc-3.0.20.tar.xz`, SHA-256
  `adc7285b4d2721cddf40eb5270cada2aaa10a334cb546fd55a06353447ba29b5`.
- Apply `apply-patch.ps1 -SourceRoot <unmodified VLC 3.0.20 tree>` once. It
  checks the unified patch, modifies five upstream Qt files plus `bin/winvlc.c`, and copies
  `flubber_panel.hpp` and `flubber_bridge.hpp` into `modules/gui/qt/`.
- Build a matching Windows x64 VLC Qt module from that tree. Do not mix a Qt
  module with a different VLC core/CRT without installed ABI verification.
- Rebuild and package `vlc.exe` from the patched `bin/winvlc.c` as well as the
  Qt module. VLC 3.0.20's module loader uses `LOAD_LIBRARY_SEARCH_SYSTEM32`;
  the patched launcher preloads four packaged Qt DLLs from the executable
  directory with `LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32`
  before libVLC starts, so its Qt plugin can link to the same handles. It
  fails visibly if a required DLL is missing.
- Run `build-bridge.ps1`; its output is
  `bridge/target/release/flubber_bridge.dll`. Place that DLL beside `vlc.exe`.
  The crate-local Cargo config links the MSVC CRT statically. `labstream` 0.1.2
  and its components are MIT-licensed Rust dependencies; they are linked into
  the DLL, with no separate `lsl.dll` import.
- Use the complete root entry `build-windows.ps1`, which builds native
  prerequisites and generated Qt sources before linking the plugin/launcher.
  The old abbreviated manual commands omitted those clean-build prerequisites.
  See the [build and packaging guide](../docs/stock-vlc-integration.md).
- The real outputs are `modules/.libs/libqt_plugin.dll` and `bin/.libs/vlc.exe`.
  Packaging includes `qwindows.dll`, `qoffscreen.dll`, the Vista style and SVG
  plugins; `qt.conf` points Plugins at `qt5/plugins`.
- Run `test-panel.ps1 -QtBin <MinGW64 Qt bin> -AppearanceJson <Planner-exported JSON>`
  to compile the isolated panel check, reject duplicate keys, verify imported
  fill/transparency and manual controls, and render a black idle PNG without
  foregrounding the desktop.

The Qt source modification and its two headers are GPL-2.0-or-later, matching
the host VLC Qt module. The Rust bridge is GPL-2.0-or-later. Packaging the
modified VLC Qt binary must retain the VideoLAN license notices and provide the
corresponding source patch and dependency notices.

## Appearance and controls

The JSON reader accepts only `vlc-flubber-appearance/v1` from the Planner's
Classic Flubber preview. It rejects duplicate and unknown keys, malformed
colors, unsupported drivers, invalid mapping ranges and files over 64 KiB.
An invalid file never replaces the active preset. The painter uses the four
axis/corner colors, idle fill, outline and halo colors, transparency, halo
width/gradient/steepness, color-anchor mode, and all six mapping controls.
A small two-axis rating indicator beside Flubber uses the exported `cursor`
color. Its 192-point circle profile and seeded
wave offsets follow the pinned upstream Planner math (see [provenance](../docs/provenance.md)); halo blur is approximated with six
antialiased strokes because Qt QPainter does not use the browser SVG filter.

Position and size are VLC-owned, independent of the Planner JSON: bottom panel
height, Flubber size, and normalized horizontal/vertical position live in VLC's
`QSettings`. The panel is black before playback and after media ends. Before
rating is active, pointer movement over the panel or VLC video surface updates
the affect coordinates. Click and release on the panel during playback to
activate relative mouse rating. The pointer is confined to the player client
area and hidden; horizontal and vertical movement map to the two normalized
axes with the previous player's 60% height travel and 150 px per-event cap.
Alt+F toggles affect arrow control. Left/Right changes valence; Up/Down
changes arousal by the existing session step when the current input allows
affect adjustment (including pause). The player claims
plain-arrow ShortcutOverride events before VLC volume/seek shortcuts, even
while paused. With no supported input, keys are reserved without changing
affect; the existing playback gate still owns CSV recording. Alt+F again restores VLC
arrows and mouse affect input. Dialogs/popups and other windows retain their
normal controls, and holding Alt+F does not toggle repeatedly. Escape releases the
pointer. Focus/capture loss releases it, emits an `Interrupt` marker and
requests pause. Stop, End, and input change also release it. VLC's stock
keyboard commands remain available outside active rating. The video child
handle and VLC's stock playback paths remain unchanged.

For Recorder sessions, optional process environment variables override only
the session parameters:

| Variable | Behavior |
| --- | --- |
| `VLC_FLUBBER_PANEL_PERCENT` | 10–100 percent of the current video surface height; invalid values are ignored. |
| `VLC_FLUBBER_STEP_PERCENT` | 1–100 percent per arrow rating step while the panel has focus; invalid values are ignored. |
| `VLC_FLUBBER_CSV_PATH` | First local media session's CSV; must be an absolute `.csv` path beside the media and is opened with `create_new`. Later sessions use unique automatic names. |

VLC 3.0.20's stock `oldrc` interface supports
`--extraintf=rc --rc-host=127.0.0.1:<port> --rc-quiet` and the `add`, `play`,
`pause`, `stop`, `volume`, `is_playing`, and `f on|off` commands. An armed Recorder
can launch idle VLC, wait for the two LSL outlets, then send `add <media>`.

The Flubber menu adds **Main feedback** shortcuts Ctrl+Shift+1/2/3 for
Flubber, 2D grid, and face morph. Ctrl+Alt+1/2/3 independently shows or hides
each element, so all three can appear together. Ctrl+Shift+A and Ctrl+Shift+M
select arrow and mouse input. **Flubber → Settings…** holds the detailed
appearance, face preset, and layout controls.

Launch VLC with `--flubber-settings=<absolute JSON path>` to load a settings
file. `--flubber-primary=flubber|grid|face`, `--flubber-input=arrows|mouse`,
and `--flubber-face=photo-reference-v3|photo-synthetic-01..08` override their
respective settings afterward. These are Qt interface options and work with
the packaged player.

For a running player, start VLC with loopback RC enabled, then use
`flubberctl.ps1 -Port <port> -Command '<command>'`. The script sends the
following stock RC extension commands to that instance:

```text
flubber primary flubber|grid|face
flubber input arrows|mouse
flubber visible flubber|grid|face on|off
flubber face photo-reference-v3|photo-synthetic-01..08
flubber load <absolute JSON path>
```

Example: `flubberctl.ps1 -Port 42123 -Command 'visible grid on'`. The existing
VLC RC interface is the command transport. The Qt callback queues state
changes to the GUI thread; malformed commands and invalid settings show an
error in the player.

## Native lifecycle audit

- Qt creates `FlubberPanel` and `FlubberBridge` on the GUI thread. The panel's
  33 ms QTimer and mouse/keyboard handlers stay on that thread. It does not
  move or replace `VideoWidget::stable`, which owns VLC's vout HWND. Qt's
  normal `VideoWidget::release()` still orphans/releases that handle. Rating
  capture starts after button release so Qt's implicit press capture cannot
  undo it. On Windows, `SetCapture`, `ClipCursor`, and balanced `ShowCursor`
  calls are confined to the foreground player and reversed on every exit path.
- Qt loads the Rust DLL from its own executable directory with `QLibrary` and
  resolves six fixed C ABI functions: `new`, `update`, `interrupt`, `status`,
  `error`, and `free`. It passes copied UTF-8 bytes and plain numeric values only; no Qt,
  VLC, or HWND pointer crosses the boundary. The bridge does not call back into
  Qt. A failed load/worker shows a one-time visible warning.
- `flubber_bridge_new()` returns an owned handle while the LSL worker starts
  asynchronously. `status` reports starting/live/failed. `update` copies
  bounded coordinates, state, media time, filename and path into Rust-owned
  memory. It never holds a caller buffer after return. The Qt timer stops
  before `free`; `free` signals and joins the owned worker.
  `QLibrary::PreventUnloadHint` retains code through process exit because
  Rust/LSL may retain process-lifetime threads or TLS destructors. Unloading
  immediately after join caused an observed shutdown access violation.
- The Rust worker owns a high-resolution waitable timer and balanced Windows
  timer period, and publishes 30 Hz LSL samples even while VLC is idle. Its
  separate bounded CSV writer opens a fresh file on video Start and closes on
  Stop. Disk errors and queue overload set terminal status for a visible Qt
  warning; they are not silently ignored. Worker failure closes outlets.
- Qt maps live VLC input states to the bridge and briefly latches a real
  `END_S`/`ERROR_S` state. `InputManager::delInput()` also emits a synthetic
  `END_S` for Stop/Next, so the latch requires a still-live input in the same
  core state. VLC 3 core also sets `END_S` during manual teardown. Installed
  testing observed `End` on manual Stop; it is a terminal-input observation,
  **not proof of natural video completion**. Preserve this stream format; a
  future completion-reason contract needs an authoritative source and version.
- The Qt boundary casts the live panel/main `winId()` to HWND for foreground,
  capture and client-rectangle checks; no HWND crosses into Rust. The bridge
  also converts six `QLibrary` symbols to fixed C ABI function pointers.
  Rust FFI pointer dereference is limited to the six
  exported functions, with a unique owner, stop-before-free order, and
  `catch_unwind` around each entry. Windows timer handle and period have RAII
  cleanup. A blocked native outlet startup can delay final worker join at
  process exit; this is an outstanding shutdown bound to test in the installed
  player.

## Current evidence and limits

The [installer verification record](../for-ai/VERIFICATION.md) identifies the
exact passing source commit and fresh Windows install test. It covers real
H.264 decoding, independent LSL, pause/resume, replay, CSV naming/cadence,
portable settings, package-local Qt loading, bounded shutdown and uninstall.
The Qt panel check covers appearance/settings and synthetic input.

These offscreen checks do not qualify fullscreen/D3D behavior, physical
mouse/keyboard capture, display onset or experiment timing. `End` is an input
teardown observation, including manual Stop/Next; it is not natural-completion
proof. CSV acquisition starts after the first positive media-time update,
observed around 0.4 seconds. Full Planner-master execution remains open.
