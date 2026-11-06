# Joint installer

The current `suite-source.yml` workflow builds VLC+ and the included Recorder
from one clean checkout, tests each exact standalone installer, then packages
and tests the joint installer. Use `./build-recorder.ps1` for the embedded app.
`build.ps1 -BuiltFromSource` requires matching source/build/smoke receipts and
generates a hash-bound component lock; it never downloads a component installer.

The legacy mode below composes two independently versioned, tested installers.
Neither mode merges application state or adds recording/control authority. The
three downloads are numbered joint, VLC+, Recorder so the joint choice appears
first. The standalone downloads are byte-identical copies of their upstream
release installers, pinned in `components.lock.json`.

Build on Windows with PowerShell 7, Git, the Windows .NET Framework C# compiler
and Inno Setup 6:

```powershell
./installer-suite/test-launcher.ps1
./installer-suite/build.ps1 -InnoCompiler 'C:\Program Files (x86)\Inno Setup 6\ISCC.exe'
```

Build requires a clean commit, verifies component SHA-256 before compiling,
and writes three installers, wrapper source, lock and receipts under
`build/suite-release`. `-ComponentDirectory` can reuse an already downloaded
cache; incorrect bytes fail rather than being executed. Component app sources,
dependencies and notices remain owned by each independent project/release.

The C# launcher uses fixed current-user uninstall registrations, verifies both
local executables before launch, and starts only absent exact-path processes.
It has no command forwarding, recording, pairing or arbitrary path argument.
`--check` checks installation and `--install-ready` refuses either open app.
The suite refuses installation while either app is open; Recorder's own
installer/uninstaller also refuses rather than killing it. Stop/save and close
both apps before upgrading.

The suite owns only its launcher and joint shortcuts. Both child installers
own their separate app registrations, shortcuts and uninstallation. Removing
the suite leaves each app and its data in place. A failed second installation
preserves the successful first app and reports the error for repair.

`test-installer.ps1` runs only in a fresh GitHub-hosted Windows account: it
checks component identities, launches the actual desktop shortcut, observes
both windows, checks repeat launch and open-Recorder refusal, removes only the
suite, then closes its own idle apps gracefully and tests separate uninstall.
It never starts recording. `.github/workflows/suite-installer.yml` publishes
only after this exact smoke test and an independent current-run artifact/hash
check. Existing tags cannot be overwritten.

The disposable install test writes a loopback-only RC fixture to `portable/vlcrc`
(VLC core's profile; Qt alone uses `portable/vlc/`). Shutdown verifies the exact test-owned executable, PID and
listener, waits for VLC's quit acknowledgement and requires actual process
exit. A failed shutdown preserves the installation for diagnosis; no force
termination fallback is used. This does not change a user's privacy choices.
