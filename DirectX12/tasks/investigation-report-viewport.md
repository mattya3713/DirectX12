# 1. DirectX12 の現在の描画設定

## 1.1 Swapchain とバックバッファ

FrameBufferCount は DirectX12.h:121-124 の static constexpr UINT で 2。コマンドアロケータ配列、フェンス値配列、swapchain の BufferCount に同じ値が使われる。

CreateSwapChain は DirectX12.cpp:341-373 で次の descriptor を設定する。

```cpp
DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
SwapChainDesc.Width              = WND_W; // 1280
SwapChainDesc.Height             = WND_H; // 720
SwapChainDesc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
SwapChainDesc.Stereo             = false;
SwapChainDesc.SampleDesc.Count   = 1;
SwapChainDesc.SampleDesc.Quality = 0;
SwapChainDesc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
SwapChainDesc.BufferCount        = FrameBufferCount; // 2
SwapChainDesc.Scaling             = DXGI_SCALING_STRETCH;
SwapChainDesc.SwapEffect          = DXGI_SWAP_EFFECT_FLIP_DISCARD;
SwapChainDesc.AlphaMode           = DXGI_ALPHA_MODE_UNSPECIFIED;
SwapChainDesc.Flags               = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
```

生成は IDXGIFactory2::CreateSwapChainForHwnd に command queue、window handle、descriptor を渡して行う (DirectX12.cpp:359-367)。生成後の descriptor は m_SwapChainDesc に保存する (369-372)。WND_W/H は SourceCode/Global.h:58-61 の 1280/720 である。

CreateRenderTarget (DirectX12.cpp:375-441) は swapchain の BufferCount 分だけ m_pBackBuffer を resize (403-404) し、各 buffer を GetBuffer で取得して RTV を作る。RTV descriptor の初期 format は R8G8B8A8_UNORM_SRGB、各 resource の実際の format で上書きする (406-420)。

## 1.2 所有 descriptor heap と現在の割り当て

DirectX12.h:198-206 の heap メンバーは次の 3 つである。

| メンバー | descriptor heap descriptor | 割り当て |
|---|---|---|
| m_pRenderTargetViewHeap | Type=RTV、NumDescriptors=2、Flags=NONE、NodeMask=0 (DirectX12.cpp:381-385) | 固定 2 スロット。heap start から各 backbuffer を順に作り、increment size で進める (400-429)。描画時は m_FrameIndex * increment を直接算出 (155-156)。bump allocator ではない。 |
| m_pDepthHeap | Type=DSV、NumDescriptors=1、Flags=NONE、NodeMask は zero 初期化 (482-485) | 固定 1 スロット。heap start に DSV を作成 (493-504)。 |
| m_pDepthSRVHeap | Type=CBV_SRV_UAV、NumDescriptors=1、Flags=SHADER_VISIBLE、NodeMask=0 (506-510) | 固定 1 スロット。heap start に depth SRV を作成 (517-524)。 |

一般用途の CBV/SRV/UAV heap や複数リソース用 descriptor allocator は DirectX12 クラスにはない。m_pDepthSRVHeap は depth SRV 1 個専用である。

depth resource は DirectX12.cpp:455-479 で R32_TYPELESS の 2D resource、ALLOW_DEPTH_STENCIL、初期状態 DEPTH_WRITE、clear depth 1.0、clear format D32_FLOAT として作成される。DSV は D32_FLOAT/TEXTURE2D/flags none (494-497)、SRV は R32_FLOAT/mip 1/TEXTURE2D (517-521)。

## 1.3 BeginDraw と EndDraw

BeginDraw は DirectX12.cpp:130-170。GetCurrentBackBufferIndex で frame index を設定し (132-133)、保存済み frame fence が完了するまで待ち (139-144)、allocator と command list を reset (146-147) する。

```cpp
auto Barrier = CD3DX12_RESOURCE_BARRIER::Transition(
    m_pBackBuffer[m_FrameIndex].Get(),
    D3D12_RESOURCE_STATE_PRESENT,
    D3D12_RESOURCE_STATE_RENDER_TARGET);
m_pCmdList->ResourceBarrier(1, &Barrier);
```

上の PRESENT -> RENDER_TARGET 遷移は DirectX12.cpp:150-152。RTV は frame-indexed handle、DSV は heap start を使い (155-159)、OM 設定は次のとおり (160)。

