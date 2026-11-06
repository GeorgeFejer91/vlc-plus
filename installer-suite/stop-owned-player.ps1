[CmdletBinding()]
param(
    [Parameter(Mandatory)][int]$ExpectedProcessId,
    [Parameter(Mandatory)][string]$ExpectedExecutable,
    [Parameter(Mandatory)][ValidateRange(1024,65535)][int]$Port
)
$ErrorActionPreference = 'Stop'
$process = Get-Process -Id $ExpectedProcessId -ErrorAction Stop
$expected = (Resolve-Path -LiteralPath $ExpectedExecutable).Path
if ($process.Path -ne $expected) { throw 'The owned player executable changed; no command was sent.' }
$listeners = @(Get-NetTCPConnection -LocalAddress '127.0.0.1' -LocalPort $Port -State Listen -ErrorAction Stop)
if ($listeners.Count -ne 1 -or $listeners[0].OwningProcess -ne $ExpectedProcessId) {
    throw 'The RC listener does not belong to the exact test-owned player.'
}
$client = [Net.Sockets.TcpClient]::new()
try {
    if (-not $client.ConnectAsync('127.0.0.1', $Port).Wait(3000)) { throw 'Owned RC connection timed out.' }
    $stream = $client.GetStream()
    $stream.ReadTimeout = 3000
    $stream.WriteTimeout = 3000
    $request = [Text.Encoding]::UTF8.GetBytes("quit`n")
    $stream.Write($request, 0, $request.Length)
    $reader = [IO.StreamReader]::new($stream)
    $acknowledged = $false
    for ($lineNumber = 0; $lineNumber -lt 128; $lineNumber++) {
        $line = $reader.ReadLine()
        if ($null -eq $line) { break }
        if ($line -match '^quit: returned 0 ') { $acknowledged = $true; break }
    }
    if (-not $acknowledged) { throw 'Owned VLC did not acknowledge quit.' }
    if (-not $process.WaitForExit(10000)) { throw 'Owned VLC acknowledged quit but did not exit; preserving installation.' }
} finally { $client.Dispose() }
Write-Output 'PASS: exact owned VLC acknowledged RC quit and exited without force termination.'
