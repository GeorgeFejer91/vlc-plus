#Requires -Version 7.0
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$InstallerPath,
    [string]$EvidenceDirectory = ''
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$installer = (Resolve-Path -LiteralPath $InstallerPath).Path
$productKey = '{F836C1D9-43B0-4A71-95D7-2854030B68AE}_is1'
$userRegistration = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\' + $productKey
$registrations = @($userRegistration,
    ('HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\' + $productKey),
    ('HKLM:\Software\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\' + $productKey))
foreach ($registration in $registrations) {
    if (Test-Path -LiteralPath $registration) { throw 'VLC+ is already installed. Use a disposable clean account for installer verification.' }
}
& python -c 'import sys; assert sys.version_info >= (3, 12), "Python 3.12 or newer is required"'
if ($LASTEXITCODE -ne 0) { throw 'Python 3.12 or newer is required.' }
if (-not $EvidenceDirectory) { $EvidenceDirectory = Join-Path $root ('build\validation\' + [Guid]::NewGuid().ToString('N')) }
if (Test-Path -LiteralPath $EvidenceDirectory) { throw 'Use a fresh evidence directory; previous evidence is preserved.' }
New-Item -ItemType Directory -Path $EvidenceDirectory | Out-Null
$evidence = (Resolve-Path -LiteralPath $EvidenceDirectory).Path
$installed = Join-Path $evidence 'installed'
$ini = Join-Path $installed 'portable\vlc\vlc-qt-interface.ini'
$media = Join-Path $evidence 'validation.mp4'
Copy-Item -LiteralPath (Join-Path $root 'test\fixtures\validation.mp4') -Destination $media
$commit = (& git -C $root rev-parse HEAD).Trim()
function Wait-OwnedProcess($process, [int]$timeoutMs) {
    if (-not $process.WaitForExit($timeoutMs)) {
        $process.Kill($true)
        $process.WaitForExit()
        throw 'The test-owned installer/uninstaller exceeded its time bound.'
    }
    if ($process.ExitCode -ne 0) { throw "Installer/uninstaller exit code $($process.ExitCode)" }
}
$previousPath = $env:PATH
$failure = $null
$uninstalled = $false
try {
    $arguments = @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/CURRENTUSER','/NOICONS','/TASKS=',('/DIR="' + $installed + '"'),('/LOG="' + (Join-Path $evidence 'install.log') + '"'))
    $process = Start-Process -FilePath $installer -ArgumentList $arguments -WindowStyle Hidden -PassThru
    Wait-OwnedProcess $process 300000
    & python (Join-Path $PSScriptRoot 'verify-installed.py') $installed --source-commit $commit --receipt (Join-Path $evidence 'integrity.json')
    if ($LASTEXITCODE -ne 0) { throw 'Installed integrity check failed.' }
    $env:PATH = "$env:USERPROFILE\.cargo\bin;$previousPath"
    Push-Location (Join-Path $root 'vlc-qt\bridge')
    try {
        & cargo build --release --locked --bin verify-player
        if ($LASTEXITCODE -ne 0) { throw 'Installed-player probe build failed.' }
    } finally { Pop-Location }
    & (Join-Path $root 'vlc-qt\bridge\target\release\verify-player.exe') (Join-Path $installed 'vlc.exe') $media $evidence
    if ($LASTEXITCODE -ne 0) { throw 'Installed-player runtime probe failed.' }
    $pluginLog = (Get-Content -Raw -LiteralPath (Join-Path $evidence 'player.log')).Replace('\','/').ToLowerInvariant()
    $expectedPlugin = ((Join-Path $installed 'qt5\plugins\platforms\qoffscreen.dll').Replace('\','/')).ToLowerInvariant()
    if (-not (($pluginLog -split "`n") | Where-Object { $_.Contains('loaded library') -and $_.Contains($expectedPlugin) })) {
        throw 'The actual loaded Qt platform plugin was not proven to come from the installed payload.'
    }
    if (-not (Test-Path -LiteralPath $ini)) { throw 'VLC+ did not persist its own portable profile.' }
    $settings = Get-Content -Raw -LiteralPath $ini
    foreach ($expected in @('primary=face','inputMode=arrows','layers\flubber\visible=false','layers\grid\visible=false','layers\face\visible=true')) {
        if (($settings -split '\r?\n') -cnotcontains $expected) { throw "Installed exclusive feedback command was not persisted: $expected" }
    }
    & python (Join-Path $PSScriptRoot 'verify-installed.py') $installed --source-commit $commit --receipt (Join-Path $evidence 'post-run-integrity.json')
    if ($LASTEXITCODE -ne 0) { throw 'Immutable payload changed during playback.' }
} catch { $failure = $_ }
finally {
    $env:PATH = $previousPath
    try {
        if (Test-Path -LiteralPath $userRegistration) {
            $registered = (Get-ItemProperty -LiteralPath $userRegistration -ErrorAction Stop).InstallLocation
            if ($registered.TrimEnd('\') -ne $installed.TrimEnd('\')) { throw 'Uninstall registration does not belong to this test; leaving it untouched.' }
            $uninstaller = Join-Path $installed 'unins000.exe'
            $process = Start-Process -FilePath $uninstaller -ArgumentList @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART') -WindowStyle Hidden -PassThru
            Wait-OwnedProcess $process 120000
            if (Test-Path -LiteralPath (Join-Path $installed 'vlc.exe')) { throw 'Uninstall left the executable behind.' }
            $uninstalled = $true
        }
    } catch {
        if (-not $failure) { $failure = $_ }
        else { [IO.File]::WriteAllText((Join-Path $evidence 'cleanup-error.txt'), $_.ToString()) }
    }
}
if ($failure) { throw $failure }
if (-not $uninstalled -or -not (Test-Path -LiteralPath $ini)) { throw 'Install/uninstall or saved-preference retention was not verified.' }
$receipt = [ordered]@{
    schema = 'vlc-plus-installer-check/v1'
    result = 'VERIFIED'
    sourceCommit = $commit
    installerSha256 = (Get-FileHash -LiteralPath $installer -Algorithm SHA256).Hash.ToLowerInvariant()
    installExitCode = 0
    uninstallExitCode = 0
    offscreen = $true
    physicalInputTested = $false
    exclusiveRepresentationsVerified = $true
    userSettingsPreserved = $true
}
[IO.File]::WriteAllText((Join-Path $evidence 'installer-check.json'), (($receipt | ConvertTo-Json) + "`n"), [Text.UTF8Encoding]::new($false))
Write-Output "VERIFIED: installed payload, decode, LSL/CSV, settings and uninstall. Evidence: $evidence"
