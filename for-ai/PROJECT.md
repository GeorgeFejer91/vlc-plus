# VLC+ project contract

## Purpose and ownership

VLC+ is a standalone Windows VLC player for continuous affect input, native
feedback, outbound LSL and video-local CSV backup. The Qt panel owns
presentation/input/settings; the Rust bridge owns its independent 30 Hz LSL
worker and CSV writer. Sampling is not owned by a rendering callback.

Planner authors studies; Runner executes full protocols; VLC+ is an independent
playback target. Flubbercorder and Remote LSL Recorder are separate recorders.
No recorder implementation, Planner editor, browser frontend or replacement
media engine is included here. `installer-suite/` composes the independently
maintained Remote LSL Recorder installer with VLC+ and owns only the joint
launcher/shortcuts. Recorder retains recording, XDF and remote-control authority.

## Overall product goal

Share explicitly versioned experiment contracts with Planner and Runner, so
one authored JSON plus assets can be used on either capable execution target.
This compatibility goal stays **open**. Full master execution, questionnaires,
variant/participant selection and recovery are not implemented in VLC+.
See [Planner integration](PLANNER-INTEGRATION.md).

## Active milestone: keyboard, joint handoff and recording qualification

- [x] Recover both saved branches and preserve the primary checkout's unfinished
  suite-shutdown changes; use the isolated `codex/complete-controls` worktree.
- [x] Alt+F reserves all arrow variants from VLC while plain arrows change
  affect; Alt+G/Alt+Shift+G cycle exactly Flubber/grid/face. Detailed settings
  release mouse capture before opening. Existing quick controls remain.
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
  not qualification. User must handle the privacy dialog; do not bypass it.
- [ ] Merge reviewed source, publish exact tested component and joint versions,
  verify public hashes, update live download links and install the joint suite.

New component candidate versions are VLC+0.3.3 and Recorder0.1.8; no new
release is claimed. The [Recorder guide](https://georgefejer91.github.io/Remote-LSL-Recorder/vlc.html)
is live with exact-file and rendered-width checks. The first new VLC installer
run built the native player and installer but failed its receipt because one
source-companion filename retained 0.3.2; that filename is now corrected.
Existing installer/source artifacts under the primary checkout's build folder
remain historical 0.3.2 candidates and must not substitute for newly tested
bytes. The current source CI and hosted installer checks must be recorded below
when observed. Full Planner-master execution and scientific/physical-device
qualification remain open.

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
