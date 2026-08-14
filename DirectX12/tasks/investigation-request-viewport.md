# Investigation Request (NOT an implementation task)

Pure research/fact-gathering. **Do not write, edit, or delete any code.** Output
only one new file: `tasks/investigation-report-viewport.md`. Do not modify any
other file.

## Context

We want to turn the 3D game view into a resizable, dockable ImGui panel (like
Unity's Scene view or Unreal's Viewport panel), instead of rendering directly to
the swapchain backbuffer at a fixed full-window size. This requires rendering the
3D scene to an offscreen texture (a render target the app controls the size of),
then displaying that texture inside an `ImGui::Image()` call within a dockable
panel, resizing the texture to match the panel's content size as the user resizes
docked panels. I need a precise, code-grounded understanding of the current D3D12
rendering setup before designing this, since getting D3D12 resource
creation/resizing/state-transitions wrong causes crashes or validation-layer
errors — precision matters here more than almost anywhere else in this project.

## What to investigate — read every file listed FULLY before reporting

1. `SourceCode/10_Ggraphic/DirectX/DirectX12.h` and `.cpp` FULLY. Specifically
   report:
   - The full swapchain setup (`FrameBufferCount`, `IDXGISwapChain` creation,
     format).
   - Every descriptor heap the class owns (RTV heap, DSV heap if any, CBV/SRV/UAV
     heap) — exact member names, exact `D3D12_DESCRIPTOR_HEAP_DESC` used for each,
     how descriptor handles are indexed/allocated today (is there a simple
     bump-allocator pattern, or fixed-size arrays with known indices?).
   - `BeginDraw()`/`EndDraw()` FULLY — exact resource barrier transitions used
     around the backbuffer (`PRESENT` <-> `RENDER_TARGET` states), exactly what
     `OMSetRenderTargets` is called with, exactly what gets cleared and to what
     color, exactly how/where the depth buffer (if any) is bound.
   - The frame-in-flight / fence pattern (`WaitForGPU()`, per-frame command
     allocators — already partially known from `DESIGN.md`'s "フレームインフラ
     イト" note, confirm exact current implementation).
   - Whether there's ANY existing precedent in this codebase for rendering to an
     offscreen texture (grep for `D3D12_RESOURCE_STATE_RENDER_TARGET`,
     `CreateCommittedResource` with a render-target-flagged
     `D3D12_RESOURCE_DESC`, or any class name suggesting an offscreen/render-
     texture already exists). Report explicitly if none exists.
   - Whether/how window resize (actual OS window resize, e.g. via `WM_SIZE`) is
     currently handled — does the swapchain get resized? This matters because the
     new offscreen texture will need equivalent resize handling, driven by ImGui
     panel size instead of OS window size.

2. `SourceCode/00_Game/30_Camera/00_Base/CameraBase.h` and `.cpp` — exactly how
   the projection matrix's aspect ratio is computed (is it read from the window/
   swapchain size each frame, or set once, or passed in externally?). Also check
   `DirectX12::SetCamera(View, Proj, Eye)` (the method `MainScene::Update()` calls
   each frame per earlier work this session) — where does the aspect ratio
   actually get baked into the projection matrix, `CameraBase::UpdateViewProjection()`
   or elsewhere? Exact file:line.

3. `SourceCode/99_Utility/Debug/Imgui/ImGuiManager.h/.cpp` FULLY — exactly how
   `Init()` sets up the DX12 ImGui backend (`ImGui_ImplDX12_Init` call — what
   descriptor heap/font-SRV-slot does it use?), and whether there's any existing
   pattern here for getting a texture into ImGui (a `D3D12_GPU_DESCRIPTOR_HANDLE`
   usable with `ImGui::Image()`) — even if unused today, report the exact
   descriptor heap setup since a new SRV for the offscreen render texture will
   need to live somewhere in this heap infrastructure.

4. Confirm current `Main::Draw()` full flow one more time (already read partially
   this session, but confirm the EXACT current order after this session's
   `DebugDockSpace` addition — note: `DebugDockSpace::Draw()`'s call site is
   currently commented out due to a same-session bug under investigation, don't
   remove that comment, just report the surrounding code as-is).

5. Report `git status --short` from `C:\Users\green\source\C++\DirectX` (paths
   only).

## Output format

Write `tasks/investigation-report-viewport.md` with headers matching points 1-5
above, exact file:line citations, and code snippets for the key
struct/heap/barrier/aspect-ratio logic. Be exhaustive — this report will be used
to design a render-to-texture viewport panel without the reader re-reading the
source themselves. No design recommendations — pure fact reporting only.
