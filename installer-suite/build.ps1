#Requires -Version 7.0
[CmdletBinding()]
param([string]$ComponentDirectory = '', [string]$InnoCompiler = 'C:\Program Files (x86)\Inno Setup 6\ISCC.exe')
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$lock = Get-Content -Raw -LiteralPath "$PSScriptRoot/components.lock.json" | ConvertFrom-Json
$commit = (& git -C $root rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or (& git -C $root status --porcelain)) { throw 'Build from a clean committed source tree.' }
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
Copy-Item -LiteralPath "$PSScriptRoot/components.lock.json" -Destination "$stage/components.json"
& git -C $root archive --format=zip "--output=$stage/SuiteSource.zip" HEAD installer-suite LICENSE
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
Copy-Item -LiteralPath "$stage/SuiteSource.zip","$PSScriptRoot/components.lock.json","$PSScriptRoot/INSTALL.txt" -Destination $output
$files = @("01_VLCPlus_and_LSL_Recorder_Setup_$($lock.bundleVersion)_x64.exe",$lock.vlc.downloadName,$lock.recorder.downloadName,'SuiteSource.zip','components.lock.json','INSTALL.txt')
$hashes = [ordered]@{}
foreach ($file in $files) { $hashes[$file] = (Get-FileHash -LiteralPath (Join-Path $output $file) -Algorithm SHA256).Hash.ToLowerInvariant() }
$receipt = [ordered]@{ schema='vlc-recorder-suite-build/v1'; sourceCommit=$commit; bundleVersion=$lock.bundleVersion; components=$lock; launcherSha256=(Get-FileHash "$stage/VLCPlus-and-Recorder.exe").Hash.ToLowerInvariant(); compilerSha256=(Get-FileHash -LiteralPath $InnoCompiler).Hash.ToLowerInvariant(); files=$hashes }
[IO.File]::WriteAllText("$output/build-receipt.json", ($receipt | ConvertTo-Json -Depth 10) + "`n", [Text.UTF8Encoding]::new($false))
$hashes['build-receipt.json'] = (Get-FileHash "$output/build-receipt.json").Hash.ToLowerInvariant()
[IO.File]::WriteAllLines("$output/SHA256SUMS.txt", @($hashes.GetEnumerator() | ForEach-Object { "$($_.Value)  $($_.Key)" }), [Text.UTF8Encoding]::new($false))
Write-Output "Built three installers from $commit in $output"
