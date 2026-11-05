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

## Active milestone: Alt+F affect arrows and bundled study clip

- [x] Trace existing keyboard routing; reproduce the absent Alt+F toggle with
  a failing Windows panel check (exit 23), then pass the focused regression.
- [x] Reuse the saved arrows/mouse mode and step size. Claim plain arrows at
  ShortcutOverride before VLC commands, including while paused; never change
  affect on shortcut probes; preserve the existing supported-input gate.
- [x] Include the exact Great Dictator study clip with identity/provenance and
  the user's 2026-11-05 explicit public-redistribution permission confirmation.
- [ ] Pass the actual Qt shortcut-map integration and native/installer checks.
- [ ] Review, merge and ship 0.3.2 with the sample in source and installer;
  update the installed application and verify the requested hotkey scope.

## Completed milestone: experimental Windows installer readiness

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
- [x] Publish the tested installer, source/provenance and checksums; verify
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
  The permission-scoped sample named in `examples/media/NOTICE.md` is explicitly
  authorized for this release; this is not a general media-publication grant.
- Historical receipts qualify only their named upstream artifact and scope.
  This extraction does not reactivate or qualify features in Affect Research.
- The GitHub repository is public by explicit user authorization on 2026-11-04.
  Review source, history and assets before any further publication.

## Session handoff

2026-11-05: working on `codex/alt-f-rating` from clean main `09744b1`.
The user requested Alt+F to toggle normal VLC arrow commands vs valence/arousal
and explicitly confirmed public redistribution permission for the exact Great
Dictator study clip. Its identity/notice is in `examples/media/NOTICE.md`.
The clip SHA-256 is `b5327e7465ec92a4c93f3236a1ebab4556cdf508e24eafe6c593eac1e13afd49`.

The focused Windows panel check failed at absent Alt+F (exit 23), then passed
with the new input dispatcher. It preserves existing step/mappings and guards,
claims ShortcutOverride before VLC, preserves the supported-input callback
(including paused adjustments), and ignores repeated Alt+F presses. Real Qt shortcut-map integration
runs in Linux CI; Windows offscreen has no foreground HWND. The modified native Qt DLL/launcher built locally. An existing Rust test
initially selected the already-running player by stream name; source-ID
isolation corrected the test, and all 3 tests now pass while that player stays
open. The Windows panel check passes; Linux shortcut-map CI is next. Next: verify CI/review, merge, build/test
and publish the 0.3.2 installer with media, then update the installed copy.

Prior 0.3.1 installer milestone remains verified/public at its exact release
and source `d2588cad1c2e0ef385e7b0eaf9b2f874efe3d7da`; its receipts do not qualify
this new candidate. Full Planner-master execution and scientific qualification
remain open. The parent Research source is unchanged.
