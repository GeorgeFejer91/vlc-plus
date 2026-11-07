# Session scope — 2026-11-07

Current task: show received valence/arousal scores in the Recorder signal
viewer (confirmed by the user), and merge the cone into the orange button's
SVG silhouette. Allowed changes are Recorder presentation, a narrow score
view module, focused tests, help and task handoff. Native commands, acquisition,
remote grants, saved formats and the recording engine stay unchanged.
Initial checkout is clean at `990ba6d`
on `codex/vlc-record-play-cone`.
Run the existing JavaScript, rendered Recorder and source/context checks.

Completed: received score readouts with units and explicit missing/disconnected
states, and an inline SVG joining the cone and orange button silhouette.
All 24 JS tests, rendered values/update/deselection/keyboard/320–1920px/200%
checks, source/context checks and the locked release build pass. See
`for-ai/PROJECT.md` for exact binary identity. Native player UI and native
XDF qualification are outside this follow-up. The installed Recorder remains
open and unchanged; no active recording or remote publication was changed.

## Earlier qualification scope

Post-qualification documentation is staged locally on
`codex/qualified-release-docs`, based on application source `2713d42`.
Allowed changes are release/download prose, native/shared help copy and the
canonical handoff/evidence. Do not publish this documentation or advance main
until suite 42320785383, its exact installed UI/data tests and exact-byte
promotion pass. No application logic, schema or acquisition changes belong in
this follow-up. Preserve the frozen application source during qualification.

Installed qualification exposed Windows GUI OEM punctuation after Alt+F moves
focus from the native video thread to Qt. On the German layout, OEM6 arrives
as acute and OEM4 as backslash; the native route already recognizes the physical
bracket keys. Scope is the existing legacy settings matcher, its Windows Qt
regression, the Windows panel CI gate, and current handoff. Preserve dedicated
digit-driver priority, other modifiers, text/modal guards, non-Windows behavior,
schemas and the independent 30 Hz LSL/CSV bridge. Initial Git was clean at
f097c95 (documentation only above application source 0edb694). The installed
0edb694 recording test and independent data inspection pass; it must not be
published while this supported GUI input path fails. Local SDK compilation is
unavailable after user-authorized cache cleanup; hosted Windows Qt must prove
the new regression red before the application fix and green afterward.
Shared hotkey help may clarify German OEM key labels and the existing layout
arrow alternative when a desktop utility reserves Alt+S; retain all 64 groups
and the bounded, measured library layout.

## Earlier expansion scope — 2026-11-06

The active follow-up expands dedicated hotkeys to the native dialog's 60
settings plus rating step. Reuse the Qt/native video-thread matcher, preserve
Alt+F4, modal/text input and existing shortcuts, and clamp every adjustment to
its dialog bounds. Settings snapshots become `vlc-feedback-settings/v3` to
include `stepPercent`, while retaining v2 and appearance-v1 imports. Alt+J
must save atomically and restore the last successful snapshot on launch.
The shared website/Recorder library documents all 64 shortcut groups; the
bounded Recorder help uses category pagination with actual text measurements.

This follow-up starts from clean main `5b94b16` in the existing controls
worktree on `codex/dedicated-flubber-hotkeys`. Allowed changes are the panel,
native matcher and their focused tests, shared hotkey library/help geometry,
documentation, provenance and task evidence. No acquisition/recording engine
or experiment-contract change is intended. Run source/native/browser checks,
independent review and exact-source CI before the final installed UI and XDF/
CSV qualification. The earlier suite at `5b94b16` passed hosted install gates;
it does not contain this expansion and is not the final candidate.

Installed qualification also owns its test runner's lifecycle. Before any
native attempt, refuse existing player/Recorder processes; send RC commands
and cleanup only to the current listener owned by the discovered test PID.
An interrupted attempt must retain its original workspace backup and recover
it after all apps close, before capturing the next attempt's baseline. This
test-only correction leaves the shipped applications unchanged.

## Earlier layout and snapshot scope

Current task: Alt+W/A/S/D moves the selected feedback by two percentage points;
Alt+plus/minus resizes it in either input mode. Alt+J writes an atomic settings
JSON snapshot beside the portable profile, remembers only successful saves,
and reloads the last saved snapshot on startup. Missing/invalid snapshots must
retain the current profile and report the error. Explicit CLI settings remain
authoritative after startup restore. Preserve affect values, acquisition,
existing shortcuts, and normal dialog/text-entry behavior.

Initial Git state: clean primary checkout at fe35248, branch
`codex/alt-layout-presets`; the existing controls worktree is untouched.
Allowed files: native panel and panel regression, shortcut documentation and
shared hotkey list/provenance, focused UI checks, current release notes and
For-AI milestone/evidence. Reuse the existing settings schema and Qt input
dispatcher. No bridge, recording, or experiment-contract changes.

Sequence: add failing runtime regressions; implement aliases and snapshot
restore; run native/source/UI checks and independent review; push/merge verified
source; build/test/publish the existing installer workflow; install the exact
tested package and verify visible controls, all three feedback modes, settings,
save/restart restore, and the bundled Recorder UI. User authorization includes
push, installation and visible UI testing. Keep unrelated apps and user files.

Local evidence: Qt panel red 61/79/80 then green; four bridge tests and 22 JS
tests pass; source/provenance/context/diff checks pass. Site plus Recorder/
controller rendered checks pass with the 17-entry shortcut list. Independent
review identified the bookmark rollback/retry cases; both are now covered.

## Earlier task scope

Outcome: finish the handoff, make Alt+G cycle exactly three feedback modes, reserve VLC arrow shortcuts during affect control, and qualify launch/record/play with emulated affect input and independently parsed XDF.

Allowed changes: native Qt panel/input routing and panel tests; hotkey documentation; joint-installer shutdown harness if reproduced; existing For-AI milestone/evidence. Preserve the Rust bridge's 30 Hz acquisition, LSL/CSV formats, media identity, and unrelated app behavior.

The user's follow-up requires the self-contained Recorder and controller inside
this repository, plus VLC+'s own live page. Scope now includes `recorder/`
(pinned upstream product import with MIT notices), `site/`, site/Recorder build
and UI checks, same-checkout component receipts and gated joint packaging. Root
For-AI remains the sole task owner. The user handled VLC privacy manually.

Initial state: clean worktree codex/complete-controls at 7f456cc. The primary checkout's three uncommitted suite-shutdown paths are preserved. The latest joint smoke failed because its RC listener was absent. The existing toolchain and installed local candidates are available. Continuing handoff authorization covers PRs, release and tested installation; participant recordings and privacy/security consent remain outside this synthetic test.
