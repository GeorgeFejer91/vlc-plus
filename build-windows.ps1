[CmdletBinding()]
param(
    [string]$MsysRoot = '',
    [string]$InnoCompiler = '',
    [ValidateRange(1, 16)][int]$Jobs = 2,
    [switch]$BuildOnly
)

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
if (-not $MsysRoot) { $MsysRoot = Join-Path $root 'build\toolchain\msys64' }
$MsysRoot = (Resolve-Path -LiteralPath $MsysRoot).Path
$bash = Join-Path $MsysRoot 'usr\bin\bash.exe'
$mingw = Join-Path $MsysRoot 'mingw64\bin'
foreach ($tool in @($bash, (Join-Path $mingw 'g++.exe'), (Join-Path $mingw 'qmake-qt5.exe'))) {
    if (-not (Test-Path -LiteralPath $tool -PathType Leaf)) { throw "Build tool missing: $tool" }
}
$lock = Get-Content -Raw -LiteralPath (Join-Path $root 'scripts\windows-build-lock.json') | ConvertFrom-Json
$downloads = Join-Path $root 'build\downloads'
New-Item -ItemType Directory -Force -Path $downloads | Out-Null
$archive = Join-Path $downloads $lock.vlcSource.file
if (-not (Test-Path -LiteralPath $archive)) {
    & curl.exe --fail --location --retry 3 --silent --show-error --output $archive $lock.vlcSource.url
    if ($LASTEXITCODE -ne 0) { throw 'Pinned VLC source download failed.' }
}
if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant() -ne $lock.vlcSource.sha256) {
    throw 'Pinned VLC source hash mismatch.'
}

