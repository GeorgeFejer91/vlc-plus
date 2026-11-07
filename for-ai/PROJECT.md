# VLC+ project contract

## Purpose and ownership

VLC+ is a standalone Windows VLC player for continuous affect input, native
feedback, outbound LSL and video-local CSV backup. The Qt panel owns
presentation/input/settings; the Rust bridge owns its independent 30 Hz LSL
worker and CSV writer. Sampling is not owned by a rendering callback.

Planner authors studies; Runner executes full protocols; VLC+ is an independent
playback target. Under the user's 2026-11-06 instruction, this repository now
includes the self-contained Remote LSL Recorder source, UI, browser controller,
locks and notices in `recorder/`. Its Rust core retains recording, XDF and
remote-control authority; VLC remains a separate Qt process. No Planner editor
or replacement media engine is added. `installer-suite/` owns joint packaging
and shortcuts. `site/` is the VLC+ live page; its build includes the local
Recorder controller and the shared hotkey list, with no sibling checkout.

## Overall product goal

Share explicitly versioned experiment contracts with Planner and Runner, so
one authored JSON plus assets can be used on either capable execution target.
This compatibility goal stays **open**. Full master execution, questionnaires,
variant/participant selection and recovery are not implemented in VLC+.
See [Planner integration](PLANNER-INTEGRATION.md).

## Current follow-up: cone playback/recording button

The desktop Recorder has one orange **Start VLC and record** cone button.
It launches the configured video through the existing Rust command, subscribes
recording, then starts playback. The separate paused-player button and its
frontend handler were removed at the user's request. Native commands, sampling,
recording formats and remote authority remain unchanged. Pretext measures the
label separately from its cone; the default setup width follows text enlargement.

Verified locally: 23 JavaScript tests, generated-source checks, source/provenance
and context checks; headless Edge exercises the single action, keyboard activation,
active-state locks, failure recovery, cone loading and label geometry at
320–1920px, plus 200% text/spacing with the session form visible. The site build
and rendered checks pass. Native playback/XDF was not rerun for this UI
change; the prior installed evidence below applies to its named release only.
Production build `pnpm tauri build --no-bundle -- --locked` passes. Executable:
`recorder/src-tauri/target/release/remote-lsl-recorder.exe`, SHA-256
`b88f61d1f527890c31a267d381f5dc7ddd24ccf281bafee7bd1f57f28aded223`.
Work is committed locally on
`codex/vlc-record-play-cone`, based on clean `70e83c8`. No push, installation or
release was performed. The installed Recorder was open and remains untouched;
applying the rebuilt executable requires a separate handoff after it closes.
The overall compatibility goal remains open.

## Milestone: keyboard, joint handoff and recording qualification

