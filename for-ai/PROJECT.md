# VLC+ project contract

## Purpose and ownership

VLC+ is a standalone Windows VLC player for continuous affect input, native
feedback, outbound LSL and video-local CSV backup. The Qt panel owns
presentation/input/settings; the Rust bridge owns its independent 30 Hz LSL
worker and CSV writer. Sampling is not owned by a rendering callback.

Planner authors studies; Runner executes full protocols; VLC+ is an independent
playback target. Flubbercorder and Remote LSL Recorder are separate recorders.
No recorder, Planner editor, browser frontend or replacement media engine is
included here.

## Overall product goal

Share explicitly versioned experiment contracts with Planner and Runner, so
one authored JSON plus assets can be used on either capable execution target.
This compatibility goal stays **open**. Full master execution, questionnaires,
variant/participant selection and recovery are not implemented in VLC+.
See [Planner integration](PLANNER-INTEGRATION.md).

## Active milestone: Windows installer readiness

Goal: deliver a usable standalone Windows x64 installer for the existing VLC+
player, with exact source/dependency provenance and fresh installed evidence.
The source extraction is complete (PR #1); its checks alone did not establish
installed-player readiness. This checklist is the sole plan/state owner.

- [ ] Establish the native toolchain and one repeatable pinned-VLC Qt/launcher
  build. Record actual compiler/Qt/dependency versions and archive hashes.
- [ ] Build/test the Rust bridge and Windows Qt panel; retain strict settings
  rejection and all three feedback modes without changing research semantics.
- [ ] Stage a complete independently runnable player with notices, matching
  source and hashes; compile the standalone installer without administrator
  requirements. Version the first standalone candidate explicitly.
- [ ] Install the exact candidate in a fresh test directory and verify its
  manifest, actual decoded video, native feedback/controls, LSL/markers, CSV
  naming/timing and bounded shutdown. Distinguish synthetic input/offscreen
  evidence from visible/physical-desktop checks.
- [ ] Verify install/uninstall behavior and rerun the applicable focused/CI
  checks. Obtain independent review and fix important findings.
- [ ] Publish the tested installer, source/provenance and checksums; verify
  downloadable bytes and record exact branch/commit/release evidence.

No full Planner-master consumer is added in this milestone. The common-recipe
product goal remains open. Do not equate usable experimental Windows playback
with scientific timing, physical input or research qualification.

## Architecture

| Path | Responsibility |
| --- | --- |
| `vlc-qt/` | VLC patch, Qt panel, settings/input, C ABI loader, panel check and local RC helper |
| `vlc-qt/bridge/` | Rust LSL worker, marker lifecycle and CSV writer |
| `assets/face/` | Synthetic atlases, manifests and retained notices |
| `package-stock-vlc.ps1`, `installer-stock-vlc.iss` | Windows staging and installer |
| `examples/` | Planner-generated interchange example |
| `docs/` | Build, formats, provenance and third-party documentation |
| `scripts/` | Standalone source validation |

## Invariants

- Preserve input, acquisition, markers, pause, CSV and appearance semantics
  during extraction. No sampling or physical display-onset claim from paint.
- Explicit schema/version dispatch and capability rejection before execution;
  no silently skipped questionnaires, reordered videos, changed sampling,
  substituted bindings, language fallback or source-JSON mutation.
- Independent projects share contracts, not imported mutable editor state.
  One versioned field has one meaning across readers.
- Preserve attributions, source locks and partial recording evidence. Never
  publish participant data, secrets, films or generated packages accidentally.
- Historical receipts qualify only their named upstream artifact and scope.
  This extraction does not reactivate or qualify features in Affect Research.
- The GitHub repository is public by explicit user authorization on 2026-11-04.
  Review source, history and assets before any further publication.

## Session handoff

2026-11-04: active branch `codex/windows-installer`, baseline `247c4e5`.
An isolated MSYS2 toolchain now exists under `build/toolchain/msys64`: GCC
16.2.0 and Qt 5.15.19. Official MSYS2/VLC downloads passed their SHA-256 locks.
Native configure and libcompat/libvlccore/libvlc builds succeeded. The first
Qt-module build exposed missing generated UI headers; the build now explicitly
requests upstream Qt generated sources before linking the module. Full native
build is still in progress; inspect `build/windows-build.log` and current Git
state before resuming. No installer has passed a fresh installed run yet.

The new repeatable build entry is `build-windows.ps1`; installed checks use a
synthetic six-second H.264 fixture, real player LSL/decode statistics and CSV,
with Qt offscreen and a private portable settings profile. The independent
probe compiles, but has not yet run against a completed package. No rendering,
physical-input or scientific qualification claim is made from this progress.

Automatic approval review rejected local Inno Setup compiler installation
(`blocked by policy`). The safe path is the preinstalled Inno Setup compiler
on GitHub's Windows-2025 runner. `windows-installer.yml` builds, installs,
checks and uninstalls a fresh candidate there, then uploads artifacts.
Next: finish the Qt/launcher build, checkpoint reviewed build changes and run
that workflow; investigate real failures before publishing the installer.
Affect Research parent source remains unchanged; shared Planner-master intake
and research qualification remain separate open milestones.
