param([Parameter(Mandatory)][string]$QtBin, [string]$OutputDirectory = '')
$ErrorActionPreference = 'Stop'
$qt = (Resolve-Path -LiteralPath $QtBin).Path
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $PSScriptRoot '../build/native-key-check' }
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$output = (Resolve-Path -LiteralPath $OutputDirectory).Path
$previous = $env:PATH
try {
    $env:PATH = "$qt;$previous"
    $flags = & (Join-Path $qt 'pkg-config.exe') --cflags --libs Qt5Widgets Qt5Gui Qt5Core
    if ($LASTEXITCODE -ne 0) { throw 'Qt pkg-config failed.' }
    & (Join-Path $qt 'g++.exe') -std=c++11 -O1 "-I$PSScriptRoot" (Join-Path $PSScriptRoot 'native-key-check.cpp') ($flags -split ' ') -o (Join-Path $output 'native-key-check.exe')
    if ($LASTEXITCODE -ne 0) { throw 'Native key test compilation failed.' }
    & (Join-Path $output 'native-key-check.exe')
    if ($LASTEXITCODE -ne 0) { throw "Native key test failed: $LASTEXITCODE" }
} finally { $env:PATH = $previous }