```cpp
m_pCmdList->OMSetRenderTargets(1, &rtvH, false, &DSVHeapPointer);
```

depth は D3D12_CLEAR_FLAG_DEPTH、depth 1.0、stencil 0 でクリア (161)。RTV は次の黒・不透明色でクリアする (163-165)。

```cpp
float ClearColor[] = { 0.f, 0.f, 0.f, 1.0f };
m_pCmdList->ClearRenderTargetView(rtvH, ClearColor, 0, nullptr);
```

viewport/scissor はそれぞれ 1 個設定する (167-169)。viewport は backbuffer から作成 (438)、scissor は swapchain の width/height から作成 (439)。

EndDraw は DirectX12.cpp:172-194。状態を次のように戻す。

```cpp
auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
    m_pBackBuffer[m_FrameIndex].Get(),
    D3D12_RESOURCE_STATE_RENDER_TARGET,
    D3D12_RESOURCE_STATE_PRESENT);
m_pCmdList->ResourceBarrier(1, &barrier);
```

これは 174-177。続いて close (180)、queue 実行 (182-184)、Present(1, 0) (186-187)、fence signal と frame slot への保存 (189-193) を行う。

## 1.4 フレームインフライトと fence

メンバーは DirectX12.h:193-196 の m_pCmdAllocators[FrameBufferCount]、m_pCmdList、m_pCmdQueue、m_FrameIndex、および 218-221 の m_pFence、m_FenceValue、m_hFenceEvent、m_FrameFenceValues[FrameBufferCount]。

CreateCommandObject (DirectX12.cpp:300-339) は frame count 分の direct allocator を作成 (308-316)、最初の allocator で command list を作成 (318-325) する。fence event は CreateEvent(nullptr, FALSE, FALSE, nullptr) (306)、fence は CreateFance で初期値 m_FenceValue/flags none として作る (586-595)。

BeginDraw は再利用する frame slot の fence 値を確認し、未完了なら SetEventOnCompletion と WaitForSingleObject(INFINITE) で待つ (139-144)。EndDraw は待たずに ++m_FenceValue を signal し、その値を m_FrameFenceValues[m_FrameIndex] に保存する (189-193)。WaitForGPU (232-247) は queue を signal して完了まで待つ。destructor は fence event を close する (33-40)。

## 1.5 オフスクリーン render target の既存 precedent

SourceCode 全体で D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET、D3D12_RESOURCE_STATE_RENDER_TARGET、CreateCommittedResource、Tex2D を検索した。RENDER_TARGET state は DirectX12.cpp:151 と 175 の backbuffer 遷移だけで、ALLOW_RENDER_TARGET は存在しない。

PMXRenderer.cpp/PMDRenderer.cpp などには texture resource 作成があるが、render-target flag はない。従って、アプリ管理サイズの offscreen render texture、そこへの RTV/SRV、専用 render-texture クラスは現状存在しない。

## 1.6 OS ウィンドウ resize

Main::MsgProc (SourceCode/00_Game/00_GameLoop/Main.cpp:341-385) は WM_NCCREATE、WM_DESTROY、WM_KEYDOWN のみを処理し、WM_SIZE は処理しない。IDXGISwapChain::ResizeBuffers も SourceCode 全体に存在しない。

従って OS resize に伴う swapchain buffer、RTV、depth、viewport/scissor の再作成・更新は実装されていない。初期作成時の固定 WND_W/WND_H が使われる。

# 2. CameraBase とアスペクト比

CameraBase の m_Aspect は CameraBase.h:90-93 にあり、SetAspect は 65-66。コンストラクタ (CameraBase.cpp:14-31) は WND_WF / WND_HF で一度初期化する。

```cpp
, m_FovY     { DEFAULT_FOVY }
, m_Aspect   { WND_WF / WND_HF }
, m_NearClip { DEFAULT_NEAR }
, m_FarClip  { DEFAULT_FAR }
```

WND_WF/WND_HF は Global.h:59-61 の 1280.f/720.f。毎フレーム window や swapchain から読む実装ではない。SetAspect の定義は CameraBase.cpp:125 にあるが、呼び出しは repo-wide で確認できない。

UpdateViewProjection は ViewUpdate と ProjectionUpdate を呼ぶ (CameraBase.cpp:33-37)。aspect ratio が projection に入る箇所は CameraBase.cpp:146-149 である。

