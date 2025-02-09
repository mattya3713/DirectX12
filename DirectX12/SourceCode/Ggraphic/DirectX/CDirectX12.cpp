#include "CDirectX12.h"
#include "Utility/String/FilePath/FilePath.h"


CDirectX12::CDirectX12()
	: m_hWnd			( nullptr )
	, m_pDxgiFactory	( nullptr )
	, m_pSwapChain		( nullptr )
	, m_pDevice12		( nullptr )
	, m_pCmdAllocator	( nullptr )
	, m_pCmdList		( nullptr )
	, m_pCmdQueue		( nullptr )
	, m_pRenderTargetViewHeap( nullptr )	
	, m_pBackBuffer		( )
	, m_pDepthBuffer	( nullptr ) 
	, m_pDepthHeap		( nullptr ) 
	, m_DepthClearValue	( ) 
	, m_pSceneConstBuff	( nullptr )
	, m_pMappedSceneData( nullptr )
	, m_pSceneDescHeap	( nullptr )
	, m_pFence			( nullptr )
	, m_FenceValue		( 0 )
	, m_pPipelineState	( nullptr )	
	, m_pRootSignature	( nullptr )
	, m_LoadLambdaTable	()
	, m_ResourceTable	()
{							 
}

CDirectX12::~CDirectX12()
{
}

bool CDirectX12::Create(HWND hWnd)
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
			m_pCmdAllocator,
			m_pCmdList,
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
			m_pDepthHeap);
		
		// ビューの設定.
		CreateSceneDesc(
			m_pMappedSceneData,
			m_pSceneConstBuff,
			m_pSceneDescHeap);

		// フェンスの表示.
		CreateFance(
			m_pFence);
	}
	catch(const std::runtime_error& Msg) {

		// エラーメッセージを表示.
		std::wstring WStr = MyString::StringToWString(Msg.what());
		_ASSERT_EXPR(false, WStr.c_str());
		return false;
	}
	
	return true;
}

void CDirectX12::UpDate()
{
}