$previousPath = $env:PATH
$previousMsystem = $env:MSYSTEM
$previousInvoking = $env:CHERE_INVOKING
$previousPathType = $env:MSYS2_PATH_TYPE
try {
    $env:PATH = "$mingw;$env:USERPROFILE\.cargo\bin;$previousPath"
    $env:MSYSTEM = 'MINGW64'
    $env:CHERE_INVOKING = '1'
    $env:MSYS2_PATH_TYPE = 'inherit'
    $env:VLCPLUS_ROOT = $root
    $env:VLCPLUS_JOBS = [string]$Jobs
    Write-Output 'Building the pinned VLC Qt module and launcher...'
    $log = Join-Path $root 'build\windows-build.log'
    & $bash -lc 'bash "$(cygpath -u "$VLCPLUS_ROOT")/scripts/build-vlc-qt.sh"' *> $log
    if ($LASTEXITCODE -ne 0) {
        Get-Content -LiteralPath $log -Tail 65
        throw "VLC native build failed; full log: $log"
    }
    & cargo test --locked --manifest-path (Join-Path $root 'vlc-qt\bridge\Cargo.toml')
    if ($LASTEXITCODE -ne 0) { throw 'Rust bridge tests failed.' }
    $rustLinkLog = Join-Path $root 'build\rust-link-args.txt'
    Push-Location (Join-Path $root 'vlc-qt\bridge')
    try {
        & cargo rustc --release --locked --lib -- --print=link-args *> $rustLinkLog
        if ($LASTEXITCODE -ne 0) { Get-Content $rustLinkLog -Tail 30; throw 'Rust release link failed.' }
    } finally { Pop-Location }
    $linker = $null
    foreach ($match in [regex]::Matches([IO.File]::ReadAllText($rustLinkLog), '"([^"\r\n]+[\\/](?:link|lld-link)\.exe)"')) {
        $candidate = $match.Groups[1].Value.Replace('\\', '\')
        if (Test-Path -LiteralPath $candidate -PathType Leaf) { $linker = $candidate; break }
    }
    if (-not $linker) { throw 'The actual MSVC linker was not captured in the release link command.' }
    & (Join-Path $root 'vlc-qt\test-panel.ps1') -QtBin $mingw -AppearanceJson (Join-Path $root 'examples\planner-appearance.v1.json')
    if ($BuildOnly) { Write-Output 'Native build and focused checks complete.'; return }

    & (Join-Path $root 'package-stock-vlc.ps1') `
        -QtPluginPath (Join-Path $root 'build\vlc-native\modules\.libs\libqt_plugin.dll') `
        -LauncherExePath (Join-Path $root 'build\vlc-native\bin\.libs\vlc.exe') `
        -BridgeDllPath (Join-Path $root 'vlc-qt\bridge\target\release\flubber_bridge.dll') `
        -MinGwBin $mingw
    if (-not $InnoCompiler) {
        $InnoCompiler = Join-Path ${env:ProgramFiles(x86)} 'Inno Setup 6\ISCC.exe'
        if (-not (Test-Path -LiteralPath $InnoCompiler)) { $InnoCompiler = Join-Path $env:ProgramFiles 'Inno Setup 6\ISCC.exe' }
    }
    if (-not (Test-Path -LiteralPath $InnoCompiler -PathType Leaf)) { throw 'Supply -InnoCompiler with the Inno Setup 6 ISCC.exe path.' }
    & $InnoCompiler (Join-Path $root 'installer-stock-vlc.iss')
    if ($LASTEXITCODE -ne 0) { throw 'Installer compilation failed.' }
    $installer = Join-Path $root 'build\stock-vlc-package\out\VLC_Plus_Setup_0.3.1_x64.exe'
    $sourceBundle = Join-Path $root 'build\stock-vlc-package\out\VLC_Plus_Source_0.3.1.zip'
    $innoEngine = Join-Path (Split-Path -Parent $InnoCompiler) 'ISCmplr.dll'
    $receipt = [ordered]@{
        schema = 'vlc-plus-build-receipt/v1'
        sourceCommit = (& git -C $root rev-parse HEAD).Trim()
        installerSha256 = (Get-FileHash -LiteralPath $installer -Algorithm SHA256).Hash.ToLowerInvariant()
        sourceBundleSha256 = (Get-FileHash -LiteralPath $sourceBundle -Algorithm SHA256).Hash.ToLowerInvariant()
        vlcSourceSha256 = $lock.vlcSource.sha256
        gcc = (& (Join-Path $mingw 'gcc.exe') --version | Select-Object -First 1)
        qt = (& (Join-Path $mingw 'qmake-qt5.exe') -query QT_VERSION)
        rustc = @(& rustc -vV)
        cargo = (& cargo --version)
        linker = @{ name = [IO.Path]::GetFileName($linker); version = (Get-Item -LiteralPath $linker).VersionInfo.FileVersion; sha256 = (Get-FileHash -LiteralPath $linker).Hash.ToLowerInvariant() }
        inno = @{ engineVersion = (Get-Item -LiteralPath $innoEngine).VersionInfo.FileVersion; compilerSha256 = (Get-FileHash -LiteralPath $InnoCompiler).Hash.ToLowerInvariant(); engineSha256 = (Get-FileHash -LiteralPath $innoEngine).Hash.ToLowerInvariant() }
        packagesSha256 = (Get-FileHash -LiteralPath (Join-Path $root 'build\toolchain-packages.txt')).Hash.ToLowerInvariant()
        operatingSystem = [Environment]::OSVersion.VersionString
        runnerImage = "$env:ImageOS $env:ImageVersion".Trim()
        bitIdenticalReproducibilityClaimed = $false
    }
    [IO.File]::WriteAllText((Join-Path (Split-Path -Parent $installer) 'build-receipt.json'), (($receipt | ConvertTo-Json -Depth 5) + "`n"), [Text.UTF8Encoding]::new($false))
    Write-Output $installer
}
finally {
    $env:PATH = $previousPath
    $env:MSYSTEM = $previousMsystem
    $env:CHERE_INVOKING = $previousInvoking
    $env:MSYS2_PATH_TYPE = $previousPathType
}