```cpp
void CameraBase::ProjectionUpdate()
{
    m_Proj = DirectX::XMMatrixPerspectiveFovLH(
        m_FovY, m_Aspect, m_NearClip, m_FarClip);
}
```

DirectX12::SetCamera (DirectX12.cpp:101-107) は渡された View/Proj/Eye をメンバーへ保存するだけで、aspect ratio は計算しない。MainScene::Update は CameraManager::Update 後に active camera の行列を SetCamera へ渡す (MainScene.cpp:119-135、引数は 126-129)。AnimationTuningScene にも同じ経路がある (AnimationTuningScene.cpp:53-68)。

別途 CreateSceneDesc (DirectX12.cpp:568-577) は初期 scene constant buffer 用に m_SwapChainDesc.Width/Height から aspectRatio を計算する。しかし通常フレームは SetCamera の行列を UpdateSceneBuffer (115-123) が mapped buffer へコピーする。通常の camera projection の aspect を焼き込む箇所は CameraBase::ProjectionUpdate である。

# 3. ImGuiManager の DX12 backend と texture handle

ImGuiManager の所有メンバーは ImGuiManager.h:103-106 の m_pDx12、m_cpSrvHeap、m_IsInitialized。m_cpSrvHeap は ImGui 専用 SRV heap で、現状は font 用である。

Init (ImGuiManager.cpp:51-82) は CreateSrvHeap を呼び (55)、context 作成と docking 有効化を行う (57-60)。DX12 backend は次で初期化する (69-76)。

```cpp
ImGui_ImplDX12_Init(
    device.Get(),
    FRAME_COUNT, // 2
    DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
    m_cpSrvHeap.Get(),
    m_cpSrvHeap->GetCPUDescriptorHandleForHeapStart(),
    m_cpSrvHeap->GetGPUDescriptorHandleForHeapStart());
```

FRAME_COUNT は ImGuiManager.h:97-100 の 2。CreateSrvHeap (ImGuiManager.cpp:96-106) の descriptor は次のとおり。

```cpp
D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
heap_desc.NumDescriptors = 1;
heap_desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
```

heap は descriptor 1 個のみで、descriptor increment による次スロット計算、SRV 追加作成、GPU handle getter、ImGui::Image 呼び出しは現状ない。Render (ImGuiManager.cpp:117-130 付近) はこの heap を SetDescriptorHeaps(1, ...) で設定してから ImGui_ImplDX12_RenderDrawData を呼ぶ。

vendored ImGui の default ini filename は Data/Library/ImGui/imgui.cpp:1321 の imgui.rul。ImGuiManager.cpp は io.IniFilename を変更していないため、current working directory 相対でこの名前が使用される。

# 4. 現在の Main::Draw() の全体フロー

Main.cpp:12-16 に DebugDockSpace.h の include がある。しかし Main::Draw は 162-187 の順序で次を実行する。

1. m_pDx12 が null なら return (165)。
2. m_pDx12->BeginDraw (168)。
3. DebugDockSpace::Draw の call site は 170-172 にあるが、//DebugDockSpace::Draw(); とコメントアウトされている。コメントには 3D 描画が真っ黒になる問題の切り分けのため一時無効化したとある。
4. DebugHud::Draw (175)。
5. DebugConsole::Draw (176)。
6. m_upSceneManager があれば m_upSceneManager->Draw (178-180)。
7. ImGuiManager::Render (183)。他の描画後、EndDraw 前に ImGui draw data を積む。
8. m_pDx12->EndDraw (186)。

Main::Loop は ImGuiManager::NewFrame を Update/Draw より前に呼ぶ (Main.cpp:280-297、呼び出しは 292)。従って Draw 時点では ImGui frame 開始済みである。

# 5. C:\Users\green\source\C++\DirectX の git status --short（パスのみ）

```text
DirectX12/Data/Library/DirectXTex
DirectX12/DirectX12.vcxproj
DirectX12/DirectX12.vcxproj.filters
DirectX12/SourceCode/00_Game/00_GameLoop/Main.cpp
DirectX12/tasks/investigation-request.md
DirectX12/SourceCode/99_Utility/Debug/Imgui/DebugDockSpace.cpp
DirectX12/SourceCode/99_Utility/Debug/Imgui/DebugDockSpace.h
DirectX12/tasks/current.md
```
