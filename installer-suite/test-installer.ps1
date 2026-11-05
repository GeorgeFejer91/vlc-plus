#Requires -Version 7.0
# Destructive only to the installations created in this fresh hosted CI account.
[CmdletBinding()]
param([Parameter(Mandatory)][string]$InstallerPath)
$ErrorActionPreference = 'Stop'
if ($env:GITHUB_ACTIONS -ne 'true' -or $env:RUNNER_ENVIRONMENT -ne 'github-hosted') { throw 'Run this fresh-install test only on a disposable GitHub-hosted runner.' }
$root = Split-Path -Parent $PSScriptRoot
$lock = Get-Content -Raw "$PSScriptRoot/components.lock.json" | ConvertFrom-Json
$source = (& git -C $root rev-parse HEAD).Trim()
$keys = @('HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\{965E973A-6173-4BBC-9DE4-6FA8A320DB93}_is1','HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\{F836C1D9-43B0-4A71-95D7-2854030B68AE}_is1','HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\Remote LSL Recorder')
foreach ($key in $keys) { if (Test-Path -LiteralPath $key) { throw 'Pre-existing app registration; refusing to modify it.' } }
if (Get-Process vlc,remote-lsl-recorder -ErrorAction SilentlyContinue) { throw 'Pre-existing app process; refusing test.' }
$suite = "$env:LOCALAPPDATA\Programs\VLCPlusRecorderSuite"
$vlc = "$env:LOCALAPPDATA\Programs\VLCPlus"
$recorder = "$env:LOCALAPPDATA\Remote LSL Recorder"
foreach ($folder in @($suite,$vlc,$recorder)) { if (Test-Path -LiteralPath $folder) { throw "Pre-existing directory: $folder" } }
$evidence = Join-Path $root 'build/suite-evidence'
New-Item -ItemType Directory -Path $evidence | Out-Null
function Run-Setup([string]$File, [string[]]$Arguments, [int]$Expected=0) {
    $p = Start-Process -FilePath $File -ArgumentList $Arguments -WindowStyle Hidden -PassThru
    if (-not $p.WaitForExit(600000)) { throw "Test-owned installer timed out: $File" }
    if ($p.ExitCode -ne $Expected) { throw "Unexpected exit $($p.ExitCode), expected $Expected : $File" }
}
Run-Setup (Resolve-Path -LiteralPath $InstallerPath).Path @('/VERYSILENT','/SUPPRESSMSGBOXES','/CURRENTUSER','/NORESTART','/TASKS=desktopshortcut',('/LOG="' + $evidence + '\install.log"'))
& python "$root/scripts/verify-installed.py" $vlc --source-commit $lock.vlc.sourceCommit --receipt "$evidence/vlc-integrity.json"
if ($LASTEXITCODE -ne 0) { throw 'VLC immutable payload failed.' }
$recorderExe = "$recorder\remote-lsl-recorder.exe"
if ((Get-FileHash -LiteralPath $recorderExe).Hash.ToLowerInvariant() -ne $lock.recorder.applicationSha256) { throw 'Recorder executable differs from its tested package.' }
$helper = "$suite\VLCPlus-and-Recorder.exe"
Run-Setup $helper @('--check')
$shortcut = Join-Path ([Environment]::GetFolderPath('Desktop')) 'VLC+ and LSL Recorder.lnk'
$shell = New-Object -ComObject WScript.Shell
if ($shell.CreateShortcut($shortcut).TargetPath -ne $helper) { throw 'Joint desktop shortcut has the wrong target.' }
Start-Process -FilePath $shortcut
function App([string]$Name,[string]$Path) { @(Get-Process -Name $Name -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $Path }) }
$limit = [DateTime]::UtcNow.AddSeconds(60)
do {
    Start-Sleep -Milliseconds 500
    $players = @(App 'vlc' "$vlc\vlc.exe")
    $recorders = @(App 'remote-lsl-recorder' $recorderExe)
} until (($players.Count -eq 1 -and $recorders.Count -eq 1 -and $players[0].MainWindowHandle -ne 0 -and $recorders[0].MainWindowHandle -ne 0) -or [DateTime]::UtcNow -gt $limit)
if ($players.Count -ne 1 -or $recorders.Count -ne 1 -or $players[0].MainWindowHandle -eq 0 -or $recorders[0].MainWindowHandle -eq 0) { throw 'Shortcut did not open both application windows.' }
if (Get-Process -Name LabRecorderCLI -ErrorAction SilentlyContinue) { throw 'Recording started without being requested.' }
$playerId = $players[0].Id; $recorderId = $recorders[0].Id
Run-Setup $helper @('--install-ready') 20
Start-Process -FilePath $shortcut
Start-Sleep -Seconds 2
if (@(App 'vlc' "$vlc\vlc.exe").Count -ne 1 -or @(App 'remote-lsl-recorder' $recorderExe).Count -ne 1 -or @(Get-Process -Id $playerId,$recorderId -ErrorAction SilentlyContinue).Count -ne 2) { throw 'Repeated shortcut duplicated/replaced an app.' }
# Refuse the joint update while the recorder is open, without touching either app.
$before = (Get-FileHash -LiteralPath $recorderExe).Hash
Run-Setup (Resolve-Path -LiteralPath $InstallerPath).Path @('/VERYSILENT','/SUPPRESSMSGBOXES','/CURRENTUSER','/NORESTART') 8
if (-not (Get-Process -Id $recorderId -ErrorAction SilentlyContinue) -or (Get-FileHash -LiteralPath $recorderExe).Hash -ne $before) { throw 'Blocked installation disturbed Recorder.' }
# Removing only the suite must preserve each independently owned program.
Run-Setup "$suite\unins000.exe" @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART')
foreach ($key in $keys[1..2]) { if (-not (Test-Path -LiteralPath $key)) { throw 'Suite removal removed an independent app.' } }
if (-not (Test-Path -LiteralPath "$vlc\vlc.exe") -or -not (Test-Path -LiteralPath $recorderExe)) { throw 'Suite removal removed a component.' }
if (Test-Path -LiteralPath $shortcut) { throw 'Suite shortcut survived uninstall.' }
# These are the exact processes created above, in a fresh account; no recording was started.
foreach ($idToClose in @($playerId,$recorderId)) {
    $p = Get-Process -Id $idToClose -ErrorAction Stop
    [void]$p.CloseMainWindow()
    if (-not $p.WaitForExit(30000)) { throw 'Test app did not close gracefully; leaving its files intact.' }
}
Run-Setup "$recorder\uninstall.exe" @('/S')
Run-Setup "$vlc\unins000.exe" @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART')
for ($attempt = 0; $attempt -lt 15 -and (Test-Path -LiteralPath $recorderExe); $attempt++) { Start-Sleep -Seconds 1 }
if ((Test-Path -LiteralPath $recorderExe) -or (Test-Path -LiteralPath "$vlc\vlc.exe")) { throw 'Independent app uninstall failed.' }
$receipt = [ordered]@{ schema='vlc-recorder-suite-smoke/v1'; result='VERIFIED'; sourceCommit=$source; installerSha256=(Get-FileHash -LiteralPath $InstallerPath).Hash.ToLowerInvariant(); bothWindowsOpened=$true; duplicateLaunchPrevented=$true; openRecorderRefused=$true; suiteRemovalPreservedApps=$true; independentUninstall=$true; recordingStarted=$false; physicalInputTested=$false }
[IO.File]::WriteAllText("$evidence/smoke.json", ($receipt | ConvertTo-Json) + "`n", [Text.UTF8Encoding]::new($false))
Write-Output 'VERIFIED: joint install, desktop shortcut, process reuse, recording protection and independent uninstall.'
