# Current Task

## Goal

Wire an actual `Player` and `Boss` into `MainScene` (both currently declared-but-never
constructed / not declared at all), so the Boss/parry systems built this session can
finally be run and observed in-game instead of only compile-checked.

## Background

Over several sessions this project built out, in order: a minimal `Boss` (state
machine: Idle/Move/Attack/ParryReaction/Dead), a `CombatCoordinator` that mediates
Player↔Boss interactions (computes reposition data on a successful parry, doesn't
touch Transforms directly), and the actual parry-detection collision wiring
(`eCollisionGroup::PlayerParry`, `Player`'s dedicated parry collider,
`PlayerState::Parry` calling `CombatCoordinator::OnParrySuccess()`). All of it builds
cleanly (0 warnings) but **none of it has ever been run** — `MainScene` currently only
sets up a `DebugCamera` and a `PMXRenderer`; it declares a `std::unique_ptr<Player>`
and `std::unique_ptr<Enemy>` member but never constructs either, so today the scene
renders nothing and both actors are permanently null.

This task is just the wiring — making the actors exist, visible, and updating. It is
*not* about tuning the fight to feel good, adding more Boss attack patterns, or fixing
anything about the parry logic itself (that logic is out of scope here — if you find a
bug in it while wiring this up, report it in `review_points`, don't fix it as part of
this task).

## Scope

- `SourceCode/00_Game/00_Scene/10_Main/MainScene.h`
- `SourceCode/00_Game/00_Scene/10_Main/MainScene.cpp`

That's it — this task is contained to these two files.

## Out of Scope

- `PlayerState::*`, `BossState::*`, `CombatCoordinator`, `Enemy` — don't modify any of
  these. If wiring reveals a real bug in one of them, stop and report it in
  `review_points` rather than fixing it (see `AGENTS.md`'s Stop conditions — this
  would be outside this task's scope).
- Don't add new gameplay features (camera cutscene triggering, HP display, win/lose
  conditions, etc.). Just get both actors existing, visible, and updating.
- Don't touch animation clip data/JSON files. If `ApplyNamedClip(...)` calls inside
  `PlayerState`/`BossState` fail to find a named clip, that's expected/pre-existing —
  not something to fix here.
- Don't rename or remove the existing `m_upEnemy` concept more than described below —
  specifically, don't also try to keep a *third*, separate plain `Enemy` instance
  around "just in case." `Boss` already *is* an `Enemy` (it inherits from it); this
  task replaces the `Enemy` slot with a `Boss` slot, it doesn't add a new one.

## Implementation Requirements

1. **`MainScene.h`**: replace the `class Enemy;` forward declaration and
   `std::unique_ptr<Enemy> m_upEnemy;` member with `class Boss;` and
   `std::unique_ptr<Boss> m_upBoss;` (`Boss.h` lives at
   `00_Game/10_Object/10_MeshObject/00_Character/20_Boss/Boss.h`).

