"""Integration Supervisor - stable基準同期・ビルド・起動スモーク・Coder起動の自動ゲート。

dispatcher.pyの上位に位置し、各Coder worktreeを「起動確認済みのstable基準」へ
同期できる状態かを機械的に判定してからCoder起動へ進む。これによりDirectorの
手動仲介(同期指示・ビルド確認・起動可否判断)を不要にする。

安全策:
- DIRTY worktreeは変更を保持したまま隔離する(reset/clean/stash/commitはしない)
- 同期は通常のgit merge(fast-forward含む)のみ。force pushは使わない
- merge conflictは自動解決せず、merge --abortでマージ前の状態へ戻した上で
  競合ファイル一覧と再開情報をmanifestへ保存する
- Debug|x64ビルドが0エラー0警告でないworktreeのCoderは起動しない
- 起動スモーク(最低10秒生存・新規クラッシュDumpなし)を満たさないworktreeも
  起動しない。PID・終了理由・新規Dumpを記録する
- 同期・ビルド・起動結果はworktree単位でdispatcher_state.jsonの"supervisor"
  キーへ書き、既存Viewerからも参照できるようにする
- ゲーム本体のコードは編集しない。修正が必要な場合はdraftタスクをmanifestへ
  生成して人間(Director)の判断を待つ

使い方:
    py -3 integration_supervisor.py pipeline             # 判定のみ(dry-run)
    py -3 integration_supervisor.py pipeline --execute   # 同期・ビルド・スモークを実施
    py -3 integration_supervisor.py pipeline --execute --no-launch
    py -3 integration_supervisor.py watch --interval 300 --execute
    py -3 integration_supervisor.py show-state

--execute がない限りworktreeへ一切書き込まない。
"""

import argparse
import json
import re
import shutil
import subprocess
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

HERE = Path(__file__).resolve().parent

DEFAULT_CONFIG_NAME = "supervisor_config.json"

# worktreeのゲート判定結果
P_READY = "READY"                  # 同期・ビルド・スモーク全てPASS。起動可能
P_DIRTY = "ISOLATED_DIRTY"         # 未コミット変更があるため隔離(変更は保持)
P_CONFLICT = "CONFLICT"            # merge conflict。自動解決しない
P_BUILD_FAIL = "BUILD_FAILED"      # ビルドが0エラー0警告未達
P_SMOKE_FAIL = "SMOKE_FAILED"      # 起動スモーク失敗
P_ERROR = "ERROR"                  # 上記以外の失敗(git不在など)

APPROVED = "APPROVED_FOR_IMPLEMENTATION"

# 起動済み扱いするdispatcher側ステータス
ACTIVE_DISPATCH_STATUSES = ("LAUNCHED", "WORKING")
UNRESOLVED_DISPATCH_STATUSES = ("DEAD_PID", "HUMAN_REVIEW")

DEFAULTS = {
    "repo_root": "../..",
    "state_file": "dispatcher_state.json",
    "log_file": "supervisor.log",
    "manifest_dir": "manifests",
    "design_dir": "../design",
    "dispatcher_dir": ".",
    "stable_branch": "stable-20260824",
    "stable_commit": "1cd2644783f438a3650346d7d6c12613b3562a95",
    "fetch_before_sync": False,
    "merge_timeout_sec": 600,
    "build": {
        "command": [
            "powershell", "-NoProfile", "-ExecutionPolicy", "Bypass",
            "-File", "DirectX12/scripts/build.ps1",
            "-Configuration", "Debug", "-Platform", "x64",
        ],
        "timeout_sec": 1800,
        "zero_warnings_required": True,
    },
    "smoke": {
        "exe_path": "DirectX12/x64/Debug/DirectX12.exe",
        # pytest等で擬似exeを差し込めるようにする逃し口。nullならexe_pathを使う
        "launch_command": None,
        "cwd_relative": "DirectX12",
        "min_alive_seconds": 10,
        "poll_interval_seconds": 0.5,
        "dump_dirs": ["Dumps", "DirectX12/Dumps"],
        "log_file": "smoke_{name}.log",
    },
    "launch": {"enabled": True},
    "worktrees": {},
}


def utcnow():
    return datetime.now(timezone.utc).isoformat()


class Log:
    """dispatcher.pyと同じ形のタイムスタンプ付きロガー。"""

    def __init__(self, path, verbose=True):
        self.path = Path(path)
        self.verbose = verbose

    def write(self, msg, to_console=None):
        line = "[%s] %s" % (
            datetime.now(timezone.utc).astimezone().strftime("%Y-%m-%d %H:%M:%S"),
            msg,
        )
        try:
            self.path.parent.mkdir(parents=True, exist_ok=True)
            with open(self.path, "a", encoding="utf-8") as f:
                f.write(line + "\n")
        except OSError:
            pass
        if to_console is None:
            to_console = self.verbose
        if to_console:
            print(msg)


def _deep_merge(base, override):
    out = dict(base)
    for k, v in (override or {}).items():
        if isinstance(v, dict) and isinstance(out.get(k), dict):
            out[k] = _deep_merge(out[k], v)
        else:
            out[k] = v
    return out


