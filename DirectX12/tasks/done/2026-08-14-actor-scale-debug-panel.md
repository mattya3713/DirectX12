# Current Task

## Goal

Add a `_DEBUG`-only ImGui panel that lets the user live-tune `Player`'s and `Boss`'s
uniform `Transform::Scale` at runtime, so the correct scale for the new `.X` models
can be found interactively (watching the existing model-size warning added in
`Character::Draw()` react live as the scale changes) instead of guessing and
rebuilding repeatedly.

## Background

`MainScene::Create()` currently hard-codes `Transform::Scale = {15.0f, 15.0f, 15.0f}`
for both `Player` and `Boss` (see `MainScene.cpp`). This value turned out to be
carried over from an unrelated earlier test and is likely wrong for the current `.X`
models (confirmed: at scale 15, Boss's displayed height came out to ~14x the
`Character::m_DamageCollider`'s reference height — see the just-added debug-only
size-warning feature in `Character::Draw()`). Rather than guess-and-rebuild, the user
wants a live slider/input to find the right value visually while the game is running.

`GameObject` (base of `Player`/`Boss`) exposes `GetTransform() const` and
`SetTransform(const Transform&)` — there's no in-place mutable scale setter, so
tuning means: read the current `Transform`, change `Scale`, write it back via
`SetTransform`.

## Scope

- `SourceCode/00_Game/00_Scene/10_Main/MainScene.cpp` only.

## Out of Scope

- Don't change the actual default scale value in `MainScene::Create()` — leave it at
  `15.0f` for now; this task only adds the ability to override it live at runtime.
  Once the user finds the right value through this panel, changing the hard-coded
  default is a separate, later step.
- Don't touch `GameObject`, `Transform`, `Character`, `Player`, `Boss`, or the
  size-warning code added in the previous task — this is purely a new debug UI block
  inside `MainScene`.
- Don't persist the tuned value anywhere (no file save/load) — this is a live,
  in-memory-only debug control, matches how e.g. `AnimationEditor`'s live tweaks work
  before an explicit Save.

## Implementation Requirements

Add a `#if _DEBUG` block inside `MainScene::Update()`, near the existing `#if _DEBUG`
F1-key block at the top of the function (see current code — keep this as a second,
separate `#if _DEBUG ... #endif` block right after it, don't merge them). Content:

```cpp
#if _DEBUG
	// 実行中にPlayer/BossのScaleを調整できるデバッグパネル(モデルサイズ調整用).
	ImGui::Begin("Actor Scale (Debug)", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

	if (m_upPlayer) {
		Transform transform = m_upPlayer->GetTransform();
		float scale = transform.Scale.x;
		ImGuiManager::Input("Player Scale", scale, true, 0.1f, 1.0f);
		if (scale > 0.0f) {
			transform.Scale = { scale, scale, scale };
			m_upPlayer->SetTransform(transform);
		}
	}

	if (m_upBoss) {
		Transform transform = m_upBoss->GetTransform();
		float scale = transform.Scale.x;
		ImGuiManager::Input("Boss Scale", scale, true, 0.1f, 1.0f);
		if (scale > 0.0f) {
			transform.Scale = { scale, scale, scale };
			m_upBoss->SetTransform(transform);
		}
	}

	ImGui::End();
#endif
```

Check `ImGuiManager::Input<T>`'s exact signature in `SourceCode/99_Utility/Debug/
Imgui/ImGuiManager.h` before writing this — the template signature is
`Input(const char* Label, T& Value, bool IsLabel = true, float Step = 0.0f, float
StepFast = 0.0f, const char* Format = "%.3f")`, confirm the argument order/defaults
match what's shown above and adjust if the actual header differs. Add
`#include "99_Utility/Debug/Imgui/ImGuiManager.h"` to `MainScene.cpp` if it isn't
already included (check first — it may already be there from other work this
session).

## Relevant Files

Read before starting:
- `SourceCode/00_Game/00_Scene/10_Main/MainScene.cpp` (what you're editing — read
  the existing `#if _DEBUG` F1-key block near the top of `Update()` for placement
  and style reference)
- `SourceCode/99_Utility/Debug/Imgui/ImGuiManager.h` (confirm `Input<T>`'s exact
  signature)
- `SourceCode/99_Utility/Transform/Transform.h` (`Scale` field)
- `SourceCode/99_Utility/Debug/Imgui/DebugHud.cpp` (style reference for
  `ImGui::Begin`/`ImGuiWindowFlags_AlwaysAutoResize`/`ImGui::End` usage)

## Acceptance Criteria

- `MainScene.cpp` compiles with 0 errors, 0 warnings in Debug|x64.
- The panel only exists in Debug builds (wrapped in `#if _DEBUG`) — confirm Release|
  x64 still compiles with 0 errors, 0 warnings and contains none of this code.
- Player and Boss's `Scale` can each be independently adjusted at runtime through
  the panel, applied uniformly to X/Y/Z.

## Build

```powershell
powershell -File scripts\build.ps1
powershell -File scripts\build.ps1 -Configuration Release
```

Expected result: 0 errors, 0 warnings for both.

## Test

No automated test suite. Manual verification (running the game and dragging/typing
into the panel to confirm the model visibly resizes) will be done by the user after
this lands — you don't need to attempt it, just make sure both builds pass.

## Notes

- Keep this minimal — just the one ImGui block, no new files, no unrelated cleanup.
