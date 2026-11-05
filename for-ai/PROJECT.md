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

## Active milestone: Alt+F affect arrows and bundled study clip

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
- [ ] Ship the independently reviewed Recorder installer with safe refusal
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

2026-11-05: Alt+F and the authorized sample merged in PR #5, exact release
source `3b23c248dfd744ebeb30c40e79a7fc5a1601fcd8`. Joint packaging and evidence
documentation are on `codex/recorder-suite`.

The input dispatcher claims ShortcutOverride before VLC, toggles only on
non-repeat Alt+F KeyPress, and preserves the existing supported-input gate
(including paused adjustments) and CSV playback gate. Windows panel and real
Linux QShortcut-map checks pass; native Qt/launcher build passes. Test-only LSL
discovery now selects the test process so a running player cannot capture it.
Independent review has no remaining important findings.

[Installer/publish run 42051579756](https://github.com/GeorgeFejer91/vlc-plus/actions/runs/42051579756)
passed: 701 immutable files verified, install/uninstall exit0, H.264 decode,
30.0015 Hz LSL, unique CSVs and shutdown126ms.
[0.3.2 experimental](https://github.com/GeorgeFejer91/vlc-plus/releases/tag/v0.3.2-experimental.1)
is public with verified asset digests. The user explicitly confirmed public
redistribution permission for the exact Great Dictator clip; its notice and
SHA-256 are in `examples/media/NOTICE.md`, and it is in source and installer.

The complete anonymous 0.3.2 installer download matches its release hash. This
PC has 0.3.2 installed; 701 immutable files/source identity pass. Foreground
hotkey/LSL/volume/paused-position checks pass using Windows key injection.
The joint launcher/tests and wrapper
compile pass locally; independent suite review has no remaining findings.
Recorder packaging PR #3 removes both force-kill and historical-uninstaller
routes. The lock pins its clean source `312a59f69614d3fb0a7849c0e559c19740f0df63`
and rebuilt installer SHA-256 `8435bae7b06257e21ee1aeb5456615d7fda1a63bf47f2ddebec85108b24458ae`.
Public promotion awaits exact hosted installer smoke; no provisional package
is approved for installation.

Next: finish Recorder exact-installer smoke/public promotion, then hosted
Windows suite smoke and publish three choices; install the exact tested joint
package locally and observe the desktop shortcut. Full Planner-master execution and
scientific qualification remain open; the parent Research source is unchanged.
