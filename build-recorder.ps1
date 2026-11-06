[CmdletBinding()]
param([switch]$CheckOnly)
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$commit = (& git -C $root rev-parse HEAD).Trim()
if (-not $CheckOnly -and (& git -C $root status --porcelain)) { throw 'Build installers from clean committed source.' }
Push-Location (Join-Path $PSScriptRoot 'recorder')
try {
    & pnpm install --frozen-lockfile
    if ($LASTEXITCODE -ne 0) { throw 'Recorder locked dependency setup failed.' }
    & ./scripts/fetch-labrecorder.ps1
    & pnpm check
    if ($LASTEXITCODE -ne 0) { throw 'Recorder source checks failed.' }
    & pnpm test:ui
    if ($LASTEXITCODE -ne 0) { throw 'Recorder rendered checks failed.' }
    if (-not $CheckOnly) {
        & pnpm tauri build --bundles nsis
        if ($LASTEXITCODE -ne 0) { throw 'Recorder installer build failed.' }
        $version = (Get-Content -Raw src-tauri/tauri.conf.json | ConvertFrom-Json).version
        $target = if ($env:CARGO_TARGET_DIR) { $env:CARGO_TARGET_DIR } else { Join-Path (Get-Location) 'src-tauri/target' }
        $installer = Join-Path $target "release/bundle/nsis/Remote LSL Recorder_$($version)_x64-setup.exe"
        $application = Join-Path $target 'release/remote-lsl-recorder.exe'
        $output = Join-Path $root 'build/recorder-release'
        New-Item -ItemType Directory -Force -Path $output | Out-Null
        $file = "Remote_LSL_Recorder_$($version)_x64-setup.exe"
        Copy-Item -LiteralPath $installer -Destination (Join-Path $output $file) -Force
        & git -C $root archive --format=zip "--output=$output/RecorderSource.zip" HEAD
        if ($LASTEXITCODE -ne 0) { throw 'Recorder matching-source archive failed.' }
        $receipt = [ordered]@{
            schema='vlc-plus-recorder-build/v1'; sourceCommit=$commit; version=$version; file=$file
            installerSha256=(Get-FileHash -LiteralPath $installer).Hash.ToLowerInvariant()
            applicationSha256=(Get-FileHash -LiteralPath $application).Hash.ToLowerInvariant()
            sourceSha256=(Get-FileHash -LiteralPath "$output/RecorderSource.zip").Hash.ToLowerInvariant()
            rustc=(& rustc --version); node=(& node --version); pnpm=(& pnpm --version)
            engine=@{ cliSha256=(Get-FileHash vendor/labrecorder-win/LabRecorderCLI.exe).Hash.ToLowerInvariant(); lslSha256=(Get-FileHash vendor/labrecorder-win/lsl.dll).Hash.ToLowerInvariant() }
        }
        [IO.File]::WriteAllText("$output/build-receipt.json", ($receipt | ConvertTo-Json -Depth 4) + "`n", [Text.UTF8Encoding]::new($false))
        Write-Output "Built self-contained Recorder $version from $commit."
    }
} finally { Pop-Location }
