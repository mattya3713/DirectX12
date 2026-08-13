# Architecture

> This file is a short orientation map for AI agents (Claude Code / Codex).
> It intentionally does **not** duplicate the project's real, continuously-updated
> design record: **`DESIGN.md`** (repo root) is the authoritative, living architecture
> and decision log — every non-trivial implementation phase this project has gone
> through is documented there with rationale, and it is kept up to date as code changes.
> **Read `DESIGN.md` before making structural decisions.** This file just tells you
> where things live and how the major pieces fit together, so `DESIGN.md` is easier
> to navigate.

## What this project is

A personal C++ / DirectX12 learning project (see `README.md`). It ports/extends an
earlier game called **Senzan** (`C:\Users\green\source\C++\Senzan\Senzan`, a separate
repo) — reimplementing its systems from scratch rather than copying code, deliberately
redesigning parts that were rushed in the original (see `DESIGN.md` for specific
before/after comparisons, e.g. `CombatCoordinator`, the `Enemy`/`Boss` split).

## Entry point / lifecycle

- `SourceCode/00_Game/00_GameLoop/WinMain.cpp` → `Main` class (`Main.h/.cpp` in the
  same folder).
- `Main::Create()` constructs the engine-level singletons/managers in a fixed order
  (`GameTime`, `MeshManager`, input devices, `DirectX12`, `ImGuiManager`,
  `CameraManager`, `SoundManager`, `CollisionDetector`, `CombatCoordinator`,
  `SceneManager`) and registers each with `ServiceLocator` (see below).
- `Main::Loop()` runs the Win32 message pump; each idle tick calls
  `GameTime::Update()` → `Input::Update()` → `Main::Update()` → `Main::Draw()`.
- `Main::Release()` tears everything down in reverse-ish order, unregistering from
  `ServiceLocator` before destroying.

## Core patterns used throughout

- **ServiceLocator** (`SourceCode/99_Utility/ServiceLocator/`) — type-keyed registry
  of non-owning pointers to engine managers (`ServiceLocator::Provide<T>(ptr)` /
  `ServiceLocator::Get<T>()`). Used instead of classic Singletons everywhere; managers
  are owned by `Main` (or by a `Scene`, for scene-scoped things) and just *registered*
  here so other code can reach them without threading references through every
  constructor.
- **StateMachine\<T\> / StateBase\<T\>** (`SourceCode/99_Utility/StateMachine/`) — a
  generic finite-state-machine template. Each owner type (`Player`, `Enemy`, `Boss`)
  gets its own state enum + a `*StateBase : public StateBase<T>` adding
  owner-specific helpers, then concrete states under a numbered `State/` subfolder.
  Note: `Boss` inherits from `Enemy` but owns a **separate**
  `StateMachine<Boss>` — `Enemy::m_StateMachine` is private and cannot be reused by a
  subclass, so `Boss` does not actually drive Enemy's own state machine (see
  `DESIGN.md`'s Boss section for the full reasoning).
- **Passkey (Attorney-Client) pattern** — `*AccessKeys.h` files (e.g.
  `CharacterAccessKeys.h`, `PlayerAccessKeys.h`) define small key classes with a
  private constructor, `friend`-ed only to the specific classes allowed to call a
  given setter. Used to gate mutators like `SetMoveVec`, `AddCombo`,
  `SetParryReactionTarget` without making them fully public.
- **Collision system** — `ColliderBase` (Visitor/double-dispatch;
  `SourceCode/00_Game/40_Collision/00_Core/`) with `BoxCollider` / `CapsuleCollider` /
  `SphereCollider` concrete shapes, and `eCollisionGroup` bitflags for who-hits-whom.
  `CollisionDetector` (ServiceLocator-registered) does brute-force O(n²) pairwise
  checks once per frame, invoked from `Main::Update()` **after** `SceneManager::Update()`
  so all Transforms are settled first. `Character::ProcessHits()` polls its own damage
  collider each `Update()` and turns hits into `HitEvent`s automatically; anything
  needing custom per-hit logic (e.g. parry detection) polls its own dedicated collider
  directly instead of relying on that generic path.

## Object hierarchy

```
GameObject          (Transform ownership; Update/Draw virtual hooks)
 └─ MeshObject       (adds mesh rendering)
     └─ Character    (adds HealthSystem, damage/attack CapsuleColliders, ProcessHits())
         ├─ Player
         └─ Enemy
             └─ Boss
```

All under `SourceCode/00_Game/10_Object/`. `Character` is the base every playable/AI
actor derives from; `Enemy` is a from-scratch minimal-AI class (Senzan had no such
class — `Boss` there inherited `Character` directly). See `DESIGN.md` for exactly
what was/wasn't ported from Senzan and why.

