# Current Task

## Goal

Replace the shared placeholder PMX model (Hatune Miku, used for both `Player` and
`Boss` since they were first wired into `MainScene`) with the dedicated `.X` models
that already exist for each: `Data/Model/X/Player/player.X` and
`Data/Model/X/Boss/boss.X`. Both actors must also actually play their named
animation clips per state (Idle/Run/Attack/etc.), not just display statically.

## Background

`MeshObject` (`00_Game/10_Object/10_MeshObject/MeshObject.h/.cpp`) currently holds a
`std::shared_ptr<PMXMesh>` directly — it is hard-coded to PMX. `PMXMesh` wraps
`PMXActor` and exposes `Update/Draw/SetWorldTransform(const Transform&)/
ApplyAnimationClip(Start,End,Speed)` (frame-range animation, driven by
`AnimationClipTable`, a name→{Start,End,Speed} text file the user edits via
`AnimationEditor`).

`.X` files use a completely different, incompatible animation model: each clip is a
**named** clip embedded directly in the file (`XSkeleton::AnimationClip`), played via
`XActor::PlayAnimation(ClipName)` (Tick-based timing, no Start/End frame concept in
the same units). `XActor` (`10_Ggraphic/X/XActor.h/.cpp`) already exists and is fully
functional (skinning, bone hierarchy, texture loading) — it shares `PMXRenderer`'s
pipeline/root signature/shader, but there is currently no facade class analogous to
`PMXMesh` wrapping it, and nothing in `00_Game/` can attach one (`MeshObject` doesn't
know `XActor` exists).

`PlayerStateBase::ApplyNamedClip(ClipName)` (`00_Game/10_Object/10_MeshObject/
00_Character/00_Player/State/PlayerStateBase.cpp`) currently does the
`AnimationClipTable` lookup itself and calls `m_pOwner->ApplyAnimationClip(Start,End,
Speed)` — this PMX-specific knowledge should move into `PMXMesh` itself so the state
code stops caring which mesh format is in use.

This task was scoped after confirming (via `grep -o 'AnimationSet [^ {]*'` on both
`.X` files) the exact clip names embedded in each model — see the mapping table below.
`Boss`'s model additionally ships two `.txt` reference files in its folder
(`boss_idleBoneList.txt`, `ボスのアニメーション番号の順番..txt`) — these are the
model author's own notes, not something this task needs to parse; the real clip names
were extracted directly from the `.X` file's `AnimationSet` blocks and are listed
below.

## Scope

- `SourceCode/10_Ggraphic/Model/IMesh.h` (new)
- `SourceCode/10_Ggraphic/PMX/PMXMesh.h/.cpp` (implement `IMesh`, absorb the
  `AnimationClipTable` lookup)
- `SourceCode/10_Ggraphic/X/XMesh.h/.cpp` (new facade wrapping `XActor`)
- `SourceCode/10_Ggraphic/X/XActor.h/.cpp` (add `StopAnimation()`)
- `SourceCode/00_Game/10_Object/10_MeshObject/MeshObject.h/.cpp`
- `SourceCode/00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/
  PlayerStateBase.h/.cpp`
- `SourceCode/00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/00_Idle/
  Idle.cpp`, `10_Run/Run.cpp`, `20_Combat/00_AttackCombo_0/AttackCombo_0.cpp`,
  `20_Combat/10_AttackCombo_1/AttackCombo_1.cpp`,
  `20_Combat/20_AttackCombo_2/AttackCombo_2.cpp`, `20_Combat/30_Parry/Parry.cpp`,
  `30_Dodge/00_DodgeExecute/DodgeExecute.cpp` (clip name string literals only)
- `SourceCode/00_Game/10_Object/10_MeshObject/00_Character/20_Boss/BossStateBase.h/
  .cpp` (add an `ApplyNamedClip` helper, mirroring `PlayerStateBase`'s but simpler —
  no table lookup, just forwards to the owner)
