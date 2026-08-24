# check-bom.ps1 ? verify (and optionally fix) UTF-8 BOM on .cpp/.h files.
#
# Why: this project requires every .cpp/.h to be saved as UTF-8 WITH BOM (no /utf-8
# compiler flag is set, so the compiler relies on the BOM ? see docs/coding-rules.md).
# Neither Claude's nor Codex's file-write tools add a BOM by default, so this has been
# a recurring manual-check chore. This script automates it.
#
# Usage:
#   powershell -File scripts\check-bom.ps1                  # check all tracked .cpp/.h, report only
#   powershell -File scripts\check-bom.ps1 -Fix              # check + fix all tracked .cpp/.h
#   powershell -File scripts\check-bom.ps1 -StagedOnly -Fix   # check + fix only currently git-staged .cpp/.h (used by the pre-commit hook)
#
# Exit code: 0 if everything has a BOM (or was fixed), 1 if any file was missing a BOM
# and -Fix was not passed (or a listed file could not be read).
#
# 2026-08-23(白虎): 検出漏れの修正。
#  - git出力のパスは既定では非ASCII文字がoctalエスケープされ、さらにPowerShell 5.1の
#    コンソールデコード(CP932)と噛み合わず Test-Path が失敗して「無視」されていた。
#    => core.quotepath=false + Console.OutputEncoding=UTF8 で正しいパスとして受ける.
#  - Test-Pathで落ちる/見つからないファイルは黙って除外せず、検出失敗として扱う.
#  - StagedOnly時のdiff-filterにRename/Type変更も追加.

param(
    [switch]$Fix,
    [switch]$StagedOnly
)

$ErrorActionPreference = 'Stop'
# リポジトルート(DirectX12)を絶対パスで解決し、CWD依存をなくす.
$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
Set-Location -LiteralPath $repoRoot

# gitが出力するパスはリポジトリ(またはworktree)ルート基準。フック経由ではGIT_DIR環境変数が
# rev-parseの解決先を狂わせるため、スクリプト位置基準で確定させる.
$gitRoot = [System.IO.Path]::GetFullPath((Join-Path $repoRoot '..'))

# gitが出力するパスはリポジトリルート基準のため、結合先もそちらに合わせる(StagedOnly時の検出漏れ対策).

# gitが出力する非ASCIIパスを正しく受け取るための設定(これがないと日本語名等が化けて無視される).
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

function Get-TargetFiles {
    # git出力をquotepath無効で受け、.cpp/.hのみ対象化する.
    if ($StagedOnly) {
        $raw = git -C $gitRoot -c core.quotepath=false diff --cached --name-only --diff-filter=ACMRT
    }
    else {
        # 追跡済み+未追跡の両方を対象にする(新規作成ファイルのBOM欠けを見逃さない).
        $tracked = git -C $gitRoot -c core.quotepath=false ls-files -- '*.cpp' '*.h'
        $others  = git -C $gitRoot -c core.quotepath=false ls-files --others --exclude-standard -- '*.cpp' '*.h'
        $raw = @($tracked; $others)
    }

    return $raw |
        Where-Object { $_ -match '\.(cpp|h)$' } |
        Where-Object { $_ -notmatch '^DirectX12/Data/Library/' } |
        Sort-Object -Unique
}

function Test-HasBom([string]$Path) {
    $full = [System.IO.Path]::GetFullPath((Join-Path $gitRoot $Path))
    if (-not [System.IO.File]::Exists($full)) { throw "file not found: $Path" }

    $bytes = [System.IO.File]::ReadAllBytes($full)
    return ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF)
}

$target_files = @(Get-TargetFiles)

if ($target_files.Count -eq 0) {
    Write-Host 'No .cpp/.h files to check.'
    exit 0
}

$missing    = @()
$unreadable = @()

foreach ($f in $target_files) {
    try
    {
        if (-not (Test-HasBom $f)) { $missing += $f }
    }
    catch
    {
        # 読めない/解決できないパスは「BOM有り」とは絶対に扱わない(検出漏れの温床).
        $unreadable += "$f : $($_.Exception.Message)"
    }
}

if ($unreadable.Count -gt 0)
{
    Write-Host 'ERROR: could not inspect the following file(s):'
    $unreadable | ForEach-Object { Write-Host "  $_" }
    Write-Host ''
    Write-Host '(These are treated as failures. Fix the paths or remove stale entries.)'
    exit 1
}

if ($missing.Count -eq 0)
{
    Write-Host "OK: all $($target_files.Count) file(s) have a BOM."
    exit 0
}

if ($Fix)
{
    $utf8Bom = New-Object System.Text.UTF8Encoding $true
    foreach ($f in $missing)
    {
        $full = [System.IO.Path]::GetFullPath((Join-Path $gitRoot $f))
        $content = [System.IO.File]::ReadAllText($full)
        [System.IO.File]::WriteAllText($full, $content, $utf8Bom)
        Write-Host "Fixed (BOM added): $f"
        if ($StagedOnly)
        {
            git -c core.quotepath=false add -- $f
        }
    }
    Write-Host "OK: fixed $($missing.Count) file(s)."
    exit 0
}
else
{
    Write-Host 'Missing BOM:'
    $missing | ForEach-Object { Write-Host "  $_" }
    Write-Host ''
    Write-Host 'Re-run with -Fix to add the BOM automatically.'
    exit 1
}
