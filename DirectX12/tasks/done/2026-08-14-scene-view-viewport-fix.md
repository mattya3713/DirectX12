# Current Task

## Goal

Fix two issues with the Scene View viewport panel:

1. **Correctness bug**: the 3D scene rendered into the offscreen
   `m_pSceneColorBuffer` still uses `m_pViewport`/`m_pScissorRect` — which are
   sized to the REAL WINDOW (recently made dynamic by the window-resize task) —
   instead of a viewport/scissor sized to the OFFSCREEN buffer's own dimensions.
   This causes the rendered 3D content to appear distorted/wrong-aspect whenever
   the offscreen buffer's size (letterboxed to 16:9, from the previous task)
   doesn't match the real window's current size/aspect (which is normal — they're
   independent by design).
2. **Cosmetic**: the letterbox/pillarbox bars around the 16:9 Scene View content
   currently rely on the ImGui window's default background color (effectively
   black/near-black). The user wants them changed to a clearly-visible gray
   instead, so the boundary between "3D content" and "empty bar" is unambiguous.

## Background

Recent history (all this session, in order): a render-to-texture Scene View
panel was added (3D renders to `m_pSceneColorBuffer` instead of the swapchain
backbuffer directly); it was made to resize dynamically to match the panel's
content size; it was then made to letterbox/pillarbox to a fixed 16:9 instead of
stretching to fill the panel; then separately, OS window-resize support was
added (`DirectX12::OnWindowResize`, recreates `m_pViewport`/`m_pScissorRect` to
match the real window's new size).

The bug: `DirectX12::BeginDraw()` — which renders the 3D scene into
`m_pSceneColorBuffer` — calls `m_pCmdList->RSSetViewports(1, m_pViewport.get())`
and `RSSetScissorRects(1, m_pScissorRect.get())`. But `m_pViewport`/
`m_pScissorRect` are the WINDOW-sized ones (correctly used elsewhere, in
`PrepareUIRenderTarget()`, for the real backbuffer/ImGui pass — that usage is
correct and must not change). Rendering into a render target of one size (the
offscreen buffer, e.g. 853x480 for 16:9-fit) using a viewport sized for a
DIFFERENT size (the real window, e.g. 1280x720 or whatever it currently is)
causes the NDC-to-pixel mapping to be wrong for that render target, distorting
the image. `m_SceneColorWidth`/`m_SceneColorHeight` already exist and track the
offscreen buffer's current actual size (from the `RequestSceneColorResize`/
`ResizeSceneColorTarget` work) — a viewport/scissor pair needs to be
created/recreated alongside that buffer, sized to those dimensions, and used in
`BeginDraw()` instead of the window-sized pair.

## Scope

- `SourceCode/10_Ggraphic/DirectX/DirectX12.h/.cpp`
- `SourceCode/99_Utility/Debug/Imgui/SceneView.cpp`

## Out of Scope

- Don't touch `m_pViewport`/`m_pScissorRect` themselves or `OnWindowResize` —
  they're correct for their actual use (the real backbuffer/ImGui pass in
  `PrepareUIRenderTarget()`). This task ADDS a second, separate viewport/scissor
  pair for the offscreen buffer, it doesn't change the existing one.
- Don't change the 16:9 target aspect ratio, the letterbox/pillarbox math, or
  the docking layout — those are correct and already working, this task only
  fixes the viewport-mismatch bug and the bar color.

## Implementation Requirements

### 1. `DirectX12` — add a viewport/scissor pair sized to the offscreen buffer