Latest checkpoint: **2026-11-07**. Installed acceptance is complete for VLC+
0.3.3 and the self-contained Recorder 0.1.8. [Exact-byte promotion
42324807124](https://github.com/GeorgeFejer91/vlc-plus/actions/runs/42324807124)
passed. All three [public installers](https://github.com/GeorgeFejer91/vlc-plus/releases/tag/v0.3.3-experimental.1)
have GitHub asset SHA-256 digests matching the retained candidate receipt;
each public download returned an executable header. The broader Planner/Runner
compatibility goal above stays open.

- [x] Include Recorder source, Rust authority, UI, controller, locks and notices
  in `recorder/`, with declared upstream adaptations and no sibling requirement.
- [x] Alt+F reserves every VLC arrow variant for affect control; plain arrows
  change valence/arousal while modified layout arrows remain reserved. Turning
  affect arrows off restores VLC seek and volume behavior.
- [x] Alt+G cycles exactly one of Flubber, grid and face morph. Alt+H cycles all
  nine characters. Alt+W/A/S/D moves feedback and Alt+plus/minus resizes it.
- [x] Provide 64 shortcut groups for all 60 detailed settings plus affect step;
  preserve numeric bounds, driver priority, modal/text editing and repeat guards.
- [x] Alt+J atomically saves a complete `vlc-feedback-settings/v3` JSON and
  restores the last successful snapshot on launch. v2/appearance-v1 imports and
  explicit subsequent overrides keep their precedence.
- [x] Pass independent review, exact-main source checks, full standalone/joint
  installer gates and ordinary-user installed controls/data qualification.
- [x] Deploy the 64-group live guide and self-contained controller, with rendered
  320–1920px and enlarged-text checks. Recorder help uses bounded category pages.
- [x] Promote the exact tested bytes and verify all three public installer
  digests and download links. README and the live-page source now list the
  verified release; later documentation commits do not change its installer.

### Exact artifact and acceptance evidence

Application source: `2713d423227206430051ffd0fd52608d1e56edd8`.
[Main source checks](https://github.com/GeorgeFejer91/vlc-plus/actions/runs/42320783208)
pass all four Windows, native/GUI, Linux and Recorder Rust/UI jobs.
[Full installer suite](https://github.com/GeorgeFejer91/vlc-plus/actions/runs/42320785383)
passes exact standalone installation, video decode, LSL/CSV, Recorder payload
identity, joint shortcut, duplicate/reuse/open-app refusal and safe uninstall.
Publication was disabled for that build.

The downloaded and installed joint candidate is 1,029,272,907 bytes, SHA-256
`1695bd9b269ceaa530bcddf89acf0ffe93492b1b72e784eb803ed02370b1088f`.
Ordinary-user installation verifies 701 immutable VLC files and exact Recorder,
LSL engine and launcher hashes. The actual Explorer desktop shortcut opened
both ordinary-user applications and both live streams, with one face visible.

The real Recorder button -> Rust -> VLC -> LSL/XDF/CSV test passes with Windows
injected keys. Independent pyxdf/CSV inspection found **1,336 affect samples at
29.999989 Hz**, **987 CSV rows**, finite bounded values, monotonic clocks,
positive/negative values on both axes and Start/Pause/Resume/Stop markers.
Paused time and volume stayed unchanged during affect-arrow input. Busy-port
refusal launched no player, sent no quit to the foreign listener and retained
the original workspace for retry.

Installed controls pass all exclusive modes, all nine faces, layout and size,
representative appearance/mapping/grid controls, German GUI key parity, all
eight color Cancel paths, exact full v3 restoration after an unsaved change and
restart, and restored normal VLC arrows. All three original profile/workspace
files were restored byte-for-byte; test apps are closed. The conflicting global
Alt+S screenshot action was temporarily isolated, restored to On and returned
to its tray; Start with Windows stayed On. The guide documents the existing
Ctrl+Alt arrow layout alternative when screenshots reserve Alt+S.

Windows GUI OEM regression first failed at exit 151 in run 42318768072.
PR16 reuses native physical-key normalization after dedicated matching; bracket,
resize claim/press, repeat and wrong-modifier regressions now pass on Windows.
Do not promote superseded run 42277964086: it has the earlier GUI defect.
Local QA also waits for packaged UI readiness and modal closure, and uses an
exact PID/loopback TCP ownership probe after PowerShell lookup timeouts. These
ignored harness corrections do not modify the installed product.

Evidence is synthetic and injected-input qualification. Physical keyboard,
phone/device, display-onset and scientific timing qualification are separate.
Current receipts remain in ignored `build/` and Recorder `.for-ai-local/`;
never commit participant data or private profiles. Git history retains the
superseded handoffs and earlier release evidence.

### Storage housekeeping

User-authorized cleanup relocated 34 cold/damaged worktrees intact to the
active Proton sync folder, preserved nine WIP snapshots with independently
verified recovery bundles, cleaned eleven worktrees and recovered about 16 GB
from rebuildable caches. Primary repositories, active shared-index trees and
recordings were preserved. **Cloud upload remains pending**; moving within C:
did not itself free the archived payload's physical bytes.

## Architecture

| Path | Responsibility |
| --- | --- |
| `vlc-qt/` | VLC patch, Qt panel, settings/input, C ABI loader, panel check and local RC helper |
| `vlc-qt/bridge/` | Rust LSL worker, marker lifecycle and CSV writer |
| `assets/face/` | Synthetic atlases, manifests and retained notices |
| `package-stock-vlc.ps1`, `installer-stock-vlc.iss` | Windows staging and installer |
| `installer-suite/` | Pinned separate component installers, joint installer and launcher; no acquisition authority |
| `recorder/` | Embedded Rust/Tauri Recorder, viewer, browser controller and pinned recording engine setup; owns XDF and remote grants |
| `site/`, `scripts/build-site.mjs` | VLC+ live page and copied self-contained browser controller |
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
