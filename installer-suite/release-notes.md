Three Windows x64 installers built and tested from this repository.

1. **01 — VLC+ 0.3.4 and Remote LSL Recorder 0.1.9.** Installs both apps and adds one desktop shortcut that opens them together.
2. **02 — VLC+ 0.3.4.** Hold arrow keys for continuous affect rating. Combine perpendicular arrows for diagonal movement and opposite arrows to cancel that axis. The grid and face respond to key changes immediately. The affect LSL stream samples at 480 Hz while the CSV log stays at 30 Hz. Press Alt+F to enable the arrows.
3. **03 — Remote LSL Recorder 0.1.9.** Use the orange **Start VLC+ and record** button with the integrated cone to record and play the selected local video. Recorder confirms subscriptions before playback and retains partial XDF on failure. The Recorder signal viewer shows live valence and arousal scores.

The Recorder source, UI, browser controller, locks and notices are included in this repository. Its per-launch session ID identifies the intended VLC LSL streams even when different computers have matching PIDs. Sampling and XDF recording remain in the native applications, independently of browser rendering.

[VLC+ live page, controller and hotkeys](https://georgefejer91.github.io/vlc-plus/).
Stop/save recordings and close both apps before updating. The joint shortcut starts neither recording nor phone access. Removing the joint launcher leaves both independently uninstallable apps and their data in place.

Matching source archives, native dependency sources, component build receipts, SHA-256 checksums and exact-run install/shortcut evidence accompany these unsigned experimental installers. Physical-phone/device and scientific timing qualification remain separate from synthetic LSL, injected keys and hosted Windows installer checks.

The shared hotkey library has 64 groups covering
all 60 detailed settings plus affect step. Dedicated shortcuts adjust appearance,
grid, mapping bounds/drivers/direction and exact colors; Shift reverses numeric
changes. Alt+J saves a complete v3 JSON snapshot with rating step and restores
the last successful snapshot on launch. Legacy v2 settings still load. Recorder
help provides category pages that fit its bounded dialog; the live guide retains
the complete list. Windows GUI shortcuts normalize physical OEM keys after
focus changes, including German bracket and resize keys.
