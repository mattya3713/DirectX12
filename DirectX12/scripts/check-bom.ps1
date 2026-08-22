# check-bom.ps1 — verify (and optionally fix) UTF-8 BOM on .cpp/.h files.
#
# Why: this project requires every .cpp/.h to be saved as UTF-8 WITH BOM (no /utf-8
# compiler flag is set, so the compiler relies on the BOM — see docs/coding-rules.md).
# Neither Claude's nor Codex's file-write tools add a BOM by default, so this has been
# a recurring manual-check chore. This script automates it.
#
# Usage:
#   powershell -File scripts\check-bom.ps1                  # check all tracked .cpp/.h, report only
#   powershell -File scripts\check-bom.ps1 -Fix              # check + fix all tracked .cpp/.h
#   powershell -File scripts\check-bom.ps1 -StagedOnly -Fix   # check + fix only currently git-staged .cpp/.h (used by the pre-commit hook)
#
# Exit code: 0 if everything has a BOM (or was fixed), 1 if any file was missing a BOM
# and -Fix was not passed.

param(
    [switch]$Fix,
    [switch]$StagedOnly
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $repoRoot

function Test-HasBom([string]$Path) {
    $bytes = [System.IO.File]::ReadAllBytes($Path)
    return ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF)
}

if ($StagedOnly) {
    $files = git diff --cached --name-only --diff-filter=ACM |
        Where-Object { $_ -match '\.(cpp|h)$' } |
        Where-Object { Test-Path $_ }
} else {
    # 追跡済み+未追跡の両方を対象にする(新規作成直後のファイルのBOM欠けを見逃さないため).
    $files = @(
        git ls-files -- '*.cpp' '*.h'
        git ls-files --others --exclude-standard -- '*.cpp' '*.h'
    ) |
        Where-Object { $_ -notmatch '^Data/Library/' } |
        Where-Object { Test-Path $_ } |
        Sort-Object -Unique
}

if (-not $files) {
    Write-Host 'No .cpp/.h files to check.'
    exit 0
}

$missing = @()
foreach ($f in $files) {
    if (-not (Test-HasBom $f)) {
        $missing += $f
    }
}

if (-not $missing) {
    Write-Host "OK: all $($files.Count) file(s) have a BOM."
    exit 0
}

if ($Fix) {
    $utf8Bom = New-Object System.Text.UTF8Encoding $true
    foreach ($f in $missing) {
        $content = [System.IO.File]::ReadAllText($f)
        [System.IO.File]::WriteAllText($f, $content, $utf8Bom)
        Write-Host "Fixed (BOM added): $f"
        if ($StagedOnly) {
            git add -- $f
        }
    }
    exit 0
} else {
    Write-Host 'Missing BOM:'
    $missing | ForEach-Object { Write-Host "  $_" }
    Write-Host ''
    Write-Host 'Re-run with -Fix to add the BOM automatically.'
    exit 1
}
