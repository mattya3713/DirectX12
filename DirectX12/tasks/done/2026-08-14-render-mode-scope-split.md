# Current Task

## Goal

Split rendering into two modes: **MainScene reverts to fully direct rendering**
(3D drawn straight to the swapchain backbuffer, exactly as it worked before the
Scene View feature existed this session — no offscreen texture, no DockSpace, no
Scene View panel), while **AnimationTuningScene keeps the current Editor-style
treatment** (DockSpace, offscreen Scene View panel, grid floor, etc. — all
unchanged). The Unity-Editor-like UI is meant only for the animation-tuning
tool, not the actual game.

## Background

Currently `Main::Draw()` calls `DebugDockSpace::Draw()`/`SceneView::Draw()`/
`DirectX12::PrepareUIRenderTarget()` **unconditionally**, regardless of which
scene (`MainScene` or `AnimationTuningScene`) is active — so the game itself
currently goes through the offscreen-render-to-texture-then-composite-via-ImGui
path too, which isn't what's wanted. `SceneManager` already tracks the active
scene internally (`m_CurrentSceneID`, `_DEBUG`-only) but has no public accessor
for it yet.

`SceneManager::eList::AnimationTuning` only exists in `_DEBUG` builds
(`#if _DEBUG` around the enum value) — so in Release builds, only `MainScene`
ever exists, meaning the "editor mode" branch described below never needs to run
there at all.

## Scope

- `SourceCode/00_Game/00_Scene/SceneManager.h/.cpp`
- `SourceCode/10_Ggraphic/DirectX/DirectX12.h/.cpp`
- `SourceCode/00_Game/00_GameLoop/Main.cpp`

## Out of Scope

- Don't touch `DebugDockSpace`, `SceneView`, `DebugGrid`,
  `AnimationTuningScene`, or `MainScene` themselves — their own internal logic
  is unchanged; this task only changes WHETHER `Main::Draw()` invokes the
  editor-mode rendering path, based on which scene is active.
- Don't remove or restructure `RequestSceneColorResize`/
  `ResizeSceneColorTarget`/the offscreen scene-color buffer machinery — it's
  still needed for `AnimationTuningScene`, just skipped when `MainScene` is
  active.
- Don't add a DebugCamera rotation/mouse-look feature — that's a separate,
  later task if wanted.

## Implementation Requirements

### 1. `SceneManager` — expose the active scene type

Add a new public, `_DEBUG`-only method to `SceneManager.h` (near the existing
`#if _DEBUG` block around `m_CurrentSceneID`):
```cpp
#if _DEBUG
	// 現在のシーンがAnimationTuningTuningかどうか(Main::Draw()の描画方式切り替え用).
	bool IsAnimationTuningActive() const noexcept { return m_CurrentSceneID == eList::AnimationTuning; }
#endif
```
(No `.cpp` change needed if implemented inline in the header, matching this
file's style for other simple accessors — check whether `SceneManager.cpp`
already has a `.cpp`-side pattern worth following instead, and match it if so.)

### 2. `DirectX12::BeginDraw()` — branch on a new parameter

Change the signature (update the declaration in `DirectX12.h` too):
```cpp
// UseOffscreenScene: trueならオフスクリーンのシーンカラーバッファへ描く(AnimationTuningScene用.
// Scene Viewパネルがそれを表示する). falseなら実際のバックバッファへ直接描く(MainScene用、
// Scene View機能が存在する前の元の描画方式).
void BeginDraw(bool UseOffscreenScene);
```

