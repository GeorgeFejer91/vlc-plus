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

2026-11-04: branch `codex/windows-installer`, checkpoint `2ef68dd` plus fixes.
The complete local VLC Qt module/launcher build succeeded with GCC 16.2.0 /
Qt 5.15.19. Rust bridge tests (3/3), release DLL and Windows offscreen Qt panel
checks passed. Actual Rust linking was observed through `--print=link-args`
(MSVC linker 14.42.34433.0 locally). No installed-player pass is claimed yet.

Independent review found packaging/proof gaps. Fixes now stage the pinned VLC
ZIP freshly; include qoffscreen and prove its loaded path; guard existing
product registrations and owned uninstall; check CSV close, naming, duration
and cadence; preserve per-package runtime notices and matching signed MSYS2
and locked Rust sources. Source collection and installed runtime tests still
need to run. Inspect current Git diff and `build/windows-build.log` on resume.

Local Inno compiler installation was rejected (`blocked by policy`); use the
preinstalled compiler on the owned GitHub Windows runner for installer output.
An initial separate compiler artifact was removed, and that job was dropped.
Only player/installer/source/evidence artifacts belong in the publication.
The first installer CI run `41917153520` was cancelled as superseded by build
fixes, not counted as a pass. Next: checkpoint fixes, stage/test the actual
payload, run corrected installer CI, fix failures and publish verified bytes.
The parent Research source is unchanged. Common Planner-master execution,
physical input/display onset and research qualification remain open scopes.
