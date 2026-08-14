# Current Task

## Goal

Add an "externally driven frame" capability to `PMXActor` and `XActor` (and thread
it through `IMesh`/`PMXMesh`/`XMesh`/`MeshObject`): a new `SetCurrentFrame(float
ActionFrame)` method that lets a caller explicitly scrub the displayed animation
pose to an exact frame, instead of the actor auto-advancing its own pose every
`Update()`. This is the foundational piece for a future "Action Timeline Editor"
(tuning Player attack actions frame-by-frame) — this task is ONLY the low-level
mesh-scrubbing capability. No JSON, no Combat changes, no Editor UI beyond one
temporary debug slider for manual verification.

## Background

This follows a design investigation (see `tasks/investigation-report.md` for the
full fact-finding this is based on). Key findings that shape this task:

- **`PMXActor`** already has frame-based animation (`m_CurrentAnimationTime`,
  `m_StartFrame`, `m_EndFrame`, etc.), but its `Update()` computes the current frame
  from **wall-clock time elapsed since construction** (`m_AnimationStartTime`), not
  from `GameTime::GetDeltaTime()`, and does this **unconditionally** — the existing
  `m_IsPlayingAnimation` member is initialized to `false` in the constructor but is
  **never read anywhere** (dead field). `PlayAnimation()`/`StopAnimation()` are
  empty stubs. There is currently no way to freeze the pose or set it to an exact
  frame from outside the class.
