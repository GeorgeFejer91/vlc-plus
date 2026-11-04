# Verification and readiness gates

Evidence must match its claim. Use `VERIFIED`, `PARTIAL`, `BLOCKED`, or
`NOT RUN`; missing tools or hardware never count as a pass.

## Standalone extraction acceptance

- Native runtime sources and atlases match the pinned upstream blobs.
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
A stuck native outlet startup and bounded shutdown remain open runtime checks.

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

## Evidence for the extraction milestone (2026-11-04)

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
