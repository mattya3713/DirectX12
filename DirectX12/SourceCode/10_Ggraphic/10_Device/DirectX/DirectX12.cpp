#include "DirectX12.h"

#include "10_Ggraphic/20_Render/Compute/AsyncComputeDemo.h"
#include "99_Utility/String/FilePath/FilePath.h"
#include "99_Utility/Debug/Imgui/ImGuiManager.h"
#include "10_Ggraphic/20_Render/Rewind/FrameRewind.h"

#include <cstring>

// このスレッドが並列記録中に使う専用コマンドリスト(nullptrならメインリスト側).
// ワーカースレッドは1フレームでjoinされるため、この領域がフレームを跨いで残ることは無い.
static thread_local ID3D12GraphicsCommandList* tls_pRecordingParallelCmdList = nullptr;


DirectX12::DirectX12()
	: m_hWnd			{ nullptr }
	, m_pDxgiFactory	{ nullptr }
	, m_pSwapChain		{ nullptr }
	, m_SwapChainDesc	{ }
	, m_pDevice12		{ nullptr }
	, m_ActiveMainListIndex { 0 }
	, m_pCmdQueue		{ nullptr }
	, m_FrameIndex		{ 0 }
	, m_pRenderTargetViewHeap{ nullptr }
	, m_pBackBuffer		{ }
	, m_pDepthBuffer	{ nullptr }
	, m_pDepthHeap		{ nullptr }
	, m_DepthClearValue	{ }
	, m_pSceneColorBuffer  { nullptr }
	, m_pSceneColorRTVHeap { nullptr }
	, m_pImGuiManagerForSceneSrv { nullptr }
	, m_SceneColorWidth { 0 }
	, m_SceneColorHeight { 0 }
	, m_SceneColorResizeRequested { false }
	, m_SceneColorRequestedWidth { 0 }
	, m_SceneColorRequestedHeight { 0 }
	, m_pFence			{ nullptr }
	, m_FenceValue		{ 0 }
	, m_pPipelineState	{ nullptr }
	, m_pRootSignature	{ nullptr }
	, m_LoadLambdaTable	{ }
	, m_ResourceTable	{ }
	, m_ViewMatrix		{ DirectX::XMMatrixIdentity() }
	, m_ProjMatrix		{ DirectX::XMMatrixIdentity() }
	, m_EyePosition		{ 0.0f, 0.0f, 0.0f }
	, m_LightViewMatrix { DirectX::XMMatrixIdentity() }
	, m_LightProjMatrix { DirectX::XMMatrixIdentity() }
	, m_LightDirection  { 0.0f, -1.0f, 0.0f, 0.003f }
	, m_LightColor      { 1.0f, 1.0f, 1.0f, 0.0f } // a=0: SetLight()未呼び出しの間は影サンプリングを無効化.
{
}

DirectX12::~DirectX12()
{
	if (m_hFenceEvent != nullptr)
	{
		CloseHandle(m_hFenceEvent);
		m_hFenceEvent = nullptr;
	}
}

bool DirectX12::Create(HWND hWnd)
{
	m_hWnd = hWnd;

#if _DEBUG
	// デバッグレイヤーをオン.
	EnableDebuglayer();

#endif _DEBUG

	try {
		
		// DXGIの生成.
		CreateDXGIFactory(
			m_pDxgiFactory);
	
		// コマンド類の生成.
		CreateCommandObject(
			m_pCmdQueue);
		
		// スワップチェーンの生成.
		CreateSwapChain(
			m_pSwapChain);

		// レンダーターゲットの作成.
		CreateRenderTarget(
			m_pRenderTargetViewHeap,
			m_pBackBuffer);

		// テクスチャロードテーブルの作成.
		CreateTextureLoadTable();

		// 深度バッファの作成.
		CreateDepthDesc(
			m_pDepthBuffer, 
			m_pDepthHeap,
			m_pDepthSRVHeap);
		
		// ビューの設定.
		CreateSceneDesc();

		// フェンスの表示.
		CreateFance(
			m_pFence);

		// GPUタイムスタンプクエリ(簡易プロファイラ用).
		CreateGpuQueryResources();

		// 巻き戻り演出用リングバッファ.
		m_upFrameRewind = std::make_unique<FrameRewind>(*this);
		m_upFrameRewind->Create();
	}
	catch(const std::runtime_error& Msg) {

		// エラーメッセージを表示.
		std::wstring WStr = MyString::StringToWString(Msg.what());
		_ASSERT_EXPR(false, WStr.c_str());
		return false;
	}
	
	return true;
}
#include <DirectXMath.h>

// カメラ行列を設定する(実際の入力処理はCameraBase派生クラス側が担当する).
void DirectX12::SetCamera(const DirectX::XMMATRIX& View, const DirectX::XMMATRIX& Proj, const DirectX::XMFLOAT3& Eye)
{
	m_ViewMatrix = View;
	m_ProjMatrix = Proj;
	m_EyePosition = Eye;
}

// 平行光源を設定する(実際のライト状態はDirectionLightクラス側が担当する).
void DirectX12::SetLight(
	const DirectX::XMMATRIX& LightView,
	const DirectX::XMMATRIX& LightProj,
	const DirectX::XMFLOAT4& Direction,
	const DirectX::XMFLOAT4& Color)
{
	m_LightViewMatrix = LightView;
	m_LightProjMatrix = LightProj;
	m_LightDirection  = Direction;
	m_LightColor      = Color;
}

// 更新
void DirectX12::Update()
{
	UpdateSceneBuffer(); // 更新

	// Async Computeデモ(180フレームごとにコンピュートキューで検証計算を実行し、
	// フェンス同期後にCPUで結果を読み戻して検証する).
	static AsyncComputeDemo s_async_compute_demo;
	static bool s_demo_initialized = false;
	if (!s_demo_initialized)
	{
		s_demo_initialized = s_async_compute_demo.Initialize(m_pDevice12.Get(), *this);
	}
	if (s_demo_initialized) { s_async_compute_demo.Tick(*this); }
}

void DirectX12::UpdateSceneBuffer()
{
	// この時点ではまだ今フレームのBeginDraw()が呼ばれておらずm_FrameIndexは前フレームの値のままなので、
	// ここではm_FrameIndexを使わずGetCurrentBackBufferIndex()を直接問い合わせて今フレーム用のスロットを
	// 求める(Present()を挟まない限りBeginDraw()が求める値と一致する. m_FrameIndex経由だと1フレーム分
	// 古いスロットへ書き込んでしまい、GPUがまだ読んでいる前フレーム分のバッファを壊す競合になる).
	if (!m_pSwapChain) { return; }

	const UINT frame_index = m_pSwapChain->GetCurrentBackBufferIndex();
	SceneData* p_scene_data = m_pMappedSceneData[frame_index];

	if (p_scene_data)
	{
		// カメラ行列はSetCamera()で設定済みのものをそのまま使う.
		p_scene_data->view = m_ViewMatrix;
		p_scene_data->proj = m_ProjMatrix;
		p_scene_data->eye  = m_EyePosition;

		// 平行光源はSetLight()で設定済みのものをそのまま使う(SetLight()未呼び出し時は影無しで動かす).
		p_scene_data->lightView      = m_LightViewMatrix;
		p_scene_data->lightProj      = m_LightProjMatrix;
		p_scene_data->lightDirection = m_LightDirection;
		p_scene_data->lightColor     = m_LightColor;
	}
	else
	{
		std::cerr << "Warning: m_pMappedSceneData[" << frame_index << "] is null in UpdateSceneBuffer()." << std::endl;
	}
}

