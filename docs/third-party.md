# Third-party and retained attribution

- VLC 3.0.20 Qt modifications and the Rust bridge declare GPL-2.0-or-later.
  The [GPL text](../LICENSE) is copied from the SHA-256-pinned official VLC
  source archive's `COPYING`. The patch and modified headers are in `vlc-qt/`.
- VLC core, bundled plugins, Qt and MinGW runtime dependencies retain their
  individual upstream notices in the official runtime. Packaging must retain
  them and provide the matching modified source and dependency notices.
- The bridge depends on Rust `labstream` (MIT) and `windows-sys`
  (MIT OR Apache-2.0); exact versions are in `vlc-qt/bridge/Cargo.lock`.
- [Affect Research BSD attribution](../licenses/AFFECT-RESEARCH-BSD-3-Clause.txt)
  remains with inherited material to which it applies. It does not replace
  VLC/Qt component licenses.
- [Face asset provenance](../assets/face/NOTICE.md) is retained from the source tree with three relative links rebased. It describes the wider historical asset family; only files
  enumerated in this extraction's manifest are included here. These are
  authored/synthetic visual assets, not emotion-recognition or diagnostic
  validation.

No FFmpeg executable, separate liblsl DLL, resvg renderer, participant data or
demo film is included in this source extraction. Redistribution of a built
runtime needs its own complete dependency/source review.