def load_config(path=None):
    """設定ファイルを読み、パス類を絶対化する。テストからは辞書を直接渡してもよい。"""
    if path is not None:
        base = Path(path)
        # メモ帳/PowerShell由来のBOM付きJSONも許容する
        raw = json.loads(base.read_text(encoding="utf-8-sig"))
        here = base.resolve().parent
    else:
        raw = {}
        here = HERE
        default_file = HERE / DEFAULT_CONFIG_NAME
        if default_file.is_file():
            raw = json.loads(default_file.read_text(encoding="utf-8-sig"))
            here = HERE
    cfg = _deep_merge(DEFAULTS, raw)
    cfg["_here"] = here
    cfg["_repo_root"] = (here / cfg["repo_root"]).resolve()
    cfg["_state_path"] = (here / cfg["state_file"]).resolve()
    cfg["_manifest_dir"] = (here / cfg["manifest_dir"]).resolve()
    cfg["_design_dir"] = (here / cfg["design_dir"]).resolve()
    cfg["_log_path"] = (here / cfg["log_file"]).resolve()
    cfg["_dispatcher_dir"] = (here / cfg["dispatcher_dir"]).resolve()
    for spec in cfg["worktrees"].values():
        p = Path(spec["path"])
        spec["_path"] = p if p.is_absolute() else (here / p).resolve()
    return cfg


def save_json_atomic(path, data):
    tmp = Path(str(path) + ".tmp")
    with open(tmp, "w", encoding="utf-8") as f:
        json.dump(data, f, ensure_ascii=False, indent=2)
    tmp.replace(path)


def load_state(path):
    path = Path(path)
    if path.exists():
        try:
            with open(path, encoding="utf-8") as f:
                data = json.load(f)
            if isinstance(data, dict):
                return data
        except (json.JSONDecodeError, OSError):
            pass
    return {}


def pid_alive(pid):
    if not pid:
        return False
    try:
        out = subprocess.run(
            ["tasklist", "/FI", "PID eq %d" % int(pid), "/NH"],
            capture_output=True, text=True, timeout=10,
        )
        return str(pid) in (out.stdout or "")
    except (OSError, subprocess.SubprocessError, ValueError):
        return False


# ---------------------------------------------------------------- git操作

def git_out(worktree, *args, timeout=60):
    """gitコマンドの実行。(rc, stdout, stderr)を返す。git不在時はrc=None。"""
    git = shutil.which("git")
    if not git:
        return None, "", "git executable not found"
    try:
        out = subprocess.run(
            [git, "-C", str(worktree)] + list(args),
            capture_output=True, text=True, encoding="utf-8",
            errors="replace", timeout=timeout,
        )
        return out.returncode, out.stdout or "", out.stderr or ""
    except (OSError, subprocess.SubprocessError) as exc:
        return None, "", str(exc)


def snapshot_worktree(path, stable_commit):
    """branch/HEAD/status/変更ファイル/stableとのahead-behindを取得する(読み取り専用)。"""
    rc_branch, branch, _ = git_out(path, "rev-parse", "--abbrev-ref", "HEAD")
    rc_head, head, _ = git_out(path, "rev-parse", "HEAD")
    _, porcelain, _ = git_out(path, "status", "--porcelain")
    changed = [l for l in (porcelain or "").splitlines() if l.strip()]
    ahead = behind = None
    if rc_head == 0 and stable_commit:
        _, ahead_s, _ = git_out(path, "rev-list", "--count", "%s..HEAD" % stable_commit)
        _, behind_s, _ = git_out(path, "rev-list", "--count", "HEAD..%s" % stable_commit)
        try:
            ahead = int(ahead_s.strip())
            behind = int(behind_s.strip())
        except ValueError:
            ahead = behind = None
    return {
        "branch": branch.strip() if rc_branch == 0 else None,
        "head": head.strip() if rc_head == 0 else None,
        "dirty": bool(changed),
        "changed_files": changed,
        "ahead_of_stable": ahead,
        "behind_stable": behind,
    }


def sync_worktree(path, stable_commit, timeout_sec, fetch_first=False):
    """stable基準への同期。通常のmergeのみ。conflict時はabortして元の状態へ戻す。

    戻り値: {"status": MERGED|FAST_FORWARD|UP_TO_DATE|CONFLICT|ERROR, ...}
    """
    _, pre_head, _ = git_out(path, "rev-parse", "HEAD")
    pre_head = pre_head.strip()

    if fetch_first:
        git_out(path, "fetch", "--all", "--prune", timeout=timeout_sec)

    rc, out, err = git_out(
        path, "merge", "--no-edit", stable_commit, timeout=timeout_sec)

    if rc == 0:
        _, new_head, _ = git_out(path, "rev-parse", "HEAD")
        new_head = new_head.strip()
        # 親が1個ならfast-forward、2個ならマージコミット
        _, parents, _ = git_out(path, "rev-list", "--parents", "-n", "1", "HEAD")
        method = "FAST_FORWARD" if len(parents.split()) <= 2 else "MERGED"
        return {"status": method, "head": new_head,
                "previous_head": pre_head, "merged_at": utcnow(),
                "conflict_files": []}

    # 失敗: 競合ファイルを洗い出してからマージ前の状態へ戻す
    _, unmerged, _ = git_out(path, "diff", "--name-only", "--diff-filter=U")
    conflict_files = [l.strip() for l in (unmerged or "").splitlines() if l.strip()]
    merging_rc, _, _ = git_out(path, "rev-parse", "-q", "--verify", "MERGE_HEAD")
    aborted = False
    if merging_rc == 0:
        git_out(path, "merge", "--abort", timeout=60)
        aborted = True
    restored_head = (git_out(path, "rev-parse", "HEAD")[1] or "").strip()
    restored = (restored_head == pre_head)

    if conflict_files or "CONFLICT" in (out + err):
        return {
            "status": "CONFLICT",
            "conflict_files": conflict_files,
            "previous_head": pre_head,
            "restored_cleanly": restored,
            "aborted": aborted,
            "merged_at": utcnow(),
        }
    return {
        "status": "ERROR",
        "stderr_tail": "\n".join((err or "").strip().splitlines()[-10:]),
        "stdout_tail": "\n".join((out or "").strip().splitlines()[-5:]),
        "previous_head": pre_head,
        "restored_cleanly": restored,
        "aborted": aborted,
        "merged_at": utcnow(),
    }


