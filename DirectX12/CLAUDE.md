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

## Learning-first development (read this before every new feature)

This project has **two goals**, not one:

1. Build a working DirectX12-based game engine.
2. Grow the user's own skill as a game engine programmer.

**Do not optimize for completion speed alone.** The measure of success for a task
is not just "it works" — it's "it works, and the user understands why." Concretely,
before implementing any new feature:

1. Extract the list of things the user would need to learn to understand this
   feature, and sort them into three buckets, presented to the user:
   - **今回必須 (required for this task)** — concepts load-bearing enough that
     without them, the user can't meaningfully review, explain, or debug the
     result.
   - **理解推奨 (recommended, not required)** — useful context that deepens
     understanding but isn't essential to follow this particular change.
   - **今回は後回し (deferred)** — real topics, deliberately out of scope for now,
     so the learning surface doesn't balloon into "learn everything at once."
2. For anything in **今回必須** the user doesn't yet understand, help them
   understand it (explain, point at the relevant existing code, work through it
   together) **before** delegating implementation to Codex. Don't hand off a task
   the user can't yet follow.
3. For design decisions, prefer letting the user propose an approach first and
   reviewing it, rather than leading with Claude's own finished design. Jumping
   straight to a polished proposal is the default failure mode to avoid here —
   only do so when the user asks for it directly, or when there's no real design
   choice to make.
4. After Codex finishes implementing, check whether the user is in a position to
   explain the implementation in their own words — not just "does it build and
   run." Any important part they can't explain becomes a new learning item, not
   something to silently paper over.

The end goal is **not** "the AI can build a game engine." It's "the user can
understand, design, and debug a game engine themselves, with AI as a force
multiplier on that ability." When this section and plain task-completion speed
pull in different directions, this section wins.

## Development workflow

For a normal feature/fix:

1. Understand what the user is actually asking for.
2. Investigate the relevant existing code.
3. Understand the current design around that area (`DESIGN.md`, the code itself).
4. Apply "Learning-first development" above: surface the 必須/推奨/後回し learning
   breakdown, and close any gaps in 今回必須 items with the user before proceeding.
5. Decide the implementation approach — for real design choices, let the user
   propose first per "Learning-first development," then review it together.
6. Update `docs/` if the approach changes something documented there.
7. Break the work into small implementation tasks.
8. Write the current task to `tasks/current.md` (see its template — be concrete:
   Codex should not have to guess scope, files, or acceptance criteria).
9. Invoke Codex CLI to implement it (see "Invoking Codex" below).
10. After Codex finishes, review with `git diff`.
11. Check the build result.
12. Check test results, where applicable (see `docs/coding-rules.md` — this project
    has no automated test suite, so this usually means manual verification).
13. Check the user can explain the implementation (see "Learning-first
    development") — treat anything they can't as a fresh learning item.
14. If there's a problem, write a follow-up instruction and have Codex fix it.
15. Review again.
16. Repeat 10–15 until there's nothing left to fix.
17. Move the finished task from `tasks/current.md` to `tasks/done/` (e.g.
    `tasks/done/2026-08-13-boss-parry-collision.md` — date-prefixed, short slug).
18. Report to the user: what changed, what was verified, what's still open.

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
