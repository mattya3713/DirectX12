"""Proposal-only agent launcher.

It runs only when no approved task is available and asks OpenCode to write a
proposal artifact. It never edits game source, task status, branches, or stable.
"""

import argparse
import json
import os
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


def alive(pid):
    if not pid:
        return False
    result = subprocess.run(
        ["tasklist", "/FI", "PID eq %s" % int(pid), "/NH"],
        capture_output=True, text=True, encoding="mbcs", errors="replace")
    return result.returncode == 0 and str(pid) in result.stdout


def resolve_opencode():
    exe = shutil.which("opencode")
    if exe and not exe.lower().endswith((".cmd", ".ps1")):
        return exe
    shim = shutil.which("opencode.cmd") or exe
    if shim:
        candidate = (Path(shim).resolve().parent / "node_modules" /
                     "opencode-ai" / "bin" / "opencode.exe")
        if candidate.is_file():
            return str(candidate)
    return shim


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--dispatcher-dir", required=True)
    args = parser.parse_args()
    d = Path(args.dispatcher_dir).resolve()
    state_path = d / "proposal_state.json"
    state = load(state_path, {"status": "IDLE", "pid": None})
    if state.get("status") == "WORKING":
        if alive(state.get("pid")):
            return 0
        state.update({"status": "IDLE", "finished_at": now(),
                      "reason": "PROPOSAL_AGENT_EXITED"})
        state_path.write_text(json.dumps(state, ensure_ascii=False, indent=2),
                              encoding="utf-8")

    finished = state.get("finished_at")
    if finished:
        try:
            elapsed = (datetime.now(timezone.utc) -
                       datetime.fromisoformat(finished)).total_seconds()
            if elapsed < 1800:
                return 0
        except ValueError:
            pass

    design = d.parent / "design"
    approved = [design / "next_feature.md"] + list(
        design.glob("next_feature_task_*.md"))
    if any(p.is_file() and "APPROVED_FOR_IMPLEMENTATION" in p.read_text(
            encoding="utf-8", errors="ignore") for p in approved):
        return 0
    exe = resolve_opencode()
    if not exe:
        state.update({"status": "HUMAN_GATE", "reason": "OPENCODE_NOT_FOUND"})
        state_path.write_text(json.dumps(state, ensure_ascii=False, indent=2), encoding="utf-8")
        return 1
    out_dir = d / "proposals"
    out_dir.mkdir(exist_ok=True)
    prompt = (
        "You are the Proposal Coder for Senzan. Do not edit source, task status, "
        "branches, or stable. Inspect only current design/state and write one "
        "new proposal markdown under .ai-project/dispatcher/proposals/ with "
        "problem, player value, scope, dependencies, risks, and suggested task "
        "breakdown. This is a suggestion only and must not approve implementation.")
    env = os.environ.copy()
    permission = d / "headless_permissions.json"
    if permission.is_file():
        env["OPENCODE_CONFIG"] = str(permission.resolve())
    log = open(d / "proposal_agent.log", "ab")
    proc = subprocess.Popen([exe, "run", prompt], cwd=str(d.parent.parent),
                            stdout=log, stderr=log, stdin=subprocess.DEVNULL, env=env)
    state.update({"status": "WORKING", "pid": proc.pid, "started_at": now()})
    state_path.write_text(json.dumps(state, ensure_ascii=False, indent=2), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
