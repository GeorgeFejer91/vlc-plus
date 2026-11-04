[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$InstallerPath,
    [string]$EvidenceDirectory = ''
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$installer = (Resolve-Path -LiteralPath $InstallerPath).Path
if (-not $EvidenceDirectory) { $EvidenceDirectory = Join-Path $root ('build\validation\' + [Guid]::NewGuid().ToString('N')) }
if (Test-Path -LiteralPath $EvidenceDirectory) { throw 'Use a fresh evidence directory; previous evidence is preserved.' }
New-Item -ItemType Directory -Path $EvidenceDirectory | Out-Null
$evidence = (Resolve-Path -LiteralPath $EvidenceDirectory).Path
$installed = Join-Path $evidence 'installed'
$media = Join-Path $evidence 'validation.mp4'
Copy-Item -LiteralPath (Join-Path $root 'test\fixtures\validation.mp4') -Destination $media
$commit = (& git -C $root rev-parse HEAD).Trim()
$arguments = @('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', '/CURRENTUSER', '/NOICONS', '/TASKS=', ('/DIR="' + $installed + '"'), ('/LOG="' + (Join-Path $evidence 'install.log') + '"'))
$process = Start-Process -FilePath $installer -ArgumentList $arguments -WindowStyle Hidden -PassThru -Wait
if ($process.ExitCode -ne 0) { throw "Installer exited with $($process.ExitCode)" }
& python (Join-Path $PSScriptRoot 'verify-installed.py') $installed --source-commit $commit --receipt (Join-Path $evidence 'integrity.json')
if ($LASTEXITCODE -ne 0) { throw 'Installed integrity check failed.' }
$previousPath = $env:PATH
try {
    $env:PATH = "$env:USERPROFILE\.cargo\bin;$previousPath"
    Push-Location (Join-Path $root 'vlc-qt\bridge')
    try {
        & cargo build --release --locked --bin verify-player
        if ($LASTEXITCODE -ne 0) { throw 'Installed-player probe build failed.' }
    } finally { Pop-Location }
    & (Join-Path $root 'vlc-qt\bridge\target\release\verify-player.exe') (Join-Path $installed 'vlc.exe') $media $evidence
    if ($LASTEXITCODE -ne 0) { throw 'Installed-player runtime probe failed.' }
} finally { $env:PATH = $previousPath }
$ini = Join-Path $installed 'portable\vlc\vlc-qt-interface.ini'
if (-not (Test-Path -LiteralPath $ini)) { throw 'VLC+ did not persist settings in its own portable profile.' }
$settings = Get-Content -Raw -LiteralPath $ini
foreach ($expected in @('primary=grid', 'inputMode=arrows', 'layers\face\visible=true', 'layers\grid\visible=true')) {
    if (-not $settings.Contains($expected)) { throw "Installed feedback command was not persisted: $expected" }
}
& python (Join-Path $PSScriptRoot 'verify-installed.py') $installed --source-commit $commit --receipt (Join-Path $evidence 'post-run-integrity.json')
if ($LASTEXITCODE -ne 0) { throw 'Payload changed during playback.' }
$uninstaller = Join-Path $installed 'unins000.exe'
$process = Start-Process -FilePath $uninstaller -ArgumentList @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART') -WindowStyle Hidden -PassThru -Wait
if ($process.ExitCode -ne 0 -or (Test-Path -LiteralPath (Join-Path $installed 'vlc.exe'))) { throw 'Uninstall did not remove the executable.' }
$receipt = [ordered]@{
    schema = 'vlc-plus-installer-check/v1'
    result = 'VERIFIED'
    sourceCommit = $commit
    installerSha256 = (Get-FileHash -LiteralPath $installer -Algorithm SHA256).Hash.ToLowerInvariant()
    installExitCode = 0
    uninstallExitCode = $process.ExitCode
    offscreen = $true
    physicalInputTested = $false
    userSettingsPreserved = (Test-Path -LiteralPath $ini)
}
[IO.File]::WriteAllText((Join-Path $evidence 'installer-check.json'), (($receipt | ConvertTo-Json) + "`n"), [Text.UTF8Encoding]::new($false))
Write-Output "VERIFIED: installed payload, real decode, LSL/CSV, feedback settings and uninstall. Evidence: $evidence"