2. **`MainScene::Create()`**: after the existing `PMXRenderer` construction succeeds,
   construct both actors:
   - `m_upPlayer = std::make_unique<Player>();`
   - Attach a mesh to it via `AttachMesh(std::make_shared<PMXMesh>(FilePath, *m_pPMXRenderer))`
     (see `MeshObject::AttachMesh` — `Player` inherits this through `Character`). Use
     model path `"Data/Model/PMX/Hatune/REM式プロセカ風初音ミクN25.pmx"` (this is the
     only fully-rigged PMX model in the project currently; there's no dedicated
     Boss/Player-specific model, so reuse it as two independent instances — this
     matches how `Enemy` was previously stood up in this project, see `DESIGN.md`).
   - Give `Player` a starting position via `SetPosition(...)`, e.g. the origin.
   - Construct `m_upBoss = std::make_unique<Boss>();`, attach its own **separate**
     `PMXMesh` instance (same file path, but a distinct `shared_ptr` — each actor
     needs independent bone/animation state, don't share one `PMXMesh` between them).
   - Position `Boss` a few units away from `Player` along one axis (e.g. Z), far
     enough to be within `Boss`'s `AggroRange` (15 units) but outside its
     `AttackRange` (3.5 units) — so on launch you can actually see `Boss` walk toward
     `Player` (Idle→Move→Attack) rather than starting already in Attack. Something
     like 8 units apart is reasonable; exact number isn't critical.
   - Wire `CombatCoordinator`: `ServiceLocator::Get<CombatCoordinator>()`, and if
     non-null, call `Initialize(m_upPlayer.get(), m_upBoss.get())`.
   - Wrap PMXMesh construction in the same `try { ... } catch (const std::runtime_error& Msg) { ... _ASSERT_EXPR(false, ...) ...}`
     pattern already used for the `PMXRenderer` construction just above it in this
     function — model loading can throw, and this project's convention is to convert
     that to an assert with the message rather than let it propagate.

3. **`MainScene::Update()`**: replace the existing
   `if (m_upEnemy && m_upPlayer) { m_upEnemy->SetTargetPos(m_upPlayer->GetPosition()); }`
   block with the `Boss` equivalent (`Boss::SetTargetPos` is inherited from `Enemy`).
   Replace the `if (m_upEnemy) { m_upEnemy->Update(); }` call with
   `if (m_upBoss) { m_upBoss->Update(); }`. Keep the existing
   `if (m_upPlayer) { m_upPlayer->Update(); }` call as-is, just make sure it still
   runs (it already does, nothing to change there beyond what's listed).

4. **`MainScene::Draw()`**: this currently only binds the PMX pipeline
   state/root-signature/topology — it never actually calls `Draw()` on either actor,
   which is why nothing has ever rendered even when actors exist.
   `MeshObject::Draw()` (inherited by both `Player` and `Boss` through `Character`)
   is what actually issues the draw call for the attached mesh, and nothing invokes
   it automatically. Add `if (m_upPlayer) { m_upPlayer->Draw(); }` and
   `if (m_upBoss) { m_upBoss->Draw(); }` after the existing pipeline-binding code.

## Relevant Files

Read before starting:
- `SourceCode/00_Game/00_Scene/10_Main/MainScene.h` / `.cpp` (what you're editing)
- `SourceCode/00_Game/10_Object/10_MeshObject/00_Character/20_Boss/Boss.h` (`Boss`'s
  public interface — note it inherits `SetTargetPos`/`GetPosition`/etc. from `Enemy`)
- `SourceCode/00_Game/10_Object/10_MeshObject/00_Character/00_Player/Player.h`
- `SourceCode/00_Game/10_Object/10_MeshObject/MeshObject.h` (`AttachMesh`, `Draw`)
- `SourceCode/10_Ggraphic/PMX/PMXMesh.h` (constructor signature)
- `SourceCode/00_Game/60_Combat/CombatCoordinator.h` (`Initialize`)
- `docs/architecture.md` for how these pieces fit together generally

## Acceptance Criteria

- `MainScene.h`/`.cpp` compile with 0 errors, 0 warnings.
- `m_upPlayer` and `m_upBoss` are both non-null after `Create()` runs successfully
  (i.e. actually constructed, not just declared).
- `CombatCoordinator::Initialize` is called with both pointers once both actors exist.
- Both actors' `Update()` and `Draw()` are called every frame `MainScene::Update()` /
  `MainScene::Draw()` run.
- No other file is modified.

## Build

```powershell
powershell -File scripts\build.ps1
```

Expected result: 0 errors, 0 warnings.

## Test

This project has no automated test suite, and you likely can't visually confirm
rendering/gameplay behavior from where you're running. Manual verification (walking
up to `Boss`, triggering a parry, watching the camera/positions react) will be done
by Claude/the user after this lands — you don't need to attempt it. What you *can*
verify yourself: the build passes, and a quick read-through confirms the construction
order in `Create()` is valid (e.g. `PMXRenderer` exists before you try to use it to
build a `PMXMesh`).

## Notes

- `Enemy`'s (and therefore `Boss`'s) AI tuning constants
  (`MoveSpeed`/`AggroRange`/`AttackRange`/`LoseRange`) are set in `Boss`'s own
  constructor already — nothing to configure here, just get the object constructed.
- If `PMXMesh` construction throws for the Hatune file for some unexpected reason,
  that's a real problem worth reporting clearly in `review_points` — don't silently
  swap in a different, untested model file to work around it.
- Don't move `MainScene.cpp`/`.h` to a different location or touch anything about
  their file paths — that's unrelated to this task and this area of the project has
  had path-reorganization churn recently; keep this change minimal and additive.
