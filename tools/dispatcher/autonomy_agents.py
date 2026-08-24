"""Repair/Integration Coder dispatcher.

Repair runs only against clean worktrees and only for bounded queue entries.
Integration runs only against explicit READY_FOR_INTEGRATION entries and never
writes the stable branch directly; it produces a reviewable integration branch.
"""

import argparse
import json
import os
import re
import shutil
import subprocess
from datetime import datetime, timezone
from pathlib import Path


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


def clean(worktree):
    result = subprocess.run(["git", "-C", str(worktree), "status", "--porcelain"],
                            capture_output=True, text=True, encoding="utf-8",
                            errors="replace")
    return result.returncode == 0 and not result.stdout.strip()


def alive(pid):
    if not pid:
        return False
    result = subprocess.run(["tasklist", "/FI", "PID eq %s" % int(pid), "/NH"],
                            capture_output=True, text=True, encoding="mbcs",
                            errors="replace")
    return result.returncode == 0 and str(pid) in result.stdout


def opencode_env(dispatcher_dir):
    env = os.environ.copy()
    permission = dispatcher_dir / "headless_permissions.json"
    if permission.is_file():
        env["OPENCODE_CONFIG"] = str(permission.resolve())
    return env


def launch(entry, dispatcher_dir, prompt):
    worktree = Path(entry["worktree"]).resolve()
    if not clean(worktree):
        entry["status"] = "HUMAN_GATE"
        entry["reason"] = "WORKTREE_DIRTY_PRESERVED"
        return None
    exe = shutil.which("opencode") or shutil.which("opencode.cmd")
    if not exe:
        entry["status"] = "HUMAN_GATE"
        entry["reason"] = "OPENCODE_NOT_FOUND"
        return None
    safe = re.sub(r"[^A-Za-z0-9_.-]", "_", entry.get("key", "agent"))
    log_dir = dispatcher_dir / "agent_logs"
    log_dir.mkdir(parents=True, exist_ok=True)
    out = open(log_dir / (safe + ".out.log"), "ab")
    err = open(log_dir / (safe + ".err.log"), "ab")
    proc = subprocess.Popen([exe, "run", prompt], cwd=str(worktree),
                            stdout=out, stderr=err, stdin=subprocess.DEVNULL,
                            env=opencode_env(dispatcher_dir))
    entry.update({"status": "WORKING", "pid": proc.pid, "started_at": now()})
    return proc.pid


def harvest(entry, dispatcher_dir, max_attempts):
    if entry.get("status") != "WORKING" or alive(entry.get("pid")):
        return
    worktree = Path(entry.get("worktree", ""))
    report = worktree / ".ai-project" / "design" / "implementation_report.local.md"
    if report.is_file():
        entry["status"] = "COMPLETED_CANDIDATE"
        entry["completed_at"] = now()
        return
    entry["attempts"] = int(entry.get("attempts", 0)) + 1
    if entry["attempts"] < max_attempts:
        entry["status"] = "READY_FOR_REPAIR"
        entry["reason"] = "CODER_EXITED_WITHOUT_REPORT"
    else:
        entry["status"] = "HUMAN_GATE"
        entry["reason"] = "REPAIR_RETRY_LIMIT_REACHED"


def prepare_integration(entry, dispatcher_dir):
    if entry.get("worktree"):
        return True
    repo = dispatcher_dir.parent.parent
    root = repo / "worktrees" / "autointegration"
    safe = re.sub(r"[^A-Za-z0-9_.-]", "_", entry.get("key", "integration"))[:60]
    path = root / safe
    path.parent.mkdir(parents=True, exist_ok=True)
    branch = "integration/%s" % safe
    stable = "stable-20260824"
    if not path.exists():
        result = subprocess.run(["git", "-C", str(repo), "worktree", "add",
                                 "-b", branch, str(path), stable],
                                capture_output=True, text=True,
                                encoding="utf-8", errors="replace")
        if result.returncode != 0:
            entry["status"] = "HUMAN_GATE"
            entry["reason"] = "INTEGRATION_WORKTREE_CREATE_FAILED(%s)" % result.stderr[-300:]
            return False
    entry["worktree"] = str(path)
    entry["integration_branch"] = branch
    return True