# ---------------------------------------------------------------- ビルド

ERROR_LINE_RE = re.compile(r"\):\s*error |\berror\s+(?:MSB|C|CLK|LNK|RC)\d+", re.IGNORECASE)
WARNING_LINE_RE = re.compile(r"\):\s*warning |\bwarning\s+(?:MSB|C|CLK|LNK|RC)\d+", re.IGNORECASE)
SUMMARY_COUNT_RE = re.compile(
    r"(\d+)\s*(?:Warning\(s\)|個の警告|警告)|(\d+)\s*(?:Error\(s\)|個のエラー|エラー)")


def parse_build_output(text):
    """ビルド出力からエラー/警告数を数える。

    基本はbuild.ps1と同じ「): error / ): warning」行カウント。ローカライズされた
    msbuildサマリーも参考値として拾う。判定には行カウントを優先する。
    msbuildは同じ診断をプロジェクト行とサマリーの両方に出すため、接頭辞(N>)を
    除いた同一行は1件にまとめる。
    """
    error_lines = []
    warning_lines = []
    seen_err = set()
    seen_warn = set()
    for raw in text.splitlines():
        line = raw.rstrip()
        key = re.sub(r"^\s*\d+>", "", line).strip()
        if not key:
            continue
        if ERROR_LINE_RE.search(line):
            if key not in seen_err:
                seen_err.add(key)
                error_lines.append(key)
        elif WARNING_LINE_RE.search(line):
            if key not in seen_warn:
                seen_warn.add(key)
                warning_lines.append(key)
    summary_err = summary_warn = None
    for m in SUMMARY_COUNT_RE.finditer(text):
        if m.group(2) is not None:
            summary_err = int(m.group(2))
        else:
            summary_warn = int(m.group(1))
    errors = len(error_lines) if error_lines else (summary_err or 0)
    warnings = len(warning_lines) if warning_lines else (summary_warn or 0)
    tail = "\n".join(text.strip().splitlines()[-15:])
    return errors, warnings, error_lines[:20], tail


def run_build(worktree, build_cfg):
    """Debug|x64ビルドを実行し、0エラー0警告かを判定する。"""
    cmd = build_cfg.get("command") or []
    timeout = int(build_cfg.get("timeout_sec", 1800))
    started = utcnow()
    if not cmd:
        return {"status": "FAIL", "reason": "NO_BUILD_COMMAND", "errors": None,
                "warnings": None, "started_at": started, "finished_at": utcnow()}
    try:
        proc = subprocess.run(
            cmd, cwd=str(worktree), capture_output=True,
            text=True, encoding="mbcs", errors="replace", timeout=timeout,
        )
        out, err, rc = proc.stdout or "", proc.stderr or "", proc.returncode
    except (OSError, subprocess.SubprocessError) as exc:
        return {"status": "FAIL", "reason": "BUILD_LAUNCH_FAILED(%s)" % exc,
                "errors": None, "warnings": None,
                "started_at": started, "finished_at": utcnow()}
    errors, warnings, samples, tail = parse_build_output(out + "\n" + err)
    status = "PASS"
    reason = None
    if rc != 0:
        status = "FAIL"
        reason = "NONZERO_EXIT(exit_code=%s)" % rc if errors == 0 else None
    elif errors > 0:
        status = "FAIL"
        reason = None
    elif warnings > 0 and build_cfg.get("zero_warnings_required", True):
        # このプロジェクトは0警告が基準(docs/coding-rules.md)。警告があれば落とす
        status = "FAIL"
        reason = "WARNINGS_OVER_ZERO(%d)" % warnings
    result = {
        "status": status,
        "exit_code": rc,
        "errors": errors,
        "warnings": warnings,
        "error_samples": samples,
        "output_tail": tail,
        "reason": reason,
        "started_at": started,
        "finished_at": utcnow(),
    }
    return result


# ---------------------------------------------------------------- 起動スモーク

DUMP_SUFFIXES = (".dmp", ".mdmp")


def dump_inventory(worktree, dump_dirs):
    """worktree内の既存クラッシュダンプ一覧(相対パスset)。"""
    found = set()
    root = Path(worktree)
    for d in dump_dirs or []:
        p = root / d
        if not p.is_dir():
            continue
        for f in p.rglob("*"):
            if f.is_file() and f.suffix.lower() in DUMP_SUFFIXES:
                # 記録はOS非依存の区切りで持つ(Viewer/logでの比較を安定させる)
                found.add(f.relative_to(root).as_posix())
    return found


