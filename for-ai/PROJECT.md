# VLC+ project contract

## Purpose and ownership

VLC+ is a standalone Windows VLC player for continuous affect input, native
feedback, outbound LSL and video-local CSV backup. The Qt panel owns
presentation/input/settings. The Rust bridge owns an independent 480 Hz
affect outlet, a 30 Hz playback and CSV schedule, and the CSV writer.

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

## Goal: keyboard feedback and LSL responsiveness

- [x] Apply the first arrow step on press and repeat held directions on one
  clock. Orthogonal keys produce normalized diagonal movement. Opposite
  directions cancel and release, focus loss and mode changes clear holds.
- [x] Remove the GUI polling delay from affect publication with a direct
  atomic pair handoff. Publish at nominal 480 Hz and keep the video CSV
  schedule at 30 Hz.
- [x] Measure software latency and verify playback and independent recording.
  The bridge loopback p95 fell from 33.81 to 2.90 ms over 48 changes.
  In the large offscreen panel benchmark, caching the grid field reduced
  dispatch-to-paint p95 from 20.82 to 1.84 ms. The final face and Flubber
  results were 0.95 and 13.41 ms respectively in the same 48-trial benchmark.
  The staged Windows player decoded 207 frames and passed pause, resume,
  replay, markers and 30 Hz CSV. LabRecorder XDF contained 3,119 affect samples
  at 478.33 Hz and Start, End and Stop. CSV measured 30.00 Hz.

One initial 480 Hz trial observed 451.43 Hz. The final clock skips elapsed
slots while preserving the next deadline phase, avoiding an extra full-period
wait after a stall. Its final focused run measured 479.07 Hz.

The Windows stage combines manifest-verified runtime files with the rebuilt
Qt module and Rust bridge. The timed endpoints are software dispatch and paint
return or localhost sample receipt. Physical keyboard and display latency
require hardware measurement. Local receipts are under `build/latency-playback-final/`
and `build/latency-xdf-final/`. The next release step is packaging and installer
verification when requested.

## Previous follow-up: Recorder scores and integrated cone

The desktop Recorder has one orange **Start VLC and record** button whose SVG
cone merges into the button silhouette; there is no separate icon tile.
It launches the configured video through the existing Rust command, subscribes
recording, then starts playback. The separate paused-player button and its
frontend handler were removed at the user's request. Native commands, sampling,
recording formats and remote authority remain unchanged. Pretext measures the
label separately from its cone; the default setup width follows text enlargement.

At the user's confirmed target, the Recorder signal viewer now displays the
latest received numeric valence and arousal scores for selected streams with
those channel labels. Label-based lookup preserves swapped channel order;
each stream keeps its own readout and supplied units. Display rounding is two
decimal places. Missing/non-finite values and disconnected streams show dashes;
deselection removes the readout. Hiding individual plots does not hide scores.
No sampling, XDF data, remote payload or native player UI changed.

Verified locally: 24 JavaScript tests, generated-source checks, source/provenance
and context checks; headless Edge exercises the single action, keyboard activation,
active-state locks, failure recovery, SVG geometry, received score updates,
waiting/disconnected/deselection states and label geometry at 320–1920px, plus
200% text/spacing with the session form visible. Native playback/XDF was not rerun for this UI
change; the prior installed evidence below applies to its named release only.
Production build `pnpm tauri build --no-bundle -- --locked` passes. Executable:
`recorder/src-tauri/target/release/remote-lsl-recorder.exe`, SHA-256
`3c34f9af9c9be46fe7a80668dccebd28fe93ed26b77062991cff07562f34dc99`.
Work is committed locally on
`codex/vlc-record-play-cone`, based on clean `990ba6d`. No push, installation or
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

## VLC+ 0.3.4 and Recorder 0.1.9

The [published experimental release](https://github.com/GeorgeFejer91/vlc-plus/releases/tag/v0.3.4-experimental.1)
contains the held arrow chords, the 480 Hz affect LSL outlet and 30 Hz CSV,
the orange combined playback and recording control, and the live valence and
arousal readouts. Source `3264b0919244e2f3590b63073c9fbdfbb38363f5` passed
[main checks](https://github.com/GeorgeFejer91/vlc-plus/actions/runs/42422838245)
and [Windows standalone and joint installer gates](https://github.com/GeorgeFejer91/vlc-plus/actions/runs/42422840156).
The hosted player probe observed 473.53 Hz and decoded 188 video frames.
Public GitHub asset digests match the tested receipts and checksum inventory.

## Recorder-controlled VLC+ interactions

The next matching VLC+ and Recorder builds add `VLC_Action ` JSON messages to
the existing marker outlet. Recorder requires a Windows appearance-lock
acknowledgement for the exact fullscreen player it launches, waits for
subscriptions before playback, and releases the lock on stop or exit.

The local staged playback probe decoded 207 frames and captured 22 action
markers alongside the original playback markers at 480.01 Hz affect cadence.
Native Recorder-core and LabRecorder verification produced a finalized XDF
with 1,208 affect samples and 11 action markers. It observed monitor-sized
fullscreen, a blocked appearance change during recording, and a successful
change after stop. Windows Security held the foreground during the probe.

## Feedback settings consolidation — 2026-11-08

- [x] Use Ctrl+Shift+S or Flubber → Settings for tuning. Ten documented
  shortcut groups remain for rating, playback feedback, layout and snapshots.
- [x] Set Flubber colors/transparency, choose Tiles or Continuous, and enter
  full-axis hold time with the tap step independent. The duration control
  follows the Experiment Planner P5 preview at research source `06e017d`.
  Grid style is presentation only; no Planner response or acquisition policy
  is imported.
- [x] Save grid style and hold time in v4 snapshots; import v2/v3 and the
  unchanged Planner appearance-v1. Retain the Recorder appearance lock.
- [x] Verify the Windows Qt dialog and native key path, settings round trips,
  source/patch provenance, and rendered Recorder and site help. The offscreen
  Qt test failed on the old dialog and passed with the consolidated controls.
  The local portable stage in `build/feedback-settings-runtime/` decoded
  191 frames; the independent probe observed 477.59 Hz, 168/31 CSV rows,
  24 interaction markers and the expected playback markers. Its evidence
  is in `build/feedback-settings-playback/`.

This is a local candidate on `codex/vlc-feedback-settings` based on
`0311d1c`. Native settings and hold-time arithmetic were tested without
foregrounding the user desktop. Physical key timing, a fresh installer and
publication are outside this pass. The broader Planner/Runner compatibility
goal stays open.

## Representation settings popups — 2026-11-08

- [x] Ctrl+Shift+1/2/3 and three Flubber menu actions open separate native
  settings for Flubber, 2D grid and Face morph. Ctrl+Shift+S still opens
  all settings, including shared input, panel height and JSON.
- [x] Render each draft in the current VLC+ window before committing.
  Cancel restores the previous display; OK saves and keeps the selected
  representation. Shared Flubber/grid colors and transparency are labelled.
  The Recorder lock, stored v4 format and acquisition path are preserved.
- [x] Windows Qt and native key checks, rendered popup size checks, source
  checks and rendered Recorder/site guides passed. The final local runtime
  decoded 207 synthetic frames, observed 479.89 Hz affect LSL, recorded
  169/31 CSV rows and emitted 24 interaction markers and the expected
  playback markers. Evidence: `build/representation-popups-final/`.

Local portable stage: `build/representation-popups-runtime/`. Branch
`codex/vlc-feedback-settings` continues from `59cf17d`. No foreground
physical input test, installation or publication was performed.

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
