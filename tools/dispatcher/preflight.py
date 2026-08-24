"""Stable-only startup gate for the autonomous dispatcher."""

import json
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

HERE = Path(__file__).resolve().parent


def now():
    return datetime.now(timezone.utc).isoformat()


def load(path, default):
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return default


def save(path, value):
    tmp = path.with_suffix(path.suffix + ".tmp")
    tmp.write_text(json.dumps(value, ensure_ascii=False, indent=2), encoding="utf-8")
    tmp.replace(path)


def git(repo, *args):
    return subprocess.run(["git", "-C", str(repo), *args], capture_output=True,
                          text=True, encoding="utf-8", errors="replace")


def main():
    dispatcher = HERE.parent.parent / ".ai-project" / "dispatcher"
    config_path = HERE / "supervisor_config.json"
    config = load(config_path, {})
    state_path = dispatcher / "preflight_state.json"
    state = load(state_path, {"failures": 0, "status": "IDLE"})
    repo = HERE.parent.parent
    branch = config.get("stable_branch")
    if not branch:
        state.update({"status": "HUMAN_GATE", "reason": "STABLE_BRANCH_NOT_CONFIGURED",
                      "updated_at": now()})
        save(state_path, state)
        return 1

    head = git(repo, "rev-parse", branch)
    if head.returncode != 0:
        state.update({"status": "HUMAN_GATE", "reason": "STABLE_BRANCH_NOT_FOUND",
                      "updated_at": now()})
        save(state_path, state)
        return 1
    stable_head = head.stdout.strip()
    worktree = repo / "worktrees" / "autopreflight" / "stable"
    worktree.parent.mkdir(parents=True, exist_ok=True)
    if not worktree.exists():
        added = git(repo, "worktree", "add", "--detach", str(worktree), stable_head)
        if added.returncode != 0:
            reason = "STABLE_WORKTREE_CREATE_FAILED: " + added.stderr[-300:]
            state.update({"status": "HUMAN_GATE", "reason": reason, "updated_at": now()})
            save(state_path, state)
            return 1
    if git(worktree, "status", "--porcelain").stdout.strip():
        state.update({"status": "HUMAN_GATE", "reason": "PREFLIGHT_WORKTREE_DIRTY",
                      "updated_at": now()})
        save(state_path, state)
        return 1
    if git(worktree, "rev-parse", "HEAD").stdout.strip() != stable_head:
        moved = git(worktree, "checkout", "--detach", stable_head)
        if moved.returncode != 0:
            state.update({"status": "HUMAN_GATE", "reason": "PREFLIGHT_CHECKOUT_FAILED",
                          "updated_at": now()})
            save(state_path, state)
            return 1

    sys.path.insert(0, str(HERE))
    import integration_supervisor as supervisor
    cfg = supervisor.load_config(str(config_path))
    build = supervisor.run_build(worktree, cfg["build"])
    smoke = None
    if build.get("status") == "PASS":
        smoke = supervisor.run_smoke(worktree, cfg["smoke"],
                                     supervisor.Log(cfg["_log_path"]), "stable")
    passed = build.get("status") == "PASS" and smoke and smoke.get("status") == "PASS"
    if passed:
        state.update({"status": "PASS", "failures": 0, "head": stable_head,
                      "build": build, "smoke": smoke, "updated_at": now()})
        save(state_path, state)
        return 0
    failures = int(state.get("failures", 0)) + 1
    state.update({"status": "HUMAN_GATE" if failures >= 5 else "RETRY",
                  "failures": failures, "head": stable_head, "build": build,
                  "smoke": smoke, "reason": "STABLE_PREFLIGHT_FAILED",
                  "updated_at": now()})
    save(state_path, state)
    return 2 if failures >= 5 else 1


if __name__ == "__main__":
    raise SystemExit(main())