def run_smoke(worktree, smoke_cfg, log, name):
    """生成されたゲームexeを起動し、最低N秒の生存と新規ダンプ無しを確認する。

    プロセスは必ず監視時間後に自プロセスだけ停止する。既存プロセスには触れない。
    """
    root = Path(worktree)
    started = utcnow()
    baseline = dump_inventory(root, smoke_cfg.get("dump_dirs"))

    cmd = smoke_cfg.get("launch_command")
    if cmd:
        argv = [str(c) for c in cmd]
    else:
        exe = root / smoke_cfg.get("exe_path", "")
        if not exe.is_file():
            return {"status": "FAIL", "reason": "EXE_NOT_FOUND(%s)" % exe,
                    "pid": None, "started_at": started, "finished_at": utcnow()}
        argv = [str(exe)]
    cwd = root / smoke_cfg.get("cwd_relative", "") if smoke_cfg.get("cwd_relative") else root
    cwd = cwd if Path(cwd).is_dir() else root

    smoke_log = root.parent / ("_supervisor_smoke_%s.log" % name)
    result = {"status": "FAIL", "pid": None, "started_at": started}
    proc = None
    try:
        with open(smoke_log, "ab") as lf:
            proc = subprocess.Popen(argv, cwd=str(cwd), stdout=lf, stderr=lf,
                                    stdin=subprocess.DEVNULL)
        result["pid"] = proc.pid
        min_alive = float(smoke_cfg.get("min_alive_seconds", 10))
        poll = float(smoke_cfg.get("poll_interval_seconds", 0.5))
        deadline = time.time() + max(min_alive, 0.0)
        exit_code = None
        while True:
            code = proc.poll()
            if code is not None:
                exit_code = code
                break
            if time.time() >= deadline:
                break
            time.sleep(min(poll, max(deadline - time.time(), 0.01)))

        alive_ok = exit_code is None
        if alive_ok:
            # 判定に使った分だけ生存。今回起動したPIDだけ後始末する
            try:
                proc.terminate()
                proc.wait(timeout=15)
            except (OSError, subprocess.SubprocessError):
                try:
                    proc.kill()
                except (OSError, subprocess.SubprocessError):
                    log.write("SMOKE_KILL_FAILED: %s pid=%s" % (name, proc.pid))
            result["killed_by_supervisor"] = True
            try:
                result["exit_code"] = proc.returncode
            except Exception:
                pass
        else:
            result["killed_by_supervisor"] = False
            result["exit_code"] = exit_code
            result["reason"] = "EXITED_EARLY(exit_code=%s, alive<%ss)" % (
                exit_code, smoke_cfg.get("min_alive_seconds", 10))

        new_dumps = sorted(dump_inventory(root, smoke_cfg.get("dump_dirs")) - baseline)
        result["new_dumps"] = new_dumps
        result["alive_confirmed_seconds"] = (
            float(smoke_cfg.get("min_alive_seconds", 10)) if alive_ok else None)
        if alive_ok and new_dumps:
            result["status"] = "FAIL"
            result["reason"] = "NEW_CRASH_DUMP(%s)" % ", ".join(new_dumps[:5])
        elif alive_ok:
            result["status"] = "PASS"
    finally:
        if proc is not None and proc.poll() is None:
            try:
                proc.terminate()
            except (OSError, subprocess.SubprocessError):
                pass
    result["finished_at"] = utcnow()
    return result


# ---------------------------------------------------------------- 状態・manifest

def sup_state(state):
    return state.setdefault("supervisor", {})


def save_sup_state(cfg, state):
    sup = sup_state(state)
    sup["updated_at"] = utcnow()
    sup["stable"] = {
        "branch": cfg["stable_branch"],
        "commit": cfg["stable_commit"],
    }
    save_json_atomic(cfg["_state_path"], state)


def write_manifest(cfg, name, kind, payload):
    """worktree単位のmanifest。競合や失敗時の再開情報を保存する。"""
    mdir = cfg["_manifest_dir"]
    mdir.mkdir(parents=True, exist_ok=True)
    doc = {"saved_at": utcnow(), "kind": kind, "worktree": name}
    doc.update(payload)
    path = mdir / ("supervisor_%s.json" % name)
    save_json_atomic(path, doc)
    return path


FIXTASK_TEMPLATE = """# Draft Task - {name} の修正(自動生成)

```
Status:
DRAFT_NOT_APPROVED(dispatcher Supervisor自動生成。Directorの承認が必要)

Assigned Line:
{name}

Summary:
Integration Supervisorが{kind}でゲートを閉じた。推測での自動修正はしないため、
以下の証跡をもとにDirectorが修正タスクへ昇格させること。

Evidence:
{evidence}

Record:
- state: dispatcher_state.json / supervisor.worktrees.{name}
- manifest: {manifest}
```
"""


def write_fixtask_draft(cfg, name, kind, evidence, manifest_path):
    """ビルド・スモーク失敗時に修正タスクのdraftをmanifestへ生成する。

    design/配下はDirector専属のため書かない。昇格はDirectorの判断で行う。
    """
    mdir = cfg["_manifest_dir"]
    mdir.mkdir(parents=True, exist_ok=True)
    ev = "\n".join("- %s" % e for e in evidence)
    text = FIXTASK_TEMPLATE.format(name=name, kind=kind, evidence=ev,
                                   manifest=manifest_path.name if manifest_path else "-")
    path = mdir / ("fixtask_draft_%s.md" % name)
    path.write_text(text, encoding="utf-8")
    return path


# ---------------------------------------------------------------- パイプライン本体

