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

- [x] Establish the native toolchain and one repeatable pinned-VLC Qt/launcher
  build. Record actual compiler/Qt/dependency versions and archive hashes.
- [x] Build/test the Rust bridge and Windows Qt panel; retain strict settings
  rejection and all three feedback modes without changing research semantics.
- [x] Stage a complete independently runnable player with notices, matching
  source and hashes; compile the standalone installer without administrator
  requirements. Version the first standalone candidate explicitly.
- [x] Install the exact candidate in a fresh test directory and verify its
  manifest, actual decoded video, native feedback/controls, LSL/markers, CSV
  naming/timing and bounded shutdown. Distinguish synthetic input/offscreen
  evidence from visible/physical-desktop checks.
- [x] Verify install/uninstall behavior and rerun the applicable focused/CI
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

2026-11-04: implementation merged in PR #3 (`627cb3e`). Exact installer source
is `d2588cad1c2e0ef385e7b0eaf9b2f874efe3d7da`. Windows installer CI
[41924794994](https://github.com/GeorgeFejer91/vlc-plus/actions/runs/41924794994)
passed: install/uninstall exit 0, package/source integrity, actual H.264 decode,
29.9991 Hz LSL, separate CSVs (169/31 rows), portable settings and package-local
Qt loading, clean shutdown in 151 ms. All Windows/Qt source checks pass.
Independent review findings were addressed; PreventUnloadHint fixes the
observed unloaded Rust/LSL DLL shutdown crash while still joining/draining
owned workers. The archived Rust sources resolve/check offline with the vendor
bundle. A separate local staged check with pylsl 1.18.2 / liblsl 1.17.x received
248 samples at 30.0006 Hz and the playback markers; that receipt identifies its
own stage commit/hash, not the CI installer.

The candidate artifact is being downloaded and hash-checked before release.
Next: publish the tested experimental installer/source/receipts, verify public
download bytes, complete this milestone and merge the readiness documentation.
The parent Research source is unchanged. Common Planner-master execution,
physical input/display onset and research qualification remain open scopes.
