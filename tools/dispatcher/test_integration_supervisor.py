"""integration_supervisor.py のテスト。実行: py -3 test_integration_supervisor.py

fixtureはtempdir内に実物のgitリポジトリとworktreeを作る。ビルド・スモークは
スタブスクリプトへ差し替えるため、テスト中にmsbuildやゲーム本体は起動しない。
"""

import json
import os
import shutil
import stat
import subprocess
import sys
import tempfile
import unittest
import unittest.mock as mock
from pathlib import Path

import integration_supervisor as sup


def _rmtree_onerror(func, path, exc_info):
    # .git配下のpackファイルが読み取り専用で削除に失敗するため外す
    os.chmod(path, stat.S_IWRITE)
    func(path)


def git(worktree, *args):
    out = subprocess.run(
        ["git", "-C", str(worktree)] + list(args),
        capture_output=True, text=True, encoding="utf-8", errors="replace",
    )
    assert out.returncode == 0, "git %s failed: %s" % (args, out.stderr)
    return (out.stdout or "").strip()


class Fixture:
    """テスト用のgitリポジトリ+worktree+Supervisor設定を作る。"""

    def __init__(self, root, test=None):
        self.test = test
        self.root = Path(root)
        self.worktrees = {}
        self.repo = self.root / "repo"
        self.design = self.root / "design"
        self.design.mkdir(parents=True)
        git(self.root, "init", "repo")
        # コミット作者を固定(環境のgitconfigに依存しない)
        for args in (["config", "user.email", "t@example.com"],
                     ["config", "user.name", "tester"],
                     ["config", "core.autocrlf", "false"]):
            git(self.repo, *args)
        (self.repo / "app.txt").write_text("line=base\n", encoding="utf-8")
        git(self.repo, "add", "-A")
        git(self.repo, "commit", "-m", "c0")
        self.main_branch = git(self.repo, "rev-parse", "--abbrev-ref", "HEAD")
        self.stable_commit = git(self.repo, "rev-parse", "HEAD")
        git(self.repo, "branch", "stable")
        self.build_count = self.root / "build_count.txt"

        stub_dir = self.root / "fixtures"
        stub_dir.mkdir(parents=True, exist_ok=True)
        stub = stub_dir / "stub_build.py"
        stub.write_text(
            "import os, sys\n"
            "open(%r, 'a').write('x')\n"
            "mode = os.environ.get('BUILD_MODE', 'ok')\n"
            "if mode == 'fail_errors':\n"
            "    print('x.cpp(10): error C2065: undefined'); sys.exit(1)\n"
            "if mode == 'fail_warnings':\n"
            "    print('x.cpp(11): warning C4100: unused'); sys.exit(0)\n"
            "print('   0 Warning(s)'); print('   0 Error(s)'); sys.exit(0)\n"
            % str(self.build_count),
            encoding="utf-8")

    def commit_on_stable(self, filename, content, msg="stable side"):
        """stable branchを進め、Supervisorの基準commitも新しいtipへ更新する。"""
        git(self.repo, "checkout", "stable")
        try:
            self.commit(filename, content, msg)
            self.stable_commit = git(self.repo, "rev-parse", "stable")
        finally:
            git(self.repo, "checkout", self.main_branch)

    def commit(self, filename, content, msg):
        path = self.repo / filename
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")
        git(self.repo, "add", "-A")
        git(self.repo, "commit", "-m", msg)
        return git(self.repo, "rev-parse", "HEAD")

    def add_worktree(self, name, branch=None):
        wt = self.root / ("wt_" + name)
        if branch:
            git(self.repo, "worktree", "add", "-b", branch, str(wt))
        else:
            git(self.repo, "worktree", "add", str(wt))
        for args in (["config", "user.email", "t@example.com"],
                     ["config", "user.name", "tester"]):
            git(wt, *args)
        self.worktrees[name] = wt
        return wt

    def build_command(self):
        return [sys.executable, str(self.root / "fixtures" / "stub_build.py")]

    def set_build_mode(self, mode):
        os.environ["BUILD_MODE"] = mode
        if self.test is not None:
            self.test.addCleanup(os.environ.pop, "BUILD_MODE", None)

    def write_cfg(self, **over):
        cfg = {
            "repo_root": str(self.root),
            "design_dir": str(self.design),
            "state_file": "state.json",
            "log_file": "supervisor.log",
            "manifest_dir": "manifests",
            "dispatcher_dir": str(self.root / "no_dispatcher_here"),
            "stable_branch": "stable",
            "stable_commit": self.stable_commit,
            "merge_timeout_sec": 120,
            "build": {
                "command": self.build_command(),
                "timeout_sec": 120,
                "zero_warnings_required": True,
            },
            "smoke": {
                "launch_command": [sys.executable, "-c",
                                   "import time; time.sleep(3)"],
                "cwd_relative": "",
                "min_alive_seconds": 1,
                "poll_interval_seconds": 0.2,
                "dump_dirs": ["Dumps"],
            },
            "launch": {"enabled": True},
            "worktrees": {n: {"path": str(p)}
                          for n, p in self.worktrees.items()},
        }
        cfg.update(over)
        path = self.root / "cfg.json"
        path.write_text(json.dumps(cfg, ensure_ascii=False, indent=2),
                        encoding="utf-8")
        return sup.load_config(path)

    def write_task_file(self, name, line):
        path = self.design / ("next_feature_task_%s.md" % name)
        path.write_text(
            "# Task\n\n```\nStatus:\nAPPROVED_FOR_IMPLEMENTATION\n\n"
            "Task ID:\ntask_%s\n\nAssigned Line:\n%s\n```\n" % (name, line),
            encoding="utf-8")
        return path


