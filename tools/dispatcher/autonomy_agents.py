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


def process_repair(queues, dispatcher_dir, max_agents):
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
    active = sum(1 for e in queues.get("integration", []) if e.get("status") == "WORKING")
    for entry in queues.get("integration", []):
        if active >= max_agents or entry.get("status") != "READY_FOR_INTEGRATION":
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
    process_repair(queues, d, args.max_agents)
    process_integration(queues, d, args.max_agents)
    queues["updated_at"] = now()
    save(path, queues)
    print(json.dumps({"repair": len(queues.get("repair", [])),
                      "integration": len(queues.get("integration", []))}))


if __name__ == "__main__":
    main()
