# smoke-test.ps1 - build, then launch DirectX12.exe briefly to catch instant crashes.
#
# Why this exists: on 2026-08-24 an uncommitted .hlsl made MainScene startup throw
# an uncaught std::runtime_error, and nobody noticed for ~5 hours because a green
# MSBuild run says nothing about whether the exe actually starts. This script runs
# the real exe after build.ps1 succeeds and judges startup purely by process
# lifetime: if the process is still alive after N seconds it is killed and the test
# passes; if it exits on its own within that window it fails, reporting the exit
# code plus any crash dumps freshly written under Dumps\ (CrashDumpHandler writes
# them relative to the working directory). No window interaction or screen capture
# is involved, so this also works headless.
#
# Usage:
#   powershell -File scripts\smoke-test.ps1                      # build Debug|x64 + smoke test
#   powershell -File scripts\smoke-test.ps1 -SmokeSeconds 10     # longer survival window
#   powershell -File scripts\smoke-test.ps1 -SkipBuild           # smoke-test the last built exe
#   powershell -File scripts\smoke-test.ps1 -Configuration Release
#
# Exit codes: 0 = survived the smoke window; 1 = exited/crashed within it;
# 2 = build failed or exe not found. The judgment reason is always printed to
# stdout, so a dispatcher can just check the exit code and log the output.

param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',

    [ValidateSet('x64', 'Win32')]
    [string]$Platform = 'x64',

    [ValidateRange(1, 120)]
    [int]$SmokeSeconds = 8,

    [switch]$SkipBuild
)

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot

if (-not $SkipBuild) {
    & (Join-Path $PSScriptRoot 'build.ps1') -Configuration $Configuration -Platform $Platform
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[smoke-test] FAIL: build exited with code $LASTEXITCODE. Smoke test not run."
        exit 2
    }
}

# VS output layout differs per platform: x64\Debug\... vs plain Debug\...
$exeCandidates = @(
    (Join-Path $repoRoot "$Platform\$Configuration\DirectX12.exe"),
    (Join-Path $repoRoot "$Configuration\DirectX12.exe")
)
$exe = $exeCandidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
if (-not $exe) {
    Write-Host "[smoke-test] FAIL: DirectX12.exe not found (looked in: $($exeCandidates -join ', '))."
    exit 2
}

# Snapshot existing dumps first so only fresh ones get reported as evidence.
$dumpsDir = Join-Path $repoRoot 'Dumps'
$knownDumps = @{}
if (Test-Path -LiteralPath $dumpsDir) {
    Get-ChildItem -LiteralPath $dumpsDir -Filter 'crash_*.dmp' |
        ForEach-Object { $knownDumps[$_.Name] = $true }
}

# cwd must be the project root so Data\ asset paths resolve and dumps land in Dumps\ there.
Write-Host "[smoke-test] Launching '$exe' (must survive ${SmokeSeconds}s)..."
$proc = Start-Process -FilePath $exe -WorkingDirectory $repoRoot -PassThru

$survived = -not $proc.WaitForExit($SmokeSeconds * 1000)

if ($survived) {
    try { $proc.Kill(); $proc.WaitForExit(5000) | Out-Null } catch { }
    Write-Host "[smoke-test] PASS: process stayed alive for ${SmokeSeconds}s (pid $($proc.Id)) and was terminated."
    exit 0
}

# The game died on its own within the window -> startup failure.
$exitCode = $proc.ExitCode
$newDumps = @()
if (Test-Path -LiteralPath $dumpsDir) {
    $newDumps = @(Get-ChildItem -LiteralPath $dumpsDir -Filter 'crash_*.dmp' |
        Where-Object { -not $knownDumps.ContainsKey($_.Name) })
}

Write-Host "[smoke-test] FAIL: process exited on its own within ${SmokeSeconds}s (pid $($proc.Id), exit code $exitCode)."
foreach ($dump in $newDumps) {
    Write-Host "[smoke-test] Crash dump: $($dump.FullName)"
}
if ($newDumps.Count -eq 0) {
    Write-Host "[smoke-test] No new crash dump was written under Dumps\."
}
exit 1