- `SourceCode/00_Game/10_Object/10_MeshObject/00_Character/20_Boss/State/00_Idle/
  Idle.cpp`, `10_Move/Move.cpp`, `20_Attack/Attack.cpp`, `30_Dead/Dead.cpp`,
  `40_ParryReaction/ParryReaction.cpp` (add one `ApplyNamedClip("...")` call each)
- `SourceCode/00_Game/00_Scene/10_Main/MainScene.cpp` (switch model paths + attach
  `XMesh` instead of `PMXMesh` for Player/Boss, set scale)
- `DirectX12.vcxproj` / `DirectX12.vcxproj.filters` (register the 2 new files)

## Out of Scope

- Don't touch `PMXActor`, `XParser`, `XSkeletonData.h`, `CollisionDetector`,
  `CombatCoordinator`, or any Boss/Player combat logic (damage, HP, hit windows).
  This task is rendering/animation wiring only.
- Don't add a `Dead` state to `Player` (it doesn't have one yet — `player_die` and
  `player_take_damage` clips exist in the file but are legitimately unused for now,
  leave them unused).
- Don't try to use Boss's `attack2/3`, `walk2/3`, `jump_attack*`, `spin_attack*`,
  `beem*`, `down*` clip variants — Boss currently has exactly one attack pattern and
  no variety in movement/reactions (see `DESIGN.md`), so only one clip per state is
  needed. Picking among the variants for future variety is a separate task.
- Don't modify `AnimationClipTable`/`AnimationEditor`/`ModelPreviewPanel` — they stay
  PMX-only and keep working for the standalone Hatune preview use case.
- Don't rename `PlayerState::eID`/`BossState::eID` enum values or add new states.

## Implementation Requirements

### 1. `IMesh` interface (new file)

`SourceCode/10_Ggraphic/Model/IMesh.h` — pure virtual interface, matching this
project's established pattern of small `I`+PascalCase interfaces (see `DESIGN.md`
"アーキテクチャ方針" and `IHealthSystem` for precedent). Forward-declare `struct
Transform;`.

```cpp
class IMesh
{
public:
	virtual ~IMesh() = default;

	virtual void Update() = 0;
	virtual void Draw() = 0;

	// ワールド変換を反映する.
	virtual void SetWorldTransform(const Transform& InTransform) = 0;

	// 名前でアニメーションクリップを再生する. 対応するクリップが見つからない場合の
	// 挙動は実装依存(PMXMesh: 何もしない / XMesh: バインドポーズへ戻す. 下記参照).
	virtual void PlayNamedClip(const std::string& ClipName) = 0;
};
```

### 2. `PMXMesh` implements `IMesh`

- `class PMXMesh final : public IMesh`.
- Add `#include "10_Ggraphic/Model/IMesh.h"` and mark `Update/Draw/SetWorldTransform`
  `override`.
- Add `void PlayNamedClip(const std::string& ClipName) override;` — move the exact
  logic currently in `PlayerStateBase::ApplyNamedClip` here: construct an
  `AnimationClipTable`, `Load(AnimationClipTable::DEFAULT_FILE_PATH)`, `Find(ClipName
  .c_str())`, and if found call the existing (already-implemented, keep as-is)
  `ApplyAnimationClip(p_clip->StartFrame, p_clip->EndFrame, p_clip->Speed)` internally.
  If not found, do nothing (unchanged behavior from today).
- Keep the existing public `ApplyAnimationClip(float,float,float)` method as-is — it's
  still used directly elsewhere (don't remove it, don't make it part of `IMesh`).

### 3. `XMesh` facade (new, mirrors `PMXMesh`'s shape exactly)

`SourceCode/10_Ggraphic/X/XMesh.h`:

