#Requires -Version 7.0
[CmdletBinding()]
param([string]$ComponentDirectory = '', [string]$InnoCompiler = 'C:\Program Files (x86)\Inno Setup 6\ISCC.exe', [switch]$BuiltFromSource)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$lock = Get-Content -Raw -LiteralPath "$PSScriptRoot/components.lock.json" | ConvertFrom-Json
$commit = (& git -C $root rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or (& git -C $root status --porcelain)) { throw 'Build from a clean committed source tree.' }
if ($BuiltFromSource) {
    $playerBuild = Get-Content -Raw "$root/build/stock-vlc-package/out/build-receipt.json" | ConvertFrom-Json
    $recorderBuild = Get-Content -Raw "$root/build/recorder-release/build-receipt.json" | ConvertFrom-Json
    $playerCheck = Get-Content -Raw "$root/build/component-evidence/vlc/installer-check.json" | ConvertFrom-Json
    $recorderCheck = Get-Content -Raw "$root/build/component-evidence/recorder/installer-check.json" | ConvertFrom-Json
    foreach ($receipt in @($playerBuild,$recorderBuild,$playerCheck,$recorderCheck)) {
        if ($receipt.sourceCommit -ne $commit) { throw 'A built component receipt belongs to different source.' }
    }
    if ($playerCheck.result -ne 'VERIFIED' -or $recorderCheck.result -ne 'VERIFIED' -or
        $playerCheck.installerSha256 -ne $playerBuild.installerSha256 -or
        $recorderCheck.installerSha256 -ne $recorderBuild.installerSha256 -or
        $recorderCheck.applicationSha256 -ne $recorderBuild.applicationSha256) { throw 'Both exact standalone installers must pass before joint packaging.' }
    if (-not $ComponentDirectory) { $ComponentDirectory = Join-Path $root 'build/source-components' }
    New-Item -ItemType Directory -Force -Path $ComponentDirectory | Out-Null
    $playerVersion = [regex]::Match([IO.File]::ReadAllText("$root/installer-stock-vlc.iss"), '#define ProductVersion "([0-9.]+)"').Groups[1].Value
    $recorderVersion = (Get-Content -Raw "$root/recorder/src-tauri/tauri.conf.json" | ConvertFrom-Json).version
    if ($playerVersion -notmatch '^\d+\.\d+\.\d+$' -or $recorderVersion -notmatch '^\d+\.\d+\.\d+$' -or
        $recorderBuild.version -ne $recorderVersion -or $recorderBuild.file -ne "Remote_LSL_Recorder_$($recorderVersion)_x64-setup.exe") { throw 'Built component version/file differs from current source.' }
    $playerFile = "VLC_Plus_Setup_$($playerVersion)_x64.exe"
    Copy-Item "$root/build/stock-vlc-package/out/$playerFile" (Join-Path $ComponentDirectory $playerFile) -Force
    Copy-Item "$root/build/recorder-release/$($recorderBuild.file)" (Join-Path $ComponentDirectory $recorderBuild.file) -Force
    $lock = [pscustomobject]@{
        schema='vlc-recorder-suite-components/v1'; sourceMode='same-checkout'; bundleVersion='0.3.3'
        vlc=@{ name='VLC+'; version=$playerVersion; repository='GeorgeFejer91/vlc-plus'; sourceCommit=$commit; file=$playerFile; sha256=$playerBuild.installerSha256; downloadName="02_VLCPlus_$($playerVersion)_Setup_x64.exe" }
        recorder=@{ name='Remote LSL Recorder'; version=$recorderBuild.version; repository='GeorgeFejer91/vlc-plus'; sourceCommit=$commit; file=$recorderBuild.file; sha256=$recorderBuild.installerSha256; applicationSha256=$recorderBuild.applicationSha256; downloadName="03_Remote_LSL_Recorder_$($recorderBuild.version)_Setup_x64.exe" }
    }
}
$stage = Join-Path $root 'build/suite-stage'
$output = Join-Path $root 'build/suite-release'
New-Item -ItemType Directory -Force -Path $stage,$output | Out-Null
if (-not $ComponentDirectory) { $ComponentDirectory = Join-Path $root 'build/suite-components' }
New-Item -ItemType Directory -Force -Path $ComponentDirectory | Out-Null
$componentPaths = @{}
foreach ($name in @('vlc','recorder')) {
    $component = $lock.$name
    if ($component.file -ne [IO.Path]::GetFileName($component.file) -or $component.sha256 -notmatch '^[a-f0-9]{64}$') { throw 'Invalid component lock.' }
    $path = Join-Path $ComponentDirectory $component.file
    if (-not (Test-Path -LiteralPath $path)) {
        if ($component.url -notmatch '^https://github.com/GeorgeFejer91/(vlc-plus|Remote-LSL-Recorder)/releases/download/') { throw 'Unexpected component origin.' }
        Invoke-WebRequest -Uri $component.url -OutFile $path
    }
    if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() -ne $component.sha256) { throw "Wrong $name installer bytes; nothing was executed." }
    $componentPaths[$name] = (Resolve-Path -LiteralPath $path).Path
}
& "$PSScriptRoot/test-launcher.ps1"
if ($LASTEXITCODE -ne 0) { throw 'Launcher tests failed.' }
$csc = "$env:WINDIR\Microsoft.NET\Framework64\v4.0.30319\csc.exe"
& $csc /nologo /target:winexe /platform:x64 /r:System.Windows.Forms.dll "/out:$stage\VLCPlus-and-Recorder.exe" "$PSScriptRoot\LaunchBoth.cs"
if ($LASTEXITCODE -ne 0) { throw 'Launcher compilation failed.' }
[IO.File]::WriteAllText("$stage/components.json", ($lock | ConvertTo-Json -Depth 6) + "`n", [Text.UTF8Encoding]::new($false))
& git -C $root archive --format=zip "--output=$stage/SuiteSource.zip" HEAD
if ($LASTEXITCODE -ne 0) { throw 'Source archive failed.' }
$defines = [ordered]@{ BundleVersion=$lock.bundleVersion; Stage=$stage; VlcInstaller=$componentPaths.vlc; RecorderInstaller=$componentPaths.recorder; VlcFile=$lock.vlc.file; RecorderFile=$lock.recorder.file; VlcSha=$lock.vlc.sha256; RecorderSha=$lock.recorder.sha256 }
$lines = foreach ($entry in $defines.GetEnumerator()) {
    if ($entry.Value -match '["\r\n]') { throw 'Unsafe Inno input.' }
    '#define ' + $entry.Key + ' "' + $entry.Value + '"'
}
[IO.File]::WriteAllLines("$stage/inputs.iss", $lines, [Text.UTF8Encoding]::new($false))
& $InnoCompiler "$PSScriptRoot/suite.iss"
if ($LASTEXITCODE -ne 0) { throw 'Joint installer compilation failed.' }
foreach ($name in @('vlc','recorder')) {
    Copy-Item -LiteralPath $componentPaths[$name] -Destination (Join-Path $output $lock.$name.downloadName)
}
Copy-Item -LiteralPath "$stage/SuiteSource.zip","$PSScriptRoot/INSTALL.txt" -Destination $output
Copy-Item -LiteralPath "$stage/components.json" -Destination "$output/components.lock.json"
$files = @("01_VLCPlus_and_LSL_Recorder_Setup_$($lock.bundleVersion)_x64.exe",$lock.vlc.downloadName,$lock.recorder.downloadName,'SuiteSource.zip','components.lock.json','INSTALL.txt')
if ($BuiltFromSource) {
    $sourceName = "VLC_Plus_Source_$($lock.vlc.version).zip"
    Copy-Item -LiteralPath @("$root/build/stock-vlc-package/out/$sourceName", "$root/build/recorder-release/RecorderSource.zip") -Destination $output
    Copy-Item "$root/build/stock-vlc-package/out/build-receipt.json" "$output/vlc-build-receipt.json"
    Copy-Item "$root/build/recorder-release/build-receipt.json" "$output/recorder-build-receipt.json"
    $files += @($sourceName,'RecorderSource.zip','vlc-build-receipt.json','recorder-build-receipt.json')
}
$hashes = [ordered]@{}
foreach ($file in $files) { $hashes[$file] = (Get-FileHash -LiteralPath (Join-Path $output $file) -Algorithm SHA256).Hash.ToLowerInvariant() }
$receipt = [ordered]@{ schema='vlc-recorder-suite-build/v1'; sourceCommit=$commit; bundleVersion=$lock.bundleVersion; components=$lock; launcherSha256=(Get-FileHash "$stage/VLCPlus-and-Recorder.exe").Hash.ToLowerInvariant(); compilerSha256=(Get-FileHash -LiteralPath $InnoCompiler).Hash.ToLowerInvariant(); files=$hashes }
[IO.File]::WriteAllText("$output/build-receipt.json", ($receipt | ConvertTo-Json -Depth 10) + "`n", [Text.UTF8Encoding]::new($false))
$hashes['build-receipt.json'] = (Get-FileHash "$output/build-receipt.json").Hash.ToLowerInvariant()
[IO.File]::WriteAllLines("$output/SHA256SUMS.txt", @($hashes.GetEnumerator() | ForEach-Object { "$($_.Value)  $($_.Key)" }), [Text.UTF8Encoding]::new($false))
Write-Output "Built three installers from $commit in $output"
