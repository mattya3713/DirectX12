# build.ps1 — locate MSBuild via vswhere and build DirectX12.vcxproj.
#
# Why this exists: the exact MSBuild.exe path depends on which VS version/edition is
# installed (e.g. "Visual Studio\18\Community" vs "2022\Professional"), which is not
# something either Claude or Codex should hardcode or re-discover by hand every time.
# This script is the one place that logic lives.
#
# Usage:
#   powershell -File scripts\build.ps1                          # Debug|x64 (default)
#   powershell -File scripts\build.ps1 -Configuration Release
#   powershell -File scripts\build.ps1 -Platform Win32
#
# Exit code is MSBuild's own exit code (0 = success). Warnings do not affect the exit
# code, so check the "warnings" line in the summary too — see docs/coding-rules.md:
# this project maintains a zero-warning build, not just "no errors."

param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',

    [ValidateSet('x64', 'Win32')]
    [string]$Platform = 'x64'
)

$ErrorActionPreference = 'Stop'

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    Write-Error "vswhere.exe not found at '$vswhere'. Is Visual Studio installed?"
    exit 1
}

$msbuild = & $vswhere -latest -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe" | Select-Object -First 1
if (-not $msbuild -or -not (Test-Path $msbuild)) {
    Write-Error "MSBuild.exe not found via vswhere. Is the 'Desktop development with C++' workload installed?"
    exit 1
}

$repoRoot = Split-Path -Parent $PSScriptRoot
$vcxproj = Join-Path $repoRoot 'DirectX12.vcxproj'

Write-Host "MSBuild: $msbuild"
Write-Host "Project: $vcxproj"
Write-Host "Config:  $Configuration|$Platform"
Write-Host ''

& $msbuild $vcxproj -p:Configuration=$Configuration -p:Platform=$Platform -m -v:normal -t:Build 2>&1 |
    Tee-Object -Variable buildOutput | Out-Host

$exitCode = $LASTEXITCODE

$errorLines   = $buildOutput | Select-String -Pattern '\): error'
$warningLines = $buildOutput | Select-String -Pattern '\): warning'

Write-Host ''
Write-Host '--- Summary ---'
Write-Host "Exit code: $exitCode"
Write-Host "Errors:    $($errorLines.Count)"
Write-Host "Warnings:  $($warningLines.Count)"

if ($warningLines.Count -gt 0) {
    Write-Host ''
    Write-Host 'Warning lines:'
    $warningLines | ForEach-Object { Write-Host "  $_" }
}

exit $exitCode