def collect_completed_entries(queues, dispatcher_dir):
    state = load(dispatcher_dir / "dispatcher_state.json", {})
    existing = {x.get("key") for x in queues.get("integration", [])}
    for entry in (state.get("entries") or {}).values():
        if entry.get("status") != "COMPLETED":
            continue
        worktree = entry.get("worktree")
        if not worktree:
            continue
        result = subprocess.run(["git", "-C", worktree, "branch", "--show-current"],
                                capture_output=True, text=True, encoding="utf-8",
                                errors="replace")
        branch = result.stdout.strip()
        result = subprocess.run(["git", "-C", worktree, "rev-parse", "HEAD"],
                                capture_output=True, text=True, encoding="utf-8",
                                errors="replace")
        head = result.stdout.strip()
        key = "%s:%s" % (branch, head)
        if branch and head and key not in existing:
            queues.setdefault("integration", []).append({
                "key": key, "line": entry.get("line"), "branch": branch,
                "head": head, "source_worktree": worktree,
                "status": "READY_FOR_INTEGRATION", "created_at": now()})
            existing.add(key)


def process_repair(queues, dispatcher_dir, max_agents):
    for entry in queues.get("repair", []):
        harvest(entry, dispatcher_dir, max_attempts=2)
    active = sum(1 for e in queues.get("repair", []) if e.get("status") == "WORKING")
    for entry in queues.get("repair", []):
        if active >= max_agents or entry.get("status") != "READY_FOR_REPAIR":
            continue
        if entry.get("phase") == "ISOLATED_DIRTY":
            entry["status"] = "HUMAN_GATE"
            entry["reason"] = "DIRTY_WORKTREE_REQUIRES_OWNER_DECISION"
            continue
        prompt = (
            "You are the bounded Repair Coder. Work only in this worktree. "
            "Investigate the recorded gate failure, preserve existing commits, "
            "make the smallest proven fix, build Debug|x64, and run the startup "
            "smoke test. Do not reset, clean, stash, or resolve unrelated files. "
            "Commit only after all checks pass; otherwise write a report and stop.\n"
            "Gate: %s\nReason: %s" % (entry.get("phase"), entry.get("reasons")))
        if launch(entry, dispatcher_dir, prompt):
            active += 1


def process_integration(queues, dispatcher_dir, max_agents):
    for entry in queues.get("integration", []):
        harvest(entry, dispatcher_dir, max_attempts=1)
    active = sum(1 for e in queues.get("integration", []) if e.get("status") == "WORKING")
    for entry in queues.get("integration", []):
        if active >= max_agents or entry.get("status") != "READY_FOR_INTEGRATION":
            continue
        if not prepare_integration(entry, dispatcher_dir):
            continue
        prompt = (
            "You are the Integration Coder. Integrate the listed source branch "
            "into a new review branch based on the current stable branch. Never "
            "rewrite stable directly. Preserve both sides, resolve only declared "
            "files, build Debug|x64 with zero errors/warnings, and run startup "
            "smoke. Commit the review branch only after passing.\n"
            "Source branch: %s\nCommit: %s" % (entry.get("branch"), entry.get("head")))
        if launch(entry, dispatcher_dir, prompt):
            active += 1


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--dispatcher-dir", required=True)
    parser.add_argument("--max-agents", type=int, default=1)
    args = parser.parse_args()
    d = Path(args.dispatcher_dir).resolve()
    path = d / "autonomy_queues.json"
    queues = load(path, {"version": 1, "repair": [], "integration": [], "proposals": []})
    collect_completed_entries(queues, d)
    process_repair(queues, d, args.max_agents)
    process_integration(queues, d, args.max_agents)
    queues["updated_at"] = now()
    save(path, queues)
    print(json.dumps({"repair": len(queues.get("repair", [])),
                      "integration": len(queues.get("integration", []))}))


if __name__ == "__main__":
    main()