void CDirectX12::BeginDraw()
{
	// DirectX処理.
	// バックバッファのインデックスを取得.
	auto BBIdx = m_pSwapChain->GetCurrentBackBufferIndex();
	auto Barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_pBackBuffer[BBIdx].Get(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_pCmdList->ResourceBarrier(1, &Barrier);

	// レンダーターゲットを指定.
	auto rtvH = m_pRenderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();
	rtvH.ptr += BBIdx * m_pDevice12->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// 深度を指定.
	auto DSVHeap = m_pDepthHeap->GetCPUDescriptorHandleForHeapStart();
	m_pCmdList->OMSetRenderTargets(1, &rtvH, false, &DSVHeap);
	m_pCmdList->ClearDepthStencilView(DSVHeap, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// 画面クリア.
	float ClearColor[] = { 1.0f,1.0f,1.0f,1.0f };//白色
	m_pCmdList->ClearRenderTargetView(rtvH, ClearColor, 0, nullptr);

	//ビューポート、シザー矩形のセット.
	m_pCmdList->RSSetViewports(1, m_pViewport.get());
	m_pCmdList->RSSetScissorRects(1, m_pScissorRect.get());
}

void CDirectX12::EndDraw()
{
	auto BBIdx = m_pSwapChain->GetCurrentBackBufferIndex();
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_pBackBuffer[BBIdx].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	m_pCmdList->ResourceBarrier(1,
		&barrier);

	// 命令のクローズ.
	m_pCmdList->Close();

	// コマンドリストの実行.
	ID3D12CommandList* cmdlists[] = { m_pCmdList.Get() };
	m_pCmdQueue->ExecuteCommandLists(1, cmdlists);
	// 待ち.
	WaitForGPU();
	// キューをクリア.
	m_pCmdAllocator->Reset();
	// 再びコマンドリストをためる準備.
	m_pCmdList->Reset(m_pCmdAllocator.Get(), nullptr);
}

// スワップチェーンを取得.
const MyComPtr<IDXGISwapChain4> CDirectX12::GetSwapChain()
{
	return m_pSwapChain;
}

// DirectX12デバイスを取得.
const MyComPtr<ID3D12Device> CDirectX12::GetDevice()
{
	return m_pDevice12;
}

// コマンドリストを取得.
const MyComPtr<ID3D12GraphicsCommandList> CDirectX12::GetCommandList()
{
	return m_pCmdList;
}

// テクスチャを取得.
MyComPtr<ID3D12Resource> CDirectX12::GetTextureByPath(const char* texpath)
{
	// リソーステーブル内を検索.
	auto [iterator, Result] = m_ResourceTable.emplace(
		texpath,
		nullptr 
	);

	if (Result) {
		// パスが未定義だった場合生成する.
		iterator->second = MyComPtr<ID3D12Resource>(CreateTextureFromFile(texpath));
	}

	// マップ内のリソースを返す
	return iterator->second;
}

void CDirectX12::SetScene()
{	
	//現在のシーン(ビュープロジェクション)をセット
	ID3D12DescriptorHeap* sceneheaps[] = { m_pSceneDescHeap.Get() };
	m_pCmdList->SetDescriptorHeaps(1, sceneheaps);
	m_pCmdList->SetGraphicsRootDescriptorTable(0, m_pSceneDescHeap->GetGPUDescriptorHandleForHeapStart());
}

// GPUの完了待ち.
void CDirectX12::WaitForGPU()
{

	m_pCmdQueue->Signal(m_pFence.Get(), ++m_FenceValue);

	if (m_pFence->GetCompletedValue() < m_FenceValue) {
		auto event = CreateEvent(nullptr, false, false, nullptr);

		// eventが正常に作成されたかを確認.
		if (event != nullptr) {
			m_pFence->SetEventOnCompletion(m_FenceValue, event);
			WaitForSingleObject(event, INFINITE);
			CloseHandle(event);
		}
		else {
			OutputDebugString(L"Failed to create event!\n");
		}
	}
}

// DXGIの生成.
void CDirectX12::CreateDXGIFactory(MyComPtr<IDXGIFactory6>& DxgiFactory)
{
#ifdef _DEBUG
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
void CDirectX12::CreateCommandObject(
	MyComPtr<ID3D12CommandAllocator>&	CmdAllocator,
	MyComPtr<ID3D12GraphicsCommandList>&CmdList,
	MyComPtr<ID3D12CommandQueue>&		CmdQueue)
{
	MyAssert::IsFailed(
		_T("コマンドリストアロケーターの生成"),
		&ID3D12Device::CreateCommandAllocator, m_pDevice12.Get(),
		D3D12_COMMAND_LIST_TYPE_DIRECT,			// 作成するコマンドアロケータの種類.
		IID_PPV_ARGS(CmdAllocator.ReleaseAndGetAddressOf()));		// (Out) コマンドアロケータ.

	MyAssert::IsFailed(
		_T("コマンドリストの生成"),
		&ID3D12Device::CreateCommandList, m_pDevice12.Get(),
		0,									// 単一のGPU操作の場合は0.
		D3D12_COMMAND_LIST_TYPE_DIRECT,		// 作成するコマンド リストの種類.
		CmdAllocator.Get(),				// アロケータへのポインタ.
		nullptr,							// ダミーの初期パイプラインが設定される?
		IID_PPV_ARGS(CmdList.ReleaseAndGetAddressOf()));				// (Out) コマンドリスト.

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
}

// スワップチェーンの作成.
void CDirectX12::CreateSwapChain(MyComPtr<IDXGISwapChain4>& SwapChain)
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
	SwapChainDesc.BufferCount = 2;									//  ﾊﾞｯｸﾊﾞｯﾌｧの数.
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
}

// レンダーターゲットの作成.
void CDirectX12::CreateRenderTarget(
	MyComPtr<ID3D12DescriptorHeap>&			RenderTargetViewHeap,
	std::vector<MyComPtr<ID3D12Resource>>&	BackBuffer)
{
	// ディスクリプタヒープ構造体の作成.
	D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
	HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;		// RTV用ヒープ.
	HeapDesc.NumDescriptors = 2;						// 2つのディスクリプタ.
	HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;	// ヒープのオプション(特になしを設定).
	HeapDesc.NodeMask = 0;								// 単一アダプタ.					

	MyAssert::IsFailed(
		_T("ディスクリプタヒープの作成"),
		&ID3D12Device::CreateDescriptorHeap, m_pDevice12.Get(),
		&HeapDesc,														// ディスクリプタヒープ構造体を登録.
		IID_PPV_ARGS(RenderTargetViewHeap.ReleaseAndGetAddressOf()));// (Out)ディスクリプタヒープ.

	// スワップチェーン構造体.
	DXGI_SWAP_CHAIN_DESC SwcDesc = {};
	MyAssert::IsFailed(
		_T("スワップチェーン構造体を取得."),
		&IDXGISwapChain4::GetDesc, m_pSwapChain.Get(),
		&SwcDesc);

	// ﾃﾞｨｽｸﾘﾌﾟﾀﾋｰﾌﾟの先頭アドレスを取り出す.
	D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandle = RenderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();

	// バックバッファをヒープの数分宣言.
	m_pBackBuffer.resize(SwcDesc.BufferCount);

	// SRGBレンダーターゲットビュー設定.
	D3D12_RENDER_TARGET_VIEW_DESC RTVDesc = {};
	RTVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	// バックバファの数分.
	for (int i = 0; i < static_cast<int>(SwcDesc.BufferCount); ++i)
	{
		MyAssert::IsFailed(
			_T("スワップチェーン内のバッファーとビューを関連づける"),
			&IDXGISwapChain4::GetBuffer, m_pSwapChain.Get(),
			static_cast<UINT>(i),
			IID_PPV_ARGS(m_pBackBuffer[i].GetAddressOf()));

		RTVDesc.Format = m_pBackBuffer[i]->GetDesc().Format;

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
void CDirectX12::CreateDepthDesc(
	MyComPtr<ID3D12Resource>&		DepthBuffer,
	MyComPtr<ID3D12DescriptorHeap>&	DepthHeap)
{
	// 深度バッファの仕様.
	D3D12_RESOURCE_DESC DepthResourceDesc = {};
	DepthResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;	// 2次元のテクスチャデータとして.
	DepthResourceDesc.Width = WND_W;									// 幅と高さはレンダーターゲットと同じ.
	DepthResourceDesc.Height = WND_H;									// 上に同じ.
	DepthResourceDesc.DepthOrArraySize = 1;								// テクスチャ配列でもないし3Dテクスチャでもない.
	DepthResourceDesc.Format = DXGI_FORMAT_D32_FLOAT;					// 深度値書き込み用フォーマット.
	DepthResourceDesc.SampleDesc.Count = 1;								// サンプルは1ピクセル当たり1つ.
	DepthResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;	// このバッファは深度ステンシルとして使用します.
	DepthResourceDesc.MipLevels = 1;
	DepthResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	DepthResourceDesc.Alignment = 0;

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

	MyAssert::IsFailed(
		_T("深度ステンシルビュー用のデスクリプタヒープを作成"),
		&ID3D12Device::CreateDescriptorHeap, m_pDevice12.Get(),
		&DsvHeapDesc,											// ヒープの設定.
		IID_PPV_ARGS(DepthHeap.ReleaseAndGetAddressOf()));	// (Out)デスクリプタヒープ.
	
	// 深度ビュー作成.
	D3D12_DEPTH_STENCIL_VIEW_DESC DsvDesc = {};
	DsvDesc.Format = DXGI_FORMAT_D32_FLOAT;					// デプスフォーマット.
	DsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;	// 2Dテクスチャ.
	DsvDesc.Flags = D3D12_DSV_FLAG_NONE;					// フラグなし.

	m_pDevice12->CreateDepthStencilView(
		m_pDepthBuffer.Get(),								// 深度バッファ.
		&DsvDesc,											// 深度ビューの設定.
		DepthHeap->GetCPUDescriptorHandleForHeapStart());// ヒープ内の位置.
}

// シーンビューの作成.
void CDirectX12::CreateSceneDesc(
	SceneData* MappedSceneData, 
	MyComPtr<ID3D12Resource>& SceneConstBuff,
	MyComPtr<ID3D12DescriptorHeap>& SceneDescHeap)
{
	DXGI_SWAP_CHAIN_DESC1 desc = {};
	auto result = m_pSwapChain->GetDesc1(&desc);
	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(SceneData) + 0xff) & ~0xff);

	MyAssert::IsFailed(
		_T("定数バッファ作成"),
		&ID3D12Device::CreateCommittedResource, m_pDevice12.Get(),
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(SceneConstBuff.ReleaseAndGetAddressOf()));

	MappedSceneData = nullptr;

	MyAssert::IsFailed(
		_T("シーン情報のマップ"),
		&ID3D12Resource::Map, SceneConstBuff.Get(),
		0, nullptr,
		(void**)&MappedSceneData);

	DirectX::XMFLOAT3 eye(0, 15, -50);
	DirectX::XMFLOAT3 target(0, 15, 0);
	DirectX::XMFLOAT3 up(0, 1, 0);
	MappedSceneData->view =
		DirectX::XMMatrixLookAtLH(
			XMLoadFloat3(&eye), 
			XMLoadFloat3(&target), 
			XMLoadFloat3(&up));

	MappedSceneData->proj =
		DirectX::XMMatrixPerspectiveFovLH
		(DirectX::XM_PIDIV4,//画角は45°
		static_cast<float>(desc.Width) / static_cast<float>(desc.Height),//アス比
		0.1f,//近い方
		1000.0f//遠い方
	);

	MappedSceneData->eye = eye;

	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = 1;//
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	MyAssert::IsFailed(
		_T(""),
		&ID3D12Device::CreateDescriptorHeap, m_pDevice12.Get(),
		&descHeapDesc,
		IID_PPV_ARGS(SceneDescHeap.ReleaseAndGetAddressOf()));

	// デスクリプタの先頭ハンドルを取得しておく.
	auto heapHandle = SceneDescHeap->GetCPUDescriptorHandleForHeapStart();

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = SceneConstBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = static_cast<UINT>(SceneConstBuff->GetDesc().Width);

	// 定数バッファビューの作成.
	m_pDevice12->CreateConstantBufferView(&cbvDesc, heapHandle);
}

// フェンスの作成.
void CDirectX12::CreateFance(MyComPtr<ID3D12Fence>& Fence)
{
	MyAssert::IsFailed(
		_T("フェンスの生成"),
		&ID3D12Device::CreateFence, m_pDevice12.Get(),
		m_FenceValue,									// 初期化子.
		D3D12_FENCE_FLAG_NONE,							// フェンスのオプション.
		IID_PPV_ARGS(Fence.ReleaseAndGetAddressOf()));// (Out) フェンス.
}

// テクスチャロードテーブルの作成.
void CDirectX12::CreateTextureLoadTable()
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

// テクスチャ名からテクスチャバッファ作成、中身をコピーする.
ID3D12Resource* CDirectX12::CreateTextureFromFile(const char* Texpath)
{
	std::string TexPath = Texpath;
	//テクスチャのロード
	DirectX::TexMetadata	Metadata = {};
	DirectX::ScratchImage	ScratchImg = {};

	//テクスチャのファイルパス. 
	std::wstring wTexPath = MyString::StringToWString(TexPath);

	//拡張子を取得.
	auto Extension = MyFilePath::GetExtension(TexPath);

	HRESULT Result = m_LoadLambdaTable[Extension](wTexPath,
		&Metadata,
		ScratchImg);

	if (FAILED(Result)) {

		// エラーメッセージを作成.
		std::string_view ErrorMessage = MyAssert::HResultToJapanese(Result);

		// メッセージボックスを表示.
		MessageBoxA(nullptr, std::string(ErrorMessage).c_str(), "Texture Load Error", MB_OK | MB_ICONERROR);

		return nullptr;
	}

	auto Image = ScratchImg.GetImage(0, 0, 0);//生データ抽出

	//WriteToSubresourceで転送する用のヒープ設定
	auto TexHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);

	// テクスチャリソースのディスクリタ.
	D3D12_RESOURCE_DESC ResDesc = {};
	ResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 2D テクスチャ.
	ResDesc.Alignment = 0;                                 // デフォルトアライメント.
	ResDesc.Width = Metadata.width;                        // テクスチャの幅.
	ResDesc.Height = static_cast<UINT>(Metadata.height);   // テクスチャの高さ.
	ResDesc.DepthOrArraySize = static_cast<UINT16>(Metadata.arraySize); // 配列のサイズ.
	ResDesc.MipLevels = static_cast<UINT16>(Metadata.mipLevels);        // MIP レベル.
	ResDesc.Format = Metadata.format;                     // テクスチャフォーマット.
	ResDesc.SampleDesc.Count = 1;                         // サンプリング (マルチサンプルはしない).
	ResDesc.SampleDesc.Quality = 0;                       // サンプリング品質 (デフォルト).
	ResDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;        // レイアウト (ドライバーに任せる).
	ResDesc.Flags = D3D12_RESOURCE_FLAG_NONE;             // 特殊なフラグなし.

	ID3D12Resource* Texbuff = nullptr;
	Result = m_pDevice12->CreateCommittedResource(
		&TexHeapProp,
		D3D12_HEAP_FLAG_NONE,//特に指定なし
		&ResDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&Texbuff)
	);

	if (FAILED(Result)) {
		return nullptr;
	}

	Result = Texbuff->WriteToSubresource(0,
		nullptr,			// 全領域へコピー.
		Image->pixels,		// 元データアドレス.
		static_cast<UINT>(Image->rowPitch),	// 1ラインサイズ.
		static_cast<UINT>(Image->slicePitch)// 全サイズ.
	);

	if (FAILED(Result)) {
		return nullptr;
	}

	return Texbuff;
}

// アダプターを見つける.
IDXGIAdapter* CDirectX12::FindAdapter(std::wstring FindWord)
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
void CDirectX12::EnableDebuglayer()
{
	ID3D12Debug* DebugLayer = nullptr;
	
	// デバッグレイヤーインターフェースを取得.
	D3D12GetDebugInterface(IID_PPV_ARGS(&DebugLayer));

	// デバッグレイヤーを有効.
	DebugLayer->EnableDebugLayer();	

	// 解放.
	DebugLayer->Release();
}