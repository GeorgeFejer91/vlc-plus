# Build and package the standalone Qt player

The current player modifies official VLC 3.0.20's Qt interface. The source
patch, native headers, Rust bridge and isolated panel check are in `vlc-qt/`.
No Affect Research sibling checkout is required. The prior LibVLC shell,
video-filter prototype and Flubbercorder sources are outside this repository.

## Build

1. Download the official source archive and verify SHA-256
   `adc7285b4d2721cddf40eb5270cada2aaa10a334cb546fd55a06353447ba29b5`.
   [The native runbook](../vlc-qt/README.md) records the upstream URL and exact
   inherited MinGW/Qt configure and build commands.
2. Apply `vlc-qt/apply-patch.ps1 -SourceRoot <clean VLC 3.0.20 source>`.
   It checks application before changing that source tree and copies both
   tracked headers into the Qt module.
3. Build the modified Qt plugin and actual launcher from that pinned tree.
   Use `modules/.libs/libqt_plugin.dll` and `bin/.libs/vlc.exe`; the libtool
   wrapper is not the installed launcher.
4. Run `vlc-qt/build-bridge.ps1` with MSVC Cargo available. Its output is
   `vlc-qt/bridge/target/release/flubber_bridge.dll`.
5. Before packaging, commit the reviewed source, assets, notices and scripts.
   From this repository root run:

```powershell
./package-stock-vlc.ps1 -QtPluginPath <built libqt_plugin.dll> `
  -LauncherExePath <built vlc.exe> -BridgeDllPath <built flubber_bridge.dll> `
  -MinGwBin <matching MSYS2 mingw64 bin>
```

The script downloads/hash-checks the pinned official Windows VLC archive,
checks the modified launcher/plugin and stages the runtime, Qt dependencies,
atlases, notices and a complete committed `source/vlc-plus-source.zip` under
`build/stock-vlc-package/stage`. It records
this standalone repository's source commit and package-file hashes. It never
searches the parent Affect Research repository for source or licenses.

Compile `installer-stock-vlc.iss` using Inno Setup 6 to produce the optional
VLC+ installer. The standalone installer has its own AppId and install folder.
The extracted source is not an installer release; do not copy historical
installed receipts onto a new package.

## Checks

```powershell
python scripts/check-source.py
cargo test --locked --manifest-path vlc-qt/bridge/Cargo.toml
./vlc-qt/test-panel.ps1 -QtBin <matching Qt bin> -AppearanceJson examples/planner-appearance.v1.json
```

The panel helper sets Qt's offscreen platform. Runtime tests still require
separate installed video/input/LSL/CSV checks. Obtain the user's opt-in before
foregrounding their live desktop for an interactive check.
