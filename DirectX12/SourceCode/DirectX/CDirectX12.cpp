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

		XMFLOAT3 vertices[] = {
		{-0.4f,-0.7f,0.0f} ,//左下
		{-0.4f,0.7f,0.0f} ,//左上
		{0.4f,-0.7f,0.0f} ,//右下
		{0.4f,0.7f,0.0f} ,//右上
		};

		// ヒープのプロパティ.
		D3D12_HEAP_PROPERTIES heapprop = {};								
		heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;						// ヒープの種類.
		heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;	// CPUページプロパティ.
		heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;	// メモリプール.

		// テクスチャリソース.
		D3D12_RESOURCE_DESC resdesc = {};							
		resdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resdesc.Width = sizeof(vertices);
		resdesc.Height = 1;
		resdesc.DepthOrArraySize = 1;
		resdesc.MipLevels = 1;
		resdesc.Format = DXGI_FORMAT_UNKNOWN;
		resdesc.SampleDesc.Count = 1;
		resdesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		resdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;


		//UPLOAD(確保は可能)
		ID3D12Resource* vertBuff = nullptr;

			MyAssert::IsFailed(
			_T(""),
			&ID3D12Device::CreateCommittedResource, m_pDevice12,
			&heapprop,
			D3D12_HEAP_FLAG_NONE,
			&resdesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&vertBuff));

		XMFLOAT3* vertMap = nullptr;

			MyAssert::IsFailed(
				_T(""), 
				&ID3D12Resource::Map, vertBuff,
				0, 
				nullptr, 
				(void**)&vertMap);

		std::copy(std::begin(vertices), std::end(vertices), vertMap);

		vertBuff->Unmap(0, nullptr);

		D3D12_VERTEX_BUFFER_VIEW vbView = {};
		vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();//バッファの仮想アドレス
		vbView.SizeInBytes = sizeof(vertices);//全バイト数
		vbView.StrideInBytes = sizeof(vertices[0]);//1頂点あたりのバイト数

		unsigned short indices[] = { 0,1,2, 2,1,3 };

		ID3D12Resource* idxBuff = nullptr;
		//設定は、バッファのサイズ以外頂点バッファの設定を使いまわして
		//OKだと思います。
		resdesc.Width = sizeof(indices);
		MyAssert::IsFailed(
			_T(""),
			&ID3D12Device::CreateCommittedResource, m_pDevice12,
			&heapprop,
			D3D12_HEAP_FLAG_NONE,
			&resdesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&idxBuff));

		//作ったバッファにインデックスデータをコピー
		unsigned short* mappedIdx = nullptr;
		idxBuff->Map(0, nullptr, (void**)&mappedIdx);
		std::copy(std::begin(indices), std::end(indices), mappedIdx);
		idxBuff->Unmap(0, nullptr);

		//インデックスバッファビューを作成
		D3D12_INDEX_BUFFER_VIEW ibView = {};
		ibView.BufferLocation = idxBuff->GetGPUVirtualAddress();
		ibView.Format = DXGI_FORMAT_R16_UINT;
		ibView.SizeInBytes = sizeof(indices);

		ID3DBlob* _vsBlob = nullptr;
		ID3DBlob* _psBlob = nullptr;

		ID3DBlob* errorBlob = nullptr;
			MyAssert::IsFailed(
				_T(""), 
				&D3DCompileFromFile,
				_T("Data\\Shader\\Basic\\BasicVertexShader.hlsl"),
				nullptr, 
				D3D_COMPILE_STANDARD_FILE_INCLUDE,
				"BasicVS", 
				"vs_5_0",
				D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
				0, 
				&_vsBlob, 
				&errorBlob);

		//if (FAILED(result)) {
		//	if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
		//		::OutputDebugStringA("ファイルが見当たりません");
		//	}
		//	else {
		//		std::string errstr;
		//		errstr.resize(errorBlob->GetBufferSize());
		//		std::copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
		//		errstr += "\n";
		//		OutputDebugStringA(errstr.c_str());
		//	}
		//	exit(1);//行儀悪いかな…
		//}
		MyAssert::IsFailed(
			_T(""), 
			&D3DCompileFromFile,
			_T("Data\\Shader\\Basic\\BasicPixelShader.hlsl"),
			nullptr, 
			D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"BasicPS", 
			"ps_5_0",
			D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
			0, 
			&_psBlob, 
			&errorBlob);

		//if (FAILED(result)) {
		//	if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
		//		::OutputDebugStringA("ファイルが見当たりません");
		//	}
		//	else {
		//		std::string errstr;
		//		errstr.resize(errorBlob->GetBufferSize());
		//		std::copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
		//		errstr += "\n";
		//		OutputDebugStringA(errstr.c_str());
		//	}
		//	exit(1);//行儀悪いかな…
		//}
		D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
			{ "POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};
		gpipeline.pRootSignature = nullptr;
		gpipeline.VS.pShaderBytecode = _vsBlob->GetBufferPointer();
		gpipeline.VS.BytecodeLength = _vsBlob->GetBufferSize();
		gpipeline.PS.pShaderBytecode = _psBlob->GetBufferPointer();
		gpipeline.PS.BytecodeLength = _psBlob->GetBufferSize();

		gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;//中身は0xffffffff

		//
		gpipeline.BlendState.AlphaToCoverageEnable = false;
		gpipeline.BlendState.IndependentBlendEnable = false;

		D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};

		//ひとまず加算や乗算やαブレンディングは使用しない
		renderTargetBlendDesc.BlendEnable = false;
		renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		//ひとまず論理演算は使用しない
		renderTargetBlendDesc.LogicOpEnable = false;

		gpipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;


		gpipeline.RasterizerState.MultisampleEnable = false;//まだアンチェリは使わない
		gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;//カリングしない
		gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;//中身を塗りつぶす
		gpipeline.RasterizerState.DepthClipEnable = true;//深度方向のクリッピングは有効に

		//残り
		gpipeline.RasterizerState.FrontCounterClockwise = false;
		gpipeline.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		gpipeline.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		gpipeline.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		gpipeline.RasterizerState.AntialiasedLineEnable = false;
		gpipeline.RasterizerState.ForcedSampleCount = 0;
		gpipeline.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;


		gpipeline.DepthStencilState.DepthEnable = false;
		gpipeline.DepthStencilState.StencilEnable = false;

		gpipeline.InputLayout.pInputElementDescs = inputLayout;//レイアウト先頭アドレス
		gpipeline.InputLayout.NumElements = _countof(inputLayout);//レイアウト配列数

		gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;//ストリップ時のカットなし
		gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//三角形で構成

		gpipeline.NumRenderTargets = 1;//今は１つのみ
		gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;//0～1に正規化されたRGBA

		gpipeline.SampleDesc.Count = 1;//サンプリングは1ピクセルにつき１
		gpipeline.SampleDesc.Quality = 0;//クオリティは最低

		ID3D12RootSignature* rootsignature = nullptr;

		D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
		rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		ID3DBlob* rootSigBlob = nullptr;
		MyAssert::IsFailed(
			_T(""), 
			&D3D12SerializeRootSignature,
			&rootSignatureDesc, 
			D3D_ROOT_SIGNATURE_VERSION_1_0, 
			&rootSigBlob, 
			&errorBlob);

		MyAssert::IsFailed(
			_T(""), 
			&ID3D12Device::CreateRootSignature,
			m_pDevice12,
			0, 
			rootSigBlob->GetBufferPointer(), 
			rootSigBlob->GetBufferSize(), 
			IID_PPV_ARGS(&rootsignature));

		rootSigBlob->Release();

		gpipeline.pRootSignature = rootsignature;
		ID3D12PipelineState* _pipelinestate = nullptr;
		MyAssert::IsFailed(
			_T(""), 
			&ID3D12Device::CreateGraphicsPipelineState,
			m_pDevice12, 
			&gpipeline, 
			IID_PPV_ARGS(&_pipelinestate));

		D3D12_VIEWPORT viewport = {};
		viewport.Width = WND_WF;//出力先の幅(ピクセル数)
		viewport.Height = WND_HF;//出力先の高さ(ピクセル数)
		viewport.TopLeftX = 0;//出力先の左上座標X
		viewport.TopLeftY = 0;//出力先の左上座標Y
		viewport.MaxDepth = 1.0f;//深度最大値
		viewport.MinDepth = 0.0f;//深度最小値


		D3D12_RECT scissorrect = {};
		scissorrect.top = 0;//切り抜き上座標
		scissorrect.left = 0;//切り抜き左座標
		scissorrect.right = scissorrect.left + static_cast<LONG>(WND_W);//切り抜き右座標
		scissorrect.bottom = scissorrect.top + static_cast<LONG>(WND_H);//切り抜き下座標

		unsigned int frame = 0;

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

			//レンダーターゲットを指定
			auto rtvH = RTVHeaps->GetCPUDescriptorHandleForHeapStart();
			rtvH.ptr += BBIndex * m_pDevice12->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			m_pCmdList->OMSetRenderTargets(1, &rtvH, false, nullptr);

			//画面クリア

			float r, g, b;
			r = (float)(0xff & frame >> 16) / 255.0f;
			g = (float)(0xff & frame >> 8) / 255.0f;
			b = (float)(0xff & frame >> 0) / 255.0f;
			float clearColor[] = { r,g,b,1.0f };//黄色
			m_pCmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);
			++frame;
			m_pCmdList->RSSetViewports(1, &viewport);
			m_pCmdList->RSSetScissorRects(1, &scissorrect);
			m_pCmdList->SetGraphicsRootSignature(rootsignature);

			m_pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			m_pCmdList->IASetVertexBuffers(0, 1, &vbView);
			m_pCmdList->IASetIndexBuffer(&ibView);


			//m_pCmdList->DrawInstanced(4, 1, 0, 0);
			m_pCmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);

			BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
			m_pCmdList->ResourceBarrier(1, &BarrierDesc);

			//命令のクローズ
			m_pCmdList->Close();



			//コマンドリストの実行
			ID3D12CommandList* cmdlists[] = { m_pCmdList };
			m_pCmdQueue->ExecuteCommandLists(1, cmdlists);
			////待ち
			m_pCmdQueue->Signal(Fence, ++FenceValue);

			if (Fence->GetCompletedValue() != FenceValue) {
				auto event = CreateEvent(nullptr, false, false, nullptr);
				Fence->SetEventOnCompletion(FenceValue, event);
				WaitForSingleObject(event, INFINITE);
				CloseHandle(event);
			}
			m_pCmdAllocator->Reset();//キューをクリア
			m_pCmdList->Reset(m_pCmdAllocator, _pipelinestate);//再びコマンドリストをためる準備


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
