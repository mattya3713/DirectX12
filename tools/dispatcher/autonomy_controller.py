"""Bounded recovery and queue bookkeeping for the long-running dispatcher.

This controller never edits a game worktree and never chooses a merge result.
It turns supervisor outcomes into explicit queues for the integration/repair
agent, with a per-(line,head,kind) attempt cap so a persistent failure cannot
create an infinite repair storm.
"""

import argparse
import json
from datetime import datetime, timezone
from pathlib import Path


def now():
    return datetime.now(timezone.utc).isoformat()


def load(path, default):
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return default


def save(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp = path.with_suffix(path.suffix + ".tmp")
    tmp.write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding="utf-8")
    tmp.replace(path)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--dispatcher-dir", default=str(Path(__file__).resolve().parents[2] / ".ai-project" / "dispatcher"))
    parser.add_argument("--max-attempts", type=int, default=2)
    args = parser.parse_args()

    root = Path(args.dispatcher_dir).resolve()
    state = load(root / "dispatcher_state.json", {})
    supervisor = state.get("supervisor", {})
    queues = load(root / "autonomy_queues.json", {
        "version": 1, "updated_at": None, "repair": [],
        "integration": [], "proposals": [], "dead_agents": []
    })
    seen = {(x.get("key"), x.get("status")) for x in queues.get("repair", [])}
    for name, entry in (supervisor.get("worktrees") or {}).items():
        phase = entry.get("phase")
        if phase not in {"ISOLATED_DIRTY", "CONFLICT", "BUILD_FAILED", "SMOKE_FAILED", "ERROR"}:
            continue
        snap = entry.get("snapshot") or {}
        key = "%s:%s:%s" % (name, snap.get("head", ""), phase)
        previous = [x for x in queues.get("repair", []) if x.get("key") == key]
        attempts = max([x.get("attempts", 0) for x in previous] or [0])
        status = "READY_FOR_REPAIR" if attempts < args.max_attempts else "HUMAN_GATE"
        marker = (key, status)
        if marker not in seen:
            queues.setdefault("repair", []).append({
                "key": key, "created_at": now(), "line": name,
                "head": snap.get("head"), "phase": phase,
                "reasons": entry.get("reasons", []),
                "attempts": attempts, "status": status,
                "policy": "repair agent must preserve worktree and prove build/smoke"
            })
    queues["updated_at"] = now()
    save(root / "autonomy_queues.json", queues)
    print(json.dumps({"repair": len(queues.get("repair", [])), "updated_at": queues["updated_at"]}))


if __name__ == "__main__":
    main()