```cpp
#pragma once

#include <memory>
#include <string>

struct Transform;
class XActor;
class PMXRenderer;

#include "10_Ggraphic/Model/IMesh.h"

class XMesh final : public IMesh
{
public:
	XMesh(const std::string& FilePath, PMXRenderer& Renderer);
	~XMesh() override;

	XMesh(const XMesh&)            = delete;
	XMesh& operator=(const XMesh&) = delete;
	XMesh(XMesh&&)                 = delete;
	XMesh& operator=(XMesh&&)      = delete;

	void Update() override;
	void Draw() override;
	void SetWorldTransform(const Transform& InTransform) override;
	void PlayNamedClip(const std::string& ClipName) override;

private:
	std::unique_ptr<XActor> m_upActor;
};
```

`XMesh.cpp`:
- Constructor: `m_upActor{ std::make_unique<XActor>(FilePath.c_str(), Renderer) }`
  (note `XActor`'s ctor takes `const char*`, and is non-copyable/non-movable itself —
  `XMesh` owns it via `unique_ptr`, same ownership shape `PMXMesh` uses via
  `shared_ptr<PMXActor>`; `unique_ptr` is fine here since nothing else needs to share
  it).
- `Update()` → `m_upActor->Update();`
- `Draw()` → `m_upActor->Draw();`
- `SetWorldTransform(const Transform& InTransform)` → `m_upActor->SetWorldMatrix(
  InTransform.GetMatrix());` (identical pattern to `PMXMesh::SetWorldTransform` —
  `Transform::GetMatrix()` already returns an `XMMATRIX`).
- `PlayNamedClip(const std::string& ClipName)`: search `m_upActor->GetClips()` for an
  entry whose name equals `ClipName`; if found, call `m_upActor->PlayAnimation(
  ClipName)`; if not found, call `m_upActor->StopAnimation()` (new method, see below).
  This fallback-to-bind-pose behavior is intentional and different from `PMXMesh`'s
  "do nothing" — it's what makes `Player`'s `Idle` state (which has no matching
  `player_idle` clip in the file — see mapping table) resolve to a sensible rest pose
  instead of freezing on whatever animation was last playing.

### 4. `XActor::StopAnimation()` (new method)

