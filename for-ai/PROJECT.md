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

## Active milestone: keyboard, joint handoff and recording qualification

### Latest status (2026-11-07)

Application source `0edb694` passed all four exact-main source checks
(42277960432) and full installer suite 42277964086. Its exact joint installer
was downloaded and SHA-256 verified as
`6a0d36b342ca898051f27fdc8c760843ff8726e9725e70979a2ed727cc8ab29a`
(1,029,268,105 bytes), installed through an ordinary-user Explorer broker and
verified against 701 immutable VLC files plus Recorder/engine/launcher hashes.
The actual desktop shortcut opened both ordinary-user apps and both LSL streams.

The real Recorder button -> Rust -> VLC -> XDF/CSV test passed with Windows
injected input. Independent pyxdf/CSV inspection found 3,169 affect samples at
29.999994 Hz, 2,487 CSV rows, positive/negative values on both axes, monotonic
clocks and Start/Pause/Resume/Stop markers. Paused time and playback volume
were preserved during affect-arrow input. Busy-port refusal also passed without
launching a player or sending quit to the foreign listener. Native controls
observed all nine faces, exclusive modes and exact v3 JSON restoration after
an unsaved adjustment and restart. These are injected-input checks, not a
physical-keyboard or scientific display-onset claim.

**Publication hold: Windows GUI OEM shortcuts.** Independent review confirmed
that Alt+F legitimately moves focus to Qt, where German OEM6 arrives as acute
and OEM4 as backslash. The native route recognizes the physical bracket keys;
the GUI legacy settings route does not. The installed check retained step 10
instead of incrementing to 11. Branch `codex/gui-oem-hotkeys` adds a failing
Windows GUI regression and makes the existing Windows CI job run the panel.
The next change must reuse native key normalization after dedicated matching,
retain digit-driver priority and all text/modal guards, then pass review and
exact-main source/installer CI. Do not promote 42277964086: a corrected
same-source installer and repeated installed qualification are required.

The earlier disk blocker is cleared. User-authorized housekeeping relocated
34 inactive/damaged checkouts (119.5 GB) intact to Proton sync, preserved nine
WIP branches with independently verified recovery bundles, cleaned eleven
worktrees and recovered about 16 GB from rebuildable caches. Primary repos,
active shared-index worktrees and recordings were preserved. Cloud upload is
still pending. Native test profile backups are restored after each closed
attempt. The screenshot utility's conflicting Alt+S was temporarily isolated
for QA and restored to On with its window returned to the tray.

The 64-group live guide remains deployed, with prior real HTTPS/82-asset and
320–1920px/enlarged-text evidence. Public download links intentionally remain
on the earlier release. Local receipts and the one-way RC command/readiness
corrections are recorded under ignored `build/`; do not treat a sent Flubber
variable command as an acknowledged RC reply. Continue from the existing
normal-user wrappers and `build/ACTIVE-QUALIFICATION.json`.

### Prior expansion evidence

The user expanded this milestone to dedicated shortcuts for all 60 detailed
settings plus rating step, a complete shared hotkey library, and hotkey-saved
JSON restored at startup. The current worktree is
`codex/dedicated-flubber-hotkeys`, based on main `5b94b16`. Native source now
uses a shared Qt/Windows matcher for appearance, grid, mapping and color keys;
Alt+F4, dialog/text entry, repeat guards and exclusive representations remain.
Snapshots use strict `vlc-feedback-settings/v3` with integer `stepPercent`;
v2/appearance-v1 imports and explicit session overrides remain supported.

Focused Windows matcher/hook and offscreen panel checks pass, including all
new action groups, numeric boundaries, mapping min/max constraints, color
Cancel/accept, v3 restore, v2 retention, invalid step and queued modal events.
The shared guide has 64 groups; Recorder help uses measured category pages
without an internal scrollbar. Independent review found and resolved German translated-symbol/VK and numpad
parity gaps, with targeted regressions. Exact source CI and installed native/XDF
qualification remain open for this expansion.

