# VLC+

VLC+ is a standalone experimental Windows VLC player for continuous
valence/arousal ratings. It keeps VLC's menus, playlist and transport controls
and adds a native feedback panel with Classic Flubber, an affect grid and
synthetic photo-face feedback. The three layers can be shown together.

The player was extracted from Affect Research commit
`16106a70b57136c7243e85a5a05d83b0789bdd2c`. The standalone Windows
installer candidate is version 0.3.1. This repository has its own source, build inputs and agent guidance.
Installer readiness and exact tested scope are recorded in
[verification](for-ai/VERIFICATION.md). This is experimental software; no
research qualification or physical display-onset guarantee is claimed.

## Use and recording

The player opens original media through VLC 3.0.20. Mouse and arrow input
control valence/arousal. **Flubber → Settings…** edits appearance, feedback
layers, input and layout; hotkeys and local RC commands are described in the
[native player guide](vlc-qt/README.md).

The Rust bridge publishes `VLC_Flubber_Affect` and `VLC_Flubber_Markers` LSL
outlets on a nominal independent 30 Hz schedule. Each played video receives a
unique `time_s,valence,arousal` CSV in its writable source folder. CSV time is
not a measured display-onset or decoded-frame timestamp. A separate recorder,
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
reads/writes its own `vlc-feedback-settings/v2` settings. A complete Planner
master JSON, language route, questionnaire protocol or experiment schedule
cannot yet be executed here. The formats are distinct and must not be renamed
or silently converted. See [JSON formats](docs/experiment-formats.md) and
[Planner integration](for-ai/PLANNER-INTEGRATION.md).

[Example appearance JSON](examples/planner-appearance.v1.json) was generated
using the original Planner exporter at the pinned extraction commit.

## Build and checks

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

The isolated Qt panel test can run offscreen. Fresh installed playback,
physical input, codecs, timing, shutdown and independent recording checks
remain separate gates in [verification](for-ai/VERIFICATION.md).

## Source and licenses

See [provenance](docs/provenance.md), the [per-file extraction manifest](docs/source-extraction.json),
[third-party notices](docs/third-party.md), [GPL text](LICENSE) and
[retained face-asset notice](assets/face/NOTICE.md). VLC's native modifications
and Rust bridge declare GPL-2.0-or-later. Retained BSD and asset attributions
remain in force for their respective material.

For development, start at [AGENTS.md](AGENTS.md).
