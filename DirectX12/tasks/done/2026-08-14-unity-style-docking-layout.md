# Current Task

## Goal

Give the debug ImGui panels a fixed, Unity/Unreal-Editor-style docked layout instead
of freely floating windows whose positions only apply on first-ever launch
(`ImGuiCond_FirstUseEver`) and otherwise drift/overlap as the user drags them.
Add a full-viewport dockspace and a programmatic default docked arrangement for
the existing debug windows, so on a fresh launch (no saved layout yet) everything
snaps into a sensible fixed arrangement around the edges of the screen, leaving the
actual 3D game view visible in the center.

## Background

`ImGuiConfigFlags_DockingEnable` is already set (`ImGuiManager.cpp:60`), and the
vendored ImGui (`Data/Library/ImGui/`, v1.90.6 WIP) includes `imgui_internal.h`,
which is required for the `ImGui::DockBuilder*` functions (they're not in the
public `imgui.h` API). Nothing else in the project currently creates a dockspace —
every window today is a plain floating `ImGui::Begin(...)` call, positioned (if at
all) via `ImGui::SetNextWindowPos(..., ImGuiCond_FirstUseEver)`.

Current fixed-title debug windows (confirmed via repo-wide grep for
`ImGui::Begin(`):
- `"Debug HUD"` — `SourceCode/99_Utility/Debug/Imgui/DebugHud.cpp`
- `"Console"` — `SourceCode/99_Utility/Debug/Imgui/DebugConsole.cpp`
- `"Scene"` — `SourceCode/00_Game/00_Scene/SceneManager.cpp`
- `"Model Select"` — `SourceCode/99_Utility/Debug/Imgui/ModelPreviewPanel.cpp`
- `"Animation Editor"` — `SourceCode/99_Utility/Debug/Imgui/AnimationEditor.cpp`
- `"Actor Scale (Debug)"` — `SourceCode/00_Game/00_Scene/10_Main/MainScene.cpp`

One window has a **dynamic** title (`"Model Size Warning: " + typeid(*this).name()`,
`Character.cpp:71`) and only appears conditionally when a size mismatch is
detected — **exclude this one from the default docked layout** (its title isn't
knowable ahead of time, and it's transient by design, not a permanent panel).

## Scope

- `SourceCode/99_Utility/Debug/Imgui/` — add ONE new file pair, e.g.
  `DebugDockSpace.h/.cpp` (naming your choice, follow this folder's existing
  naming style)
- `SourceCode/00_Game/00_GameLoop/Main.h/.cpp` (wire the new dockspace draw call)
- `DirectX12.vcxproj` / `DirectX12.vcxproj.filters` (register the new files)

## Out of Scope

- Don't turn the actual 3D game view into a dockable ImGui panel/texture (that
  would require rendering the scene to an offscreen render target first — a much
  bigger change). The 3D view stays rendered directly to the swapchain as today;
  the dockspace's central node should be a **passthru** node (transparent,
  undockable-into by the background) so the 3D view remains visible through it.
- Don't change any existing window's own content/logic — only add dockspace
  scaffolding and (once, on first build of a default layout) assign each window to
  a dock node by title. The windows' own `ImGui::Begin(...)` calls don't need to
  change at all — `DockBuilderDockWindow` works purely by matching the window's
  title string from outside that window's own code.
- Don't add a "Reset Layout" button/menu in this task — just get the default
  layout building correctly on first run (no saved docking state yet). A
  reset-layout affordance can be a small follow-up if wanted later.
- Don't dock the dynamic `"Model Size Warning: ..."` window.

## Implementation Requirements

### 1. New `DebugDockSpace` (static class, mirrors `DebugHud`'s shape)

`SourceCode/99_Utility/Debug/Imgui/DebugDockSpace.h`:
```cpp
#pragma once

class DebugDockSpace
{
public:
	static void Draw();
};
```

