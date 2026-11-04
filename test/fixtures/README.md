# Synthetic installed-player input

`validation.mp4` is generated test material: six seconds of FFmpeg `testsrc2`,
640×360, 30 Hz, H.264/yuv420p, 180 frames, without audio or participant data.

SHA-256: `82fc7904c7967c396122d13eeae89486084db97e168faf12ad0ef34a625720ee`.

Generation command (FFmpeg build `2025-02-06-git-6da82b4485`):

```text
ffmpeg -f lavfi -i testsrc2=duration=6:size=640x360:rate=30 -an -c:v libx264 -preset fast -crf 28 -pix_fmt yuv420p -movflags +faststart validation.mp4
```

It is copied to a fresh test directory before each installed-player probe.
It is not a study stimulus, participant recording or bundled auto-play demo.