def classify_and_process(cfg, state, log, name, spec, execute):
    """worktree1台分の判定と処理。結果をstateへ書き、エントリを返す。"""
    sup = sup_state(state)
    entries = sup.setdefault("worktrees", {})
    old = entries.get(name, {})
    wt = spec["_path"]
    stable = cfg["stable_commit"]

    entry = old if isinstance(old, dict) else {}
    entry.update({"name": name, "path": str(wt), "updated_at": utcnow()})
    if not wt.is_dir():
        entry.update({"phase": P_ERROR, "eligible_for_launch": False,
                      "reasons": ["WORKTREE_MISSING(%s)" % wt],
                      "snapshot": None})
        entries[name] = entry
        return entry

    snap = snapshot_worktree(wt, stable)
    if snap["head"] is None:
        entry.update({"phase": P_ERROR, "eligible_for_launch": False,
                      "reasons": ["GIT_UNAVAILABLE_OR_NOT_A_REPO"],
                      "snapshot": snap})
        entries[name] = entry
        return entry

    entry["snapshot"] = snap
    reasons = []

    # --- 再開: 同じHEAD×同じstableで判定済みなら重い処理を省く ---
    cache_ok = (
        execute and old.get("phase") == P_READY
        and (old.get("build") or {}).get("head") == snap["head"]
        and (old.get("smoke") or {}).get("head") == snap["head"]
        and old.get("_stable_commit") == stable
    )
    if cache_ok:
        entry["phase"] = P_READY
        entry["eligible_for_launch"] = True
        entry["reasons"] = ["CACHED_RESULT_REUSED(head=%s)" % snap["head"][:12]]
        entries[name] = entry
        return entry

    # --- DIRTY: 変更を保持したまま隔離する ---
    if snap["dirty"]:
        entry.update({
            "phase": P_DIRTY,
            "eligible_for_launch": False,
            "reasons": ["WORKTREE_DIRTY_CHANGES_PRESERVED(%d files)" % len(snap["changed_files"])],
            "_stable_commit": stable,
        })
        entries[name] = entry
        log.write("ISOLATE: %s DIRTY(%d files)。変更は保持され同期・起動は行わない"
                  % (name, len(snap["changed_files"])))
        return entry

    # --- 同期(失敗の再試行ストーム防止: 同一HEAD×同一stableのconflictは保持) ---
    conflict_persisted = (
        execute and old.get("phase") == P_CONFLICT
        and (old.get("sync") or {}).get("previous_head") == snap["head"]
        and old.get("_stable_commit") == stable
    )
    sync = entry.get("sync") or {}
    need_sync = (snap["behind_stable"] or 0) > 0
    if conflict_persisted:
        # 競合は自動解決しない。同じ状態でmergeを繰り返すだけなので保持して止まる
        log.write("CONFLICT_HOLD: %s 同一HEADでの前回conflictを保持し、自動再試行しない"
                  " (--refreshで再試行)" % name)
        entry.update({
            "phase": P_CONFLICT,
            "eligible_for_launch": False,
            "reasons": ["MERGE_CONFLICT_HELD(前回の競合未解消。--refreshで再試行)"],
            "sync": old.get("sync"),
            "_stable_commit": stable,
        })
        entries[name] = entry
        return entry
    elif execute and need_sync:
        log.write("SYNC: %s behind=%d -> git merge %s"
                  % (name, snap["behind_stable"], cfg["stable_branch"]))
        sync = sync_worktree(wt, stable, cfg.get("merge_timeout_sec", 600),
                             fetch_first=bool(cfg.get("fetch_before_sync")))
        if sync["status"] == "CONFLICT":
            manifest = write_manifest(cfg, name, "sync_conflict", {
                "stable": {"branch": cfg["stable_branch"], "commit": stable[:12]},
                "pre_merge_head": sync.get("previous_head"),
                "conflict_files": sync.get("conflict_files"),
                "restored_cleanly": sync.get("restored_cleanly"),
                "resume": "git -C \"%s\" merge %s を手動実行して競合を解消すること。"
                          "自動解決は行っていない。" % (wt, cfg["stable_branch"]),
            })
            log.write("CONFLICT: %s files=%s manifest=%s"
                      % (name, sync.get("conflict_files"), manifest))
            entry.update({
                "phase": P_CONFLICT,
                "eligible_for_launch": False,
                "reasons": ["MERGE_CONFLICT_ISOLATED(files=%d)"
                            % len(sync.get("conflict_files") or [])],
                "sync": sync, "build": None, "smoke": None,
                "manifest": str(manifest), "_stable_commit": stable,
            })
            entries[name] = entry
            return entry
        if sync["status"] == "ERROR":
            entry.update({"phase": P_ERROR, "eligible_for_launch": False,
                          "reasons": ["SYNC_ERROR"],
                          "sync": sync, "_stable_commit": stable})
            entries[name] = entry
            log.write("SYNC_ERROR: %s %s" % (name, sync.get("stderr_tail")))
            return entry
        snap = snapshot_worktree(wt, stable)
        entry["snapshot"] = snap
    elif need_sync:
        sync = {"status": "PLANNED(merge %s)" % cfg["stable_branch"]}
    else:
        sync = {"status": "UP_TO_DATE", "head": snap["head"]}
    entry["sync"] = sync

    if not execute:
        # dry-run: ここからの予定だけ提示する
        entry.update({
            "phase": "(dry-run) BUILD_AND_SMOKE_PLANNED" if not cache_ok else P_READY,
            "eligible_for_launch": False,
            "reasons": ["DRY_RUN(no mutation)"],
            "_stable_commit": stable,
        })
        entries[name] = entry
        return entry

    # --- ビルド(同一HEADの結果はPASS/FAILとも再利用。FAILの自動再試行はしない) ---
    build = entry.get("build") or {}
    if build.get("head") == snap["head"]:
        if build.get("status") != "PASS":
            log.write("BUILD_HOLD: %s 同一HEADでの前回失敗を保持し、再試行しない"
                      " (--refreshで再試行)" % name)
            entry.update({"phase": P_BUILD_FAIL, "eligible_for_launch": False,
                          "reasons": ["BUILD_FAILED_HELD(%s)"
                                      % (build.get("reason") or "unknown")]})
            entries[name] = entry
            return entry
        entry["reasons"] = ["BUILD_CACHED(head=%s)" % snap["head"][:12]]
    else:
        log.write("BUILD: %s (%s)" % (name, "Debug|x64"))
        build = run_build(wt, cfg["build"])
        build["head"] = snap["head"]
        if build["status"] != "PASS":
            manifest = write_manifest(cfg, name, "build_failed", {
                "head": snap["head"], "exit_code": build.get("exit_code"),
                "errors": build.get("errors"), "warnings": build.get("warnings"),
                "error_samples": build.get("error_samples"),
            })
            evidence = [
                "exit_code=%s errors=%s warnings=%s"
                % (build.get("exit_code"), build.get("errors"), build.get("warnings")),
            ] + (build.get("error_samples") or [])[:5]
            fixtask = write_fixtask_draft(cfg, name, "ビルド失敗(Debug|x64)",
                                          evidence, manifest)
            log.write("BUILD_FAIL: %s errors=%s warnings=%s reason=%s"
                      % (name, build.get("errors"), build.get("warnings"),
                         build.get("reason")))
            entry.update({
                "phase": P_BUILD_FAIL, "eligible_for_launch": False,
                "reasons": [build.get("reason") or "BUILD_FAILED(errors=%s,warnings=%s)"
                            % (build.get("errors"), build.get("warnings"))],
                "build": build, "smoke": None,
                "manifest": str(manifest), "fixtask_draft": str(fixtask),
                "_stable_commit": stable,
            })
            entries[name] = entry
            return entry
        entry["build"] = build

    # --- 起動スモーク(同一HEADの結果はPASS/FAILとも再利用) ---
    smoke = entry.get("smoke") or {}
    if smoke.get("head") == snap["head"]:
        if smoke.get("status") != "PASS":
            log.write("SMOKE_HOLD: %s 同一HEADでの前回失敗を保持し、再試行しない"
                      " (--refreshで再試行)" % name)
            entry.update({"phase": P_SMOKE_FAIL, "eligible_for_launch": False,
                          "reasons": ["SMOKE_FAILED_HELD(%s)"
                                      % (smoke.get("reason") or "unknown")]})
            entries[name] = entry
            return entry
    else:
        log.write("SMOKE: %s (最低%s秒生存を確認)" % (name, cfg["smoke"].get("min_alive_seconds")))
        smoke = run_smoke(wt, cfg["smoke"], log, name)
        smoke["head"] = snap["head"]
        if smoke["status"] != "PASS":
            manifest = write_manifest(cfg, name, "smoke_failed", {
                "head": snap["head"], "pid": smoke.get("pid"),
                "exit_code": smoke.get("exit_code"),
                "reason": smoke.get("reason"),
                "new_dumps": smoke.get("new_dumps"),
                "resume": "原因を調査するまでこのworktreeのCoder起動を停止する。",
            })
            evidence = ["pid=%s" % smoke.get("pid"),
                        "exit_code=%s" % smoke.get("exit_code"),
                        "reason=%s" % smoke.get("reason")] + \
                       ["new_dump=%s" % d for d in (smoke.get("new_dumps") or [])]
            fixtask = write_fixtask_draft(cfg, name, "起動スモーク失敗",
                                          evidence, manifest)
            log.write("SMOKE_FAIL: %s pid=%s reason=%s dumps=%s"
                      % (name, smoke.get("pid"), smoke.get("reason"),
                         smoke.get("new_dumps")))
            entry.update({
                "phase": P_SMOKE_FAIL, "eligible_for_launch": False,
                "reasons": [smoke.get("reason") or "SMOKE_FAILED"],
                "smoke": smoke, "manifest": str(manifest),
                "fixtask_draft": str(fixtask), "_stable_commit": stable,
            })
            entries[name] = entry
            return entry
        entry["smoke"] = smoke

    entry.update({
        "phase": P_READY,
        "eligible_for_launch": True,
        "reasons": ["SYNCED(%s)" % (entry.get("sync", {}).get("status") or "UP_TO_DATE"),
                    "BUILD_PASS(errors=0,warnings=%s)" % build.get("warnings"),
                    "SMOKE_PASS(alive>=%ss,dumps=0)"
                    % cfg["smoke"].get("min_alive_seconds")],
        "_stable_commit": stable,
    })
    entries[name] = entry
    log.write("READY: %s (head=%s)" % (name, snap["head"][:12]))
    return entry


