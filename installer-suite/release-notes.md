Three Windows x64 installers built and tested from this repository.

1. **01 — VLC+ 0.3.5 and Remote LSL Recorder 0.1.10.** Installs the matching pair and adds one desktop shortcut that opens both apps.
2. **02 — VLC+ 0.3.5.** Tune Flubber, the 2D grid and Face morph in separate settings popups with a live preview. Choose Tiles or Continuous for the grid and adjust full-axis hold time independently of the tap step. VLC+ publishes playback and control actions on the LSL marker stream alongside the existing playback markers.
3. **03 — Remote LSL Recorder 0.1.10.** The orange **Start VLC and record** button launches the selected video fullscreen. Recorder confirms the Flubber appearance lock and LSL subscriptions before playback. Appearance and layout unlock after recording stops. Use this workflow with VLC+ 0.3.5.

The Recorder source, UI, browser controller, locks and notices are included in this repository. Its per-launch session ID identifies the intended VLC LSL streams even when different computers have matching PIDs. Sampling and XDF recording remain in the native applications, independently of browser rendering.

[VLC+ live page, controller and hotkeys](https://georgefejer91.github.io/vlc-plus/).
Stop/save recordings and close both apps before updating. The joint shortcut starts neither recording nor phone access. Removing the joint launcher leaves both independently uninstallable apps and their data in place.

Matching source archives, native dependency sources, component build receipts, SHA-256 checksums and exact-run install/shortcut evidence accompany these unsigned experimental installers. Physical-phone/device and scientific timing qualification remain separate from synthetic LSL, injected keys and hosted Windows installer checks.

Use Ctrl+Shift+1/2/3 for the representation popups or Ctrl+Shift+S for the full
settings dialog. Alt+J saves a v4 settings snapshot and the last saved snapshot
loads on launch. VLC+ also reads v2 and v3 snapshots. Affect streaming runs at
480 Hz and the video-local CSV runs at 30 Hz.
