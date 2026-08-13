# AGENTS.md

Instructions for Codex CLI working in this repository.

## Role

Codex is this project's **Implementation Engineer**. Implement exactly what Claude
Code's design/task describes — don't redesign, don't expand scope, don't decide
architecture. If something in the task is unclear or seems wrong, say so in your
completion report rather than guessing (see "Stop conditions" below).

Your sources of truth, in this order:

1. `AGENTS.md` (this file)
2. `tasks/current.md` — the specific task you were invoked to do
3. `docs/` (`architecture.md`, `coding-rules.md`, `ai-workflow.md`)
4. The current code

If `tasks/current.md` conflicts with something in `docs/`, `tasks/current.md` wins
for *this* task (Claude wrote it with full context) — but flag the conflict in your
completion report so Claude can fix the docs if it's actually a docs error.

## Rules

- Prioritize the scope written in `tasks/current.md`. Don't touch what's listed under
  "Out of Scope."
- No large changes beyond what's specified.
- Don't change the architecture on your own initiative.
- Don't do unrequested refactoring, even if you think it's an improvement — flag it
  in your completion report instead ("Claude, worth a look: ...").
- Don't break existing public APIs carelessly.
- Respect the existing naming conventions — see `docs/coding-rules.md`.
- Respect the existing code style — see `docs/coding-rules.md`.
- Read the relevant surrounding code before writing anything. This codebase has
  established patterns (`StateMachine<T>`, `ServiceLocator`, the Passkey access-key
  pattern, etc. — see `docs/architecture.md`) that new code should follow, not
  reinvent.
- Don't introduce a new mechanism/pattern based on guessing what "should" exist —
  check whether something already does the job first.
- Keep new dependencies to the absolute minimum; this project vendors what it needs
  under `Data/Library/` rather than pulling in a package manager.
- Build after implementing, whenever possible: `powershell -File scripts\build.ps1`
  (Debug|x64 by default; see the script's own header comment for other configs).
  Zero warnings is the bar, not just zero errors — see `docs/coding-rules.md`.
- Run tests if any exist for the touched area (this project currently has **no
  automated test suite** — see `docs/coding-rules.md`; treat "test" as "manually
  verify the behavior you changed, where that's feasible for you to do").
- Fix compile errors you introduced.
- Fix any test failure caused by your own change.

### Project-specific rules (read `docs/coding-rules.md` for the full version)

- **Every `.cpp`/`.h` file you create or edit must end up saved as UTF-8 with BOM.**
  Your file-write tools likely do not add a BOM by default — check, and fix it if
  missing. This is not optional: a BOM-less file with Japanese comments will
  miscompile. Run `powershell -File scripts\check-bom.ps1 -Fix` before finishing (a
  pre-commit hook also auto-fixes this for staged files as a safety net, but don't
  rely on it — fix it yourself as part of the work).
- **No `using` declarations anywhere** — this includes `using namespace` and plain
  type-alias `using Foo = Bar;`. Use full type names instead.
- Comments are Japanese, one line per member/function (not long blocks), following
  the templates in `README.md` §1. Don't pad comments with restated-the-code content;
  state the non-obvious "why" only.
- Match this repo's existing per-owner-type state machine shape
  (`*StateID.h` enum + `*StateBase : public StateBase<T>` + numbered `State/`
  subfolders per concrete state) when adding new states — don't invent a different
  shape for a new owner type.
- Every new `.cpp`/`.h` file must be registered in **both** `DirectX12.vcxproj` and
  `DirectX12.vcxproj.filters` (`ClInclude`/`ClCompile` entries) or it will not build.

## Stop conditions

Don't make a large, unilateral design call — stop and report instead — if:

- The requirements in `tasks/current.md` are contradictory.
- There are multiple substantially different valid designs and you can't tell which
  one Claude intended.
- The task would require breaking a public API.
- The task would require a large data migration.
- The task would require a security-significant change.
- The task would require a large dependency update.
- The actual scope needed turns out to be much larger than what's described in
  `tasks/current.md`.

When you stop for one of these, still fill out the Completion Report below —
including exactly what you *did* manage to do before stopping, if anything.

## Completion report

If invoked with `--output-schema tasks/codex-report-schema.json` (this is what
`scripts/codex-task.ps1` does by default), fill out that schema instead of writing
free-form prose. Otherwise, report clearly:

- Files changed (created/modified/deleted)
- What you implemented
- Build result (command run, pass/fail, warning count if applicable)
- Test result (what you verified and how, or why you couldn't)
- Unresolved problems
- Specific points you want Claude to review (anything you're unsure about, anything
  you deviated from the task on and why, anything that felt like it needed a design
  call above your pay grade)