- **`XActor`** has no "frame" concept at all — it advances `m_CurrentTime` (Tick
  units, not frames) via `GameTime::GetDeltaTime() * m_Skeleton.TicksPerSecond`,
  unconditionally, whenever `m_CurrentClipIndex >= 0`. `TicksPerSecond` is a
  per-file value (from `XSkeleton::SkeletalData`, defaults to 4800 if the file
  doesn't specify one). There's no external getter or setter for the current
  time/tick.
- We're defining a new, format-agnostic **"Action Frame"** concept: a plain
  30fps-based frame number (`ActionFrame`), independent of whichever underlying
  format (PMX frames or X ticks) actually backs a given model. For PMX, `ActionFrame`
  maps 1:1 to the existing frame concept. For X, it converts via
  `Tick = ActionFrame / 30.0f * TicksPerSecond` (using that specific clip's own
  `TicksPerSecond`, not a hardcoded constant).

## Scope

- `SourceCode/10_Ggraphic/PMX/PMXActor.h/.cpp`
- `SourceCode/10_Ggraphic/X/XActor.h/.cpp`
- `SourceCode/10_Ggraphic/Model/IMesh.h`
- `SourceCode/10_Ggraphic/PMX/PMXMesh.h/.cpp`
- `SourceCode/10_Ggraphic/X/XMesh.h/.cpp`
- `SourceCode/00_Game/10_Object/10_MeshObject/MeshObject.h/.cpp`
- `SourceCode/99_Utility/Debug/Imgui/ModelPreviewPanel.h/.cpp` (adding ONE
  temporary debug slider only, see Implementation Requirements — not a full
  timeline UI)

## Out of Scope

- No `AttackActionData`/JSON schema work, no changes to `Combat.h/.cpp` or any
  `AttackCombo_*` state, no changes to `AnimationClipTable`/`AnimationEditor`.
  Those are later stages.
- Don't change `PlayNamedClip`'s existing behavior or call sites (Idle/Run/etc.
  must keep working exactly as before — this task adds a NEW, separate,
  opt-in method, it doesn't touch the existing free-running path's call sites).
- Don't build any timeline/track visualization — the verification UI for this task
  is a single `ImGuiManager::Input`/`Tweak` float slider, nothing more.
- Don't touch `Character`, `Player`, `Boss`, or any State files.

## Implementation Requirements

### 1. `PMXActor` — repurpose the existing (currently dead) `m_IsPlayingAnimation` flag

Read `PMXActor.h`/`.cpp` fully first — this task changes existing behavior, not
just adds new methods, so precision matters.

- **Constructor**: change `m_IsPlayingAnimation`'s initializer from `false` to
  `true`. This preserves the CURRENT effective behavior (today it always free-runs
  regardless of the flag, since the flag is never checked — initializing to `true`
  and then gating `Update()`'s wall-clock formula behind it, as described next,
  keeps default behavior identical).
- **`Update()`**: wrap the existing wall-clock frame-computation block (the
  `deltaTimeChrono`/`fmod`/`m_CurrentAnimationTime = ...` logic) in
  `if (m_IsPlayingAnimation) { ...existing code unchanged... }`. When false, skip
  it entirely (frame stays frozen at whatever it currently is — either from the
  last free-running frame, or from an explicit `SetCurrentFrame` call).
- **New method** (public, near `SetPlaybackRange`/`SetAnimationSpeed`):
  ```cpp
  // 外部から明示的にフレームを指定する(以後 SetCurrentFrame/StepFrame等で明示制御するまで
  // 自動再生を停止する. Action Timeline Editorのフレームスクラブ用).
  void SetCurrentFrame(float ActionFrame) noexcept
  {
      m_CurrentAnimationTime = ActionFrame;
      m_IsPlayingAnimation   = false;
  }
  ```
- Before making the `m_IsPlayingAnimation` default-flip, grep the whole codebase
  for every call site that constructs a `PMXActor` or calls `PMXMesh`/`MeshObject`'s
  `Update()` path, to confirm nothing already depends on the old "always advances
  regardless of any flag" behavior in a way this change would break. Report what
  you find in `review_points` even if nothing looks affected — this is exactly the
  kind of default-behavior change that's worth double-checking.

### 2. `XActor` — add a new flag (no existing field to repurpose here)

- Add a new private member `bool m_IsExternallyDriven = false;` (default `false` =
  preserves current always-auto-advance behavior).
- In `Update()`, wrap the existing `m_CurrentTime += GameTime::GetDeltaTime() *
  TicksPerSecond; ...fmod...` block in `if (!m_IsExternallyDriven) { ...unchanged... }`.
- **New method**:
  ```cpp
  // 外部から明示的にActionFrame(30fps換算)を指定する. 以後は自動再生を停止する.
  void SetCurrentFrame(float ActionFrame) noexcept
  {
      if (m_CurrentClipIndex < 0) { return; }
      m_CurrentTime = ActionFrame / 30.0f * static_cast<float>(m_Skeleton.TicksPerSecond);
      m_IsExternallyDriven = true;
  }
  ```
  Place it near `PlayAnimation`/`StopAnimation` in the public section.

### 3. `IMesh.h` — add the pure virtual method

```cpp
// 外部から明示的にActionFrame(30fps換算のフォーマット非依存フレーム番号)を指定する.
virtual void SetCurrentFrame(float ActionFrame) = 0;
```

### 4. `PMXMesh`/`XMesh` — override and forward

`PMXMesh::SetCurrentFrame(float ActionFrame) override` → `m_pActor->SetCurrentFrame(ActionFrame);`
`XMesh::SetCurrentFrame(float ActionFrame) override` → `m_upActor->SetCurrentFrame(ActionFrame);`
(Check each file's actual member name for its owned actor pointer before writing
this — confirm `m_pActor`/`m_upActor` are still the correct names.)

### 5. `MeshObject` — add a forwarding method

```cpp
// アタッチ中のメッシュへActionFrameを直接指定する(未アタッチなら何もしない).
void SetCurrentFrame(float ActionFrame) noexcept { if (m_pMesh) { m_pMesh->SetCurrentFrame(ActionFrame); } }
```
This isn't exercised by anything yet (Combat integration is a later task) — it
just needs to compile and be correct by inspection.

### 6. `ModelPreviewPanel` — one temporary debug slider for manual verification

Add a single float slider (e.g. `ImGuiManager::Tweak("Action Frame (Debug)",
value, 0.0f, 120.0f)`) to whichever ImGui block `ModelPreviewPanel::Draw()`/the
`AnimationEditor` panel already renders in (read the file first to find the most
natural spot — likely right where `AnimationEditor`'s existing PMX Start/End/Speed
controls are drawn, or a new small standalone block if that's cleaner). When the
value changes, call `SetCurrentFrame(value)` on whichever actor
(`m_pPMXActor`/`m_upXActor` — confirm actual member names) is currently loaded.
This is ONLY for this task's manual verification — it's fine for it to be a bit
rough, it is not the final Editor UI.

## Relevant Files

Read before starting:
- `SourceCode/10_Ggraphic/PMX/PMXActor.h/.cpp` (full read required — this task
  changes its `Update()` behavior, not just adds a method)
- `SourceCode/10_Ggraphic/X/XActor.h/.cpp` (full read required, same reason)
- `SourceCode/10_Ggraphic/Model/IMesh.h`
- `SourceCode/10_Ggraphic/PMX/PMXMesh.h/.cpp`
- `SourceCode/10_Ggraphic/X/XMesh.h/.cpp`
- `SourceCode/00_Game/10_Object/10_MeshObject/MeshObject.h/.cpp`
- `SourceCode/99_Utility/Debug/Imgui/ModelPreviewPanel.h/.cpp`
- `tasks/investigation-report.md` (background — the fact-finding this task is
  based on; read section A.2 and A.3 in particular)

## Acceptance Criteria

- Debug|x64 and Release|x64 both compile with 0 errors, 0 warnings (aside from the
  known pre-existing unrelated `Vertex.hlsl` Release warning).
- Existing free-running animation behavior is unchanged by default: Idle/Run/
  DodgeExecute/etc. (Player/Boss states using `PlayNamedClip`) and
  `ModelPreviewPanel`'s existing PMX/X preview behavior must still work exactly as
  before when `SetCurrentFrame` is never called.
- With the new debug slider, dragging it visibly changes the displayed pose for
  BOTH a loaded PMX model and a loaded X model, and the pose stays frozen at that
  frame (doesn't keep animating on its own) once set.
- No changes to `Combat`, `AttackCombo_*`, `AnimationClipTable`, `AnimationEditor`,
  `Character`, `Player`, `Boss`.

## Build

```powershell
powershell -File scripts\build.ps1
powershell -File scripts\build.ps1 -Configuration Release
```

Expected result: 0 errors, 0 warnings for both (aside from the known pre-existing
`Vertex.hlsl` Release warning — not part of this task).

## Test

No automated test suite. Manual verification (launching, switching between a PMX
and an X model in `ModelPreviewPanel`, dragging the new debug slider, confirming
the pose scrubs and freezes correctly for both) will be done by Claude/the user
after this lands — you don't need to attempt it, just make sure both builds pass
and the logic reads correctly.

## Notes

- This is intentionally a small, isolated first slice of a much bigger planned
  feature (see `tasks/investigation-report.md`). Don't anticipate or scaffold the
  later stages (JSON schema, Combat integration, timeline UI) — just this.
- If anything about the `PMXActor::m_IsPlayingAnimation` default-flip looks
  risky once you've read the actual call sites, stop and report it in
  `review_points` rather than guessing — this is exactly the kind of judgment call
  worth flagging per `AGENTS.md`'s Stop conditions.
