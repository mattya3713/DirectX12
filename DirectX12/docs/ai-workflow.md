# AI Workflow: Claude Code × Codex CLI

This project uses two AI agents with distinct roles:

- **Claude Code** — Lead Developer / Architect / Reviewer. Talks to the user,
  understands the request, investigates the codebase, makes design decisions, breaks
  work into small tasks, and reviews what Codex produces. Full role definition:
  `CLAUDE.md` (repo root).
- **Codex CLI** — Implementation Engineer. Reads the task Claude wrote, implements
  exactly that, builds, and reports back. Full role definition: `AGENTS.md` (repo
  root).

The user talks to **Claude Code only**. Codex is invoked by Claude, not by the user
directly.

## Flow

```
User
 │  (feature request / bug report / question)
 ▼
Claude Code
 ├─ Analyze the request
 ├─ Investigate existing code
 ├─ Decide the design / approach
 └─ Write a small, concrete task
 ▼
tasks/current.md
 ▼
Codex (via `codex exec`)
 ├─ Read tasks/current.md + AGENTS.md + relevant code
 ├─ Implement the described scope
 ├─ Build
 └─ Test (manually, where applicable — see docs/coding-rules.md)
 ▼
git diff
 ▼
Claude Code
 ├─ Review the diff against the task and docs/coding-rules.md
 │
 ├─ OK  → mark task done, move tasks/current.md → tasks/done/,
 │        report to the user
 │
 └─ NG  → write a follow-up instruction (still Claude's job to decide
           *what's* wrong; Codex just fixes what it's told)
      │
      └──────────────→ Codex fixes → git diff → Review again
```

## Why this split

- Codex works from a **narrow, explicit task description**, not from the raw user
  request. This keeps its changes scoped and predictable, and means Claude — who has
  the full conversation context and has actually read the surrounding code — is the
  one making design calls, not Codex working from assumptions.
- Claude reviews via `git diff` before anything is considered done. This project has
  no automated test suite (see `docs/coding-rules.md`), so the diff review plus a
  build check *is* the primary quality gate.
- Small, single-purpose tasks make bad Codex output cheap to catch and re-request —
  a large, vague task makes review much harder and increases the chance of scope
  creep or an approach Claude wouldn't have chosen.

## What Claude does *not* delegate to Codex

Per `CLAUDE.md`: very small (few-line) fixes, documentation-only changes, and
anything where invoking Codex would clearly cost more than it saves. Claude also
takes over directly if Codex gets stuck on something it can't resolve within its
task scope — see `AGENTS.md`'s "Stop conditions."

## Where things live

| Thing | Location |
|---|---|
| Claude's role/instructions | `CLAUDE.md` |
| Codex's role/instructions | `AGENTS.md` |
| Current task for Codex | `tasks/current.md` |
| Completed tasks (history) | `tasks/done/` |
| Architecture orientation | `docs/architecture.md` (points to `DESIGN.md` for depth) |
| Coding rules digest | `docs/coding-rules.md` (points to `README.md` for depth) |
| Living design/decision log | `DESIGN.md` (repo root — pre-existing, not part of this setup) |