Add new members (mirror `m_pViewport`/`m_pScissorRect`'s existing types exactly):
```cpp
std::unique_ptr<D3D12_VIEWPORT> m_pSceneColorViewport;
std::unique_ptr<D3D12_RECT>     m_pSceneColorScissorRect;
```

In `ResizeSceneColorTarget(UINT Width, UINT Height)` (which both the initial
`CreateSceneColorTarget` and later resizes already funnel through — confirm this
by reading the current file first), after the color buffer/RTV/SRV are
(re)created, add:
```cpp
m_pSceneColorViewport.reset(new CD3DX12_VIEWPORT(m_pSceneColorBuffer.Get()));
m_pSceneColorScissorRect.reset(new CD3DX12_RECT(0, 0, static_cast<LONG>(Width), static_cast<LONG>(Height)));
```
(Match whatever exact pattern `CreateRenderTarget()` uses for constructing
`m_pViewport`/`m_pScissorRect` from a resource — mirror it precisely rather than
inventing a different construction style.)

In `BeginDraw()`, change:
```cpp
m_pCmdList->RSSetViewports(1, m_pViewport.get());
m_pCmdList->RSSetScissorRects(1, m_pScissorRect.get());
```
to:
```cpp
m_pCmdList->RSSetViewports(1, m_pSceneColorViewport.get());
m_pCmdList->RSSetScissorRects(1, m_pSceneColorScissorRect.get());
```
**Only in `BeginDraw()`** — do NOT change the identical-looking lines in
`PrepareUIRenderTarget()` (those are correct as-is, they're for the real
backbuffer sized to the real window).

### 2. `SceneView.cpp` — gray letterbox/pillarbox bars

In `SceneView::Draw()`, before drawing the centered `ImGui::Image()` call, fill
the full available content region with a clearly-visible gray using the window's
draw list, e.g.:
```cpp
constexpr ImU32 LETTERBOX_COLOR = IM_COL32(60, 60, 60, 255); // はっきり分かる灰色.

const ImVec2 window_pos = ImGui::GetCursorScreenPos();
ImGui::GetWindowDrawList()->AddRectFilled(
	window_pos,
	ImVec2(window_pos.x + available.x, window_pos.y + available.y),
	LETTERBOX_COLOR);
```
Call this BEFORE the cursor-position-offset + `ImGui::Image()` call already
there (so the image draws on top of the gray fill, only the bar areas remain
visible gray). Read the current file first and place this correctly relative to
the existing `available`/`offset`/`ImGui::SetCursorPos` logic — adapt variable
names to whatever's actually there rather than assuming the exact names above.

## Relevant Files

Read before starting:
- `SourceCode/10_Ggraphic/DirectX/DirectX12.h/.cpp` FULLY — `BeginDraw()`,
  `PrepareUIRenderTarget()`, `CreateSceneColorTarget()`,
  `ResizeSceneColorTarget()`, and how `m_pViewport`/`m_pScissorRect` are built in
  `CreateRenderTarget()` (for the construction pattern to mirror).
- `SourceCode/99_Utility/Debug/Imgui/SceneView.cpp` (current letterbox logic from
  the previous task).

## Acceptance Criteria

- Debug|x64 and Release|x64 both compile with 0 errors, 0 warnings (aside from
  the known pre-existing unrelated `Vertex.hlsl` Release warning).
- The 3D content inside Scene View is never distorted/stretched, regardless of
  the real window's size/aspect ratio, as long as it's rendering into the
  correctly-sized offscreen buffer with a matching viewport.
- The letterbox/pillarbox bar areas are visibly gray, not black/theme-default.
- The already-working window-resize (`OnWindowResize`) and Scene View
  dynamic-resize behavior are both unaffected — don't regress either.

## Build

```powershell
powershell -File scripts\build.ps1
powershell -File scripts\build.ps1 -Configuration Release
```

Expected result: 0 errors, 0 warnings for both.

## Test

No automated test suite. Manual verification (checking the 3D content's aspect
ratio stays correct at various window sizes, confirming the bars are gray) will
be done by Claude/the user after this lands — but if you can launch and check
yourself, please do and report exactly what you observed in `test`.

## Notes

- This is a precise, scoped bug fix — don't refactor `BeginDraw()`/
  `PrepareUIRenderTarget()` beyond the one line-pair change specified. If
  anything about which viewport is "correct" for which pass seems ambiguous once
  you've read the actual current code, stop and report it in `review_points`
  rather than guessing.