`DebugDockSpace.cpp` — needs `#include "imgui_internal.h"` (for `DockBuilder*`,
public `imgui.h` alone isn't enough) alongside the usual `ImGuiManager.h` include.
Implement the standard ImGui "fullscreen dockspace host" pattern:

```cpp
void DebugDockSpace::Draw()
{
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGuiWindowFlags host_flags =
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
		ImGuiWindowFlags_NoBackground;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("DockSpaceHost", nullptr, host_flags);
	ImGui::PopStyleVar(3);

	const ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");

	// 初回起動(未保存レイアウト)時のみ、既定の配置を組み立てる.
	if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr)
	{
		ImGui::DockBuilderRemoveNode(dockspace_id);
		ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_PassthruCentralNode);
		ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->WorkSize);

		ImGuiID center = dockspace_id;
		ImGuiID left   = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left,  0.20f, nullptr, &center);
		ImGuiID right  = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.25f, nullptr, &center);
		ImGuiID bottom = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down,  0.25f, nullptr, &center);
		ImGuiID top_left;
		ImGuiID rest_left = ImGui::DockBuilderSplitNode(left, ImGuiDir_Up, 0.30f, &top_left, &left);

		ImGui::DockBuilderDockWindow("Debug HUD", top_left);
		ImGui::DockBuilderDockWindow("Scene", rest_left);
		ImGui::DockBuilderDockWindow("Model Select", rest_left);
		ImGui::DockBuilderDockWindow("Animation Editor", right);
		ImGui::DockBuilderDockWindow("Actor Scale (Debug)", right);
		ImGui::DockBuilderDockWindow("Console", bottom);

		ImGui::DockBuilderFinish(dockspace_id);
	}

	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
	ImGui::End();
}
```
Treat this as a starting layout, not gospel — if something about the exact
`DockBuilderSplitNode` call sequence/argument order doesn't match this vendored
1.90.6 API precisely, fix it to compile correctly and produce a reasonable
left/right/bottom/top-left split; the exact split ratios and grouping aren't
critical, just get a working non-overlapping default arrangement covering all 6
windows listed above.

### 2. Wire into `Main.cpp`

Add `#include "99_Utility/Debug/Imgui/DebugDockSpace.h"`, and call
`DebugDockSpace::Draw();` in `Main::Draw()` **before** `DebugHud::Draw();` (the
dockspace host window must be created before the windows that dock into it are
drawn, so it needs to run first in the frame — check `ImGuiManager::NewFrame()`
already runs earlier in `Main::Loop()`, this only concerns ordering among the
`Draw()`-time ImGui calls).

### 3. Register new files

Add `DebugDockSpace.h`/`DebugDockSpace.cpp` to `DirectX12.vcxproj` and
`DirectX12.vcxproj.filters`, matching the existing `SourceCode\99_Utility\Debug\
Imgui\` filter group.

## Relevant Files

Read before starting:
- `SourceCode/99_Utility/Debug/Imgui/ImGuiManager.h/.cpp` (existing setup,
  `ConfigFlags`, `NewFrame`/`Render` call sites)
- `SourceCode/99_Utility/Debug/Imgui/DebugHud.h/.cpp` (style/shape to mirror)
- `SourceCode/00_Game/00_GameLoop/Main.cpp` (`Draw()` — where to call
  `DebugDockSpace::Draw()`)
- `Data/Library/ImGui/imgui_internal.h` (confirm the exact `DockBuilder*`
  function signatures available in this vendored version before writing calls
  against them)

## Acceptance Criteria

- Debug|x64 and Release|x64 both compile with 0 errors, 0 warnings (aside from the
  known pre-existing unrelated `Vertex.hlsl` Release warning).
- On a fresh launch (delete any existing `imgui.ini`/`imgui.rul` layout file first
  to test the "no saved layout yet" path — locate the actual filename in use by
  checking `ImGuiManager.cpp`/`io.IniFilename`), all 6 listed windows dock into a
  non-overlapping arrangement automatically, with the 3D game view still visible
  in the center.
- The `"Model Size Warning: ..."` dynamic window is NOT part of the forced layout
  (it's fine if it floats freely wherever it happens to appear).
- Once docked, the layout persists across relaunches the same way window positions
  already do today (via the existing ini/rul mechanism) — don't add any new
  persistence code, docking state saving is automatic once
  `ImGuiConfigFlags_DockingEnable` is set and a normal ini file is in use.

## Build

```powershell
powershell -File scripts\build.ps1
powershell -File scripts\build.ps1 -Configuration Release
```

Expected result: 0 errors, 0 warnings for both.

## Test

No automated test suite. Manual verification (launching with no saved layout,
confirming the 6 windows dock into place, confirming the 3D view is still visible)
will be done by Claude/the user after this lands.

## Notes

- Keep this minimal — one new dockspace host, one default-layout build-out, wiring
  into `Main.cpp`. No reset-layout UI, no per-window flag changes, no touching any
  existing window's own draw code.
