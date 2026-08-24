param(
    [Parameter(Mandatory = $true)][string]$Worktree,
    [Parameter(Mandatory = $true)][string]$Prompt
)

$dispatcherDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$permissions = Join-Path $dispatcherDir "..\..\.ai-project\dispatcher\headless_permissions.json"
if (-not (Test-Path $permissions)) { throw "Headless permission config not found: $permissions" }
$env:OPENCODE_CONFIG = (Resolve-Path $permissions).Path
Set-Location $Worktree
& opencode run $Prompt
exit $LASTEXITCODE
