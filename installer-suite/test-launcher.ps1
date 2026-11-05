#Requires -Version 7.0
$ErrorActionPreference = 'Stop'
$stage = Join-Path (Split-Path -Parent $PSScriptRoot) 'build/suite-stage'
New-Item -ItemType Directory -Force -Path $stage | Out-Null
$csc = "$env:WINDIR\Microsoft.NET\Framework64\v4.0.30319\csc.exe"
& $csc /nologo /target:exe /main:LaunchBothTests /platform:x64 /r:System.Windows.Forms.dll "/out:$stage\LaunchBoth.Tests.exe" "$PSScriptRoot\LaunchBoth.cs" "$PSScriptRoot\LaunchBoth.Tests.cs"
if ($LASTEXITCODE -ne 0) { throw 'Launcher test compilation failed.' }
& "$stage\LaunchBoth.Tests.exe"
if ($LASTEXITCODE -ne 0) { throw 'Launcher tests failed.' }
