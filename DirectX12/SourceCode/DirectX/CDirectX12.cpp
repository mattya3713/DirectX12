#include "CDirectX12.h"

CDirectX12::CDirectX12()
	: m_pDevice12		( nullptr )
	, m_pDxgiFactory	( nullptr )
	, m_pSwapChain		( nullptr )
	, m_pCmdAllocator	( nullptr )
	, m_pCmdList		( nullptr )
	, m_pCmdQueue		( nullptr )
	, m_Vertex			()
{
}

CDirectX12::~CDirectX12()
{
}

bool CDirectX12::Create(HWND hWnd)
{
#if _DEBUG
	// デバッグレイヤーをオン.
	EnableDebuglayer();

#endif _DEBUG


	try {
		MyAssert::IsFailed(
			_T("DXGIの生成"),
			&CreateDXGIFactory1,
			IID_PPV_ARGS(&m_pDxgiFactory));

		// フィーチャレベル列挙.
		D3D_FEATURE_LEVEL levels[] = {
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

		D3D_FEATURE_LEVEL FeatureLevel;

		HRESULT Ret = S_OK;
		for (auto lv : levels)
		{
			// DirectX12を実体化.
			if (D3D12CreateDevice(
				FindAdapter(L"NVIDIA"),				// グラボを選択, nullptrで自動選択.
				lv,									// フィーチャーレベル.
				IID_PPV_ARGS(&m_pDevice12)) == S_OK)// (Out)Direct12.
			{
				// フィーチャーレベル.
				FeatureLevel = lv;
				break;
			}
		}

		MyAssert::IsFailed(
			_T("コマンドリストアロケーターの生成"),
			&ID3D12Device::CreateCommandAllocator, m_pDevice12,
			D3D12_COMMAND_LIST_TYPE_DIRECT,			// 作成するコマンドアロケータの種類.
			IID_PPV_ARGS(&m_pCmdAllocator));		// (Out) コマンドアロケータ.

		MyAssert::IsFailed(
			_T("コマンドリストの生成"),
			&ID3D12Device::CreateCommandList, m_pDevice12,
			0,										// 単一のGPU操作の場合は0.
			D3D12_COMMAND_LIST_TYPE_DIRECT,			// 作成するコマンド リストの種類.
			m_pCmdAllocator,						// アロケータへのポインタ.
			nullptr,								// ダミーの初期パイプラインが設定される?
			IID_PPV_ARGS(&m_pCmdList));				// (Out) コマンドリスト.

		// コマンドキュー構造体の作成.
		D3D12_COMMAND_QUEUE_DESC CmdQueueDesc = {};
		CmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;				// タイムアウトなし.
		CmdQueueDesc.NodeMask = 0;										// アダプターを一つしか使わないときは0でいい.
		CmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;	// プライオリティは特に指定なし.
		CmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;				// コマンドリストと合わせる.

		MyAssert::IsFailed(
			_T("キューの作成"),
			&ID3D12Device::CreateCommandQueue, m_pDevice12,
			&CmdQueueDesc,
			IID_PPV_ARGS(&m_pCmdQueue));

		// スワップ チェーン構造体の設定.
		DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
		SwapChainDesc.Width = WND_W;									//  画面の幅.
		SwapChainDesc.Height = WND_H;									//  画面の高さ.
		SwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;				//  表示形式.
		SwapChainDesc.Stereo = false;									//  全画面モードかどうか.
		SwapChainDesc.SampleDesc.Count = 1;										//  ピクセル当たりのマルチサンプルの数.
		SwapChainDesc.SampleDesc.Quality = 0;										//  品質レベル(0~1).
		SwapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;					//  ﾊﾞｯｸﾊﾞｯﾌｧのメモリ量.
		SwapChainDesc.BufferCount = 2;										//  ﾊﾞｯｸﾊﾞｯﾌｧの数.
		SwapChainDesc.Scaling = DXGI_SCALING_STRETCH;						//  ﾊﾞｯｸﾊﾞｯﾌｧのｻｲｽﾞがﾀｰｹﾞｯﾄと等しくない場合のｻｲｽﾞ変更の動作.
		SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;			//  ﾌﾘｯﾌﾟ後は素早く破棄.
		SwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;				//  ｽﾜｯﾌﾟﾁｪｰﾝ,ﾊﾞｯｸﾊﾞｯﾌｧの透過性の動作
		SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;	//  ｽﾜｯﾌﾟﾁｪｰﾝ動作のｵﾌﾟｼｮﾝ(ｳｨﾝﾄﾞｳﾌﾙｽｸ切り替え可能ﾓｰﾄﾞ).

		MyAssert::IsFailed(
			_T("スワップチェーンの作成"),
			&IDXGIFactory2::CreateSwapChainForHwnd, m_pDxgiFactory,
			m_pCmdQueue,									// コマンドキュー.
			hWnd,											// ウィンドウハンドル.
			&SwapChainDesc,									// スワップチェーン設定.
			nullptr,										// ひとまずnullotrでよい.TODO : なにこれ
			nullptr,										// これもnulltrでよう
			(IDXGISwapChain1**)&m_pSwapChain);				// (Out)スワップチェーン.

		// ディスクリプタヒープ構造体の作成.
		D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
		HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;		// ヒープ内のディスクリプタの肩を指定(RenderTargetView).
		HeapDesc.NumDescriptors = 2;						// ヒープ内のディスクリプタの数(表裏の２つ).	
		HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;	// ヒープのオプションを指定.
		HeapDesc.NodeMask = 0;								// ディスクリプタヒープが適用されるノード(単一アダプタの場合は0).					

		// ディスクリプタヒープ.
		ID3D12DescriptorHeap* RTVHeaps = nullptr;

		MyAssert::IsFailed(
			_T("ディスクリプタヒープの作成"),
			&ID3D12Device::CreateDescriptorHeap, m_pDevice12,
			&HeapDesc,										// ディスクリプタヒープ構造体を登録.
			IID_PPV_ARGS(&RTVHeaps));						// (Out)ディスクリプタヒープ.

		// スワップチェーン構造体.
		DXGI_SWAP_CHAIN_DESC swcDesc = {};
		MyAssert::IsFailed(
			_T("ディスクリプタヒープの作成"),
			&IDXGISwapChain4::GetDesc, m_pSwapChain,
			&swcDesc);


		// ﾃﾞｨｽｸﾘﾌﾟﾀﾋｰﾌﾟの戦闘アドレスを取り出す.
		D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandle = RTVHeaps->GetCPUDescriptorHandleForHeapStart();

		// バックバッファをヒープの数分宣言.
		std::vector<ID3D12Resource*> BackBaffer(swcDesc.BufferCount);

		// バックバファの数分.
		for (int i = 0; i < static_cast<int>(swcDesc.BufferCount); ++i)
		{
			MyAssert::IsFailed(
				_T("%d個目のスワップチェーン内のバッファーとビューを関連づける", i + 1),
				&IDXGISwapChain4::GetBuffer, m_pSwapChain,
				static_cast<UINT>(i),
				IID_PPV_ARGS(&BackBaffer[i]));

			// レンダーターゲットビューを生成する.
			m_pDevice12->CreateRenderTargetView(
				BackBaffer[i],
				nullptr,
				DescriptorHandle);

			// ポインタをずらす.
			DescriptorHandle.ptr += m_pDevice12->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		}

		ID3D12Fence* Fence = nullptr;
		UINT64 FenceValue = 0;
		MyAssert::IsFailed(
			_T("フェンスの生成"),
			&ID3D12Device::CreateFence, m_pDevice12,
			FenceValue,									// 初期化子.
			D3D12_FENCE_FLAG_NONE,						// フェンスのオプション.
			IID_PPV_ARGS(&Fence));						// (Out) フェンス.

		while (true)
		{
			// 現在のバックバッファを取得.
			auto BBIndex = m_pSwapChain->GetCurrentBackBufferIndex();

			D3D12_RESOURCE_BARRIER BarrierDesc = {};
			BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;						// 共用体の型.
			BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;							// フラグ.
			BarrierDesc.Transition.pResource = BackBaffer[BBIndex];							// ﾊﾞｯｸﾊﾞｯﾌｧﾘｿｰｽ.
			BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;	// 遷移のサブリソースのインデックス.
			BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;				// ?
			BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;			// ?

			//レンダーターゲットを指定.
			auto rtvH = RTVHeaps->GetCPUDescriptorHandleForHeapStart();
			rtvH.ptr += static_cast<ULONG_PTR>(BBIndex * m_pDevice12->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));

			m_pCmdList->OMSetRenderTargets(
				1,			// レンダーターゲット数(1でよい).
				&rtvH,		// レンダーターゲットハンドルの先頭アドレス.
				false,		// 複数時に連続しているか.
				nullptr);	// ハンドル(nullptr)でよい.

			// 画面クリア.
			float CrearColor[] = { 0.f, 0.5f, 0.f, 1.f };
			m_pCmdList->ClearRenderTargetView(
				rtvH,
				CrearColor,
				0,
				nullptr);

			BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

			m_pCmdList->ResourceBarrier(1, &BarrierDesc);

			// 命令を終了する.
			m_pCmdList->Close();

			//	コマンドリストを実行する.
			ID3D12CommandList* CmdList[] = { m_pCmdList };
			m_pCmdQueue->ExecuteCommandLists(
				1,			// 実行するコマンドリスト数. 
				CmdList);	// コマンドリストの先頭アドレス.

			m_pCmdQueue->ExecuteCommandLists(1, CmdList);
			m_pCmdQueue->Signal(Fence, ++FenceValue);


			if (Fence->GetCompletedValue() != FenceValue) {
				auto event = CreateEvent(nullptr, false, false, nullptr);
				Fence->SetEventOnCompletion(FenceValue, event);
				WaitForSingleObject(event, INFINITE);
				CloseHandle(event);
			}
			m_pCmdAllocator->Reset();				// キューをクリア.
			m_pCmdList->Reset(m_pCmdAllocator, nullptr);// 再びコマンドリストをためる準備.


			//フリップ
			m_pSwapChain->Present(1, 0);
		}
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
	DebugLayer->Release();
}