# ---------------------------------------------------------------- Coder起動

FENCE_RE = re.compile(r"```[a-zA-Z]*\s*\n(.*?)```", re.DOTALL)


def parse_task_fields(path):
    """Task File冒頭コードブロックのKey: value抽出(dispatcher.pyと同じ書式)。"""
    try:
        text = Path(path).read_text(encoding="utf-8")
    except OSError:
        return {}
    m = FENCE_RE.search(text)
    if not m:
        return {}
    fields = {}
    lines = m.group(1).splitlines()
    i = 0
    while i < len(lines):
        raw = lines[i]
        line = raw.strip()
        if not line or ":" not in line or raw.startswith(" "):
            i += 1
            continue
        key, _, rest = line.partition(":")
        key = key.strip().lower().replace(" ", "_")
        rest = rest.strip()
        if rest:
            fields[key] = rest
            i += 1
        else:
            vals = []
            j = i + 1
            while j < len(lines) and lines[j].strip():
                vals.append(lines[j].strip())
                j += 1
            if vals:
                fields[key] = "\n".join(vals)
            i = max(j, i + 1)
    return fields


class DispatcherDelegate:
    """実際のopencode起動はdispatcher.pyへ委譲する(二重起動防止の実績ロジックを流用)。"""

    def __init__(self, dispatcher_dir, log):
        self.dir = Path(dispatcher_dir)
        self.log = log
        self.module = None
        self.config = None
        self.error = None
        try:
            if str(self.dir) not in sys.path:
                sys.path.insert(0, str(self.dir))
            import dispatcher as mod
            self.module = mod
            self.config = mod.load_config()
        except Exception as exc:
            self.error = repr(exc)

    @property
    def available(self):
        return self.module is not None

    def approved_tasks(self):
        """承認済みTask Fileを古い順で返す。[(path, fields), ...]"""
        mod = self.module
        design = Path(self.config["_design_dir"])
        files = sorted(design.glob(self.config["task_file_glob"]),
                       key=lambda p: p.stat().st_mtime)
        result = []
        for p in files:
            fields = mod.parse_task_file(p)
            status_full = (fields.get("status") or "").strip()
            status = status_full.splitlines()[0].strip() if status_full else ""
            if status == APPROVED:
                result.append((p, fields))
        return result

    def line_of(self, fields):
        return self.module.normalize_line(fields.get("assigned_line"), self.config)

    def rel_of(self, path):
        return str(Path(path).relative_to(Path(self.config["_design_dir"])))

    def entry_status(self, rel):
        state = self.module.load_state(self.dir / self.config["state_file"])
        e = state.get("entries", {}).get(rel)
        return (e or {}).get("status")

    def live_pid_on_line(self, line):
        state = self.module.load_state(self.dir / self.config["state_file"])
        for e in state.get("entries", {}).values():
            if e.get("line") == line and e.get("status") in ACTIVE_DISPATCH_STATUSES \
                    and self.module.pid_alive(e.get("pid")):
                return e.get("pid")
        return None

    def launch(self, path):
        """dispatcherのlaunch_taskへ委譲。opt_in/DIRTY/競合チェック込み。"""
        state = self.module.load_state(self.dir / self.config["state_file"])
        return self.module.launch_task(Path(path), self.config, state, self.log)