## Scenes

- `SourceCode/00_Game/00_Scene/00_Base/SceneBase.h` (pure virtual
  `Initialize/Create/Update/LateUpdate/Draw`) + `SceneManager.h/.cpp` (deferred
  scene-switching — `LoadScene()` only reserves the switch; the actual swap happens at
  the top of the *next* `SceneManager::Update()`, so a scene can never destroy itself
  mid-`Update()`).
- `SourceCode/00_Game/00_Scene/10_Main/MainScene.*` — the main gameplay scene.
- `SourceCode/00_Game/00_Scene/Ex_Test/AnimationTuning/AnimationTuningScene.*` —
  `_DEBUG`-only scene for animation tuning, reachable via F1 from `MainScene`.

  **Unknown / Needs investigation**: as of the last check, `MainScene` had its
  `Player`/`Enemy` member pointers declared but not constructed anywhere (an
  in-progress reorganization by the project owner). Confirm current state before
  assuming actors are wired into the scene.

## Rendering

- `SourceCode/10_Ggraphic/` — `DirectX12` (device/swapchain/command-list setup,
  frame-in-flight pipelining), `PMXRenderer`/`PMXActor`/`PMXMesh` (MikuMikuDance model
  pipeline, the primary model format in use), `PMDActor`/`PMDRenderer` (older format),
  `XActor`/`XParser` (DirectX legacy `.x` format support).
- Shaders: Debug config compiles `.hlsl` at runtime (`D3DCompileFromFile`, fast
  iteration); Release precompiles to `.cso` via `fxc.exe` in a `PostBuildEvent` and
  loads via `D3DReadFileToBlob` (no shader source ships in Release builds). See
  `DESIGN.md`'s shader-distribution section.

## Cameras

`SourceCode/00_Game/30_Camera/` — `CameraBase` (abstract: position/look/yaw/pitch/FOV,
`Shake()`) with concrete `ThirdPersonCamera` / `LookAtCamera` / `DebugCamera` /
`FirstPersonCamera` / `KeyframeCamera` (scripted one-shot camera sequences, added for
combat cutscenes). `CameraManager` is a name-keyed registry with one active camera at
a time; `PlayOneShot(...)` temporarily switches to a `KeyframeCamera` and restores the
previous camera automatically when it finishes.

## Combat mediation

`SourceCode/00_Game/60_Combat/CombatCoordinator.h/.cpp` — mediates interactions that
need to read/write both `Player` and `Boss` at once (currently: parry reaction
positioning). Deliberately redesigned from Senzan's original (see `DESIGN.md`):
it only *computes* choreography data and *triggers* state transitions; each actor's
own state is what actually writes to its Transform, so there is never more than one
writer touching a given Transform in the same frame.

`CombatCoordinator` receives `PlayerCombatView` / `BossCombatView` values rather
than concrete actor pointers. Each view wraps a non-owning actor pointer and exposes
only position queries and the matching parry-reaction entry point, without adding
interface inheritance to either actor. Ownership remains exclusively with
`MainScene`, which clears the views before destroying its actors.

## Debug tooling (`_DEBUG` only)

`SourceCode/99_Utility/Debug/Imgui/` — vendored Dear ImGui
(`ImGuiManager`), plus `DebugHud` (FPS/camera info), `AnimationEditor`,
`ModelPreviewPanel` (Unity-Editor-style always-available model preview, independent
of the active scene).

## Build system

- MSBuild (`DirectX12.vcxproj`), 4 configs: Debug/Release × Win32/x64 (x64 is the one
  actually used/verified this project cycle).
- `Directory.Build.targets` copies `Data\Shader`/`Data\Model`/`Data\Sound`/`Data\Font`
  into the output directory post-build (`xcopy /D`, skips unchanged files).
- `Data/Library/DirectXTex` is a vendored git submodule.
- **No automated test suite exists** (no test project, no test folder, confirmed by
  search). Verification this project relies on is: build with zero warnings, then
  manual in-app testing (see `docs/coding-rules.md` / `AGENTS.md` for what "Build" and
  "Test" mean here in practice).

## Project records worth knowing about

- `DESIGN.md` (root) — the living architecture/decision log. **Update this after any
  non-trivial implementation phase**, matching its existing style (what was built,
  what was deliberately deferred, and why).
- `devlog/YYYY-MM-DD.md` — write-ups of nontrivial bug investigations (root-cause
  narratives), not a changelog. Only add an entry for something that took real
  investigation to track down.
- `README.md` (root) — project blurb + a summary of the external C++/DirectX coding
  standard. See `docs/coding-rules.md` for the AI-agent-facing digest of this.
