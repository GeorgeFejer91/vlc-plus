# Verification and readiness gates

Evidence must match its claim. Use `VERIFIED`, `PARTIAL`, `BLOCKED`, or
`NOT RUN`; missing tools or hardware never count as a pass.

## Standalone extraction acceptance

- Unmodified imports/atlases match upstream; declared adaptations retain
  upstream identity and the bridge lifetime fix has an exact standalone hash.
- No source/build/license path requires an Affect Research sibling checkout.
- The common-recipe target and current appearance-only support are explicit.
- Required notices and a complete committed source archive accompany packaging.
- Repository context passes; the intended public remote matches local HEAD.

## Focused checks

From the repository root, with Python 3.12+, PowerShell 7, Git and MSVC Cargo:

```powershell
python scripts/check-source.py
cargo fmt --manifest-path vlc-qt/bridge/Cargo.toml -- --check
cargo test --locked --manifest-path vlc-qt/bridge/Cargo.toml
cargo build --release --locked --manifest-path vlc-qt/bridge/Cargo.toml
powershell -NoProfile -File for-ai/scripts/check-context.ps1 -ProjectRoot .
```

The source audit verifies imported file digests, the Planner-generated fixture,
JSON syntax, standalone package references and local documentation links. It
is provenance and structure evidence, not a second runtime JSON validator.
The Rust tests exercise non-overwriting CSV paths and live independent LSL
sampling/markers/CSV. They do not decode a video or measure display onset.

Parse each product `.ps1` using PowerShell's language parser. Apply
`vlc-qt/apply-patch.ps1` to a fresh official VLC 3.0.20 source tree after
verifying the archive hash in `docs/source-extraction.json`. Re-extract a clean
tree when repeating the patch-application check.

## Isolated Qt panel

```powershell
./vlc-qt/test-panel.ps1 -QtBin <matching MinGW64 Qt bin> -AppearanceJson examples/planner-appearance.v1.json
```

This runs Qt offscreen. It tests the actual Planner appearance fixture,
duplicate-key rejection, colors/transparency, layer/grid/face behavior, input
gating, settings round-trip and invalid-setting retention. The CI Linux Qt
job tests portable panel logic; it is not Windows DLL or full VLC evidence.
The Windows CI job runs source/context/syntax checks and the Rust bridge suite.

## Integrated and installed gates

Build the modified VLC launcher/plugin against the pinned source and matching
runtime, stage with `package-stock-vlc.ps1`, and build the standalone Inno
installer. Inspect file hashes and `source/vlc-plus-source.zip` against the
recorded commit. Test an exact fresh installed package with real original
video, mouse/keyboard input, focus loss, pause/resume, natural End/Stop/Next,
all feedback layers, CSV failures and an independent LSL/XDF receiver.
Normal shutdown is verified below; a deliberately stuck native outlet startup
remains an untested failure case.

Obtain explicit opt-in before foregrounding the user's live desktop. Do not
close unrelated applications. Unit tests, offscreen rendering, successful
compilation and old upstream receipts do not qualify the installed player or
scientific timing. No new installed check is required to claim only a verified
source extraction.

Full master compatibility additionally requires every gate in
[Planner integration](PLANNER-INTEGRATION.md). The overall product goal stays
open while those gates are missing.

## Publication

```powershell
git diff --check
powershell -NoProfile -File for-ai/scripts/check-context.ps1 -ProjectRoot . -RequireRemote
git status --short
git ls-remote origin refs/heads/main
gh repo view GeorgeFejer91/vlc-plus --json visibility,defaultBranchRef,url
```

Stage intended paths, review source/license changes, commit coherently and push
only under the user's extraction/publication authorization. Never force push.
The final gate requires clean Git and local HEAD equal to remote `main`.
Inspect CI for the exact published commit; record failure or pending state.

## Historical extraction evidence (before installer work)

- VERIFIED: 33 imported file entries; runtime and atlas digests unchanged;
  standalone package references, Planner fixture and local links checked.
- VERIFIED: native PowerShell syntax and `cargo fmt --check`.
- VERIFIED: the context checker tolerates no branch-name output (detached
  HEAD); local regression failed before removing the unnecessary `.Trim()`
  and passed afterward. PR CI exercises the real detached checkout.
- VERIFIED: all three Rust bridge tests, including live LSL samples and exact
  Start/Interrupt/Stop labels; release bridge build succeeded.
- VERIFIED: official VLC source SHA-256 and patch application to a fresh tree.
- VERIFIED in CI: Windows source/syntax/context/bridge checks and Linux Qt
  offscreen panel tests passed for `490ddc4` (run linked in `PROJECT.md`).
- VERIFIED: the complete committed-source archive contains every imported
  file and runs its audit independently, with no generated binaries/recordings.
- NOT RUN locally: isolated Qt panel and full modified VLC build; the matching
  MinGW64 Qt toolchain is unavailable in this environment.
- NOT RUN: fresh installed player/physical input/research qualification. The
  task is source extraction and publication; no installer release is claimed.
- Publication, independent review and exact CI result: see the sole active
  checklist/handoff in [PROJECT.md](PROJECT.md).