def launch_ready_worktrees(cfg, state, log, delegate, only=None):
    """READYなworktreeへ、次の承認済みタスクを1件ずつ起動する。

    起動可否はSupervisorのゲート(READY)+dispatcher側の検証(opt_in等)の
    二段構え。どちらかが拒否すれば起動しない。
    """
    sup = sup_state(state)
    results = sup.setdefault("launches", [])
    ready = {
        n for n, e in (sup.get("worktrees") or {}).items()
        if e.get("eligible_for_launch")
    }
    if only:
        ready &= set(only)
    if delegate is None or not delegate.available:
        log.write("LAUNCH_SKIP: dispatcher.pyを利用できないため起動は行わない(%s)"
                  % (delegate.error if delegate else "delegate=None"))
        return results
    if not ready:
        log.write("LAUNCH: READYなworktreeなし。未達worktree分の判定だけ記録する")

    busy_lines = set()
    launched_this_cycle = set()

    for path, fields in delegate.approved_tasks():
        fname = Path(path).name
        line = delegate.line_of(fields)
        rec = {"at": utcnow(), "file": fname, "line": line,
               "worktree_name": None, "result": None, "reasons": []}
        if line not in cfg["worktrees"]:
            rec["result"] = "SKIP"
            rec["reasons"] = ["UNKNOWN_LINE"]
        elif line in busy_lines or line in launched_this_cycle:
            rec["result"] = "SKIP"
            rec["reasons"] = ["CONFLICT_SAME_LINE(このサイクルで既に起動済み/起動予定)"]
        elif line not in ready:
            gate = ((sup.get("worktrees") or {}).get(line) or {}).get("phase")
            rec["worktree_name"] = line
            rec["result"] = "BLOCKED_BY_GATE"
            rec["reasons"] = ["WORKTREE_NOT_READY(phase=%s)" % gate]
        else:
            rel = delegate.rel_of(path)
            status = delegate.entry_status(rel)
            if status in ACTIVE_DISPATCH_STATUSES:
                rec["result"] = "SKIP"
                rec["reasons"] = ["ALREADY_DISPATCHED(status=%s)" % status]
            elif status in UNRESOLVED_DISPATCH_STATUSES:
                rec["result"] = "SKIP"
                rec["reasons"] = ["PREVIOUS_RUN_UNRESOLVED(status=%s)" % status]
            else:
                live = delegate.live_pid_on_line(line)
                if live:
                    rec["result"] = "SKIP"
                    rec["reasons"] = ["LIVE_AGENT_ON_LINE(pid=%s)" % live]
                else:
                    rc = delegate.launch(path)
                    rec["result"] = "LAUNCHED" if rc == 0 else "REJECTED_BY_DISPATCHER"
                    busy_lines.add(line)
                    launched_this_cycle.add(line)
        results.append(rec)
        log.write("LAUNCH_DECISION: %s -> %s %s"
                  % (fname, rec["result"], ";".join(rec["reasons"])))
    return results