PR14's first source run 42272980335 passed Windows, native key routing and
Recorder, but Linux Qt failed the retained text-entry assertion at exit 124.
An ignored function key bubbled from the editor to a non-editor parent. A
shared focus/ancestor guard now covers direct, filtered and queued native
commands; a portable forced-bubbling test reproduced red exit 135 first.
Independent review found no remaining issue. Fresh Linux CI is required.

PR13 merged at `5b94b16`. Its four source/UI jobs pass (42255769856), and full
same-checkout suite run
[42256759568](https://github.com/GeorgeFejer91/vlc-plus/actions/runs/42256759568)
passes both exact standalone installs and the joint shortcut/reuse/refusal/
uninstall gates. Publication was disabled. Its downloaded joint candidate
SHA-256 is `9a047013008d71a086e8d45478385892d1e186f5987e8cfac69056568d6d04c8`.
Local silent installation verified 701 VLC immutable files, Recorder/engine
hashes and shortcut metadata; it does not contain the new dedicated shortcuts.

Launching that shortcut through ordinary Explorer exposed a local installation
namespace issue: Recorder was installed into Codex's private LocalCache, so the
ordinary launcher reports Recorder missing. No qualified normal-user joint
installation or full changing-input XDF/CSV result is claimed. Install the final
candidate through an ordinary-user Explorer-launched wrapper, then repeat the
actual shortcut and native recording qualification. C: is low on space; verified task-generated duplicate assets and a superseded downloaded candidate
have been removed, with hash/remote-retention receipts preserved. The user was asked to
free space while source review continues.

The earlier Windows keyboard defect was real: native video-thread GetMessage
consumed modified arrows before Qt. The scoped own-process hook and regression
were merged in PR11 with exclusive modes/all nine faces; PR12 added Alt layout
and snapshots. The installed native video-thread input path remains a required
qualification gate. Direct owned-window Win32 input is now explicitly
authorized by the user because Computer Use activation failed. Earlier privacy,
firewall and Windows Security dialogs were handled manually by the user; no
consent was automated and no new prompt is currently reported.

Pages 42250902744 passed: all 82 public assets byte-matched the then-audited
build, with the 18-group guide at 320–1920px and enlarged text. Re-deploy and
verify the expanded 64-group library after review. The publisher can promote
exact qualified artifacts using `promote_run_id`, but only from a successful
same-repository main dispatch at the identical source commit. Do not publish
the older candidate as the completed expansion.

The parallel chat has agreed to leave main/builds/installations/desktop input
untouched while this chat owns the combined qualification. Historical local
handoffs remain on preservation branches `bc50de8` and `87af57a`.

- [ ] Finish independent review and exact-source checks for 64 hotkey groups,
  all detailed fields, saved rating step and automatic v3 restore.
- [ ] Install the final exact suite in the ordinary user context; verify actual
  joint shortcut, native-video arrows, exclusive modes/all faces, appearance/
  mapping/grid/color shortcuts and saved/unsaved/restart behavior.
- [ ] Run Start VLC+ and record with injected positive/negative valence/arousal,
  independently inspect XDF and CSV, then promote exact tested bytes and verify
  public downloads and expanded live guide.

The checklist below retains earlier evidence and remains subordinate to this
latest status until the final installed qualification and publication pass.

- [x] Add Alt+W/A/S/D movement and Alt+plus/minus resizing in both input modes;
  preserve affect values and normal dialogs. Alt+J saves a settings snapshot;
  startup restores the last successful snapshot, with safe invalid/missing-file
  handling and explicit command-line settings retaining precedence. Windows
  native regressions pass, including failed bookmark persistence and retry.
- [ ] Pass native regressions, rendered and installed UI checks, independent
  review, exact source CI and installer gates; publish and install the tested
  update under the user's 2026-11-06 instruction.

- [x] Include self-contained Recorder source/UI/controller/locks/notices in
  `recorder/`, pinned to upstream `242ad90` with declared local adaptations.
  Its 22 JavaScript tests, 23 release Rust tests, controller/tab UI checks and
  pinned LabRecorder preparation pass locally without a sibling checkout.
- [ ] Pass the same-checkout component/standalone/joint build in
  `suite-source.yml`, then publish its exact tested three choices.
- [x] Publish VLC+'s own [live page](https://georgefejer91.github.io/vlc-plus/)
  with its hotkey list and local controller. Pages run 42250902744 passes.
  All 82 live assets match the audited build; actual rendered 320–1920 px,
  200% text/spacing, all 18 entries and own controller navigation pass.
- [x] Recover both saved branches and preserve the primary checkout's unfinished
  suite-shutdown changes; use the isolated `codex/complete-controls` worktree.
- [x] Alt+F reserves all arrow variants from VLC while plain arrows change
  affect; Alt+G/Alt+Shift+G cycle exactly Flubber/grid/face. Detailed settings
  release mouse capture before opening. Turning affect arrows off also restores
  Ctrl+Alt+arrows to VLC. Windows panel regression passes; Linux shortcut-map
  regression covers plain and modified restoration.
- [x] Pass Windows Qt offscreen controls/rendering, all nine face atlases,
  Rust bridge tests, source/context and launcher checks; independent review
  found and corrected capture handling. Recorder verifies RC PID ownership and
  supplies a random per-launch stream session ID: the pinned Windows LSL
  library reports hostname `unknown`, so hostname alone is not host identity.
  Ordinary launches retain their original PID-based source IDs.
- [x] Trace absent hosted RC listener to the wrong profile path; core reads
  `portable/vlcrc`, whereas Qt uses `portable/vlc/`. Keep process ownership,
  acknowledgement, real exit and uninstall assertions.
- [x] Full hosted joint install/uninstall run
  [42217605478](https://github.com/GeorgeFejer91/vlc-plus/actions/runs/42217605478)
  passes at `4b90bcba626d77d87ce53e5441f0ed8a7e651141`, with original pinned
  released components. Joint installer SHA-256:
  `65cd15e5a85f2c5e40cfdeee401c20ac194e040141b8dbf1cfa615d6661a28e1`.
  Fresh both-window shortcut, reuse, open-app refusal, graceful shutdown,
  suite-only preservation and both independent uninstalls pass. No recording.
- [ ] Qualify updated installed player plus Recorder's Start VLC+ and record
  button, emulated valence/arousal changes, unchanged paused VLC time/volume,
  terminal markers, CSV and independently read XDF. Native launch/record/play
  reached recording but the privacy dialog blocked keyboard input: FAILED,
  not qualification. The user has now handled the privacy dialog manually;
  resume the updated installed check. No consent was automated.
  The subsequent exact a44cbbe installed candidate verified all 701 immutable
  files. Embedded Recorder launched it and auto-selected the session-specific
  affect/marker streams. The keyboard phase then
  timed out behind a new Windows Firewall prompt for LabRecorderCLI. This is
  still FAILED qualification; the user has been asked to handle the prompt.
  The native script also had an independently reproduced Playwright 1.64.1
  bug: an async false predicate resolves immediately, so its Start wait was
  invalid and paused playback before the bridge's first positive media time.
  Explicit awaited-snapshot polling fixes the test. A native button diagnostic
  now saves Start/Pause/Resume/End/Stop, independently verified in XDF; it is
  not the full foreground changing-input qualification. Volume must be queried
  while playing: VLC RC refuses that command while paused.
- [ ] Merge reviewed source, publish exact tested component and joint versions,
  verify public hashes, update live download links and install the joint suite.

Reviewed source merged via PR9 at `0d84abc`; all source/UI CI checks passed.
Same-checkout suite run 42227148826 built this source; no complete
same-checkout installer qualification is claimed yet. A follow-up adds the
native-script correction and requires positive, current-child LabRecorder
subscription acknowledgements as well as exact XDF headers before playback.
The pinned engine can write a header even after an open_stream timeout. Its
failure regression covers missing, duplicate and stale acknowledgements;
23 Rust tests and release clippy pass. The explicit native backend check also
passes; independent XDF inspection finds Start/End/Stop and continuous affect.

Run 42227148826 subsequently FAILED the Recorder installed SHA assertion after
both builds and the exact VLC standalone test passed. Tauri patches its bundle
marker for NSIS and then restores the raw compiled binary; those two variants
have different hashes. `scripts/recorder_payload.py` predicts only the pinned
CLI's single exact UNK-to-NSS change, records raw and packaged hashes, and keeps
the installed SHA assertion strict. A local isolated NSIS bundle was extracted:
all 10,241,536 payload bytes exactly match that prediction (receipt under
`build/recorder-hash-test/`). It is packaging evidence, not a release candidate.
PR10 contains the readiness/harness and receipt corrections; rerun the full
same-checkout suite after its reviewed merge. No joint/new release is qualified.

New component candidate versions are VLC+0.3.3 and Recorder0.1.8; no new
release is claimed. The [Recorder guide](https://georgefejer91.github.io/Remote-LSL-Recorder/vlc.html)
is live with exact-file and rendered-width checks. VLC+ now owns `site/` and
the included controller; deploy that page rather than relying on the separate
Recorder website. The first new VLC installer
run built the native player and installer but failed its receipt because one
source-companion filename retained 0.3.2; that filename is now corrected.
Existing installer/source artifacts under the primary checkout's build folder
remain historical 0.3.2 candidates and must not substitute for newly tested
bytes. The current source CI and hosted installer checks must be recorded below
when observed. Native VLC installer run
[42220389099](https://github.com/GeorgeFejer91/vlc-plus/actions/runs/42220389099)
passes at `a44cbbe`, including source/immutable payload, decode, LSL/CSV and
uninstall. The combined source build includes newer embedded-source changes
and must pass separately. Full Planner-master execution and scientific/physical-device
qualification remain open.

The first embedded source CI checks passed all 21 JS and 19 Rust tests and
clippy, but UI CI failed: a 2.5-second cold navigation deadline and an
intermittent BRSP fixture authentication wait. Navigation now has its own
30-second bound; fixture protocol phases/errors are logged for diagnosis.
Local rendered checks pass for both surfaces, recording controls, panel sharing,
the own-site routes, all 14 hotkeys and text enlargement. Legacy standalone
and pinned-component workflows now build/check only; `suite-source.yml` is
the sole publisher for the 0.3.3 tag.

## Completed milestone: Alt+F affect arrows and bundled study clip

- [x] Trace existing keyboard routing; reproduce the absent Alt+F toggle with
  a failing Windows panel check (exit 23), then pass the focused regression.
- [x] Reuse the saved arrows/mouse mode and step size. Claim plain arrows at
  ShortcutOverride before VLC commands, including while paused; never change
  affect on shortcut probes; preserve the existing supported-input gate.
- [x] Include the exact Great Dictator study clip with identity/provenance and
  the user's 2026-11-05 explicit public-redistribution permission confirmation.
- [x] Pass the actual Qt shortcut-map integration and native/installer checks.
- [x] Review, merge and ship 0.3.2 with the sample in source and installer;
  anonymously download/hash it and update the installed application. Its 701
  immutable installed files and release source identity pass verification.
- [x] Observe the installed foreground hotkey on this PC: Alt+F changes saved
  mode; paused Down/Left update arousal/valence with volume/position unchanged;
  normal arrows restore VLC volume. Windows injected keys, not physical input.

## Active milestone: separate Recorder and joint installer

- [x] Keep both programs independent; pin component installers and add a small
  current-user launcher with no recording/remote commands.
- [x] Pass launcher checks for quoted/local paths, missing-component rejection,
  opening both apps and reusing existing instances. Compile the Inno wrapper.
- [x] Ship the independently reviewed Recorder installer with safe refusal
  while it is open, exact source/provenance and numeric/marker XDF evidence.
- [ ] Verify fresh joint installation, actual desktop shortcut/both windows,
  duplicate prevention, recording protection and independent uninstall in CI.
- [ ] Review/merge/publish all three verified choices, joint first; confirm
  public download identities and update the current evidence here.
- [ ] Install the final tested joint package on this PC, verify the bundled
  film and both installed apps, and open both from the desktop shortcut.
  Recording and phone control remain explicit user actions. This local
  installation is authorized by the user's 2026-11-05 goal instruction.

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

## Archived handoff (before the 2026-11-06 continuation)

The following checkpoint is historical. The current user has explicitly
resumed implementation and qualification; its stop instruction no longer
applies. Use the active milestone above for current status.

The user requested **push code first, then push this handoff** on 2026-11-05.
Stop installation/release work at this checkpoint and leave the incomplete
milestone open for the future agent. No participant recording or phone control
was started. The structured installation goal is paused for this handoff.

### Git and publication checkpoint

- All product code is pushed and merged through PR #8. Candidate main is
  `1c63cf20997df9a755adc4a61195de351b1b66bb`. Windows/Qt source CI passes
  for reviewed PR #8 head `70d9eb0`; matching-tree main/branch runs were
  still in progress at handoff and must be rechecked.
- The committed-code checkpoint was separately pushed first to
  `codex/suite-installed-evidence`, matching that exact main commit. This
  handoff is a following documentation commit on the same branch. Verify Git
  before resuming; do not assume this handoff has been merged to main.
- VLC+ [0.3.2 experimental](https://github.com/GeorgeFejer91/vlc-plus/releases/tag/v0.3.2-experimental.1)
  is public, built from `3b23c248dfd744ebeb30c40e79a7fc5a1601fcd8`.
  Installer SHA-256:
  `69800b0a4e4891830100c922d50b1706deffac4b1962a0cad12eb4170e81306c`.
  [Exact installer run](https://github.com/GeorgeFejer91/vlc-plus/actions/runs/42051579756)
  passes decode, 701 immutable files, nominal 30 Hz LSL, CSV and uninstall.
- Recorder [0.1.7 experimental](https://github.com/GeorgeFejer91/Remote-LSL-Recorder/releases/tag/v0.1.7-experimental.1)
  is public, built/tagged at `312a59f69614d3fb0a7849c0e559c19740f0df63`.
  Installer SHA-256:
  `8435bae7b06257e21ee1aeb5456615d7fda1a63bf47f2ddebec85108b24458ae`;
  installed app SHA-256:
  `e1c22554f5d23a28378cb491151d088e4fc84f9cb6b4ad5f6e10b68427a9c48c`.
  [Exact installer smoke](https://github.com/GeorgeFejer91/Remote-LSL-Recorder/actions/runs/42086293188)
  passes running-app install/uninstall refusal without termination, safe
  same-version reinstall, clean exit and removal. Synthetic numeric/marker
  XDF was independently parsed. Its separate repo main is synchronized at
  `461d62265e3cffcb1bea024c6ade901e8f9132c5`; physical devices/phone stay open.
- **No joint 0.3.3 release exists.** README retains the joint option first,
  labels it in validation, and links the two available standalone downloads.
  Do not present the joint package as ready or install a provisional candidate.

### What is verified

Alt+F claims ShortcutOverride before VLC, toggles saved arrows/mouse only on
non-repeat KeyPress, and preserves supported playing/paused/opening input and
CSV playback gates. Windows panel, real Linux Qt shortcut-map and all three
Rust bridge tests pass. The installed foreground check used Windows injected
keys: Alt+F changed saved mode; paused Down/Left changed LSL arousal/valence by
the existing step with volume and position unchanged; normal arrows restored
VLC volume. This is not physical-keyboard or display-onset qualification.

The exact authorized Great Dictator clip is committed and packaged. Its
`examples/media/NOTICE.md` records the user's explicit public redistribution
permission and hash. This PC has VLC+0.3.2 installed; 701 immutable files and
release source identity verify. The included clip visibly decoded here.

`installer-suite/` implements three numbered installer assets with joint
first, pins both components, and owns only its launcher/joint shortcuts.
It refuses installation with either app open. Recorder's own installer removes
stock force-kill and historical-uninstaller routes. Joint launch starts no
recording or remote control. Independent review found no product-code issues;
local C# checks and the complete build script pass.

### Remaining failure and next action

[Latest joint run 42089203636](https://github.com/GeorgeFejer91/vlc-plus/actions/runs/42089203636)
**FAILED**. It passed fresh install, exact component identity, both windows
from the actual desktop shortcut, repeated launch without duplication,
open-app refusal preserving Recorder, and suite-only removal preserving both
apps. Cleanup then logged `Closing owned vlc window: VLC media player` and
failed because the owned VLC process did not exit within30seconds after a
local nonforced `taskkill /PID` request. Publication was correctly skipped.
Separate component uninstall assertions have therefore not completed in the
joint test. Earlier runs corrected an expected Inno refusal code8 to documented
code7 and showed that one `Process.CloseMainWindow()` also failed to close VLC.

Next agent: diagnose the exact owned VLC windows/shutdown on the disposable
runner before changing the harness again. Pinned VLC has an application-modal
first-run privacy widget; Microsoft's CloseMainWindow API refuses a disabled
parent, but the actual remaining shutdown cause is **not yet established**.
A repeated-close hypothesis was reviewed then replaced before testing; do not
claim it is verified. The existing installed-player probe already uses VLC's
local RC `quit` command; a CI-only control fixture is a possible investigation,
not an implemented fix. Preserve the real process-exit/uninstall assertions;
never turn a failed cleanup into PASS or add a force-kill fallback. No privacy
consent or settings have been changed on the user's PC.

After the full joint smoke passes, the existing manual workflow can publish
`v0.3.3-experimental.1` from main with `publish_release=true`. Its publication
job validates exact current-run source, component hashes and a closed artifact
inventory. Then verify public downloads, install that tested joint package
here, verify both app identities and the bundled film, and observe both from
its desktop shortcut. Keep recording and phone control opt-in.

### Local resume aids and boundaries

- Generated artifacts/receipts are ignored under `build/`: final safe Recorder
  files in `recorder-component/`, verified VLC download in
  `alt-f-release-0.3.2/`, local suite candidate/receipt in `suite-release/`,
  and foreground hotkey receipt `alt-f-desktop-hotkey-check.json`. These are
  local evidence, not a passed joint release. Old provisional candidates must
  never be substituted for a newly tested exact artifact.
- `build/install-tested-suite.ps1` is an uncommitted local helper prepared to
  verify public build/smoke hashes, refuse open apps/recording CLI, scope temp
  files to the project drive, install and verify registered payload/shortcut.
  It has **not run**. Review it before use; system-drive free space was limited.
- Recorder0.1.7 already existed on this PC before this work. It has **not** been
  updated to the new safe package here. The joint launcher is **not installed**
  here. No VLC/Recorder/LabRecorderCLI process was running at the checkpoint.
- Both repos are public. `afourcade` has VLC write permission; a Recorder write
  invitation was sent and may require acceptance. Recheck before claiming it
  has been accepted.
- Parent Affect Research source stays unchanged at
  `25a429024ace0074119c2a44fb4e661670f0e84d`, on its existing safety branch.
  Full Planner-master execution and scientific qualification remain open.
