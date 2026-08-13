# Current Task

## Goal

Add a `_DEBUG`-only visual sanity check: when a `Character` (`Player`/`Boss`)'s
displayed model is rendering clearly too large or too small relative to its
collision capsule, show an ImGui warning at draw time.

## Background

`PMXActor`/`XActor` already have (just added, already in the codebase — don't
re-implement these two, they're done):

```cpp
// PMXActor.h (public, already added):
#if _DEBUG
	float GetLocalHeight() const noexcept { return m_LocalHeight; }
#endif
// PMXActor.h (private, already added):
#if _DEBUG
	float m_LocalHeight = 0.0f; // バインドポーズでの高さ(Y方向 max-min. モデルサイズ検知用).
#endif
// PMXActor.cpp constructor, right after `parser.Load(filepath, m_ModelData);` (already added):
#if _DEBUG
		{
			float min_y = FLT_MAX;
			float max_y = -FLT_MAX;
			for (const Model::Vertex& vertex : m_ModelData.Vertices)
			{
				min_y = std::min(min_y, vertex.Position.y);
				max_y = std::max(max_y, vertex.Position.y);
			}
			m_LocalHeight = (max_y > min_y) ? (max_y - min_y) : 0.0f;
		}
#endif
```

`XActor.h/.cpp` have the exact same pattern already added (same method name,
same member name, same computation, right after `parser.LoadSkeletal(FilePath,
m_ModelData, m_Skeleton);` in `XActor`'s constructor).

This task is just plumbing `GetLocalHeight()` up through `IMesh` → `MeshObject` →
`Character`, and adding the actual comparison + ImGui warning in `Character::Draw()`.
Everything in this task must be wrapped in `#if _DEBUG` — this is a debug-only
diagnostic, it must not affect Release builds at all (no cost, no code).

`Character` already owns `m_DamageCollider` (a `CapsuleCollider`, see
`Character.cpp`'s constructor — `Height` is set to `2.0f` for both `Player` and
`Boss`, this is the "expected" gameplay-tuned size to compare against). `Character`
does not currently override `Draw()` — it inherits `MeshObject::Draw()` directly.

## Scope

- `SourceCode/10_Ggraphic/Model/IMesh.h`
- `SourceCode/10_Ggraphic/PMX/PMXMesh.h/.cpp`
- `SourceCode/10_Ggraphic/X/XMesh.h/.cpp`
- `SourceCode/00_Game/10_Object/10_MeshObject/MeshObject.h`
- `SourceCode/00_Game/10_Object/10_MeshObject/00_Character/Character.h/.cpp`

## Out of Scope

- Don't touch `PMXActor.h/.cpp` or `XActor.h/.cpp` — the `GetLocalHeight()`/
  `m_LocalHeight` addition described above is already done in both. Just call the
  existing method, don't re-add it.
- Don't touch `Player`/`Boss`/any State files — no new virtual methods needed on
  them (see the `typeid(*this).name()` approach below, which needs zero changes to
  `Player.h`/`Boss.h`).
- Don't change `m_DamageCollider`'s tuning values (Radius/Height) — it's used only
  as a read-only reference for this comparison, not modified.
- Don't add this check anywhere other than `Character::Draw()` (not `Update()`, not
  a separate always-on debug window).

## Implementation Requirements

### 1. `IMesh.h` — add one pure virtual method

```cpp
#if _DEBUG
	// バインドポーズでのY軸方向の高さ(Scaleを掛ける前. モデルサイズ検知用. Debugビルドのみ).
	virtual float GetLocalHeight() const = 0;
#endif
```
Add this inside the `IMesh` class, after the existing `PlayNamedClip` declaration.

### 2. `PMXMesh.h/.cpp` — override, forward to `PMXActor`

`PMXMesh.h`, inside the class, after `ApplyAnimationClip`'s declaration:
```cpp
#if _DEBUG
	float GetLocalHeight() const override;
#endif
```
`PMXMesh.cpp`:
```cpp
#if _DEBUG
float PMXMesh::GetLocalHeight() const
{
	return m_pActor->GetLocalHeight();
}
#endif
```

### 3. `XMesh.h/.cpp` — override, forward to `XActor`

Same pattern as step 2, but `m_upActor->GetLocalHeight()` (note: `XMesh` owns its
actor via `std::unique_ptr<XActor> m_upActor`, not `shared_ptr` — check the exact
member name in the current `XMesh.h` before writing this, it may differ slightly
from `PMXMesh`'s `m_pActor`).

### 4. `MeshObject.h` — add a forwarding getter

Add near the existing `ApplyAnimationClip`-turned-`PlayNamedClip` declaration (public
section):
```cpp
#if _DEBUG
	// アタッチ中メッシュのバインドポーズ高さ(モデルサイズ検知用. 未アタッチなら0. Debugビルドのみ).
	float GetLocalHeight() const noexcept { return m_pMesh ? m_pMesh->GetLocalHeight() : 0.0f; }
#endif
```
This can be inline in the header (trivial one-liner, matches this file's existing
style for simple getters) — no `.cpp` change needed for this one.

### 5. `Character.h` — declare a `Draw()` override

Add to the `public:` section (near the existing `Update()` override):
```cpp
	// MeshObject::Draw()の後、_DEBUGビルドのみモデルサイズの妥当性をチェックする.
	void Draw() override;
```

### 6. `Character.cpp` — implement `Draw()`

Add near the top of the file (anonymous namespace, alongside any existing
constants — if there isn't one yet, add one):
```cpp
namespace {
	constexpr float SIZE_WARNING_RATIO_MIN = 0.5f; // このY方向は「妥当なサイズ」の下限比率.
	constexpr float SIZE_WARNING_RATIO_MAX = 2.0f; // 上限比率.
}
```
Add the `#include`s needed: `#include "99_Utility/Debug/Imgui/ImGuiManager.h"` and
`#include <typeinfo>` (for `typeid`).

Implement:
```cpp
void Character::Draw()
{
	MeshObject::Draw();

#if _DEBUG
	const float local_height = GetLocalHeight();
	if (local_height <= 0.0f) { return; }

	const float expected_height = m_DamageCollider.GetHeight();
	if (expected_height <= 0.0f) { return; }

	const float world_height = local_height * GetTransform().Scale.y;
	const float ratio = world_height / expected_height;

	if (ratio < SIZE_WARNING_RATIO_MIN || ratio > SIZE_WARNING_RATIO_MAX)
	{
		const std::string window_title = std::string("Model Size Warning: ") + typeid(*this).name();
		ImGui::Begin(window_title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::TextColored(
			ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
			ratio > 1.0f ? IMGUI_JP("大きすぎます！") : IMGUI_JP("小さすぎます！"));
		ImGui::Text("Height: %.2f / Expected: %.2f (x%.2f)", world_height, expected_height, ratio);
		ImGui::End();
	}
#endif
}
```
`typeid(*this).name()` on MSVC returns a readable string like `"class Player"` /
`"class Boss"` with no extra RTTI setup needed beyond what's already enabled in this
project (verify `/GR` isn't disabled in the vcxproj before relying on this — if RTTI
turns out to be disabled project-wide, stop and report it in `review_points` rather
than enabling `/GR` yourself, that's a build-setting change outside this task's
scope). This avoids adding any new virtual method to `Player`/`Boss` just to get a
display name.

`IMGUI_JP(str)` is an existing macro (`ImGuiManager.h`) for embedding Japanese string
literals in `ImGui::` calls — see `DebugHud.cpp`/other files under
`SourceCode/99_Utility/Debug/Imgui/` for existing usage examples.

## Relevant Files

Read before starting:
- `SourceCode/10_Ggraphic/PMX/PMXActor.h` (see the already-added `GetLocalHeight()`/
  `m_LocalHeight` — read this first to confirm the exact signature you're forwarding)
- `SourceCode/10_Ggraphic/X/XActor.h` (same, for the X-format side)
- `SourceCode/10_Ggraphic/Model/IMesh.h`
- `SourceCode/10_Ggraphic/PMX/PMXMesh.h/.cpp`
- `SourceCode/10_Ggraphic/X/XMesh.h/.cpp`
- `SourceCode/00_Game/10_Object/10_MeshObject/MeshObject.h/.cpp`
- `SourceCode/00_Game/10_Object/10_MeshObject/00_Character/Character.h/.cpp`
- `SourceCode/99_Utility/Debug/Imgui/DebugHud.cpp` (pattern reference for
  `ImGui::Begin`/`ImGuiWindowFlags_AlwaysAutoResize`/`IMGUI_JP` usage)

## Acceptance Criteria

- All listed files compile with 0 errors, 0 warnings in Debug|x64.
- Also verify Release|x64 compiles with 0 errors, 0 warnings (this is the actual
  point of this task — confirm the `#if _DEBUG` guards correctly strip all of this
  code out and nothing outside those guards references it unconditionally).
- Launching the Debug build with `Transform::Scale` intentionally set far outside
  [0.5x, 2x] of the collider height for either actor would show the ImGui warning
  window (you don't need to actually test this manually — this project has no
  automated test suite — just make sure the logic is correct by reading it back).
- No changes to `Player.h`/`Boss.h`/any State files/`PMXActor`/`XActor`.

## Build

```powershell
powershell -File scripts\build.ps1
powershell -File scripts\build.ps1 -Configuration Release
```

Expected result: 0 errors, 0 warnings for both.

## Test

No automated test suite. Manual in-game verification (temporarily setting a wrong
scale to confirm the warning appears, then reverting) will be done by Claude/the
user after this lands — you don't need to attempt it, just make sure the build
passes in both configurations.

## Notes

- Keep this minimal — no new files, no unrelated cleanup.
- If `typeid(*this).name()` turns out to need RTTI enabling (`/GR`) that isn't
  already on, stop and report it in `review_points` instead of changing project-wide
  compiler settings yourself.
