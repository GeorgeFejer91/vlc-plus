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

2026-11-04: branch `codex/windows-installer`. Full native build succeeded with
GCC 16.2.0 / Qt 5.15.19; Rust tests 3/3, release DLL and Windows offscreen Qt
panel passed. Actual MSVC linker 14.42.34433.0 was captured locally. Source CI
passed `a7ce6f2`; installer CI failed on missing generated-source directories,
now fixed. Source signatures use the selected MSYS2 GPG trust database with
both valid-signature and full/ultimate-trust gates (pacman-key's --init omits a
lock-never option its verify wrapper requires). ICU licenses live under share/icu.

Independent review packaging findings are being closed with fresh pinned-ZIP
staging, full manifests, qoffscreen load-path evidence, guarded/owned installer
testing and exact runtime notices + matching signed sources. Local source
collection is running. Native offscreen playback decoded video, published LSL
and wrote CSV, but caught a shutdown crash in flubber_bridge.dll_unloaded.
PreventUnloadHint is the minimal runtime lifetime fix; rebuild and rerun pending.
Manual Stop can produce End because VLC 3 uses END_S for teardown; document
that existing marker limitation and never call it natural completion proof.

Local Inno installation was rejected by automatic policy. Installer generation
and install/uninstall tests use the owned GitHub Windows runner's preinstalled
compiler. The unrelated compiler artifact was deleted and its job removed.
No installed-player or installer pass/release is claimed yet. Next: verify the
shutdown fix, checkpoint and run corrected CI, resolve actual failures, publish
exact tested installer/source/evidence, then update this checklist.
The parent Research source is unchanged. Common Planner-master execution,
physical input/display onset and research qualification remain open scopes.