# ---------------------------------------------------------------- コマンド

def run_pipeline(cfg, log, execute=False, do_launch=True, only=None, refresh=False):
    state = load_state(cfg["_state_path"])
    sup = sup_state(state)
    prev_run = sup.get("run_id")
    sup["run_id"] = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S")
    if prev_run:
        sup["resumed_from_run_id"] = prev_run
    sup["mode"] = "execute" if execute else "dry-run"

    targets = {n: s for n, s in cfg["worktrees"].items()
               if not only or n in only}
    if refresh:
        # --refresh: 同一HEADでも同期・ビルド・スモークをやり直す。手動再試行用
        for name, spec in targets.items():
            e = (sup.get("worktrees") or {}).get(name)
            if isinstance(e, dict):
                for k in ("sync", "build", "smoke", "manifest", "fixtask_draft"):
                    e.pop(k, None)
                log.write("REFRESH: %s のキャッシュを破棄して再判定する" % name)
    for name in sorted(targets):
        try:
            classify_and_process(cfg, state, log, name, targets[name], execute)
        except Exception as exc:
            # 1worktreeの失敗で全体を落とさない(過去にwatcherごと落ちた実障害あり)
            entries = sup.setdefault("worktrees", {})
            entries[name] = {
                "name": name, "path": str(targets[name]["_path"]),
                "phase": P_ERROR, "eligible_for_launch": False,
                "reasons": ["PIPELINE_EXCEPTION(%r)" % exc],
                "updated_at": utcnow(),
            }
            log.write("PIPELINE_ERROR: %s %r" % (name, exc))

    if execute and do_launch and cfg["launch"].get("enabled", True):
        delegate = DispatcherDelegate(cfg["_dispatcher_dir"], log)
        launch_ready_worktrees(cfg, state, log, delegate, only=only)

    save_sup_state(cfg, state)
    print_summary(cfg, state)
    return 0


def print_summary(cfg, state):
    sup = sup_state(state)
    print("=" * 72)
    print("Integration Supervisor [%s] stable=%s/%s"
          % (sup.get("mode"), cfg["stable_branch"], cfg["stable_commit"][:12]))
    print("=" * 72)
    rows = sup.get("worktrees") or {}
    order = sorted(rows.values(), key=lambda e: e.get("phase") != P_READY)
    for e in order:
        print("%-14s %-32s %s" % (
            e.get("phase"), e.get("name"), (e.get("snapshot") or {}).get("branch")))
        for r in e.get("reasons") or []:
            print("    - %s" % r)
    for r in (sup.get("launches") or [])[-20:]:
        print("LAUNCH %-28s %-18s %s" % (
            r.get("file"), r.get("result"), ";".join(r.get("reasons") or [])))


def cmd_pipeline(args, cfg, log):
    return run_pipeline(cfg, log, execute=args.execute,
                        do_launch=not args.no_launch, only=args.only,
                        refresh=args.refresh)


def cmd_watch(args, cfg, log):
    log.write("watch開始 interval=%ds execute=%s" % (args.interval, args.execute))
    try:
        while True:
            try:
                run_pipeline(cfg, log, execute=args.execute,
                             do_launch=not args.no_launch, only=args.only)
            except Exception as exc:
                log.write("WATCH_LOOP_ERROR(継続します): %r" % exc)
            time.sleep(args.interval)
    except KeyboardInterrupt:
        log.write("watch終了(ユーザー中断)")
    return 0


def cmd_show_state(args, cfg, log):
    print(json.dumps(load_state(cfg["_state_path"]).get("supervisor", {}),
                     ensure_ascii=False, indent=2))
    return 0


def main(argv=None):
    common = argparse.ArgumentParser(add_help=False)
    common.add_argument("--config", default=None, help="設定JSONのパス")
    parser = argparse.ArgumentParser(
        description=__doc__.splitlines()[0], parents=[common])
    sub = parser.add_subparsers(dest="cmd")
    p_pipe = sub.add_parser("pipeline", help="全worktreeの判定と処理(既定dry-run)",
                            parents=[common])
    p_pipe.add_argument("--execute", action="store_true",
                        help="同期・ビルド・スモークを実際に実行する")
    p_pipe.add_argument("--no-launch", action="store_true",
                        help="--execute時でもCoder起動を行わない")
    p_pipe.add_argument("--only", nargs="*", default=None,
                        help="対象worktree名を限定する")
    p_pipe.add_argument("--refresh", action="store_true",
                        help="同一HEADのキャッシュ(成功/失敗)を破棄して再実行する")
    p_watch = sub.add_parser("watch", help="pipelineの常駐ループ", parents=[common])
    p_watch.add_argument("--interval", type=int, default=300)
    p_watch.add_argument("--execute", action="store_true")
    p_watch.add_argument("--no-launch", action="store_true")
    p_watch.add_argument("--only", nargs="*", default=None)
    sub.add_parser("show-state", help="supervisor状態を表示", parents=[common])

    args = parser.parse_args(argv)
    if args.cmd is None:
        args.cmd = "pipeline"

    cfg = load_config(args.config)
    log = Log(cfg["_log_path"])
    handlers = {"pipeline": cmd_pipeline, "watch": cmd_watch,
                "show-state": cmd_show_state}
    return handlers[args.cmd](args, cfg, log)


if __name__ == "__main__":
    sys.exit(main())
