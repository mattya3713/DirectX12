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
		SwapChainDesc.Width = WND_W;									// 画面の幅.
		SwapChainDesc.Height = WND_H;									// 画面の高さ.
		SwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;				// 表示形式.
		SwapChainDesc.Stereo = false;									// 全画面モードかどうか.
		SwapChainDesc.SampleDesc.Count = 1;								// ピクセル当たりのマルチサンプルの数.
		SwapChainDesc.SampleDesc.Quality = 0;							// 品質レベル(0~1).
		SwapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;				// ﾊﾞｯｸﾊﾞｯﾌｧのメモリ量.
		SwapChainDesc.BufferCount = 2;									// ﾊﾞｯｸﾊﾞｯﾌｧの数.
		SwapChainDesc.Scaling = DXGI_SCALING_STRETCH;					// ﾊﾞｯｸﾊﾞｯﾌｧのｻｲｽﾞがﾀｰｹﾞｯﾄと等しくない場合のｻｲｽﾞ変更の動作.
		SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;		// ﾌﾘｯﾌﾟ後は素早く破棄.
		SwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;			// ｽﾜｯﾌﾟﾁｪｰﾝ,ﾊﾞｯｸﾊﾞｯﾌｧの透過性の動作
		SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;	// ｽﾜｯﾌﾟﾁｪｰﾝ動作のｵﾌﾟｼｮﾝ(ｳｨﾝﾄﾞｳﾌﾙｽｸ切り替え可能ﾓｰﾄﾞ).

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


		// ﾃﾞｨｽｸﾘﾌﾟﾀﾋｰﾌﾟの先頭アドレスを取り出す.
		D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandle = RTVHeaps->GetCPUDescriptorHandleForHeapStart();

		//SRGBレンダーターゲットビュー設定
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		//↓これやると色味はだいぶマシになるが、バックバッファとの
		//フォーマットの食い違いによりDebugLayerにエラーが出力される
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

		// バックバッファをヒープの数分宣言.
		std::vector<ID3D12Resource*> BackBaffer(swcDesc.BufferCount);

		// バックバファの数分.
		for (int i = 0; i < static_cast<int>(swcDesc.BufferCount); ++i)
		{
			MyAssert::IsFailed(
				_T("%d個目のスワップチェーン内のバッファーとビューを関連づける"),
				&IDXGISwapChain4::GetBuffer, m_pSwapChain,
				static_cast<UINT>(i),
				IID_PPV_ARGS(&BackBaffer[i]));

			// レンダーターゲットビューを生成する.
			m_pDevice12->CreateRenderTargetView(
				BackBaffer[i],
				&rtvDesc,
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


		char Signature[3];
		PMDHeader Pmdheader = {};
		FILE* fp;
			
		auto err = fopen_s(&fp, "Data\\Model\\初音ミク.pmd", "rb");

		if (fp == nullptr) {
			char strerr[256];
			strerror_s(strerr, 256, err);
			return -1;
		}

		// シグネクチャ.
		fread(Signature, sizeof(Signature), 1, fp);
		// PMDヘッダー.
		fread(&Pmdheader, sizeof(Pmdheader), 1, fp);

		// 頂点数.
		unsigned int VertNum = 0;
		fread(&VertNum, sizeof(VertNum), 1, fp);

		std::string result = "VertNum : " + std::to_string(VertNum);

		OutputDebugStringA(result.c_str());

		std::vector<PMDVertex> Vertices(VertNum);//バッファ確保
		for (auto i = 0; i < VertNum; i++)
		{
			fread(&Vertices[i], PmdVertexSize, 1, fp);
		}

		// インデックス数.
		unsigned int IndicesNum;
		fread(&IndicesNum, sizeof(IndicesNum), 1, fp);

		auto HeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto ResDesc = CD3DX12_RESOURCE_DESC::Buffer(Vertices.size() * sizeof(PMDVertex));

		//UPLOAD(確保は可能)
		ID3D12Resource* vertBuff = nullptr;
		m_pDevice12->CreateCommittedResource(
			&HeapProp,
			D3D12_HEAP_FLAG_NONE,
			&ResDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&vertBuff));

		PMDVertex* vertMap = nullptr;
		vertBuff->Map(0, nullptr, (void**)&vertMap);
		std::copy(Vertices.begin(), Vertices.end(), vertMap);
		vertBuff->Unmap(0, nullptr);

		D3D12_VERTEX_BUFFER_VIEW vbView = {};
		vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();//バッファの仮想アドレス
		vbView.SizeInBytes = static_cast<UINT>(Vertices.size() * sizeof(PMDVertex));//全バイト数
		vbView.StrideInBytes = sizeof(PMDVertex);//1頂点あたりのバイト数

		std::vector<unsigned short> indices(IndicesNum);

		fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);
		fclose(fp);

		ID3D12Resource* idxBuff = nullptr;
		HeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		ResDesc = CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(indices[0]));
		//設定は、バッファのサイズ以外頂点バッファの設定を使いまわして
		//OKだと思います。
		m_pDevice12->CreateCommittedResource(
			&HeapProp,
			D3D12_HEAP_FLAG_NONE,
			&ResDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&idxBuff));

		//作ったバッファにインデックスデータをコピー
		unsigned short* mappedIdx = nullptr;
		idxBuff->Map(0, nullptr, (void**)&mappedIdx);
		std::copy(indices.begin(), indices.end(), mappedIdx);
		idxBuff->Unmap(0, nullptr);

		//インデックスバッファビューを作成
		D3D12_INDEX_BUFFER_VIEW ibView = {};
		ibView.BufferLocation = idxBuff->GetGPUVirtualAddress();
		ibView.Format = DXGI_FORMAT_R16_UINT;
		ibView.SizeInBytes = static_cast<UINT>(indices.size() * sizeof(indices[0]));

		// ブロブを作成(汎用的なデータの塊を表す型).
		ID3DBlob* VSBlob = nullptr;
		ID3DBlob* PSBlob = nullptr;

		// シェーダーのエラーハンドル.
		// MEMO : 詳細にエラーが出るのでMyAssertではなくBlobでエラーを取得する.
		ID3DBlob* ErrorBlob = nullptr;
		HRESULT Result = S_OK;

		Result = D3DCompileFromFile(
			_T("Data\\Shader\\Basic\\BasicVertexShader.hlsl"),	// ファイル名.
			nullptr, 											// シェーダーマクロオブジェクト.
			D3D_COMPILE_STANDARD_FILE_INCLUDE,					// インクルードオブジェクト
			"BasicVS", 											// エントリーポイント.
			"vs_5_0",											// どのシェーダーか.
			D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,	// シェーダコンパイルオプション.
			0, 													// エフェクトコンパイルオプション.
			&VSBlob,											// (Out)シェーダー受け取り.
			&ErrorBlob);										// (Out)エラー用ポインタ.

		// エラーチェック.
		ShaderCompileError(Result, ErrorBlob);

		MyAssert::IsFailed(
			_T("ピクセルシェーダーのコンパイル"),
			&D3DCompileFromFile,
			_T("Data\\Shader\\Basic\\BasicPixelShader.hlsl"),		// ファイル名.
			nullptr,												// シェーダーマクロオブジェクト.
			D3D_COMPILE_STANDARD_FILE_INCLUDE,						// インクルードオブジェクト.
			"BasicPS", 												// エントリーポイント.
			"ps_5_0",												// どのシェーダーか.
			D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,		// シェーダコンパイルオプション.
			0, 														// エフェクトコンパイルオプション.
			&PSBlob, 												// (Out)シェーダー受け取り.
			&ErrorBlob);											// (Out)エラー用ポインタ.

		// エラーチェック.
		ShaderCompileError(Result, ErrorBlob);

		// 頂点レイアウトを設定.
		D3D12_INPUT_ELEMENT_DESC InputLayout[] =
		{
			{	"POSITION",									// セマンティクス.
				0,											// 同じセマンティクス名の時に使うインデックス.
				DXGI_FORMAT_R32G32B32_FLOAT,				// フォーマット(要素数とビット数で型を表す).
				0,											// 入力スロットインデックス.
				D3D12_APPEND_ALIGNED_ELEMENT,				// データのオフセット位置.
				D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,	// 1つの入力スロットの入力データクラスを識別する型(D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATAでよい).
				0
			},

			{	"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
							   D3D12_APPEND_ALIGNED_ELEMENT,
							   D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
			},

			{	"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
							   D3D12_APPEND_ALIGNED_ELEMENT,
							   D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
			},

			{	"BONE_NO", 0, DXGI_FORMAT_R16G16_FLOAT, 0,
							   D3D12_APPEND_ALIGNED_ELEMENT,
							   D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
			},

			{	"WEIGHT", 0, DXGI_FORMAT_R8_UINT, 0,
							   D3D12_APPEND_ALIGNED_ELEMENT,
							   D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
			},

			{	"EDGE_FLG", 0, DXGI_FORMAT_R8_UINT, 0,
							   D3D12_APPEND_ALIGNED_ELEMENT,
							   D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
			},
		};

		// グラフィックパイプラインステート.
		D3D12_GRAPHICS_PIPELINE_STATE_DESC GraphicsPipelineState = {};
		GraphicsPipelineState.pRootSignature = nullptr;						// ルートシグネクチャのポインタ.
		GraphicsPipelineState.VS.pShaderBytecode = VSBlob->GetBufferPointer();	// バーテックスシェーダへのポインタ.
		GraphicsPipelineState.VS.BytecodeLength = VSBlob->GetBufferSize();		// バーテックスシェーダーのサイズ.
		GraphicsPipelineState.PS.pShaderBytecode = PSBlob->GetBufferPointer();	// ピクセルシェーダのポインタ.
		GraphicsPipelineState.PS.BytecodeLength = PSBlob->GetBufferSize();		// ピクセルシェーダのサイズ.

		GraphicsPipelineState.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;			// ブレンド状態のサンプルマスク.

		GraphicsPipelineState.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		GraphicsPipelineState.BlendState.RenderTarget->BlendEnable = true;
		GraphicsPipelineState.BlendState.RenderTarget->SrcBlend = D3D12_BLEND_SRC_ALPHA;
		GraphicsPipelineState.BlendState.RenderTarget->DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		GraphicsPipelineState.BlendState.RenderTarget->BlendOp = D3D12_BLEND_OP_ADD;

		// アルファのブレンドステート.
		// TODO : 本実装の時はこの構造体をいじってアルファの設定を行う.
		//D3D12_RENDER_TARGET_BLEND_DESC RenderTargetBlendDesc = {};

		////ひとまず加算や乗算やαブレンディングは使用しない
		//RenderTargetBlendDesc.BlendEnable = false;
		//RenderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		////ひとまず論理演算は使用しない
		//RenderTargetBlendDesc.LogicOpEnable = false;

		//GraphicsPipelineState.BlendState.RenderTarget[0] = RenderTargetBlendDesc;


		GraphicsPipelineState.RasterizerState.MultisampleEnable = false;				// まだアンチェリは使わない.
		GraphicsPipelineState.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;	// カリングしない.
		GraphicsPipelineState.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;// 中身を塗りつぶす.
		GraphicsPipelineState.RasterizerState.DepthClipEnable = true;					// 深度方向のクリッピングは有効に.

		//残り
		GraphicsPipelineState.RasterizerState.FrontCounterClockwise = false;
		GraphicsPipelineState.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		GraphicsPipelineState.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		GraphicsPipelineState.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		GraphicsPipelineState.RasterizerState.AntialiasedLineEnable = false;
		GraphicsPipelineState.RasterizerState.ForcedSampleCount = 0;
		GraphicsPipelineState.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;



		GraphicsPipelineState.DepthStencilState.DepthEnable = false;
		GraphicsPipelineState.DepthStencilState.StencilEnable = false;

		GraphicsPipelineState.InputLayout.pInputElementDescs = InputLayout;//レイアウト先頭アドレス
		GraphicsPipelineState.InputLayout.NumElements = _countof(InputLayout);//レイアウト配列数

		GraphicsPipelineState.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;//ストリップ時のカットなし
		GraphicsPipelineState.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//三角形で構成

		GraphicsPipelineState.NumRenderTargets = 1;//今は１つのみ
		GraphicsPipelineState.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;//0～1に正規化されたRGBA

		GraphicsPipelineState.SampleDesc.Count = 1;//サンプリングは1ピクセルにつき１
		GraphicsPipelineState.SampleDesc.Quality = 0;//クオリティは最低

		ID3D12RootSignature* Rootsignature = nullptr;
		D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc = {};
		RootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		D3D12_DESCRIPTOR_RANGE descTblRange[2] = {};//テクスチャと定数の２つ
		descTblRange[0].NumDescriptors = 1;//テクスチャひとつ
		descTblRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;//種別はテクスチャ
		descTblRange[0].BaseShaderRegister = 0;//0番スロットから
		descTblRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		descTblRange[1].NumDescriptors = 1;//定数ひとつ
		descTblRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;//種別は定数
		descTblRange[1].BaseShaderRegister = 0;//0番スロットから
		descTblRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		D3D12_ROOT_PARAMETER rootparam = {};
		rootparam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootparam.DescriptorTable.pDescriptorRanges = &descTblRange[0];//デスクリプタレンジのアドレス
		rootparam.DescriptorTable.NumDescriptorRanges = 2;//デスクリプタレンジ数
		rootparam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//全てのシェーダから見える

		RootSignatureDesc.pParameters = &rootparam;//ルートパラメータの先頭アドレス
		RootSignatureDesc.NumParameters = 1;//ルートパラメータ数

		D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//横繰り返し
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//縦繰り返し
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//奥行繰り返し
		samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;//ボーダーの時は黒
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;//補間しない(ニアレストネイバー)
		samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;//ミップマップ最大値
		samplerDesc.MinLOD = 0.0f;//ミップマップ最小値
		samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;//オーバーサンプリングの際リサンプリングしない？
		samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//ピクセルシェーダからのみ可視

		RootSignatureDesc.pStaticSamplers = &samplerDesc;
		RootSignatureDesc.NumStaticSamplers = 1;

		ID3DBlob* rootSigBlob = nullptr;
		// MEMO : 詳細にエラーが出るのでMyAssertではなくBlobでエラーを取得する.
		// ルート署名をシリアル化.
		Result = D3D12SerializeRootSignature(
			&RootSignatureDesc,				// ルート署名.
			D3D_ROOT_SIGNATURE_VERSION_1_0,	// バージョン指定.
			&rootSigBlob,					// (Out)ルート署名ブロブ.
			&ErrorBlob);					// (Out)エラー出力.

		// エラーチェック.
		ShaderCompileError(Result, ErrorBlob);

		MyAssert::IsFailed(
			_T(""),
			&ID3D12Device::CreateRootSignature, 
			m_pDevice12,
			0,
			rootSigBlob->GetBufferPointer(),
			rootSigBlob->GetBufferSize(),
			IID_PPV_ARGS(&Rootsignature));

		rootSigBlob->Release();

		GraphicsPipelineState.pRootSignature = Rootsignature;
		ID3D12PipelineState* _pipelinestate = nullptr;

		MyAssert::IsFailed(
			_T("グラフィックパイプラインの作成"),
			&ID3D12Device::CreateGraphicsPipelineState, m_pDevice12,
			&GraphicsPipelineState,
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

		//WICテクスチャのロード
		TexMetadata metadata = {};
		ScratchImage scratchImg = {};

		// 画像の読み出し.
		Result = LoadFromWICFile(L"Data\\Image\\textest200x200.png", WIC_FLAGS_NONE, &metadata, scratchImg);

		const Image* Image = scratchImg.GetImage(0, 0, 0);//生データ抽出

		// WriteToSubresourceで転送する用のヒープ設定
		D3D12_HEAP_PROPERTIES texHeapProp = {};
		texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;//特殊な設定なのでdefaultでもuploadでもなく
		texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;//ライトバックで
		texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;//転送がL0つまりCPU側から直で
		texHeapProp.CreationNodeMask = 0;//単一アダプタのため0
		texHeapProp.VisibleNodeMask = 0;//単一アダプタのため0

		//ResourceDesc = {};
		//ResourceDesc.Format = metadata.format;//RGBAフォーマットsrvDesc.Format と合わせなくて位はいけない
		//ResourceDesc.Width = static_cast<UINT>(metadata.width);//幅
		//ResourceDesc.Height = static_cast<UINT>(metadata.height);//高さ
		//ResourceDesc.DepthOrArraySize = static_cast<uint16_t>(metadata.arraySize);//2Dで配列でもないので１
		//ResourceDesc.SampleDesc.Count = 1;//通常テクスチャなのでアンチェリしない
		//ResourceDesc.SampleDesc.Quality = 0;//
		//ResourceDesc.MipLevels = static_cast<uint16_t>(metadata.mipLevels);//ミップマップしないのでミップ数は１つ
		//ResourceDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);//2Dテクスチャ用
		//ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;//レイアウトについては決定しない
		//ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;//とくにフラグなし

		ID3D12Resource* texbuff = nullptr;
		//MyAssert::IsFailed(
		//	_T(""),
		//	&ID3D12Device::CreateCommittedResource, m_pDevice12,
		//	&texHeapProp,
		//	D3D12_HEAP_FLAG_NONE,//特に指定なし
		//	&ResourceDesc,
		//	D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,//テクスチャ用(ピクセルシェーダから見る用)
		//	nullptr,
		//	IID_PPV_ARGS(&texbuff)
		//);

		//MyAssert::IsFailed(
		//	_T(""),
		//	&ID3D12Resource::WriteToSubresource, texbuff,
		//	0,
		//	nullptr,//全領域へコピー
		//	Image->pixels,//元データアドレス
		//	static_cast<UINT>(Image->rowPitch),//1ラインサイズ
		//	static_cast<UINT>(Image->slicePitch)//全サイズ
		//);

		

		ID3D12Resource* constBuff = nullptr;
		HeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		ResDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(XMMATRIX) + 0xff) & ~0xff);
		

		XMMATRIX* mapMatrix;//マップ先を示すポインタ

		//定数バッファ作成
		XMMATRIX worldMat = XMMatrixIdentity();
		XMFLOAT3 eye(0, 10, -15);
		XMFLOAT3 target(0, 10, 0);
		XMFLOAT3 up(0, 1, 0);
		auto viewMat = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
		auto projMat = XMMatrixPerspectiveFovLH(XM_PIDIV2,//画角は90°
			WND_WF / WND_HF,//アス比
			1.0f,//近い方
			100.0f//遠い方
		);


		//通常テクスチャビュー作成
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = metadata.format;//DXGI_FORMAT_R8G8B8A8_UNORM;//RGBA(0.0f～1.0fに正規化)
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;//後述
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
		srvDesc.Texture2D.MipLevels = 1;//ミップマップは使用しないので1

		ID3D12DescriptorHeap* basicDescHeap = nullptr;
		D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
		descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;//シェーダから見えるように
		descHeapDesc.NodeMask = 0;//マスクは0
		descHeapDesc.NumDescriptors = 1;//CBV1つ
		descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;//デスクリプタヒープ種別

		m_pDevice12->CreateCommittedResource(
			&HeapProp,
			D3D12_HEAP_FLAG_NONE,
			&ResDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&constBuff)
		);
		m_pDevice12->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&basicDescHeap));//生成

		////デスクリプタの先頭ハンドルを取得しておく
		auto basicHeapHandle = basicDescHeap->GetCPUDescriptorHandleForHeapStart();
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = constBuff->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = static_cast<UINT>(constBuff->GetDesc().Width);
		//定数バッファビューの作成
		m_pDevice12->CreateConstantBufferView(&cbvDesc, basicHeapHandle);
		static unsigned int frame = 0;

		while (true)
		{
			// 現在のバックバッファを取得
			auto BBIndex = m_pSwapChain->GetCurrentBackBufferIndex();

			// バックバッファをレンダーターゲットに遷移
			{
				const CD3DX12_RESOURCE_BARRIER Barrier = CD3DX12_RESOURCE_BARRIER::Transition(
					BackBaffer[BBIndex],
					D3D12_RESOURCE_STATE_PRESENT,
					D3D12_RESOURCE_STATE_RENDER_TARGET
				);

				m_pCmdList->ResourceBarrier(1, &Barrier);
			}

			m_pCmdList->SetPipelineState(_pipelinestate);

			// レンダーターゲットを指定.
			auto rtvH = RTVHeaps->GetCPUDescriptorHandleForHeapStart();
			rtvH.ptr += BBIndex * m_pDevice12->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			m_pCmdList->OMSetRenderTargets(1, &rtvH, false, nullptr);

			// 画面クリア.
			float r = (float)(0xff & frame >> 16) / 255.0f;
			float g = (float)(0xff & frame >> 8) / 255.0f;
			float b = (float)(0xff & frame >> 0) / 255.0f;
			float clearColor[] = { r, g, b, 1.0f };
			m_pCmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

			++frame; 

			m_pCmdList->RSSetViewports(1, &viewport);
			m_pCmdList->RSSetScissorRects(1, &scissorrect);
			m_pCmdList->SetGraphicsRootSignature(Rootsignature);

			m_pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			//m_pCmdList->IASetVertexBuffers(0, 1, &VerticeBufferView);
			m_pCmdList->IASetIndexBuffer(&ibView);

			m_pCmdList->SetGraphicsRootSignature(Rootsignature);
			m_pCmdList->SetDescriptorHeaps(1, &basicDescHeap);
			m_pCmdList->SetGraphicsRootDescriptorTable(0, basicDescHeap->GetGPUDescriptorHandleForHeapStart());

			// 描画実行
			m_pCmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);

			// バックバッファを表示状態に遷移
			{
				const CD3DX12_RESOURCE_BARRIER Barrier = CD3DX12_RESOURCE_BARRIER::Transition(
					BackBaffer[BBIndex],
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					D3D12_RESOURCE_STATE_PRESENT
				);

				m_pCmdList->ResourceBarrier(1, &Barrier);
			}
		
			// 命令のクローズ
			m_pCmdList->Close();

			// コマンドリストの実行
			ID3D12CommandList* cmdlists[] = { m_pCmdList };
			m_pCmdQueue->ExecuteCommandLists(1, cmdlists);

			// 待ち
			m_pCmdQueue->Signal(Fence, ++FenceValue);
			if (Fence->GetCompletedValue() != FenceValue) {
				auto event = CreateEvent(nullptr, false, false, nullptr);
				Fence->SetEventOnCompletion(FenceValue, event);
				WaitForSingleObject(event, INFINITE);
				CloseHandle(event);
			}

			// フリップ
			m_pSwapChain->Present(1, 0);

			m_pCmdAllocator->Reset(); // キューをクリア
			m_pCmdList->Reset(m_pCmdAllocator, _pipelinestate); // 再びコマンドリストをためる準備

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

// ErroeBlobに入ったエラーを出力.
void CDirectX12::ShaderCompileError(const HRESULT& Result, ID3DBlob* ErrorMsg)
{
	if (FAILED(Result)) {
		std::wstring errstr;

		if (Result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
			errstr = L"ファイルが見当たりません";
		}
		else {
			if (ErrorMsg) {
				// ErrorMsg があるの場合.
				errstr.resize(ErrorMsg->GetBufferSize());
				std::copy_n(static_cast<const char*>(ErrorMsg->GetBufferPointer()), ErrorMsg->GetBufferSize(), errstr.begin());
				errstr += L"\n";
			}
			else {
				// ErrorMsg がないの場合.
				errstr = L"ErrorMsg is null";
			}
		}

		// エラーメッセージをアサーションで表示.
		_ASSERT_EXPR(false, errstr.c_str());
	}
}
