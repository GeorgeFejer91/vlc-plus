# Build and package the standalone Qt player

VLC+ modifies official VLC 3.0.20's Qt interface. No Affect Research sibling
checkout is required. The build uses the pinned source and runtime hashes in
[scripts/windows-build-lock.json](../scripts/windows-build-lock.json) and
[the package script](../package-stock-vlc.ps1).

## Windows build

Use Windows x64, PowerShell 7, Python 3.12+, Git, MSVC Rust and MSYS2 MINGW64.
The checked toolchain uses GCC 16.2.0 and Qt 5.15.19. In an MSYS2 MINGW64 shell,
install the same packages listed in the
[installer workflow](../.github/workflows/windows-installer.yml). Inno Setup 6
is needed for installer output; the GitHub Windows runner provides it.

From a clean, committed checkout:

```powershell
./build-windows.ps1 -MsysRoot <MSYS2 root> -InnoCompiler <ISCC.exe>
```

The entry script verifies the official source archive, restores and patches a
generated source tree, configures an out-of-tree build, builds native
prerequisites and generated Qt sources, then builds the actual Qt DLL and
launcher, Rust bridge, and offscreen panel check. `-BuildOnly` stops after the
native and focused checks.

Packaging always extracts the verified original Windows ZIP into an empty
stage. It replaces only the selected native binaries, resolves Qt/MinGW DLLs,
adds the feedback assets, isolated portable profile, original notices and
exact committed project source. It collects matching signed MSYS2 source
packages and locked Rust vendor sources in a separate companion ZIP.

Outputs are under `build/stock-vlc-package/out`:

- `VLC_Plus_Setup_0.3.2_x64.exe`
- `VLC_Plus_Source_0.3.2.zip`
- `build-receipt.json`

The receipt records actual compiler/linker/Qt/Inno identities and artifact
hashes; `build/toolchain-packages.txt` records package versions. VLC inputs are
pinned. Rolling toolchain packages are recorded, so byte-identical binary
reproducibility is not claimed.

## Fresh installed verification

```powershell
./scripts/test-installer.ps1 -InstallerPath build/stock-vlc-package/out/VLC_Plus_Setup_0.3.2_x64.exe
```

Use a disposable clean Windows account. The script refuses an existing VLC+
registration, installs into a fresh test directory, verifies all immutable
hashes and source identity, and launches only its own player process offscreen.
It checks actual synthetic-video decoding, independent LSL, pause/resume,
replay, unique CSV files, cadence, package-local Qt loading, portable settings
and bounded shutdown, then uninstalls its owned registration and checks that
preferences remain. Evidence is retained under `build/validation`.

This does not test physical input or display onset. Obtain explicit opt-in
before foregrounding the user's live desktop. Current installed evidence and
the terminal-marker/CSV-start limitations are in
[verification](../for-ai/VERIFICATION.md).
