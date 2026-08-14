# codex-task.ps1 — run the current delegated task end-to-end: Codex → build → diff.
#
# Combines the three separate steps Claude would otherwise run one at a time
# (invoke Codex, build, review the diff) into a single call, and captures Codex's
# structured completion report to a file so Claude can read it without the raw
# stdout noise. See docs/ai-workflow.md and CLAUDE.md ("Invoking Codex").
#
# Usage:
#   powershell -File scripts\codex-task.ps1
#   powershell -File scripts\codex-task.ps1 -SkipBuild        # if the task is docs-only etc.
#   powershell -File scripts\codex-task.ps1 -Configuration Release -Platform Win32
#
# This does not commit, push, or move tasks/current.md to tasks/done/ — that stays a
# deliberate step Claude takes after reviewing the output (see CLAUDE.md).

param(
    [switch]$SkipBuild,
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',
    [ValidateSet('x64', 'Win32')]
    [string]$Platform = 'x64',
    [string]$Model = 'gpt-5.6-luna'  # project default model. Pass -Model '' to use Codex's own configured default instead.
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $repoRoot

$taskFile = Join-Path $repoRoot 'tasks\current.md'
if (-not (Test-Path $taskFile)) {
    Write-Error "tasks\current.md not found. Write the task before invoking Codex."
    exit 1
}

$reportFile = Join-Path $repoRoot 'tasks\.codex-last-report.json'
$schemaFile = Join-Path $repoRoot 'tasks\codex-report-schema.json'

Write-Host '=== 1/3: Codex implementation ==='
$modelArgs = @()
if ($Model) { $modelArgs = @('--model', $Model) }

& codex exec `
    --cd $repoRoot `
    --sandbox danger-full-access `
    --output-schema $schemaFile `
    --output-last-message $reportFile `
    @modelArgs `
    "tasks/current.md を読み、AGENTS.md のルールに従って現在のタスクを実装してください。完了したら指定されたスキーマに従って結果を報告してください。"
$codexExit = $LASTEXITCODE

Write-Host ''
Write-Host '=== Codex report ==='
if (Test-Path $reportFile) {
    Get-Content $reportFile -Raw | Write-Host
} else {
    Write-Host '(no report file produced)'
}

if ($SkipBuild) {
    Write-Host ''
    Write-Host '=== 2/3: Build (skipped) ==='
} else {
    Write-Host ''
    Write-Host '=== 2/3: Build ==='
    & (Join-Path $PSScriptRoot 'build.ps1') -Configuration $Configuration -Platform $Platform
}
$buildExit = $LASTEXITCODE

Write-Host ''
Write-Host '=== 3/3: git diff (stat) ==='
git diff --stat
Write-Host ''
Write-Host 'Run `git diff` (no --stat) for the full diff, or `git diff -- <path>` for one file.'

Write-Host ''
Write-Host '--- Summary ---'
Write-Host "Codex exit code: $codexExit"
if (-not $SkipBuild) { Write-Host "Build exit code:  $buildExit" }
Write-Host "Report file:      $reportFile"
