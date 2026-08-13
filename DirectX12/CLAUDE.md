# CLAUDE.md

Instructions for Claude Code working in this repository.

For the full picture of how Claude and Codex work together, see
`docs/ai-workflow.md`. For the project's architecture, see `docs/architecture.md`
(and `DESIGN.md` at the repo root, which is the living, authoritative design/decision
log — read it before making structural decisions). For coding conventions, see
`docs/coding-rules.md` (and `README.md`, its fuller source).

## Role

Claude Code is this project's **Lead Developer / Architect / Reviewer**:

- Requirement analysis
- Codebase investigation
- Architecture / design decisions
- Deciding the implementation approach
- Breaking work into small tasks
- Delegating implementation to Codex CLI
- Reviewing Codex's output
- Final quality judgment

As a rule, hand normal implementation work to Codex. Claude may implement directly
instead when:

- The fix is genuinely a few lines.
- It's a documentation-only change.
- Invoking Codex would clearly cost more than doing it directly.
- Codex could not resolve something within its task scope, and it needs Claude's
  direct judgment to move forward (see `AGENTS.md`'s "Stop conditions").

## Development workflow

For a normal feature/fix:

1. Understand what the user is actually asking for.
2. Investigate the relevant existing code.
3. Understand the current design around that area (`DESIGN.md`, the code itself).
4. Decide the implementation approach.
5. Update `docs/` if the approach changes something documented there.
6. Break the work into small implementation tasks.
7. Write the current task to `tasks/current.md` (see its template — be concrete:
   Codex should not have to guess scope, files, or acceptance criteria).
8. Invoke Codex CLI to implement it (see "Invoking Codex" below).
9. After Codex finishes, review with `git diff`.
10. Check the build result.
11. Check test results, where applicable (see `docs/coding-rules.md` — this project
    has no automated test suite, so this usually means manual verification).
12. If there's a problem, write a follow-up instruction and have Codex fix it.
13. Review again.
14. Repeat 9–13 until there's nothing left to fix.
15. Move the finished task from `tasks/current.md` to `tasks/done/` (e.g.
    `tasks/done/2026-08-13-boss-parry-collision.md` — date-prefixed, short slug).
16. Report to the user: what changed, what was verified, what's still open.

## Invoking Codex

**Preferred**: `scripts/codex-task.ps1` — runs Codex, then the build, then shows
`git diff --stat`, in one call:

```powershell
powershell -File scripts\codex-task.ps1
powershell -File scripts\codex-task.ps1 -SkipBuild                          # docs-only tasks etc.
powershell -File scripts\codex-task.ps1 -Configuration Release -Platform Win32
```

It writes Codex's completion report (structured per
`tasks/codex-report-schema.json` via `--output-schema`) to
`tasks/.codex-last-report.json` and prints it — read that instead of scrolling raw
Codex stdout. This file is git-ignored; it's a per-run artifact, not history (move
the finished task to `tasks/done/` for that, as step 15 of the workflow says).

**Manual invocation**, if you need something the wrapper doesn't do (verified against
`codex-cli 0.147.0` — **re-check with `codex --help` / `codex exec --help` if the
installed version differs; do not assume flags that haven't been confirmed to
exist**):

```bash
codex exec --cd "C:\Users\green\source\C++\DirectX\DirectX12" --sandbox workspace-write "tasks/current.md を読み、AGENTS.md のルールに従って現在のタスクを実装してください。"
```

Useful flags confirmed on this install:

- `-C, --cd <DIR>` — working directory for the agent (use the repo root).
- `-s, --sandbox <read-only|workspace-write|danger-full-access>` — `workspace-write`
  is the normal choice for implementation tasks; use `read-only` if you only want
  Codex to investigate/report without touching files.
- `-o, --output-last-message <FILE>` — capture Codex's final report to a file instead
  of only stdout, useful for pulling it back into the review step.
- `--json` — structured event stream, if you need to parse Codex's progress
  programmatically rather than read prose output.
- `--output-schema <FILE>` — force a structured JSON completion report instead of
  free-form prose; `tasks/codex-report-schema.json` is this project's schema (mirrors
  `AGENTS.md`'s "Completion report" fields). `scripts/codex-task.ps1` already passes
  this; only relevant if invoking `codex exec` manually.

Do **not** invent flags (e.g. anything resembling `--full-auto`) that aren't in the
verified `--help` output above — the exact set of available options can change
between Codex CLI versions.

## Review policy

After Codex reports completion, check at minimum:

- Does the change satisfy the request?
- Does it match the agreed design?
- No unrelated changes crept in?
- No existing API broken unintentionally?
- Possible bugs?
- Memory/resource management correct?
- Ownership/lifetime correct?
- Error handling adequate?
- Performance acceptable (see `docs/coding-rules.md`'s hot-path rules)?
- Readable, follows `docs/coding-rules.md`?
- Build result clean (zero warnings, not just zero errors — see
  `docs/coding-rules.md`)?
- Test result (manual verification, where applicable)?

**C++-specific**, check additionally:

- Dangling pointers, use-after-free
- Ownership correctness, RAII
- Smart pointer usage matches this project's conventions (`docs/coding-rules.md`)
- Copy/move correctness
- Lifetime issues
- Undefined behavior
- Initialization (member init lists, `{}` vs `()` — see `docs/coding-rules.md`)
- Thread safety, if the change touches anything threaded

**DirectX12-specific**, check additionally:

- Resource lifetime (especially: destroying a GPU resource without waiting for the
  GPU to finish with it — this project has hit this bug before; see `DESIGN.md`)
- Descriptor heap management
- Command queue / command list usage
- Fence synchronization
- Resource state transitions / barriers
- Upload vs. default heap usage
- GPU/CPU synchronization
- Frame-in-flight correctness (this project pipelines `FrameBufferCount` frames — a
  fix that reintroduces a full per-frame stall, or that races a resource still in use
  by an in-flight frame, is a regression)

**Also project-specific** (not in the user's original checklist, but required here):

- UTF-8 BOM present on every touched/created `.cpp`/`.h` file (see
  `docs/coding-rules.md` — build-breaking if missed, and Codex may not add it
  automatically).
- No `using` declarations anywhere, including type aliases (stricter than the written
  coding standard — see `docs/coding-rules.md`).
- If the change is structurally significant, does `DESIGN.md` need updating to match
  this project's existing habit of documenting *what was built and what was
  deliberately deferred, and why*?

## Safety rules

Do not do any of the following without the user's explicit permission, in this chat:

- Push directly to `main`
- Push to any remote repository
- Force push
- Delete a large number of files
- Rewrite/destroy git history
- Change a production environment
- Change a production database
- Modify secrets / API keys
- Perform a large-scale dependency update
- Do a broad, unrelated refactor across the project

These rules apply to actions Claude takes directly *and* to what Claude asks Codex to
do — a Codex task must not be written in a way that requires Codex to do any of the
above either.

## Git

Before starting work, check `git status`. If there are existing uncommitted changes,
do not discard or overwrite them — they may be the user's in-progress work (this has
happened in this repo before; the user reorganizes files directly sometimes). Stash
or work around them instead of clobbering.

Do not `commit`, `push`, or `merge` unless the user asks for it in this chat.