class FakeDelegate:
    """dispatcher.py起動委譲の置き換え。呼び出し記録だけ残す。"""

    def __init__(self, fixture=None, task_list=None, statuses=None, live_pid=None):
        self.available = True
        self.error = None
        self.calls = []
        self.statuses = statuses or {}
        self.live_pid = live_pid
        self.tasks = []
        if fixture is not None and task_list is not None:
            self.bind(fixture, task_list)

    def bind(self, fixture, task_list):
        self.tasks = [(fixture.write_task_file(n, line), {"assigned_line": line})
                      for n, line in task_list]
        return self

    def approved_tasks(self):
        return list(self.tasks)

    def line_of(self, fields):
        return fields.get("assigned_line")

    def rel_of(self, path):
        return Path(path).name

    def entry_status(self, rel):
        return self.statuses.get(rel)

    def live_pid_on_line(self, line):
        return self.live_pid

    def launch(self, path):
        self.calls.append(str(path))
        return 0


class SupervisorTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.mkdtemp(prefix="supv_test_")
        self.addCleanup(shutil.rmtree, self.tmp, True, _rmtree_onerror)
        self.fx = Fixture(self.tmp, test=self)

    def run_pipeline(self, cfg, execute=True, delegate=None, only=None,
                     refresh=False):
        log = sup.Log(cfg["_log_path"], verbose=False)
        if delegate is None:
            with mock.patch.object(sup, "DispatcherDelegate") as cls:
                cls.return_value.available = False
                rc = sup.run_pipeline(cfg, log, execute=execute,
                                      do_launch=True, only=only, refresh=refresh)
        else:
            with mock.patch.object(sup, "DispatcherDelegate",
                                   lambda d, l: delegate):
                rc = sup.run_pipeline(cfg, log, execute=execute,
                                      do_launch=True, only=only, refresh=refresh)
        state = sup.load_state(cfg["_state_path"])
        entries = state["supervisor"]["worktrees"]
        launches = state["supervisor"].get("launches") or []
        return entries, launches, state

    # ------------------------------------------------------------ 基本フロー

    def test_clean_behind_worktree_reaches_ready_with_launch(self):
        wt = self.fx.add_worktree("suzaku", "line/suzaku")
        head_before = git(wt, "rev-parse", "HEAD")
        # stableだけ先へ進める → worktreeはbehind=1でクリーン
        self.fx.commit_on_stable("stable_only.txt", "s\n", "c1 stable side")
        cfg = self.fx.write_cfg()
        fake = FakeDelegate().bind(self.fx, [("a", "suzaku")])

        entries, launches, _ = self.run_pipeline(cfg, delegate=fake)

        e = entries["suzaku"]
        self.assertEqual(e["phase"], sup.P_READY)
        self.assertTrue(e["eligible_for_launch"])
        self.assertEqual(e["sync"]["status"], "FAST_FORWARD")
        self.assertEqual(e["build"]["status"], "PASS")
        self.assertEqual(e["build"]["errors"], 0)
        self.assertEqual(e["smoke"]["status"], "PASS")
        self.assertNotEqual(e["snapshot"]["head"], head_before)
        self.assertTrue((wt / "stable_only.txt").exists())
        launched = [r for r in launches if r["result"] == "LAUNCHED"]
        self.assertEqual(len(launched), 1)
        self.assertEqual(len(fake.calls), 1)

    def test_up_to_date_clean_worktree_skips_merge(self):
        self.fx.add_worktree("byakko", "line/byakko")
        cfg = self.fx.write_cfg()
        entries, _, _ = self.run_pipeline(cfg, only=["byakko"])
        e = entries["byakko"]
        self.assertEqual(e["phase"], sup.P_READY)
        self.assertEqual(e["sync"]["status"], "UP_TO_DATE")

    def test_dry_run_does_not_mutate_anything(self):
        wt = self.fx.add_worktree("seiryu", "line/seiryu")
        self.fx.commit_on_stable("stable_only.txt", "s\n", "c1")
        cfg = self.fx.write_cfg()
        entries, _, _ = self.run_pipeline(cfg, execute=False, only=["seiryu"])
        e = entries["seiryu"]
        self.assertIn("PLANNED(merge stable)", e["sync"]["status"])
        self.assertFalse(e["eligible_for_launch"])
        self.assertFalse((wt / "stable_only.txt").exists())
        self.assertFalse(self.fx.build_count.exists())

    # ------------------------------------------------------------ DIRTY隔離

    def test_dirty_worktree_isolated_and_changes_preserved(self):
        wt = self.fx.add_worktree("genbu", "line/genbu")
        target = wt / "app.txt"
        original = target.read_text(encoding="utf-8")
        target.write_text(original + "local edit\n", encoding="utf-8")
        cfg = self.fx.write_cfg()
        fake = FakeDelegate().bind(self.fx, [("g", "genbu")])

        entries, _, _ = self.run_pipeline(cfg, delegate=fake)

        e = entries["genbu"]
        self.assertEqual(e["phase"], sup.P_DIRTY)
        self.assertFalse(e["eligible_for_launch"])
        self.assertTrue(any("WORKTREE_DIRTY_CHANGES_PRESERVED" in r
                            for r in e["reasons"]))
        self.assertIsNone(e.get("build"))
        self.assertEqual(target.read_text(encoding="utf-8"),
                         original + "local edit\n")
        self.assertEqual(fake.calls, [])

    # ------------------------------------------------------------ 競合

    def test_conflict_aborts_records_manifest_without_retry(self):
        # stable側とworktree側で同じ行を別々に編集して競合させる
        wt = self.fx.add_worktree("kouryuu", "line/kouryuu")
        (wt / "app.txt").write_text("line=coder\n", encoding="utf-8")
        git(wt, "add", "-A")
        git(wt, "commit", "-m", "coder edit")
        pre_head = git(wt, "rev-parse", "HEAD")
        self.fx.commit_on_stable("app.txt", "line=stable\n", "stable edit")

        cfg = self.fx.write_cfg()
        entries, _, _ = self.run_pipeline(cfg, only=["kouryuu"])
        e = entries["kouryuu"]
        self.assertEqual(e["phase"], sup.P_CONFLICT)
        self.assertFalse(e["eligible_for_launch"])
        self.assertEqual(e["sync"]["conflict_files"], ["app.txt"])
        self.assertTrue(e["sync"]["restored_cleanly"])
        # merge --abortによりマージ前の状態へ戻っている
        self.assertEqual(git(wt, "rev-parse", "HEAD"), pre_head)
        self.assertEqual(git(wt, "status", "--porcelain"), "")

        manifest = json.loads(
            (Path(cfg["_manifest_dir"]) / "supervisor_kouryuu.json")
            .read_text(encoding="utf-8"))
        self.assertEqual(manifest["kind"], "sync_conflict")
        self.assertIn("merge", manifest["resume"])

        # 同一HEADでの再実行では競合を自動再試行しない
        entries2, _, _ = self.run_pipeline(cfg, only=["kouryuu"])
        self.assertEqual(entries2["kouryuu"]["phase"], sup.P_CONFLICT)
        self.assertIn("CONFLICT_HOLD",
                      (Path(self.tmp) / "supervisor.log").read_text(encoding="utf-8"))

    # ------------------------------------------------------------ ビルドゲート

    def test_zero_warning_policy_blocks_launch_and_holds(self):
        self.fx.add_worktree("line7", "line/line7")
        self.fx.set_build_mode("fail_warnings")
        cfg = self.fx.write_cfg()
        fake = FakeDelegate().bind(self.fx, [("l7", "line7")])
        entries, _, _ = self.run_pipeline(cfg, only=["line7"], delegate=fake)
        e = entries["line7"]
        self.assertEqual(e["phase"], sup.P_BUILD_FAIL)
        self.assertEqual(e["build"]["warnings"], 1)
        self.assertFalse(e["eligible_for_launch"])
        self.assertEqual(fake.calls, [])
        # 同一HEADでの再実行でも自動再試行しない(HOLD)
        before = self.fx.build_count.read_text(encoding="utf-8")
        entries2, _, _ = self.run_pipeline(cfg, only=["line7"])
        self.assertEqual(entries2["line7"]["phase"], sup.P_BUILD_FAIL)
        self.assertEqual(self.fx.build_count.read_text(encoding="utf-8"), before)

    def test_build_error_saves_summary_manifest_and_fixtask_draft(self):
        self.fx.add_worktree("line8", "line/line8")
        self.fx.set_build_mode("fail_errors")
        cfg = self.fx.write_cfg()
        entries, _, _ = self.run_pipeline(cfg, only=["line8"])
        e = entries["line8"]
        self.assertEqual(e["phase"], sup.P_BUILD_FAIL)
        self.assertGreaterEqual(e["build"]["errors"], 1)
        self.assertIn("error C2065", "\n".join(e["build"]["error_samples"]))
        manifest = json.loads(
            (Path(cfg["_manifest_dir"]) / "supervisor_line8.json")
            .read_text(encoding="utf-8"))
        self.assertEqual(manifest["kind"], "build_failed")
        fixtask = Path(e["fixtask_draft"])
        self.assertTrue(fixtask.is_file())
        self.assertIn("DRAFT_NOT_APPROVED", fixtask.read_text(encoding="utf-8"))

    # ------------------------------------------------------------ スモークゲート

    def test_smoke_quick_exit_blocks_launch_and_saves_reason(self):
        self.fx.add_worktree("w1", "line/w1")
        cfg = self.fx.write_cfg(smoke={
            "launch_command": [sys.executable, "-c", "import sys; sys.exit(3)"],
            "cwd_relative": "", "min_alive_seconds": 1,
            "poll_interval_seconds": 0.2, "dump_dirs": ["Dumps"]})
        entries, _, _ = self.run_pipeline(cfg, only=["w1"])
        e = entries["w1"]
        self.assertEqual(e["phase"], sup.P_SMOKE_FAIL)
        self.assertIn("EXITED_EARLY", e["reasons"][0])
        self.assertIsInstance(e["smoke"]["pid"], int)
        self.assertEqual(e["smoke"]["exit_code"], 3)
        manifest = json.loads(
            (Path(cfg["_manifest_dir"]) / "supervisor_w1.json")
            .read_text(encoding="utf-8"))
        self.assertEqual(manifest["kind"], "smoke_failed")
        self.assertIn("pid", manifest)

    def test_smoke_new_crash_dump_blocks_launch(self):
        self.fx.add_worktree("w2", "line/w2")
        snippet = (
            "import time, pathlib\n"
            "d = pathlib.Path('Dumps'); d.mkdir(exist_ok=True)\n"
            "(d / 'crash.dmp').write_bytes(b'dump')\n"
            "time.sleep(3)\n")
        cfg = self.fx.write_cfg(smoke={
            "launch_command": [sys.executable, "-c", snippet],
            "cwd_relative": "", "min_alive_seconds": 1,
            "poll_interval_seconds": 0.2, "dump_dirs": ["Dumps"]})
        entries, _, _ = self.run_pipeline(cfg, only=["w2"])
        e = entries["w2"]
        self.assertEqual(e["phase"], sup.P_SMOKE_FAIL)
        self.assertIn("NEW_CRASH_DUMP", e["reasons"][0])
        self.assertEqual(e["smoke"]["new_dumps"], ["Dumps/crash.dmp"])

    # ------------------------------------------------------------ 再開・再試行

    def test_restart_restores_state_and_skips_rerun(self):
        self.fx.add_worktree("w3", "line/w3")
        cfg = self.fx.write_cfg()
        entries, _, _ = self.run_pipeline(cfg, only=["w3"])
        self.assertEqual(entries["w3"]["phase"], sup.P_READY)
        runs_after_first = self.fx.build_count.read_text(encoding="utf-8")

        # Supervisor再起動を想定: 新しいLog/stateで同じ処理をもう一度
        entries2, _, _ = self.run_pipeline(cfg, only=["w3"])
        self.assertEqual(entries2["w3"]["phase"], sup.P_READY)
        self.assertIn("CACHED_RESULT_REUSED", entries2["w3"]["reasons"][0])
        self.assertEqual(self.fx.build_count.read_text(encoding="utf-8"),
                         runs_after_first)

    def test_new_commit_invalidates_cache_and_reruns(self):
        wt = self.fx.add_worktree("w4", "line/w4")
        cfg = self.fx.write_cfg()
        entries, _, _ = self.run_pipeline(cfg, only=["w4"])
        self.assertEqual(entries["w4"]["phase"], sup.P_READY)
        # クリーンな新規コミット → HEADが変わるのでビルド・スモークをやり直す
        (wt / "extra.txt").write_text("x\n", encoding="utf-8")
        git(wt, "add", "-A")
        git(wt, "commit", "-m", "extra")
        entries2, _, _ = self.run_pipeline(cfg, only=["w4"])
        self.assertEqual(entries2["w4"]["phase"], sup.P_READY)
        self.assertEqual(len(self.fx.build_count.read_text(encoding="utf-8")), 2)

    def test_refresh_flag_forces_full_rerun(self):
        self.fx.add_worktree("w5", "line/w5")
        cfg = self.fx.write_cfg()
        self.run_pipeline(cfg, only=["w5"])
        entries, _, _ = self.run_pipeline(cfg, only=["w5"], refresh=True)
        self.assertEqual(entries["w5"]["phase"], sup.P_READY)
        self.assertNotIn("CACHED_RESULT_REUSED", entries["w5"]["reasons"][0])
        self.assertEqual(len(self.fx.build_count.read_text(encoding="utf-8")), 2)

    # ------------------------------------------------------------ 起動判断

    def test_duplicate_task_and_conflict_same_line_rejected(self):
        self.fx.add_worktree("w6", "line/w6")
        cfg = self.fx.write_cfg()
        fake = FakeDelegate().bind(self.fx, [("t1", "w6"), ("t2", "w6")])
        entries, launches, _ = self.run_pipeline(cfg, only=["w6"], delegate=fake)
        results = {r["file"]: r["result"] for r in launches}
        self.assertEqual(results["next_feature_task_t1.md"], "LAUNCHED")
        t2 = next(r for r in launches if r["file"] == "next_feature_task_t2.md")
        self.assertEqual(t2["result"], "SKIP")
        self.assertIn("CONFLICT_SAME_LINE", t2["reasons"][0])
        self.assertEqual(len(fake.calls), 1)

    def test_already_dispatched_task_not_relaunched(self):
        self.fx.add_worktree("w7", "line/w7")
        cfg = self.fx.write_cfg()
        fake = FakeDelegate().bind(self.fx, [("t3", "w7")])
        fake.statuses["next_feature_task_t3.md"] = "WORKING"
        _, launches, _ = self.run_pipeline(cfg, only=["w7"], delegate=fake)
        rec = launches[0]
        self.assertEqual(rec["result"], "SKIP")
        self.assertIn("ALREADY_DISPATCHED", rec["reasons"][0])
        self.assertEqual(fake.calls, [])

    def test_dead_pid_task_requires_human_review(self):
        self.fx.add_worktree("w8", "line/w8")
        cfg = self.fx.write_cfg()
        fake = FakeDelegate().bind(self.fx, [("t4", "w8")])
        fake.statuses["next_feature_task_t4.md"] = "DEAD_PID"
        _, launches, _ = self.run_pipeline(cfg, only=["w8"], delegate=fake)
        self.assertEqual(launches[0]["result"], "SKIP")
        self.assertIn("PREVIOUS_RUN_UNRESOLVED", launches[0]["reasons"][0])
        self.assertIn("DEAD_PID", launches[0]["reasons"][0])
        self.assertEqual(fake.calls, [])

    def test_unready_worktree_blocked_by_gate(self):
        wt = self.fx.add_worktree("w9", "line/w9")
        (wt / "scratch.txt").write_text("wip\n", encoding="utf-8")
        cfg = self.fx.write_cfg()
        fake = FakeDelegate().bind(self.fx, [("t5", "w9")])
        _, launches, _ = self.run_pipeline(cfg, only=["w9"], delegate=fake)
        rec = launches[0]
        self.assertEqual(rec["result"], "BLOCKED_BY_GATE")
        self.assertIn("ISOLATED_DIRTY", rec["reasons"][0])
        self.assertEqual(fake.calls, [])

    def test_live_agent_on_line_prevents_second_agent(self):
        self.fx.add_worktree("w10", "line/w10")
        cfg = self.fx.write_cfg()
        # 実在するPIDを使ってtasklistベースの生存判定も通す
        fake = FakeDelegate(live_pid=os.getpid()).bind(self.fx, [("t6", "w10")])
        _, launches, _ = self.run_pipeline(cfg, only=["w10"], delegate=fake)
        self.assertEqual(launches[0]["result"], "SKIP")
        self.assertIn("LIVE_AGENT_ON_LINE", launches[0]["reasons"][0])
        self.assertEqual(fake.calls, [])

    # ------------------------------------------------------------ 補助

    def test_snapshot_captures_branch_head_status_changed_files(self):
        wt = self.fx.add_worktree("w11", "line/w11")
        (wt / "dirty.txt").write_text("d\n", encoding="utf-8")
        snap = sup.snapshot_worktree(wt, self.fx.stable_commit)
        self.assertEqual(snap["branch"], "line/w11")
        self.assertEqual(snap["head"], git(wt, "rev-parse", "HEAD"))
        self.assertTrue(snap["dirty"])
        self.assertIn("?? dirty.txt", snap["changed_files"])
        self.assertEqual(snap["ahead_of_stable"], 0)
        self.assertEqual(snap["behind_stable"], 0)


if __name__ == "__main__":
    unittest.main(verbosity=2)