## Windows installer milestone

`build-windows.ps1` is the complete build/package entry. It builds native
prerequisites and generated Qt sources, then the Qt module/launcher, bridge and
panel check; packaging uses the pinned original VLC ZIP as its fresh input.
`build-receipt.json` records actual GCC/Qt/Rust/Cargo/linker/Inno identities,
package inventory and output hashes. The local MSYS bootstrap lock is not a
promise that a rolling CI toolchain is byte-identically reproducible.

`scripts/test-installer.ps1` requires a clean disposable Windows account (or
proven absence of every existing VLC+ uninstall registration). It installs to
a fresh folder without shortcuts or admin, verifies every immutable payload
hash/source identity, then runs the independent native player probe using its
own synthetic H.264 clip. The probe has no host Qt/plugin path and uses an
owned child PID, loopback controls and exact source-id LSL discovery. It checks
actual decoded frames, natural-end and manual-stop runs, pause/resume, active-video
CSV cadence, unique replay files, bounded shutdown and its own portable UI
settings. The exact Qt platform plugin load path must belong to the package.
Uninstall verifies registration ownership, preserves settings and retains all
evidence. Visible video/physical input/display onset are not proven offscreen.

Matching sources and original notices are required for the actual shipped
MSYS2 DLLs and locked Rust crates. Source package signatures are verified using
the selected MSYS2 keyring. A failed license/source lookup blocks packaging;
do not omit a dependency to make the candidate pass. Do not publish unrelated
compiler installations. Hold installer publication until the fresh installed
check and independent review findings are resolved with named evidence.

The installed probe accepts the documented VLC 3 terminal-state observation
on manual Stop as well as natural completion. Its receipt explicitly records
`terminalEndMayIncludeManualStop`; `End` alone is not an experiment-completion
signal. This limitation blocks that scientific interpretation, not playback.

## Verified Windows candidate (2026-11-04)

