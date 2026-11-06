# Session scope — 2026-11-06

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