In `XActor.h`, add `void StopAnimation() noexcept { m_CurrentClipIndex = -1; }` next
to `PlayAnimation`'s declaration (public). `m_CurrentClipIndex == -1` is already the
documented "bind pose, not playing a clip" state used at construction — just exposing
a way to return to it after a clip has played. No `.cpp` change needed if implemented
inline in the header (matches this file's existing style of small inline getters).

### 5. `MeshObject` becomes format-agnostic

- Change `#include "10_Ggraphic/PMX/PMXMesh.h"` forward-declare to `class IMesh;` in
  the header instead (only `.cpp` needs `IMesh.h`, and only for the pointer type —
  actually since `IMesh` is used as `shared_ptr<IMesh>` a forward declaration in the
  header is sufficient, `.cpp` doesn't need a concrete include at all since it only
  calls virtual methods).
- `std::shared_ptr<PMXMesh> m_pMesh;` → `std::shared_ptr<IMesh> m_pMesh;`.
- `void AttachMesh(std::shared_ptr<PMXMesh> pMesh)` → `void AttachMesh(std::shared_ptr
  <IMesh> pMesh)`.
- Replace `void ApplyAnimationClip(float StartFrame, float EndFrame, float Speed);`
  with `void PlayNamedClip(const std::string& ClipName);` (forwards to `m_pMesh->
  PlayNamedClip(ClipName)` if `m_pMesh` is non-null, same null-check style as
  `Update()`/`Draw()` already use). This is a rename+signature change, not an
  addition — check every caller (see step 6/7) is updated, don't leave the old
  method behind.

### 6. `PlayerStateBase::ApplyNamedClip` simplifies

`PlayerStateBase.h`: keep the method (same name, same call sites in the 7 state
files stay untouched at the call-site level — only the string literals they pass
change, see the mapping table), but its `.cpp` body becomes a one-line forward:

```cpp
void PlayerStateBase::ApplyNamedClip(const char* ClipName) const
{
	m_pOwner->PlayNamedClip(ClipName);
}
```

Remove the now-unused `#include "10_Ggraphic/PMX/AnimationClipTable.h"` from
`PlayerStateBase.cpp` (that logic moved into `PMXMesh`, see step 2).

### 7. `BossStateBase::ApplyNamedClip` (new, mirrors the above)

Add to `BossStateBase.h/.cpp` (same shape as `PlayerStateBase`'s, `protected`):

```cpp
// Bossのメッシュへ名前でクリップを適用する(未対応ならバインドポーズへ戻る. XMesh参照).
void ApplyNamedClip(const char* ClipName) const;
```
```cpp
void BossStateBase::ApplyNamedClip(const char* ClipName) const
{
	GetBoss()->PlayNamedClip(ClipName);
}
```

### 8. Clip name mapping — update/add these exact calls

**Player** (`ApplyNamedClip(...)` call sites — update the string literal only, method
name/location in each file stays the same):

| State file | Old string | New string |
|---|---|---|
| `00_Idle/Idle.cpp` | `"Idle"` | `"Idle"` (unchanged — intentionally doesn't match any real clip, falls through to `XMesh`'s bind-pose fallback, see step 3) |
| `10_Run/Run.cpp` | `"Run"` | `"player_run"` |
| `20_Combat/00_AttackCombo_0/AttackCombo_0.cpp` | `"AttackCombo_0"` | `"player_attack1"` |
| `20_Combat/10_AttackCombo_1/AttackCombo_1.cpp` | `"AttackCombo_1"` | `"player_attack2"` |
| `20_Combat/20_AttackCombo_2/AttackCombo_2.cpp` | `"AttackCombo_2"` | `"player_attack3"` |
| `20_Combat/30_Parry/Parry.cpp` | `"Parry"` | `"player_parry"` |
| `30_Dodge/00_DodgeExecute/DodgeExecute.cpp` | `"DodgeExecute"` | `"player_perfect_dodge"` |

**Boss** (new calls — add one `ApplyNamedClip("...")` call in each state's `Enter()`,
following the exact pattern `PlayerState`'s states use: call it once when entering the
state, not every `Update()`):

| State file | Clip name to add |
|---|---|
| `State/00_Idle/Idle.cpp` | `"boss_idle"` |
| `State/10_Move/Move.cpp` | `"boss_walk1"` |
| `State/20_Attack/Attack.cpp` | `"boss_attack1"` |
| `State/30_Dead/Dead.cpp` | `"boss_die"` |
| `State/40_ParryReaction/ParryReaction.cpp` | `"boss_take_damage"` |

Look at each file's existing `Enter()` to find where to add the call (same place
`Idle.cpp`/`Run.cpp` etc. call `ApplyNamedClip` in `PlayerState`, i.e. once in
`Enter()`, not in `Update()`).

### 9. `MainScene::Create()` — switch models

Replace the single shared `model_path` (Hatune) with two separate paths, and attach
`XMesh` instead of `PMXMesh`:

```cpp
m_upPlayer = std::make_unique<Player>();
m_upPlayer->AttachMesh(std::make_shared<XMesh>("Data/Model/X/Player/player.X", *m_pPMXRenderer));
{
	Transform player_transform;
	player_transform.Position = { 0.0f, 0.0f, 0.0f };
	player_transform.Scale    = { 15.0f, 15.0f, 15.0f };
	m_upPlayer->SetTransform(player_transform);
}

m_upBoss = std::make_unique<Boss>();
m_upBoss->AttachMesh(std::make_shared<XMesh>("Data/Model/X/Boss/boss.X", *m_pPMXRenderer));
{
	Transform boss_transform;
	boss_transform.Position = { 0.0f, 0.0f, 8.0f };
	boss_transform.Scale    = { 15.0f, 15.0f, 15.0f };
	m_upBoss->SetTransform(boss_transform);
}
```

The `15.0f` scale is carried over from the only other place this project has
instantiated an `XActor`-backed model (a standalone test instance, see `DESIGN.md`'s
"15倍スケール" note) — it's a reasonable starting point but **may look wrong** (too
big/small) since Player/Boss's collision capsules and camera framing were tuned
against the PMX Hatune model's original scale. If it looks clearly wrong when
launched, that's expected and fine to leave — report the observation in
`review_points`, don't try to auto-tune it (visual tuning is a follow-up, not this
task). Add `#include "10_Ggraphic/X/XMesh.h"` to `MainScene.cpp`, and you can remove
the now-unused `#include "10_Ggraphic/PMX/PMXMesh.h"` for these two actors' construction
if `PMXMesh` isn't used anywhere else in this file (check `ModelPreviewPanel` usage
first — it may still need it via a different path; if in doubt, leave the include).