Exact source: `d2588cad1c2e0ef385e7b0eaf9b2f874efe3d7da`, merged by
[PR #3](https://github.com/GeorgeFejer91/vlc-plus/pull/3).
[Installer CI 41924794994](https://github.com/GeorgeFejer91/vlc-plus/actions/runs/41924794994)
is VERIFIED, including fresh install and uninstall, both exit 0. Installer
SHA-256: `f594ab045f93599fa0eef37ae34c048b940acfa77dc6083c859b21ebf36180d4`.

The CI build used GCC 16.2.0, Qt 5.15.19, Rust/Cargo 1.99.1, MSVC linker
14.51.36260.0 and self-reported Inno Setup 6.7.1. The compiler files expose
0.0.0.0 version resources; the receipt preserves their exact SHA-256 identities
and the CI compiler output supplies the engine version.

The installed probe observed 204 decoded frames at its statistics checkpoint,
264 independent affect samples at 29.9991 Hz, exact documented pause/resume,
end/stop and replay chronology, two CSVs with 169/31 rows, first samples at
0.405/0.403 s and last active times 6.005565/1.402968 s. Normal shutdown was
151 ms. Every immutable installed file and exact project-source identity were
verified before/after playback. The loaded Qt platform plugin came from the
package; the portable profile persisted the requested feedback/input settings
and survived uninstall. Windows bridge/source and Qt panel CI checks pass.

The local source companion additionally passed an offline frozen Rust check
using only its shipped vendor sources. Corruption, path escapes and duplicate
manifests were rejected by the integrity verifier. A separate local staged
pylsl/liblsl interoperability check passed; it is scoped to that stage's own
bridge digest, not substituted for CI installer evidence.

The [experimental release](https://github.com/GeorgeFejer91/vlc-plus/releases/tag/v0.3.1-experimental.1)
carries exact build, installed-probe, integrity and installer receipts,
matching source and checksums. No physical input, fullscreen/D3D, other codecs,
XDF recording or scientific timing qualification is claimed. Full Planner
master/sequence/questionnaire execution remains unimplemented. `End` cannot
prove natural completion, and CSV does not promise acquisition at time zero.

Publication is VERIFIED: all 11 release asset sizes and GitHub-computed SHA-256
digests match the local verified files. The release tag points to the exact
CI-tested source above. A fresh anonymous public download returned the complete
399,669,521-byte installer with the same SHA-256. Matching source ZIP SHA-256:
`9eadee1cbf1c2d59c07b767a23054d45b1c2b4e150d507118e783d0326004bb7`.
The local installer is under `build/release-0.3.1/`; that generated directory is
ignored and is not committed into Git.

## Alt+F candidate checks

The new Windows panel regression first failed at exit 23 because Alt+F was
absent, then passed with the input dispatcher. It checks toggle/repeat,
ShortcutOverride without affect mutation, exclusive arrows while input is
unavailable, and restoring normal keys. The production input callback remains
unchanged: supported playing, paused and opening states can adjust affect;
the Rust worker still owns the playback gate for CSV recording.

[PR #5](https://github.com/GeorgeFejer91/vlc-plus/pull/5) source CI passes,
including Linux Qt's real QShortcut-map integration from a video-like sibling
widget. It observes normal VLC-style shortcuts before toggle, suppression and
affect updates after toggle, retained ownership while input is unavailable,
and restored shortcuts afterward. Windows offscreen cannot prove foreground
HWND capture, so no physical-keyboard claim is taken from that test alone.

The new native Qt module/launcher built locally. All three Rust tests pass
with source-ID-isolated discovery while the user's player remains open. The
new full installer validation/publication run is
[42051579756](https://github.com/GeorgeFejer91/vlc-plus/actions/runs/42051579756),
source `3b23c248dfd744ebeb30c40e79a7fc5a1601fcd8`. Both installer and publication jobs passed for this exact source. Fresh
install/uninstall exit 0; 701 immutable files/source identity verified before
and after playback. The installed probe observed 204 decoded frames at the
statistics checkpoint, 264 affect samples at 30.0015 Hz, 169/31 CSV rows, and
normal shutdown in 126 ms. These are new 0.3.2 receipts, not inherited 0.3.1 proof.

[Release 0.3.2](https://github.com/GeorgeFejer91/vlc-plus/releases/tag/v0.3.2-experimental.1)
is public. All uploaded server digests match the release checksum file. The
installer SHA-256 is `69800b0a4e4891830100c922d50b1706deffac4b1962a0cad12eb4170e81306c`;
matching source ZIP SHA-256 is
`b93aa69a95091682ef6725cf921681ee786c0f57aedfe258aa9d737c88b7320e`.
The bundled video matches the permission-scoped hash in its notice.

The anonymous public installer download matches its release SHA-256. The
0.3.2 update on this PC completed with exit 0; `verify-installed.py` verifies
701 immutable files and exact release source identity. A foreground installed
check using Windows Computer Use key injection additionally observed Alt+F
persistently toggling arrows/mouse. With arrows active, paused Down and Left
changed LSL arousal/valence by the existing 0.1 step while volume and video
position stayed unchanged. With affect arrows off, Down/Up changed VLC volume
and restored the baseline. The bundled film visibly decoded. This is injected
key/foreground Windows evidence, not a physical keyboard or display-onset test.

## Joint installer gates

The self-contained source build is `.github/workflows/suite-source.yml`.
`build-recorder.ps1` installs the frozen frontend lock, prepares the pinned
recording engine, runs source/UI checks and creates the embedded app installer
plus exact source/build receipt. Its hosted smoke reads the current source
version and compares the installed executable against the exact packaged-byte
hash: `scripts/recorder_payload.py` applies only Tauri CLI2.11.4's documented
single UNK-to-NSS marker change in memory. The restored compiled binary hash is
recorded separately. A real isolated NSIS payload was extracted and compared
byte-for-byte with the prediction; no other change is accepted.
Both standalone
smokes must pass at the same root commit before `build.ps1 -BuiltFromSource`.
The joint check consumes the resulting component lock; publication validates
both standalone receipts, complete source archives and the joint receipt.
`python installer-suite/test-source-release.py` proves rejection of stale proof,
unexpected assets and corrupted components. Old standalone-release locks remain
a separately scoped legacy mode.

`node scripts/build-site.mjs`, `node scripts/test-site-safety.mjs` and
`node scripts/check-site.mjs` build and verify the VLC+ page/controller, actual
Pretext/font assets, 14 shortcuts, 320–1920 px, 200% text/spacing and controller
privilege separation. The builder rejects linked output ancestors before
removing its generated directory. Verify the actual Pages deployment and files
before describing the website as live.

Run `installer-suite/test-launcher.ps1` for fixed installed-path resolution,
both-app launch, existing-instance reuse and missing-component preflight.
`installer-suite/build.ps1` requires clean committed source and pinned SHA-256
component installers. See the [packaging guide](../installer-suite/README.md).

Before publishing, the fresh GitHub-hosted Windows check must verify installed
component identity, both windows from the actual desktop shortcut, repeated
launch without duplication, safe refusal with Recorder open, suite-only
removal preserving both apps, and separate app uninstallation. No recording
or phone pairing starts. The Recorder project independently owns its XDF and
no-kill installer evidence. These scopes do not replace physical-device,
physical-input, end-to-end participant-session or scientific qualification.

VLC+ playback readiness requires exact complete XDF stream headers and positive
subscription acknowledgements from the current pinned LabRecorder child. The
engine can write headers after an open_stream timeout; metadata alone is not
subscription proof. The pinned CLI resolves a frozen batch before starting its stream workers;
all resolved streams must acknowledge opening. Prefix counting tolerates
interleaved names and newlines, including duplicate names. Historical logs
never satisfy a new recording. The current child must still be running.
Malformed prelude evidence fails closed with video paused. Control characters
in metadata/query fields are rejected before engine spawn; queries also bind
the selected name/hostname to prevent different reused-ID metadata entering
the readiness log.

Native marker waits explicitly poll awaited snapshots outside page.waitForFunction:
Playwright1.64.1 resolves an async false predicate immediately. The corrected
native button diagnostic independently saved all five Start/Pause/Resume/End/Stop
markers. Full foreground emulated affect/transport/XDF qualification remains
pending the user's Windows Firewall prompt choice and final tested installers.
