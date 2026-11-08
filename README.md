# VLC+

VLC+ is a standalone experimental Windows VLC player for continuous
valence/arousal ratings. It keeps VLC's menus, playlist and transport controls
and adds a native feedback panel with Classic Flubber, an affect grid and
synthetic photo-face feedback. Exactly one representation is visible at a time.

The complete [Remote LSL Recorder](recorder/README.md) is included in this
repository: Rust/Tauri desktop source, live viewer, browser controller, locked
dependencies and pinned recording-engine setup. It needs no sibling checkout.
The [VLC+ live page](https://georgefejer91.github.io/vlc-plus/) contains the
hotkey list and [remote LSL controller](https://georgefejer91.github.io/vlc-plus/remote/).

The player was extracted from Affect Research commit
`16106a70b57136c7243e85a5a05d83b0789bdd2c`. The standalone Windows
installer is version 0.3.4 (experimental). This repository has its own source, build inputs and agent guidance.
Installer readiness and exact tested scope are recorded in
[verification](for-ai/VERIFICATION.md). This is experimental software; no
research qualification or physical display-onset guarantee is claimed.

## Install

Windows x64 installer choices:

1. [VLC+ and Remote LSL Recorder together (0.3.4 / 0.1.9)](https://github.com/GeorgeFejer91/vlc-plus/releases/download/v0.3.4-experimental.1/01_VLCPlus_and_LSL_Recorder_Setup_0.3.4_x64.exe), with one desktop shortcut that opens both apps.
2. [VLC+ only (0.3.4)](https://github.com/GeorgeFejer91/vlc-plus/releases/download/v0.3.4-experimental.1/02_VLCPlus_0.3.4_Setup_x64.exe).
3. [Remote LSL Recorder only (0.1.9)](https://github.com/GeorgeFejer91/vlc-plus/releases/download/v0.3.4-experimental.1/03_Remote_LSL_Recorder_0.1.9_Setup_x64.exe).

Stop/save recordings and close both apps before installing or updating. Run
the installer for your own account. The joint shortcut opens both apps;
select streams and press Record in Recorder when ready. Recording and phone
control are not enabled by the shortcut. Each app remains separately
uninstallable; removing the joint launcher leaves both programs and data in
place. See the [joint packaging guide](installer-suite/README.md).

The installers are unsigned. VLC+ uses its own settings profile and does not register
file associations. Matching sources, SHA-256 checksums and build/installed
verification receipts are attached to the release.

## Use and recording

The player opens original media through VLC 3.0.20. Mouse and arrow input
control valence/arousal. **Flubber → Settings…** edits appearance, feedback
layers, input and layout; hotkeys and local RC commands are described in the
[native player guide](vlc-qt/README.md).

Use **Alt+W/A/S/D** to move the selected feedback, **Alt+plus/minus** to resize
it, and **Alt+J** to save a JSON snapshot. The last successful snapshot loads
automatically on the next launch. These layout shortcuts work with mouse or
arrow input and leave the affect rating unchanged.

In this build, **Ctrl+Shift+S** opens **Flubber Settings** from the player or
video. Use **Flubber** and **Colors** for transparency and colors, **2D grid**
to choose Tiles or Continuous, and **Input & display** to set how many
milliseconds a held arrow takes to cross an affect axis. Settings keep the
tap step separate from hold time. The Recorder's **VLC+ setup → Hotkeys**
and the [player guide](vlc-qt/README.md) list the remaining shortcuts.

Alt+J saves a full `vlc-feedback-settings/v4` snapshot including the grid
style and hold time. Earlier v2 and v3 files can still be imported. Explicit
session overrides retain their documented precedence.

Press **Alt+F** to toggle affect arrows. Left/Right changes valence and Up/Down
changes arousal. Hold Left and Up together to move diagonally. The first step
happens on key press, and VLC+ repeats the combined direction while the keys
are held. Opposite directions cancel. Press Alt+F again to restore normal
VLC arrow controls. Affect can still be adjusted while paused.

The included [Great Dictator study clip](examples/media/dictator-3-study.mp4)
is in `examples/media/` in the repository and installed application. Its
[media notice](examples/media/NOTICE.md) records provenance, exact identity and
redistribution permission separately from the software license.

The Rust bridge publishes `VLC_Flubber_Affect` at a nominal 480 Hz. It receives
each changed affect pair directly from Qt and publishes independently of
rendering. The `VLC_Flubber_Markers` outlet continues to carry playback events.
CSV sampling stays at 30 Hz while the video plays. Each played video receives a
unique `time_s,valence,arousal` CSV in its writable source folder. CSV time is
not a measured display-onset or decoded-frame timestamp. Acquisition starts
after VLC reports its first positive media time; the first CSV row can be
about 0.4 seconds into a clip, rather than at time zero. A separate recorder,
such as Flubbercorder or Remote LSL Recorder, can subscribe and write XDF;
VLC+ itself does not write XDF.

**Marker limit:** `End` reports VLC 3 input teardown, which can also occur on
manual Stop/Next. It does not prove natural completion. `Stop` closes the CSV
session. Do not infer protocol completion from `End` alone.

## Planner and Runner compatibility

The intended workflow is **Experiment Planner → the same versioned experiment
JSON and assets → Experiment Runner or VLC+**, with explicit capability checks.
That complete-recipe workflow remains to be implemented in VLC+.

Today VLC+ imports Planner's standalone `vlc-flubber-appearance/v1` JSON and
reads/writes its own `vlc-feedback-settings/v4` settings, with v2 and v3 imports supported. A complete Planner
master JSON, language route, questionnaire protocol or experiment schedule
cannot yet be executed here. The formats are distinct and must not be renamed
or silently converted. See [JSON formats](docs/experiment-formats.md) and
[Planner integration](for-ai/PLANNER-INTEGRATION.md).

[Example appearance JSON](examples/planner-appearance.v1.json) was generated
using the original Planner exporter at the pinned extraction commit.

## Build and checks

Build/check the included Recorder from this checkout with
`./build-recorder.ps1` (or `-CheckOnly`); it installs the frozen pnpm dependencies,
prepares the hash-pinned official LabRecorder engine, runs source and rendered
checks, and builds the Windows NSIS installer. Node 24, pnpm and the MSVC Rust
toolchain are required. The runtime bundles its engine and needs no separate
source repository or Node installation.

Build the website and controller with `node scripts/build-site.mjs`; output is
`build/site/`. Its Pretext runtime and fonts come from the included Recorder's
locked, vendored assets. GitHub Pages publishes only that public output.

- Windows x64; MSVC Rust toolchain for the bridge.
- The pinned official VLC 3.0.20 source and Windows runtime.
- MSYS2 MinGW64 GCC and matching Qt 5; inherited verified toolchain was
  GCC 16.2.0 / Qt 5.15.19. Revalidate any different toolchain.
- Inno Setup 6 for the installer (GitHub Windows runners provide it).
- PowerShell 7 and Python 3.12 or newer for build/package verification.

Use `./build-windows.ps1 -MsysRoot <MSYS2 root> -InnoCompiler <ISCC.exe>`;
follow the [build and packaging guide](docs/stock-vlc-integration.md). The
[Windows installer workflow](.github/workflows/windows-installer.yml) also
performs a fresh install, real decoded-video/LSL/CSV check and uninstall in its
own clean Windows account. It does not republish compiler tools.

VLC+ uses a separate executable-local portable settings profile. Matching
project, Rust and signed MSYS2 source archives accompany the installer as a
separate source bundle. The build pins VLC inputs and records the actual
compiler/dependency identities; byte-identical binary reproducibility is not
claimed.
These checks do not need an Affect Research checkout:

```powershell
python scripts/check-source.py
cargo test --locked --manifest-path vlc-qt/bridge/Cargo.toml
powershell -NoProfile -File for-ai/scripts/check-context.ps1 -ProjectRoot .
```

The fresh installed H.264/LSL/CSV and uninstall checks passed offscreen.
Physical input, other codecs, display onset and scientific timing remain
separate scopes in [verification](for-ai/VERIFICATION.md).

## Source and licenses

See [provenance](docs/provenance.md), the [per-file extraction manifest](docs/source-extraction.json),
[third-party notices](docs/third-party.md), [GPL text](LICENSE) and
[retained face-asset notice](assets/face/NOTICE.md). VLC's native modifications
and Rust bridge declare GPL-2.0-or-later. Retained BSD and asset attributions
remain in force for their respective material.

For development, start at [AGENTS.md](AGENTS.md).