void DirectX12::BeginDraw(bool UseOffscreenScene)
{
	if (m_SceneColorResizeRequested)
	{
		WaitForGPU();
		ResizeSceneColorTarget(m_SceneColorRequestedWidth, m_SceneColorRequestedHeight);
		m_SceneColorResizeRequested = false;
	}

	// このフレームで使うバックバッファのインデックス(EndDraw()まで使い回す).
	m_FrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();
	m_bUseOffscreenScene = UseOffscreenScene;

	// このバックバッファ用のアロケータをGPUがまだ使用中でないか確認してからReset()する.
	// (Reset()は「そのアロケータから確保したコマンドの実行がGPU側で全て終わっている」場合のみ有効.
	// 通常は2フレーム前(FrameBufferCount=2)の処理なので、ここではほぼ待たない.
	// これによりPresentの直後に毎回GPUの完了を待つ必要がなくなり、CPUとGPUが並行して動ける).
	// メイン3本+並列スロット全てが同一フレームの同一フェンス値で実行されるため、
	// 1つのフェンス値で全部のアロケータの再利用可否を管理できる).
	if (m_pFence->GetCompletedValue() < m_FrameFenceValues[m_FrameIndex]) {
		if (m_hFenceEvent != nullptr) {
			m_pFence->SetEventOnCompletion(m_FrameFenceValues[m_FrameIndex], m_hFenceEvent);
			WaitForSingleObject(m_hFenceEvent, INFINITE);
		}
	}

	// メインリスト(前半/後半/終端)と並列記録スロットをフレーム先頭でまとめて初期化する.
	for (UINT i = 0; i < MainListCount; ++i) {
		m_pCmdAllocators[m_FrameIndex][i]->Reset();
		m_pCmdLists[i]->Reset(m_pCmdAllocators[m_FrameIndex][i].Get(), nullptr);
		m_bMainListClosed[i] = false;
	}
	for (UINT Slot = 0; Slot < ParallelRecordSlotCount; ++Slot) {
		m_pParallelCmdAllocators[m_FrameIndex][Slot]->Reset();
		m_pParallelCmdLists[Slot]->Reset(m_pParallelCmdAllocators[m_FrameIndex][Slot].Get(), nullptr);
		m_bParallelSlotClosed[Slot] = false;
	}
	tls_pRecordingParallelCmdList = nullptr;
	m_ActiveMainListIndex = 0;

	ID3D12GraphicsCommandList* p_cmd_list = CurrentMainCmdList();

	if (UseOffscreenScene)
	{
		// 3DシーンはオフスクリーンのシーンカラーバッファへPIXEL_SHADER_RESOURCE→RENDER_TARGETで描く
		// (実際のバックバッファはPrepareUIRenderTarget()でImGui用に別途RENDER_TARGETへ遷移させる.
		// Scene ViewパネルがこのバッファをImGui::Image()でサンプルする).
		auto Barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_pSceneColorBuffer.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		p_cmd_list->ResourceBarrier(1, &Barrier);

		// レンダーターゲットを指定(オフスクリーンのシーンカラーバッファ).
		auto rtvH = m_pSceneColorRTVHeap->GetCPUDescriptorHandleForHeapStart();

		// 深度を指定.
		auto DSVHeapPointer = m_pDepthHeap->GetCPUDescriptorHandleForHeapStart();
		p_cmd_list->OMSetRenderTargets(1, &rtvH, false, &DSVHeapPointer);
		p_cmd_list->ClearDepthStencilView(DSVHeapPointer, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		// 画面クリア.
		float ClearColor[] = { 0.f,0.f,0.f,1.0f };
		p_cmd_list->ClearRenderTargetView(rtvH, ClearColor, 0, nullptr);

		//ビューポート、0.シザー矩形のセット.
		p_cmd_list->RSSetViewports(1, m_pSceneColorViewport.get());
		p_cmd_list->RSSetScissorRects(1, m_pSceneColorScissorRect.get());
	}
	else
	{
		// 実際のバックバッファへ直接描画する(MainScene用).
		auto Barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_pBackBuffer[m_FrameIndex].Get(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		p_cmd_list->ResourceBarrier(1, &Barrier);

		auto rtvH = m_pRenderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += m_FrameIndex * m_pDevice12->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		auto DSVHeapPointer = m_pDepthHeap->GetCPUDescriptorHandleForHeapStart();
		p_cmd_list->OMSetRenderTargets(1, &rtvH, false, &DSVHeapPointer);
		p_cmd_list->ClearDepthStencilView(DSVHeapPointer, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		float ClearColor[] = { 0.f,0.f,0.f,1.0f };
		p_cmd_list->ClearRenderTargetView(rtvH, ClearColor, 0, nullptr);

		p_cmd_list->RSSetViewports(1, m_pViewport.get());
		p_cmd_list->RSSetScissorRects(1, m_pScissorRect.get());
	}
}

void DirectX12::RestoreMainRenderTargets()
{
	ID3D12GraphicsCommandList* p_cmd_list = CurrentMainCmdList();

	if (m_bUseOffscreenScene)
	{
		// オフスクリーンのシーンカラーバッファへ復帰.
		auto rtvH = m_pSceneColorRTVHeap->GetCPUDescriptorHandleForHeapStart();
		auto DSVHeapPointer = m_pDepthHeap->GetCPUDescriptorHandleForHeapStart();
		p_cmd_list->OMSetRenderTargets(1, &rtvH, false, &DSVHeapPointer);
		p_cmd_list->RSSetViewports(1, m_pSceneColorViewport.get());
		p_cmd_list->RSSetScissorRects(1, m_pSceneColorScissorRect.get());
	}
	else
	{
		// バックバッファへ復帰.
		auto rtvH = m_pRenderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += m_FrameIndex * m_pDevice12->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		auto DSVHeapPointer = m_pDepthHeap->GetCPUDescriptorHandleForHeapStart();
		p_cmd_list->OMSetRenderTargets(1, &rtvH, false, &DSVHeapPointer);
		p_cmd_list->RSSetViewports(1, m_pViewport.get());
		p_cmd_list->RSSetScissorRects(1, m_pScissorRect.get());
	}
}

// ===== 敗北時巻き戻り演出(FrameRewindへの委譲) =====

// 毎フレーム、現在のバックバッファ内容を縮小リングへ1枚保存する.
void DirectX12::CaptureForRewind()
{
	if (m_upFrameRewind) { m_upFrameRewind->Capture(); }
}

// 巻き戻り逆再生を開始する.
bool DirectX12::StartRewindPlayback()
{
	return m_upFrameRewind ? m_upFrameRewind->StartPlayback() : false;
}

// 巻き戻り逆再生を1フレーム分描画する.
void DirectX12::DrawRewindFrame()
{
	if (m_upFrameRewind) { m_upFrameRewind->DrawPlaybackFrame(); }
}

// 巻き戻り逆再生中か.
bool DirectX12::IsRewindActive() const noexcept
{
	return m_upFrameRewind && m_upFrameRewind->IsActive();
}

void DirectX12::PrepareUIRenderTarget()
{
	ID3D12GraphicsCommandList* p_cmd_list = CurrentMainCmdList();

	// オフスクリーンのシーンカラーバッファをRENDER_TARGET→PIXEL_SHADER_RESOURCEへ
	// (このフレームのImGui::Image()でサンプルできるようにする).
	auto ToSrv = CD3DX12_RESOURCE_BARRIER::Transition(m_pSceneColorBuffer.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	p_cmd_list->ResourceBarrier(1, &ToSrv);

	// 実際のバックバッファをImGui描画用にPRESENT→RENDER_TARGETへ(EndDraw()で戻す).
	auto ToRt = CD3DX12_RESOURCE_BARRIER::Transition(m_pBackBuffer[m_FrameIndex].Get(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	p_cmd_list->ResourceBarrier(1, &ToRt);

	auto rtvH = m_pRenderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();
	rtvH.ptr += m_FrameIndex * m_pDevice12->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	p_cmd_list->OMSetRenderTargets(1, &rtvH, false, nullptr);

	// ドッキングされていない隙間に前フレームの残像が出ないようクリアする(ImGuiパネルは後で上書きされる).
	float ClearColor[] = { 0.f,0.f,0.f,1.0f };
	p_cmd_list->ClearRenderTargetView(rtvH, ClearColor, 0, nullptr);

	p_cmd_list->RSSetViewports(1, m_pViewport.get());
	p_cmd_list->RSSetScissorRects(1, m_pScissorRect.get());
}

void DirectX12::CreateSceneColorTarget(ImGuiManager& ImGuiMgr)
{
	m_pImGuiManagerForSceneSrv = &ImGuiMgr;

	DXGI_SWAP_CHAIN_DESC1 Desc = {};
	MyAssert::IsFailed(
		_T("スワップチェーンの取り出し(シーンカラーバッファ用)"),
		&IDXGISwapChain4::GetDesc1, m_pSwapChain.Get(),
		&Desc);

	ResizeSceneColorTarget(Desc.Width, Desc.Height);
}

void DirectX12::RequestSceneColorResize(UINT Width, UINT Height) noexcept
{
	if (Width == 0 || Height == 0)
	{
		return;
	}

	if (Width == m_SceneColorWidth && Height == m_SceneColorHeight)
	{
		return;
	}

	m_SceneColorRequestedWidth = Width;
	m_SceneColorRequestedHeight = Height;
	m_SceneColorResizeRequested = true;
}

void DirectX12::OnWindowResize(UINT Width, UINT Height)
{
	if (Width == 0 || Height == 0 || !m_pSwapChain)
	{
		return;
	}

	WaitForGPU();

	for (auto& BackBuffer : m_pBackBuffer)
	{
		BackBuffer.Reset();
	}
	m_pDepthBuffer.Reset();
	m_pDepthHeap.Reset();
	m_pDepthSRVHeap.Reset();

	MyAssert::IsFailed(
		_T("スワップチェーンのバッファーをリサイズ"),
		&IDXGISwapChain4::ResizeBuffers, m_pSwapChain.Get(),
		FrameBufferCount, Width, Height,
		m_SwapChainDesc.Format, m_SwapChainDesc.Flags);

	CreateRenderTarget(m_pRenderTargetViewHeap, m_pBackBuffer);
	CreateDepthDesc(m_pDepthBuffer, m_pDepthHeap, m_pDepthSRVHeap);

	// バックバッファが作り直されたため巻き戻り演出用のSRVも張り直す.
	if (m_upFrameRewind) { m_upFrameRewind->RefreshBackBufferSRVs(); }
}

void DirectX12::ResizeSceneColorTarget(UINT Width, UINT Height)
{
	m_pSceneColorBuffer.Reset();


	D3D12_RESOURCE_DESC ColorResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, Width, Height, 1, 1, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

	D3D12_CLEAR_VALUE ColorClearValue = {};
	ColorClearValue.Format    = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	ColorClearValue.Color[0]  = 0.0f;
	ColorClearValue.Color[1]  = 0.0f;
	ColorClearValue.Color[2]  = 0.0f;
	ColorClearValue.Color[3]  = 1.0f;

	D3D12_HEAP_PROPERTIES ColorHeapProperty = {};
	ColorHeapProperty.Type                  = D3D12_HEAP_TYPE_DEFAULT;
	ColorHeapProperty.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	ColorHeapProperty.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;

	MyAssert::IsFailed(
		_T("シーンカラーバッファリソースを作成"),
		&ID3D12Device::CreateCommittedResource, m_pDevice12.Get(),
		&ColorHeapProperty,
		D3D12_HEAP_FLAG_NONE,
		&ColorResourceDesc,
		// 毎フレームBeginDraw()の遷移元と一致させるため、PIXEL_SHADER_RESOURCEを初期状態にする
		// (最初のBeginDraw()も含め、常に同じ遷移で扱えるようにするため).
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&ColorClearValue,
		IID_PPV_ARGS(m_pSceneColorBuffer.ReleaseAndGetAddressOf()));

	D3D12_DESCRIPTOR_HEAP_DESC RtvHeapDesc = {};
	RtvHeapDesc.NumDescriptors = 1;
	RtvHeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	RtvHeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	MyAssert::IsFailed(
		_T("シーンカラーバッファ用RTVヒープを作成"),
		&ID3D12Device::CreateDescriptorHeap, m_pDevice12.Get(),
		&RtvHeapDesc,
		IID_PPV_ARGS(m_pSceneColorRTVHeap.ReleaseAndGetAddressOf()));

	D3D12_RENDER_TARGET_VIEW_DESC ColorRtvDesc = {};
	ColorRtvDesc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	ColorRtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	m_pDevice12->CreateRenderTargetView(
		m_pSceneColorBuffer.Get(),
		&ColorRtvDesc,
		m_pSceneColorRTVHeap->GetCPUDescriptorHandleForHeapStart());

	m_pSceneColorViewport.reset(new CD3DX12_VIEWPORT(m_pSceneColorBuffer.Get()));
	m_pSceneColorScissorRect.reset(new CD3DX12_RECT(0, 0, static_cast<LONG>(Width), static_cast<LONG>(Height)));

	// ImGuiのSRVヒープ(スロット1)へ直接SRVを作成する(ImGui::Image()から参照できるようにするため).
	D3D12_SHADER_RESOURCE_VIEW_DESC ColorSrvDesc = {};
	ColorSrvDesc.Format                    = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	ColorSrvDesc.ViewDimension              = D3D12_SRV_DIMENSION_TEXTURE2D;
	ColorSrvDesc.Texture2D.MipLevels        = 1;
	ColorSrvDesc.Shader4ComponentMapping    = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	m_pDevice12->CreateShaderResourceView(
		m_pSceneColorBuffer.Get(),
		&ColorSrvDesc,
		m_pImGuiManagerForSceneSrv->GetSceneTextureCpuHandle());

	m_SceneColorWidth = Width;
	m_SceneColorHeight = Height;
}

// 並列記録を開始する.
bool DirectX12::BeginParallelRecording(UINT Slot)
{
	if (Slot >= ParallelRecordSlotCount || !m_pParallelCmdLists[Slot]) { return false; }
	if (tls_pRecordingParallelCmdList != nullptr) { return false; } // 同一スレッドの二重開始は不備.

	tls_pRecordingParallelCmdList = m_pParallelCmdLists[Slot].Get();
	return true;
}

// 並列記録を完了する.
void DirectX12::EndParallelRecording()
{
	if (tls_pRecordingParallelCmdList == nullptr) { return; }

	// 対応するスロットを逆引きしてCloseし、EndDraw()のバッチ対象へ登録する.
	for (UINT Slot = 0; Slot < ParallelRecordSlotCount; ++Slot)
	{
		if (m_pParallelCmdLists[Slot].Get() == tls_pRecordingParallelCmdList)
		{
			tls_pRecordingParallelCmdList->Close();
			m_bParallelSlotClosed[Slot] = true;
			break;
		}
	}
	tls_pRecordingParallelCmdList = nullptr;
}

// メインパス後半のコマンドリストへ記録先を切り替える.
void DirectX12::SwitchToDeferredMainList()
{
	constexpr UINT DeferredIndex = RenderBatchOrder::MainDeferred;

	// 前半(インデックス0)以外から呼ばれた場合は何もしない(二重切替防止).
	if (m_ActiveMainListIndex != RenderBatchOrder::MainFirst) { return; }

	m_bMainListClosed[m_ActiveMainListIndex] = m_pCmdLists[m_ActiveMainListIndex]->Close() == S_OK;
	m_ActiveMainListIndex = DeferredIndex;
}

// 現在アクティブなメインリストの記録先を返す.
ID3D12GraphicsCommandList* DirectX12::CurrentMainCmdList()
{
	return m_pCmdLists[m_ActiveMainListIndex].Get();
}

void DirectX12::EndDraw()
{
	// 終端リストへPRESENT遷移とクエリ解決を記録する(全リストの中で最後に実行される).
	ID3D12GraphicsCommandList* p_final_list = m_pCmdLists[RenderBatchOrder::MainFinal].Get();

	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_pBackBuffer[m_FrameIndex].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	p_final_list->ResourceBarrier(1, &barrier);

	// 記録されたタイムスタンプクエリを読み取りバッファへ解決する(Close前に行う).
	ResolveGpuQueries();

	p_final_list->Close();
	m_bMainListClosed[RenderBatchOrder::MainFinal] = true;

	// 未Closeのメインリストを閉じる(並列記録へ切り替えなかったフレームでは前半/後半がここで閉じる).
	for (UINT i = 0; i < MainListCount; ++i)
	{
		if (!m_bMainListClosed[i])
		{
			m_pCmdLists[i]->Close();
			m_bMainListClosed[i] = true;
		}
	}

	// 全コマンドリストを実行順(RenderBatchOrder::Build()の規則)に並べ、
	// 1回のExecuteCommandLists()へまとめて渡す(仕様: 待ち合わせ後に一括実行).
	const std::vector<UINT> ExecuteOrder = RenderBatchOrder::Build(m_bParallelSlotClosed, ParallelRecordSlotCount);

	ID3D12CommandList* CmdListsToExecute[MainListCount + ParallelRecordSlotCount] = {};
	UINT NumLists = 0;
	for (UINT Id : ExecuteOrder)
	{
		switch (Id)
		{
			case RenderBatchOrder::MainFirst:    { CmdListsToExecute[NumLists++] = m_pCmdLists[RenderBatchOrder::MainFirst].Get(); break; }
			case RenderBatchOrder::MainDeferred: { CmdListsToExecute[NumLists++] = m_pCmdLists[RenderBatchOrder::MainDeferred].Get(); break; }
			case RenderBatchOrder::MainFinal:    { CmdListsToExecute[NumLists++] = m_pCmdLists[RenderBatchOrder::MainFinal].Get(); break; }
			default:                             { CmdListsToExecute[NumLists++] = m_pParallelCmdLists[Id - RenderBatchOrder::ParallelBase].Get(); break; }
		}
	}

	m_pCmdQueue->ExecuteCommandLists(NumLists, CmdListsToExecute);

	// SwapChain の Present を呼び出す (ここで一度だけ行われる)
	m_pSwapChain->Present(1, 0);

	// このフレームの完了を示すフェンス値を発行するだけで、ここでは待たない.
	// (待つのは次にこのバックバッファ番号(m_FrameIndex)を使うBeginDraw()の役目.
	// そちらは通常FrameBufferCountフレーム分後なので、実質待たずに済むことがほとんど).
	m_pCmdQueue->Signal(m_pFence.Get(), ++m_FenceValue);
	m_FrameFenceValues[m_FrameIndex] = m_FenceValue;
}

// スワップチェーンを取得.
const MyComPtr<IDXGISwapChain4> DirectX12::GetSwapChain()
{
	return m_pSwapChain;
}

// GPUタイムスタンプクエリ用のヒープと読み取りバッファを作成する.
void DirectX12::CreateGpuQueryResources()
{
	// タイムスタンプクエリヒープ(バックバッファ数フレーム分のスロット).
	D3D12_QUERY_HEAP_DESC query_heap_desc = {};
	query_heap_desc.Type          = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
	query_heap_desc.Count         = MaxGpuTimestamps * FrameBufferCount;
	query_heap_desc.NodeMask      = 0;

	if (FAILED(m_pDevice12->CreateQueryHeap(&query_heap_desc, IID_PPV_ARGS(m_pGpuQueryHeap.ReleaseAndGetAddressOf())))) {
		return; // 非対応環境ではプロファイラのGPU計測を無効化するだけで続行.
	}

	// 解決結果の読み取りバッファ(読み取り型ヒープにマップしたまま使う).
	const UINT64 readback_size = static_cast<UINT64>(sizeof(std::uint64_t)) * MaxGpuTimestamps * FrameBufferCount;
	auto readback_desc = CD3DX12_RESOURCE_DESC::Buffer(readback_size);
	auto readback_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);

	if (FAILED(m_pDevice12->CreateCommittedResource(&readback_heap, D3D12_HEAP_FLAG_NONE, &readback_desc,
		D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(m_pGpuQueryReadback.ReleaseAndGetAddressOf())))) {
		m_pGpuQueryHeap.Reset();
		return;
	}

	if (FAILED(m_pGpuQueryReadback->Map(0, nullptr, reinterpret_cast<void**>(&m_pMappedGpuQueries)))) {
		m_pGpuQueryHeap.Reset();
		m_pGpuQueryReadback.Reset();
		return;
	}

	m_pCmdQueue->GetTimestampFrequency(&m_GpuTimestampFrequency);
}

// タイムスタンプの記録を積む.
void DirectX12::WriteGpuTimestamp(UINT IndexInFrame)
{
	if (!m_pGpuQueryHeap || IndexInFrame >= MaxGpuTimestamps) { return; }
	CurrentMainCmdList()->EndQuery(m_pGpuQueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP,
		m_FrameIndex * MaxGpuTimestamps + IndexInFrame);
}

// 記録したクエリ結果を読み取りバッファへ解決する.
void DirectX12::ResolveGpuQueries()
{
	if (!m_pGpuQueryHeap || !m_pGpuQueryReadback) { return; }

	CurrentMainCmdList()->ResolveQueryData(m_pGpuQueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP,
		m_FrameIndex * MaxGpuTimestamps, MaxGpuTimestamps,
		m_pGpuQueryReadback.Get(), m_FrameIndex * MaxGpuTimestamps);
}

// 完了済みフレームのタイムスタンプ差分をミリ秒で取得する.
float DirectX12::ReadGpuMilliseconds(UINT StartIndexInFrame, UINT EndIndexInFrame) const
{
	if (!m_pMappedGpuQueries || m_GpuTimestampFrequency == 0 ||
		StartIndexInFrame >= MaxGpuTimestamps || EndIndexInFrame >= MaxGpuTimestamps) { return -1.0f; }

	// 現在のバックバッファインデックス領域はBeginDraw()でフェンス待ち済み(2フレーム前完了)のため安全に読める.
	const std::uint64_t start = m_pMappedGpuQueries[m_FrameIndex * MaxGpuTimestamps + StartIndexInFrame];
	const std::uint64_t end   = m_pMappedGpuQueries[m_FrameIndex * MaxGpuTimestamps + EndIndexInFrame];
	if (end <= start) { return -1.0f; } // 未記録 or 同一フレーム未完了.

	return static_cast<float>(static_cast<double>(end - start) / static_cast<double>(m_GpuTimestampFrequency) * 1000.0);
}

// DirectX12デバイスを取得.
const MyComPtr<ID3D12Device> DirectX12::GetDevice()
{
	return m_pDevice12;
}

// コマンドリストを取得(並列記録中のスレッドは専用リスト、それ以外は現在のメインリスト).
const MyComPtr<ID3D12GraphicsCommandList> DirectX12::GetCommandList()
{
	if (tls_pRecordingParallelCmdList != nullptr) {
		return MyComPtr<ID3D12GraphicsCommandList>(tls_pRecordingParallelCmdList);
	}
	return m_pCmdLists[m_ActiveMainListIndex];
}

// テクスチャを取得.
MyComPtr<ID3D12Resource> DirectX12::GetTextureByPath(const char* texpath)
{
	// リソーステーブル内を検索.
	auto [iterator, Result] = m_ResourceTable.emplace(
		texpath,
		nullptr 
	);

	if (Result) {
		// パスが未定義だった場合生成する.
		iterator->second = CreateTextureFromFile(texpath);
	}

	// マップ内のリソースを返す
	return iterator->second;
}

// GPUの完了待ち.
// コンピュートキューからフェンスをシグナルする(Async Compute).
UINT64 DirectX12::SignalComputeFence()
{
	++m_ComputeFenceValue;

	// NOTE: MyAssert::IsFailedのテンプレート制約がメンバ関数ポインタ+複数引数の
	//       組み合わせで解決しないため、ここでは直接HRESULTを判定する.
	if (FAILED(m_cpComputeQueue->Signal(m_pComputeFence.Get(), m_ComputeFenceValue)))
	{
		return m_ComputeFenceValue;
	}

	return m_ComputeFenceValue;
}

// グラフィックスキューへコンピュート完了待ちを挿入する(キュー間同期).
void DirectX12::GraphicsWaitComputeFence(UINT64 Value)
{
	if (Value == 0) { return; }
	m_pCmdQueue->Wait(m_pComputeFence.Get(), Value);
}
void DirectX12::WaitForGPU()
{
	m_pCmdQueue->Signal(m_pFence.Get(), ++m_FenceValue);

	if (m_pFence->GetCompletedValue() < m_FenceValue) {
		// eventが正常に作成されたかを確認.
		if (m_hFenceEvent != nullptr) {
			m_pFence->SetEventOnCompletion(m_FenceValue, m_hFenceEvent);
			WaitForSingleObject(m_hFenceEvent, INFINITE);
		}
		else {
			OutputDebugString(L"Failed to create event!\n");
		}
	}
}

// DXGIの生成.
void DirectX12::CreateDXGIFactory(MyComPtr<IDXGIFactory6>& DxgiFactory)
{
#ifdef _DEBUG
	//HRESULT result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(DxgiFactory.ReleaseAndGetAddressOf()));

	MyAssert::IsFailed(
		_T("DXGIの生成"),
		&CreateDXGIFactory2,
		DXGI_CREATE_FACTORY_DEBUG,			// デバッグモード.
		IID_PPV_ARGS(DxgiFactory.ReleaseAndGetAddressOf()));		// (Out)DXGI.
#else // _DEBUG
	MyAssert::IsFailed(
		_T("DXGIの生成"),
		&CreateDXGIFactory1,
		IID_PPV_ARGS(m_pDxgiFactory.ReleaseAndGetAddressOf()));
#endif

	// フィーチャレベル列挙.
	D3D_FEATURE_LEVEL Levels[] = {
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1,
	};

	HRESULT Ret = S_OK;
	D3D_FEATURE_LEVEL FeatureLevel;

	for (auto Lv: Levels)
	{
		// DirectX12を実体化.
		if (D3D12CreateDevice(
			FindAdapter(L"NVIDIA"),				// グラボを選択.
			Lv,									// フィーチャーレベル.
			IID_PPV_ARGS(m_pDevice12.ReleaseAndGetAddressOf())) == S_OK)// (Out)Direct12.
		{
			// フィーチャーレベル.
			FeatureLevel = Lv;
			break;
		}
	}

}

// コマンド類の生成.
void DirectX12::CreateCommandObject(
	MyComPtr<ID3D12CommandQueue>&		CmdQueue)
{
	m_hFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	// バックバッファの数だけコマンドアロケータを用意する(1フレーム1つだと、
	// Presentの直後に毎回GPUの完了を待たないとReset()できず、CPU/GPUが完全に直列化されてしまう).
	// メイン側は前半/後半/終端の3本分、並列側はスロット数ぶん(同じくフレーム単位で使い回す).
	for (UINT i = 0; i < FrameBufferCount; ++i) {
		for (UINT ListIdx = 0; ListIdx < MainListCount; ++ListIdx) {
			MyAssert::IsFailed(
				_T("コマンドリストアロケーターの生成(メイン)"),
				&ID3D12Device::CreateCommandAllocator, m_pDevice12.Get(),
				D3D12_COMMAND_LIST_TYPE_DIRECT,			// 作成するコマンドアロケータの種類.
				IID_PPV_ARGS(m_pCmdAllocators[i][ListIdx].ReleaseAndGetAddressOf()));		// (Out) コマンドアロケータ.
		}

		for (UINT Slot = 0; Slot < ParallelRecordSlotCount; ++Slot) {
			MyAssert::IsFailed(
				_T("コマンドリストアロケーターの生成(並列)"),
				&ID3D12Device::CreateCommandAllocator, m_pDevice12.Get(),
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(m_pParallelCmdAllocators[i][Slot].ReleaseAndGetAddressOf()));
		}
	}

	for (UINT ListIdx = 0; ListIdx < MainListCount; ++ListIdx) {
		MyAssert::IsFailed(
			_T("コマンドリストの生成(メイン)"),
			&ID3D12Device::CreateCommandList, m_pDevice12.Get(),
			0,									// 単一のGPU操作の場合は0.
			D3D12_COMMAND_LIST_TYPE_DIRECT,		// 作成するコマンド リストの種類.
			m_pCmdAllocators[0][ListIdx].Get(),	// アロケータへのポインタ(最初のフレームで使う分).
			nullptr,							// ダミーの初期パイプラインが設定される?
			IID_PPV_ARGS(m_pCmdLists[ListIdx].ReleaseAndGetAddressOf()));				// (Out) コマンドリスト.
	}

	for (UINT Slot = 0; Slot < ParallelRecordSlotCount; ++Slot) {
		MyAssert::IsFailed(
			_T("コマンドリストの生成(並列)"),
			&ID3D12Device::CreateCommandList, m_pDevice12.Get(),
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			m_pParallelCmdAllocators[0][Slot].Get(),
			nullptr,
			IID_PPV_ARGS(m_pParallelCmdLists[Slot].ReleaseAndGetAddressOf()));
	}

	// コマンドキュー構造体の作成.
	D3D12_COMMAND_QUEUE_DESC CmdQueueDesc = {};
	CmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;				// タイムアウトなし.
	CmdQueueDesc.NodeMask = 0;										// アダプターを一つしか使わないときは0でいい.
	CmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;	// プライオリティは特に指定なし.
	CmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;				// コマンドリストと合わせる.

	MyAssert::IsFailed(
		_T("キューの作成"),
		&ID3D12Device::CreateCommandQueue, m_pDevice12.Get(),
		&CmdQueueDesc,
		IID_PPV_ARGS(CmdQueue.ReleaseAndGetAddressOf()));

	// 非同期コンピュートキュー(グラフィックスキューと並行実行用. Async Compute).
	D3D12_COMMAND_QUEUE_DESC ComputeQueueDesc = CmdQueueDesc;
	ComputeQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;

	MyAssert::IsFailed(
		_T("コンピュートキューの作成"),
		&ID3D12Device::CreateCommandQueue, m_pDevice12.Get(),
		&ComputeQueueDesc,
		IID_PPV_ARGS(m_cpComputeQueue.ReleaseAndGetAddressOf()));

	CreateFance(m_pComputeFence); // グラフィックス⇔コンピュート間の同期フェンス.
}

// スワップチェーンの作成.
void DirectX12::CreateSwapChain(MyComPtr<IDXGISwapChain4>& SwapChain)
{
	// スワップ チェーン構造体の設定.
	DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
	SwapChainDesc.Width = WND_W;									//  画面の幅.
	SwapChainDesc.Height = WND_H;									//  画面の高さ.
	SwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;				//  表示形式.
	SwapChainDesc.Stereo = false;									//  全画面モードかどうか.
	SwapChainDesc.SampleDesc.Count = 1;								//  ピクセル当たりのマルチサンプルの数.
	SwapChainDesc.SampleDesc.Quality = 0;							//  品質レベル(0~1).
	SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	//  ﾊﾞｯｸﾊﾞｯﾌｧのメモリ量.
	SwapChainDesc.BufferCount = FrameBufferCount;					//  ﾊﾞｯｸﾊﾞｯﾌｧの数(コマンドアロケータの数と合わせる).
	SwapChainDesc.Scaling = DXGI_SCALING_STRETCH;					//  ﾊﾞｯｸﾊﾞｯﾌｧのｻｲｽﾞがﾀｰｹﾞｯﾄと等しくない場合のｻｲｽﾞ変更の動作.
	SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;		//  ﾌﾘｯﾌﾟ後は素早く破棄.
	SwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;			//  ｽﾜｯﾌﾟﾁｪｰﾝ,ﾊﾞｯｸﾊﾞｯﾌｧの透過性の動作
	SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;	//  ｽﾜｯﾌﾟﾁｪｰﾝ動作のｵﾌﾟｼｮﾝ(ｳｨﾝﾄﾞｳﾌﾙｽｸ切り替え可能ﾓｰﾄﾞ).

	MyAssert::IsFailed(
		_T("スワップチェーンの作成"),
		&IDXGIFactory2::CreateSwapChainForHwnd, m_pDxgiFactory.Get(),
		m_pCmdQueue.Get(),								// コマンドキュー.
		m_hWnd,											// ウィンドウハンドル.
		&SwapChainDesc,									// スワップチェーン設定.
		nullptr,										// ひとまずnullotrでよい.TODO : なにこれ
		nullptr,										// これもnulltrでよう
		(IDXGISwapChain1**)SwapChain.ReleaseAndGetAddressOf());	// (Out)スワップチェーン.

	MyAssert::IsFailed(
		_T("スワップチェーンディスクリプションの取得"),
		&IDXGISwapChain1::GetDesc1, SwapChain.Get(),
		&m_SwapChainDesc); // m_SwapChainDesc に格納
}

// レンダーターゲットの作成.
void DirectX12::CreateRenderTarget(
	MyComPtr<ID3D12DescriptorHeap>&			RenderTargetViewHeap,
	std::vector<MyComPtr<ID3D12Resource>>&	BackBuffer)
{
	// ディスクリプタヒープ構造体の作成.
	D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
	HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;		// RTV用ヒープ.
	HeapDesc.NumDescriptors = 2;						// 2つのディスクリプタ.
	HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;	// ヒープのオプション(特になしを設定).
	HeapDesc.NodeMask = 0;								// 単一アダプタ.					

	if (!RenderTargetViewHeap)
	{
		MyAssert::IsFailed(
			_T("ディスクリプタヒープの作成"),
			&ID3D12Device::CreateDescriptorHeap, m_pDevice12.Get(),
			&HeapDesc,
			IID_PPV_ARGS(RenderTargetViewHeap.ReleaseAndGetAddressOf()));
	}

	// スワップチェーン構造体.
	DXGI_SWAP_CHAIN_DESC SwcDesc = {};
	MyAssert::IsFailed(
		_T("スワップチェーン構造体を取得."),
		&IDXGISwapChain4::GetDesc, m_pSwapChain.Get(),
		&SwcDesc);

	// ﾃﾞｨｽｸﾘﾌﾟﾀﾋｰﾌﾟの先頭アドレスを取り出す.
	D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandle = RenderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();

	// バックバッファをヒープの数分宣言.
	BackBuffer.resize(SwcDesc.BufferCount);

	// SRGBレンダーターゲットビュー設定.
	D3D12_RENDER_TARGET_VIEW_DESC RTVDesc = {};
	RTVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	// バックバファの数分.
	for (UINT i = 0; i < (SwcDesc.BufferCount); ++i)
	{
		MyAssert::IsFailed(
			_T("スワップチェーン内のバッファーとビューを関連づける"),
			&IDXGISwapChain4::GetBuffer, m_pSwapChain.Get(),
			i,
			IID_PPV_ARGS(BackBuffer[i].GetAddressOf()));

		RTVDesc.Format = BackBuffer[i]->GetDesc().Format;

		// レンダーターゲットビューを生成する.
		m_pDevice12->CreateRenderTargetView(
			BackBuffer[i].Get(),
			&RTVDesc,
			DescriptorHandle);

		// ポインタをずらす.
		DescriptorHandle.ptr += m_pDevice12->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	DXGI_SWAP_CHAIN_DESC1 Desc = {};
	MyAssert::IsFailed(
		_T("画面幅を取得"),
		&IDXGISwapChain4::GetDesc1, m_pSwapChain.Get(),
		&Desc);

	m_pViewport.reset(new CD3DX12_VIEWPORT(BackBuffer[0].Get()));
	m_pScissorRect.reset(new CD3DX12_RECT(0, 0, Desc.Width, Desc.Height));

}

// 深度バッファ作成.
void DirectX12::CreateDepthDesc(
	MyComPtr<ID3D12Resource>&		DepthBuffer,
	MyComPtr<ID3D12DescriptorHeap>&	DepthHeap,
	MyComPtr<ID3D12DescriptorHeap>& DepthSRVHeap)
{
	DXGI_SWAP_CHAIN_DESC1 desc = {};
	MyAssert::IsFailed(
		_T("スワップチェーンの取り出し"),
		&IDXGISwapChain4::GetDesc1, m_pSwapChain.Get(),
		&desc);

	// 深度バッファの仕様.
	D3D12_RESOURCE_DESC DepthResourceDesc = 
		CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32_TYPELESS, 
		desc.Width, desc.Height);
	DepthResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	// デプス用ヒーププロパティ.
	D3D12_HEAP_PROPERTIES DepthHeapProperty = {};
	DepthHeapProperty.Type = D3D12_HEAP_TYPE_DEFAULT;					// DEFAULTだから後はUNKNOWNでよし.
	DepthHeapProperty.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	DepthHeapProperty.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	// このクリアバリューが重要な意味を持つ.
	m_DepthClearValue.DepthStencil.Depth = 1.0f;		// 深さ１(最大値)でクリア.
	m_DepthClearValue.Format = DXGI_FORMAT_D32_FLOAT;	// 32bit深度値としてクリア.

	MyAssert::IsFailed(
		_T("深度バッファリソースを作成"),
		&ID3D12Device::CreateCommittedResource, m_pDevice12.Get(),
		&DepthHeapProperty,							// ヒーププロパティの設定.
		D3D12_HEAP_FLAG_NONE,						// ヒープのオプション(特になしを設定).
		&DepthResourceDesc,							// リソースの仕様.
		D3D12_RESOURCE_STATE_DEPTH_WRITE,			// リソースの初期状態.
		&m_DepthClearValue,							// 深度バッファをクリアするための設定.
		IID_PPV_ARGS(DepthBuffer.ReleaseAndGetAddressOf())); // (Out)深度バッファ.

	// 深度ステンシルビュー用のデスクリプタヒープを作成
	D3D12_DESCRIPTOR_HEAP_DESC DsvHeapDesc = {};
	DsvHeapDesc.NumDescriptors = 1;                   // 深度ビュー1つ.
	DsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;// デスクリプタヒープのタイプ.
	DsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	MyAssert::IsFailed(
		_T("深度ステンシルビュー用のデスクリプタヒープを作成"),
		&ID3D12Device::CreateDescriptorHeap, m_pDevice12.Get(),
		&DsvHeapDesc,										// ヒープの設定.
		IID_PPV_ARGS(DepthHeap.ReleaseAndGetAddressOf()));	// (Out)デスクリプタヒープ.
	
	// 深度ビュー作成.
	D3D12_DEPTH_STENCIL_VIEW_DESC DsvDesc = {};
	DsvDesc.Format = DXGI_FORMAT_D32_FLOAT;					// デプスフォーマット.
	DsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;	// 2Dテクスチャ.
	DsvDesc.Flags = D3D12_DSV_FLAG_NONE;					// フラグなし.

	D3D12_CPU_DESCRIPTOR_HANDLE handle = DepthHeap->GetCPUDescriptorHandleForHeapStart();

	m_pDevice12->CreateDepthStencilView(
		m_pDepthBuffer.Get(),								// 深度バッファ.
		&DsvDesc,											// 深度ビューの設定.
		DepthHeap->GetCPUDescriptorHandleForHeapStart());	// ヒープ内の位置.

	D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
	HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	HeapDesc.NodeMask = 0;
	HeapDesc.NumDescriptors = 1;
	HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	MyAssert::IsFailed(
		_T("SRVのディスクリプタヒープを作成"),
		&ID3D12Device::CreateDescriptorHeap, m_pDevice12,
		&HeapDesc, IID_PPV_ARGS(DepthSRVHeap.ReleaseAndGetAddressOf()));
	
	D3D12_SHADER_RESOURCE_VIEW_DESC DepthSrvResDesc = {};
	DepthSrvResDesc.Format = DXGI_FORMAT_R32_FLOAT;
	DepthSrvResDesc.Texture2D.MipLevels = 1;
	DepthSrvResDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	DepthSrvResDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

	auto srvHandle = DepthSRVHeap->GetCPUDescriptorHandleForHeapStart();
	m_pDevice12->CreateShaderResourceView(DepthBuffer.Get(), &DepthSrvResDesc, srvHandle);

}

// シーンビューの作成.
void DirectX12::CreateSceneDesc()
{
	// ---- リソース作成 ----
	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	// sizeof(SceneData) を256バイトの倍数に切り上げ
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(SceneData) + 0xff) & ~0xff);

	DirectX::XMFLOAT3 eye_pos(0, 0, -50);
	DirectX::XMFLOAT3 target_pos(0, 0, 0);
	DirectX::XMFLOAT3 up_vec(0, 1, 0);
	const DirectX::XMMATRIX initial_view =
		DirectX::XMMatrixLookAtLH(
		DirectX::XMLoadFloat3(&eye_pos),
		DirectX::XMLoadFloat3(&target_pos),
		DirectX::XMLoadFloat3(&up_vec));

	// アスペクト比はレンダリングループ内で更新するか、Dx12::Initialize()で一度設定
	// ここでは、既に m_pSwapChain が初期化されていると仮定し、その幅と高さを使用
	float aspectRatio = static_cast<float>(m_SwapChainDesc.Width) / static_cast<float>(m_SwapChainDesc.Height);
	const DirectX::XMMATRIX initial_proj =
		DirectX::XMMatrixPerspectiveFovLH
		(DirectX::XM_PIDIV4, // 画角は45°
		aspectRatio,         // アス比
		0.1f,                // 近い方
		1000.0f              // 遠い方
		);

	// FrameBufferCount分スロットを作る(CPUが次フレーム分をUpdateSceneBuffer()で書き込む間、
	// GPUが前フレーム分を読み終えていない、という競合を単一バッファでは防げないため).
	for (UINT i = 0; i < FrameBufferCount; ++i)
	{
		MyAssert::IsFailed(
			_T("定数バッファ作成 (Scene)"),
			&ID3D12Device::CreateCommittedResource, m_pDevice12.Get(), // クラスメンバーのデバイスを使用
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(m_pSceneConstBuff[i].ReleaseAndGetAddressOf()));

		// ---- バッファをマップ ----
		MyAssert::IsFailed(
			_T("シーン情報のマップ"),
			&ID3D12Resource::Map, m_pSceneConstBuff[i].Get(),
			0, nullptr,
			reinterpret_cast<void**>(&m_pMappedSceneData[i]));

		// ---- 初期値設定 ----
		if (m_pMappedSceneData[i]) {
			m_pMappedSceneData[i]->view = initial_view;
			m_pMappedSceneData[i]->proj = initial_proj;
			m_pMappedSceneData[i]->eye  = eye_pos;
		}
		else {
			std::cerr << "Error: m_pMappedSceneData[" << i << "] is null after mapping Scene Constant Buffer." << std::endl;
		}
	}
}

// フェンスの作成.
void DirectX12::CreateFance(MyComPtr<ID3D12Fence>& Fence)
{
	MyAssert::IsFailed(
		_T("フェンスの生成"),
		&ID3D12Device::CreateFence, m_pDevice12.Get(),
		m_FenceValue,									// 初期化子.
		D3D12_FENCE_FLAG_NONE,							// フェンスのオプション.
		IID_PPV_ARGS(Fence.ReleaseAndGetAddressOf()));// (Out) フェンス.
}

// テクスチャロードテーブルの作成.
void DirectX12::CreateTextureLoadTable()
{
	m_LoadLambdaTable["sph"] =
		m_LoadLambdaTable["spa"] =
		m_LoadLambdaTable["bmp"] =
		m_LoadLambdaTable["png"] =
		m_LoadLambdaTable["jpg"] =
		[](const std::wstring& path, DirectX::TexMetadata* meta, DirectX::ScratchImage& img)->HRESULT {
		return LoadFromWICFile(path.c_str(), DirectX::WIC_FLAGS_NONE, meta, img);
		};

	m_LoadLambdaTable["tga"] = [](const std::wstring& path, DirectX::TexMetadata* meta, DirectX::ScratchImage& img)->HRESULT {
		return LoadFromTGAFile(path.c_str(), meta, img);
		};

	m_LoadLambdaTable["dds"] = [](const std::wstring& path, DirectX::TexMetadata* meta, DirectX::ScratchImage& img)->HRESULT {
		return LoadFromDDSFile(path.c_str(), DirectX::DDS_FLAGS_NONE, meta, img);
		};

}

// CPUデータをDestResourceへ同期的にアップロードする(一時コマンドリスト+フェンス待機).
void DirectX12::UploadBufferSync(ID3D12Resource* DestResource, const void* SrcData, UINT64 Size, D3D12_RESOURCE_STATES StateAfter)
{
	if (DestResource == nullptr || SrcData == nullptr || Size == 0) { return; }

	// --- 中間アップロードリソースを作成し、CPUデータをコピーする ---
	MyComPtr<ID3D12Resource> upload_buffer;
	const auto upload_heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	const auto upload_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(Size);
	MyAssert::IsFailed(_T("UploadBufferSync: アップロードバッファの作成"),
		&ID3D12Device::CreateCommittedResource, m_pDevice12.Get(),
		&upload_heap_prop, D3D12_HEAP_FLAG_NONE, &upload_buffer_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(upload_buffer.ReleaseAndGetAddressOf()));

	void* p_mapped = nullptr;
	MyAssert::IsFailed(_T("UploadBufferSync: アップロードバッファをマップ"),
		&ID3D12Resource::Map, upload_buffer.Get(), 0, nullptr, &p_mapped);
	std::memcpy(p_mapped, SrcData, static_cast<size_t>(Size));
	upload_buffer->Unmap(0, nullptr);

	// --- ロード専用の一時コマンドリストでコピー+バリアを記録する ---
	// (毎フレームの共有コマンドリストm_pCmdListとは独立させる. フレーム描画中に呼ばれても
	// そちらの記録状態へ影響しないようにするため).
	MyComPtr<ID3D12CommandAllocator> temp_allocator;
	MyComPtr<ID3D12GraphicsCommandList> temp_cmd_list;
	MyAssert::IsFailed(_T("UploadBufferSync: 一時コマンドアロケータの生成"),
		&ID3D12Device::CreateCommandAllocator, m_pDevice12.Get(),
		D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(temp_allocator.ReleaseAndGetAddressOf()));
	MyAssert::IsFailed(_T("UploadBufferSync: 一時コマンドリストの生成"),
		&ID3D12Device::CreateCommandList, m_pDevice12.Get(),
		0, D3D12_COMMAND_LIST_TYPE_DIRECT, temp_allocator.Get(), nullptr,
		IID_PPV_ARGS(temp_cmd_list.ReleaseAndGetAddressOf()));

	temp_cmd_list->CopyBufferRegion(DestResource, 0, upload_buffer.Get(), 0, Size);

	const CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		DestResource, D3D12_RESOURCE_STATE_COPY_DEST, StateAfter);
	temp_cmd_list->ResourceBarrier(1, &barrier);

	temp_cmd_list->Close();

	// --- 実行してGPU完了を同期的に待つ(完了後はupload_buffer/temp_*が安全に破棄される) ---
	ID3D12CommandList* pp_command_lists[] = { temp_cmd_list.Get() };
	m_pCmdQueue->ExecuteCommandLists(_countof(pp_command_lists), pp_command_lists);

	MyComPtr<ID3D12Fence> upload_fence;
	HANDLE upload_fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	UINT64 fence_value_for_upload = 0;
	MyAssert::IsFailed(_T("UploadBufferSync: フェンスの生成"),
		&ID3D12Device::CreateFence, m_pDevice12.Get(),
		fence_value_for_upload, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(upload_fence.ReleaseAndGetAddressOf()));

	m_pCmdQueue->Signal(upload_fence.Get(), ++fence_value_for_upload);

	if (upload_fence->GetCompletedValue() < fence_value_for_upload)
	{
		if (upload_fence_event != nullptr)
		{
			MyAssert::IsFailed(_T("UploadBufferSync: フェンスイベント設定"),
				&ID3D12Fence::SetEventOnCompletion, upload_fence.Get(), fence_value_for_upload, upload_fence_event);
			WaitForSingleObject(upload_fence_event, INFINITE);
		}
	}
	if (upload_fence_event != nullptr) { CloseHandle(upload_fence_event); }
}

// テクスチャ名からテクスチャバッファ作成、中身をコピーする.
MyComPtr<ID3D12Resource> DirectX12::CreateTextureFromFile(const char* Texpath)
{
	std::string TexPath = Texpath;
	DirectX::TexMetadata Metadata = {};
	DirectX::ScratchImage ScratchImg = {};

	std::wstring wTexPath = MyString::StringToWString(TexPath);
	auto Extension = MyFilePath::GetExtension(TexPath);

	HRESULT Result = m_LoadLambdaTable[Extension](wTexPath, &Metadata, ScratchImg);

	if (FAILED(Result)) {
		// テクスチャ1枚の欠損でアプリ全体をブロックしないよう、モーダル表示はせず
		// ログのみに留める(呼び出し側がnullptrを見てデフォルトテクスチャへフォールバックする).
		std::string_view ErrorMessage = MyAssert::HResultToJapanese(Result);
		std::cerr << "Texture Load Error(" << TexPath << "): " << ErrorMessage << std::endl;
		return MyComPtr<ID3D12Resource>();
	}

	// --- 1. GPUがシェーダーから読み取るためのデフォルトヒープ上のテクスチャリソースを作成 ---
	D3D12_RESOURCE_DESC TexResDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		Metadata.format,
		Metadata.width,
		static_cast<UINT>(Metadata.height),
		static_cast<UINT16>(Metadata.arraySize),
		static_cast<UINT16>(Metadata.mipLevels)
	);

	MyComPtr<ID3D12Resource> textureResource = {};
	CD3DX12_HEAP_PROPERTIES heapPropsDefault(D3D12_HEAP_TYPE_DEFAULT);

	Result = m_pDevice12->CreateCommittedResource(
		&heapPropsDefault,
		D3D12_HEAP_FLAG_NONE,
		&TexResDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, // 初期状態はコピー先
		nullptr,
		IID_PPV_ARGS(textureResource.ReleaseAndGetAddressOf())
	);
	if (FAILED(Result)) {
		return MyComPtr<ID3D12Resource>();
	}

	// --- 2. CPUからGPUへのデータ転送用の中間アップロードリソースを作成 ---
	UINT64 uploadBufferSize = 0;
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;

	for (size_t arraySlice = 0; arraySlice < Metadata.arraySize; ++arraySlice)
	{
		for (size_t mipLevel = 0; mipLevel < Metadata.mipLevels; ++mipLevel)
		{
			const DirectX::Image* img = ScratchImg.GetImage(mipLevel, arraySlice, 0);
			if (!img) {
				return MyComPtr<ID3D12Resource>();
			}

			D3D12_SUBRESOURCE_DATA sd;
			sd.pData = img->pixels;
			sd.RowPitch = img->rowPitch;
			sd.SlicePitch = img->slicePitch;
			subresources.push_back(sd);
		}
	}

	m_pDevice12->GetCopyableFootprints(
		&TexResDesc,
		0,
		static_cast<UINT>(subresources.size()),
		0,
		nullptr,
		nullptr,
		nullptr,
		&uploadBufferSize
	);

	MyComPtr<ID3D12Resource> textureUploadHeap = {}; // ローカル変数として MyComPtr を宣言

	CD3DX12_HEAP_PROPERTIES heapPropsUpload(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC bufferResDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

	Result = m_pDevice12->CreateCommittedResource(
		&heapPropsUpload,
		D3D12_HEAP_FLAG_NONE,
		&bufferResDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(textureUploadHeap.ReleaseAndGetAddressOf())
	);
	if (FAILED(Result)) {
		return MyComPtr<ID3D12Resource>();
	}

	// --- 3. コマンドリストにコピーとバリアコマンドを追加 ---
	// テクスチャロード専用の一時コマンドリストとアロケータを使用する
	MyComPtr<ID3D12CommandAllocator> tempCmdAllocator;
	MyComPtr<ID3D12GraphicsCommandList> tempCmdList;

	MyAssert::IsFailed(
		_T("一時コマンドアロケータの生成"),
		&ID3D12Device::CreateCommandAllocator, m_pDevice12.Get(),
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(tempCmdAllocator.ReleaseAndGetAddressOf()));

	MyAssert::IsFailed(
		_T("一時コマンドリストの生成"),
		&ID3D12Device::CreateCommandList, m_pDevice12.Get(),
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		tempCmdAllocator.Get(),
		nullptr,
		IID_PPV_ARGS(tempCmdList.ReleaseAndGetAddressOf()));

	// 初期状態はクローズなので、リセットして開く
	tempCmdList->Reset(tempCmdAllocator.Get(), nullptr);

	// UpdateSubresources は CopyCommandList を引数に取る
	UpdateSubresources(
		tempCmdList.Get(), // tempCmdList を使用
		textureResource.Get(),
		textureUploadHeap.Get(),
		0,
		0,
		static_cast<UINT>(subresources.size()),
		subresources.data()
	);

	// テクスチャリソースをシェーダーリソース状態に遷移
	CD3DX12_RESOURCE_BARRIER textureBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		textureResource.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	tempCmdList->ResourceBarrier(1, &textureBarrier);

	tempCmdList->Close(); // コマンドリストを閉じる

	// --- 4. コマンドリストを実行し、GPUの完了を待機 ---
	ID3D12CommandList* ppCommandLists[] = { tempCmdList.Get() };
	m_pCmdQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists); // メインのキューで実行

	// --- GPUがこのアップロードを完了するまで同期的に待機 ---
	MyComPtr<ID3D12Fence> uploadFence;
	HANDLE uploadFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	UINT64 fenceValueForUpload = 0;

	MyAssert::IsFailed(
		_T("アップロード用フェンスの生成"),
		&ID3D12Device::CreateFence, m_pDevice12.Get(),
		fenceValueForUpload,
		D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(uploadFence.ReleaseAndGetAddressOf()));

	// キューにシグナルコマンドを積む
	m_pCmdQueue->Signal(uploadFence.Get(), ++fenceValueForUpload);

	// イベントがシグナルされるまで待つ
	if (uploadFence->GetCompletedValue() < fenceValueForUpload) {
		if (uploadFenceEvent != nullptr) {
			MyAssert::IsFailed(
				_T("アップロード用フェンスイベント設定"),
				&ID3D12Fence::SetEventOnCompletion, uploadFence.Get(),
				fenceValueForUpload, uploadFenceEvent);
			WaitForSingleObject(uploadFenceEvent, INFINITE);
		}
		else {
			OutputDebugString(L"Failed to create upload fence event!\n");
		}
	}
	// イベントハンドルをクローズ
	if (uploadFenceEvent != nullptr) {
		CloseHandle(uploadFenceEvent);
	}
	// ここで textureUploadHeap, tempCmdAllocator, tempCmdList, uploadFence は安全に解放される
	// （すべてローカル変数で MyComPtr であるため、スコープを抜けると自動的に Release される）

	// リソースを保持するために m_ResourceTable に追加する
	m_ResourceTable[Texpath] = textureResource;

	return textureResource;
}

// アダプターを見つける.
IDXGIAdapter* DirectX12::FindAdapter(std::wstring FindWord)
{
	// アタブター(見つけたグラボを入れる).
	std::vector <IDXGIAdapter*> Adapter;

	// ここに特定の名前を持つアダプターが入る.
	IDXGIAdapter* TmpAdapter = nullptr;

	// forですべてのアダプターをベクター配列に入れる.
	for (int i = 0; m_pDxgiFactory->EnumAdapters(i, &TmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i) {
		Adapter.push_back(TmpAdapter);
	}

	// 取り出したアダプターから情報を持ってくる.
	for (auto Adpt : Adapter) {

		DXGI_ADAPTER_DESC Adesc = {};

		// アダプター情報を取り出す.
		Adpt->GetDesc(&Adesc);

		// 名前を取り出す.
		std::wstring strDesc = Adesc.Description;

		// NVIDIAなら格納.
		if (strDesc.find(FindWord) != std::string::npos) {
			return Adpt;
		}
	}

	return nullptr;
}

// デバッグモードを起動.
void DirectX12::EnableDebuglayer()
{
	ID3D12Debug* DebugLayer = nullptr;
	
	// デバッグレイヤーインターフェースを取得.
	D3D12GetDebugInterface(IID_PPV_ARGS(&DebugLayer));

	// デバッグレイヤーを有効.
	DebugLayer->EnableDebugLayer();	

	// 解放.
	DebugLayer->Release();
}