### 10. Register new files

Add `IMesh.h`, `XMesh.h`, `XMesh.cpp` to `DirectX12.vcxproj` and
`DirectX12.vcxproj.filters` (mirror the existing `10_Ggraphic/X/` and add a
`10_Ggraphic/Model/` filter entry if one doesn't already exist for `ModelData.h`/
`IModelParser.h` — check first, they may already share one).

## Relevant Files

Read before starting:
- `SourceCode/10_Ggraphic/PMX/PMXMesh.h/.cpp` (the class you're mirroring)
- `SourceCode/10_Ggraphic/X/XActor.h/.cpp` (what `XMesh` wraps)
- `SourceCode/00_Game/10_Object/10_MeshObject/MeshObject.h/.cpp`
- `SourceCode/00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/
  PlayerStateBase.h/.cpp`
- `SourceCode/00_Game/10_Object/10_MeshObject/00_Character/20_Boss/BossStateBase.h/
  .cpp` (mirror `PlayerStateBase`'s shape)
- `SourceCode/99_Utility/Transform/Transform.h` (`Scale` field, `GetMatrix()`)
- `SourceCode/00_Game/00_Scene/10_Main/MainScene.cpp`
- `docs/architecture.md`, `DESIGN.md` for how these pieces fit together generally

## Acceptance Criteria

- All listed files compile with 0 errors, 0 warnings.
- `MainScene`'s `Player` uses `Data/Model/X/Player/player.X`, `Boss` uses
  `Data/Model/X/Boss/boss.X`, both via `XMesh` (not `PMXMesh`).
- Every Player/Boss state listed in the mapping table calls `ApplyNamedClip` with the
  exact new string shown.
- `PMXMesh` still works standalone (don't break `ModelPreviewPanel`/
  `AnimationEditor`/the Hatune preview path — these aren't in scope to modify, but
  verify by reading their usage that your `PMXMesh`/`MeshObject` signature changes
  don't break their call sites; if they call `MeshObject::ApplyAnimationClip` or
  `AttachMesh` directly, they need the same rename applied, in which case update them
  too and note it in `review_points`).
- No other gameplay logic (collision, combat, HP, CombatCoordinator) is touched.

## Build

```powershell
powershell -File scripts\build.ps1
```

Expected result: 0 errors, 0 warnings.

## Test

This project has no automated test suite. Manual in-game verification (does Player/
Boss actually show the new model, do animations play per state) will be done by
Claude/the user after this lands — you don't need to attempt it. What you *can*
verify yourself: the build passes, and a read-through confirms every call site listed
in the mapping table was actually updated (not just added the new infrastructure).

## Notes

- If `XActor`'s constructor throws for either `.X` file for some unexpected reason,
  wrap it in the same `try { ... } catch (const std::runtime_error& Msg) { ... 
  _ASSERT_EXPR(false, ...) }` pattern already used around the `PMXMesh`/`PMXRenderer`
  construction in `MainScene::Create()` — don't silently swap in a different model.
- Don't move any file's location or rename anything not explicitly listed above.
- Keep this change minimal and additive beyond what's specified — no unrelated
  cleanup, no touching Combat/Collision code even if something looks improvable while
  you're in there (report it in `review_points` instead, per `AGENTS.md`'s Stop
  conditions).