Read the CURRENT `BeginDraw()` body first (it currently unconditionally targets
the offscreen scene color buffer — this is the code that becomes the
`UseOffscreenScene == true` branch, keep it byte-for-byte identical, just nested
under an `if`). Add a new `else` branch for `UseOffscreenScene == false` that
restores the ORIGINAL direct-to-backbuffer behavior (this is what `BeginDraw()`
looked like before this session's Scene View work — reconstruct it precisely
per this exact spec, don't improvise):

```cpp
else
{
	// 実際のバックバッファへ直接描く(MainScene用).
	auto Barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_pBackBuffer[m_FrameIndex].Get(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_pCmdList->ResourceBarrier(1, &Barrier);

	auto rtvH = m_pRenderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();
	rtvH.ptr += m_FrameIndex * m_pDevice12->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	auto DSVHeapPointer = m_pDepthHeap->GetCPUDescriptorHandleForHeapStart();
	m_pCmdList->OMSetRenderTargets(1, &rtvH, false, &DSVHeapPointer);
	m_pCmdList->ClearDepthStencilView(DSVHeapPointer, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	float ClearColor[] = { 0.f,0.f,0.f,1.0f };
	m_pCmdList->ClearRenderTargetView(rtvH, ClearColor, 0, nullptr);

	m_pCmdList->RSSetViewports(1, m_pViewport.get());
	m_pCmdList->RSSetScissorRects(1, m_pScissorRect.get());
}
```

The fence-wait / `m_FrameIndex` assignment / command-allocator-reset /
pending-scene-color-resize-check logic at the TOP of `BeginDraw()` (before the
offscreen-vs-backbuffer branching) stays common to both branches, unchanged —
only the render-target-selection portion (the part that currently unconditionally
targets `m_pSceneColorBuffer`) needs the `if/else` split described above.

**Do not change `EndDraw()` or `PrepareUIRenderTarget()`** — `EndDraw()`'s
existing "transition backbuffer RENDER_TARGET→PRESENT, close/execute/present"
logic is already correct for BOTH modes (in `UseOffscreenScene == false` mode,
the backbuffer is already `RENDER_TARGET` straight out of `BeginDraw()`'s new
`else` branch; in `true` mode, it becomes `RENDER_TARGET` via
`PrepareUIRenderTarget()` as before) — verify this reasoning holds once you've
read the actual current code, and report in `review_points` if it doesn't.

### 3. `Main.cpp` — branch `Draw()` by active scene

In `Main::Draw()`, compute once near the top (after the existing `if
(!m_pDx12) return;` guard):
```cpp
#if _DEBUG
	const bool is_editor_scene = m_upSceneManager && m_upSceneManager->IsAnimationTuningActive();
#else
	constexpr bool is_editor_scene = false;
#endif
```

Then:
- `m_pDx12->BeginDraw(is_editor_scene);`
- Wrap the existing `DebugDockSpace::Draw();` call in `if (is_editor_scene) { ... }`.
- Keep `DebugHud::Draw();` and `DebugConsole::Draw();` unconditional (these
  predate the Scene View work and should keep showing in both scenes, as simple
  floating windows when not docked — i.e. when `is_editor_scene` is false, they
  just render as normal floating ImGui windows over the game, same as they did
  before DockSpace/SceneView existed).
- Wrap the existing `SceneView::Draw();` call in `if (is_editor_scene) { ... }`.
- Keep `if (m_upSceneManager) { m_upSceneManager->Draw(); }` unconditional (both
  scenes still need their own `Draw()` called — `MainScene`'s just goes straight
  to the now-correctly-selected render target from step 2).
- Wrap the existing `m_pDx12->PrepareUIRenderTarget();` call in `if
  (is_editor_scene) { ... }` (in `MainScene`/direct mode, this step is skipped
  entirely — the backbuffer is already correctly bound from `BeginDraw(false)`,
  and `DebugHud`/`DebugConsole`'s ImGui draw commands will render on top of the
  already-drawn 3D scene in the SAME pass, exactly as this project's rendering
  worked before Scene View existed).
- Keep `ImGuiManager::Render();` and the final `m_pDx12->EndDraw();` unconditional.

## Relevant Files

Read before starting:
- `SourceCode/00_Game/00_Scene/SceneManager.h/.cpp` FULLY
- `SourceCode/10_Ggraphic/DirectX/DirectX12.h/.cpp` FULLY — `BeginDraw()`,
  `EndDraw()`, `PrepareUIRenderTarget()` (this is core rendering code, read
  carefully before changing)
- `SourceCode/00_Game/00_GameLoop/Main.cpp` (`Draw()`, current full sequence)

## Acceptance Criteria

- Debug|x64 and Release|x64 both compile with 0 errors, 0 warnings (aside from
  the known pre-existing unrelated `Vertex.hlsl` Release warning).
- Launching directly into `MainScene` (the default) shows the 3D game rendered
  directly full-window, with `Debug HUD`/`Console` as simple floating windows
  (not docked, no Scene View panel, no gray letterbox bars, no grid floor).
- Pressing F1 to switch to `AnimationTuningScene` shows the full Editor UI
  exactly as before this task (DockSpace layout, Scene View panel with
  letterboxing and grid floor).
- Pressing F1 again to return to `MainScene` correctly switches back to direct
  rendering (no leftover DockSpace/Scene View artifacts).

## Build

```powershell
powershell -File scripts\build.ps1
powershell -File scripts\build.ps1 -Configuration Release
```

Expected result: 0 errors, 0 warnings for both.

## Test

No automated test suite. Manual verification (launching, confirming MainScene
renders directly with no editor chrome, pressing F1 and confirming
AnimationTuningScene still shows the full Editor UI, pressing F1 again and
confirming MainScene returns to direct rendering) will be done by Claude/the
user after this lands — but if you can launch and check yourself, please do and
report exactly what you observed in `test`.

## Notes

- This is a scope/routing correction, not new rendering logic — the two render
  paths (offscreen-composite vs direct) already both exist in the codebase in
  some form; this task's job is choosing between them correctly based on active
  scene, not inventing new rendering behavior.
- If anything about the `BeginDraw()`/`EndDraw()`/`PrepareUIRenderTarget()`
  resource-state assumptions doesn't hold once you've read the actual current
  code, stop and report it in `review_points` rather than guessing — this is
  core D3D12 resource-lifetime code.
