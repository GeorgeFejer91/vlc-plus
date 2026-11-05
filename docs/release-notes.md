# VLC+ 0.3.2 experimental

Press **Alt+F** in the player to toggle affect arrow control:

- **On:** Left/Right changes valence; Up/Down changes arousal. VLC's plain-arrow
  volume/seek shortcuts are suppressed, including during pause. The existing
  input gate permits affect adjustment while paused; CSV remains playback-gated.
- **Off:** normal VLC arrow controls return and mouse affect input is selected.

The mode uses the existing saved input setting and step size. Holding Alt+F
does not repeatedly toggle. Dialogs, menus and other applications keep their
normal keyboard behavior. Existing Ctrl+Shift+A/M shortcuts remain available.

The Great Dictator study clip is in `examples/media/dictator-3-study.mp4` in the
repository and installed player. Its notice records provenance, exact hash and
the repository owner's explicit redistribution-permission confirmation.
Software licenses do not relicense the audiovisual material.

The installer workflow tests a fresh install, real synthetic-video decoding,
independent LSL reception, pause/resume/replay, unique CSVs, settings, bounded
shutdown and uninstall before publication. The panel checks cover Alt+F,
shortcut ownership, supported-input gating and restored VLC commands. Exact source,
toolchain identities and output hashes are in the attached receipts.

This remains experimental and unsigned. Full Planner-master execution and
scientific timing/physical-input qualification are open. `End` can also report
manual input teardown, and CSV acquisition can begin around 0.4 seconds.
