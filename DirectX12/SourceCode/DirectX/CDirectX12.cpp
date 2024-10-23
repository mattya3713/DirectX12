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
	m_hWnd = hWnd;

#if _DEBUG
	// デバッグレイヤーをオン.
	EnableDebuglayer();

#endif _DEBUG

	try {
		
		// DXGIの生成.
		CreateDXGIFactory();
	
		// コマンド類の生成.
		CreateCommandObject();

		// スワップチェインの生成.
		CreateSwapChain();

		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;//レンダーターゲットビューなので当然RTV
		heapDesc.NodeMask = 0;
		heapDesc.NumDescriptors = 2;//表裏の２つ
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;//特に指定なし
		ID3D12DescriptorHeap* rtvHeaps = nullptr;
		result = m_pDevice12->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps));
		DXGI_SWAP_CHAIN_DESC swcDesc = {};
		result = m_pSwapChain->GetDesc(&swcDesc);
		std::vector<ID3D12Resource*> _backBuffers(swcDesc.BufferCount);
		D3D12_CPU_DESCRIPTOR_HANDLE rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();

		//SRGBレンダーターゲットビュー設定
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;


		for (size_t i = 0; i < swcDesc.BufferCount; ++i) {
			result = m_pSwapChain->GetBuffer(static_cast<UINT>(i), IID_PPV_ARGS(&_backBuffers[i]));
			rtvDesc.Format = _backBuffers[i]->GetDesc().Format;
			m_pDevice12->CreateRenderTargetView(_backBuffers[i], &rtvDesc, rtvH);
			rtvH.ptr += m_pDevice12->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		}

		//深度バッファ作成
		//深度バッファの仕様
		D3D12_RESOURCE_DESC depthResDesc = {};
		depthResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;//2次元のテクスチャデータとして
		depthResDesc.Width = WND_WF;//幅と高さはレンダーターゲットと同じ
		depthResDesc.Height = WND_HF;//上に同じ
		depthResDesc.DepthOrArraySize = 1;//テクスチャ配列でもないし3Dテクスチャでもない
		depthResDesc.Format = DXGI_FORMAT_D32_FLOAT;//深度値書き込み用フォーマット
		depthResDesc.SampleDesc.Count = 1;//サンプルは1ピクセル当たり1つ
		depthResDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;//このバッファは深度ステンシルとして使用します
		depthResDesc.MipLevels = 1;
		depthResDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		depthResDesc.Alignment = 0;


		//デプス用ヒーププロパティ
		D3D12_HEAP_PROPERTIES depthHeapProp = {};
		depthHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;//DEFAULTだから後はUNKNOWNでよし
		depthHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		depthHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

		//このクリアバリューが重要な意味を持つ
		D3D12_CLEAR_VALUE _depthClearValue = {};
		_depthClearValue.DepthStencil.Depth = 1.0f;//深さ１(最大値)でクリア
		_depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;//32bit深度値としてクリア

		ID3D12Resource* depthBuffer = nullptr;
		result = m_pDevice12->CreateCommittedResource(
			&depthHeapProp,
			D3D12_HEAP_FLAG_NONE,
			&depthResDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE, //デプス書き込みに使用
			&_depthClearValue,
			IID_PPV_ARGS(&depthBuffer));

		//深度のためのデスクリプタヒープ作成
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};//深度に使うよという事がわかればいい
		dsvHeapDesc.NumDescriptors = 1;//深度ビュー1つのみ
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;//デプスステンシルビューとして使う
		ID3D12DescriptorHeap* dsvHeap = nullptr;
		result = m_pDevice12->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap));

		//深度ビュー作成
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;//デプス値に32bit使用
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;//2Dテクスチャ
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;//フラグは特になし
		m_pDevice12->CreateDepthStencilView(depthBuffer, &dsvDesc, dsvHeap->GetCPUDescriptorHandleForHeapStart());


		ID3D12Fence* _fence = nullptr;
		UINT64 _fenceVal = 0;
		result = m_pDevice12->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));

		ShowWindow(hWnd, SW_SHOW);//ウィンドウ表示

		//PMDヘッダ構造体
		struct PMDHeader {
			float version; //例：00 00 80 3F == 1.00
			char model_name[20];//モデル名
			char comment[256];//モデルコメント
		};
		char signature[3];
		PMDHeader pmdheader = {};
		FILE* fp;
		auto err = fopen_s(&fp, "Data\\Model\\初音ミク.pmd", "rb");
		if (fp == nullptr) {
			return -1;
		}
		fread(signature, sizeof(signature), 1, fp);
		fread(&pmdheader, sizeof(pmdheader), 1, fp);

		unsigned int vertNum;//頂点数
		fread(&vertNum, sizeof(vertNum), 1, fp);

		constexpr unsigned int pmdvertex_size = 38;//頂点1つあたりのサイズ
		std::vector<PMDVertex> vertices(vertNum);//バッファ確保
		for (auto i = 0; i < vertNum; i++)
		{
			fread(&vertices[i], pmdvertex_size, 1, fp);
		}

		unsigned int IndicesNum;//インデックス数
		fread(&IndicesNum, sizeof(IndicesNum), 1, fp);
		auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(vertices.size() * sizeof(PMDVertex));

		
		//UPLOAD(確保は可能)
		ID3D12Resource* vertBuff = nullptr;
		result = m_pDevice12->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&vertBuff));

		PMDVertex* vertMap = nullptr;
		result = vertBuff->Map(0, nullptr, (void**)&vertMap);
		std::copy(vertices.begin(), vertices.end(), vertMap);
		vertBuff->Unmap(0, nullptr);

		D3D12_VERTEX_BUFFER_VIEW vbView = {};
		vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();//バッファの仮想アドレス
		vbView.SizeInBytes = static_cast<UINT>(vertices.size() * sizeof(PMDVertex));//全バイト数
		vbView.StrideInBytes = sizeof(PMDVertex);//1頂点あたりのバイト数

		std::vector<unsigned short> indices(IndicesNum);

		fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);

		// マテリアルの読み込み.
		unsigned int MaterialNum;
		fread(&MaterialNum, sizeof(MaterialNum), 1, fp);

		std::vector<PMDMaterial> pmdMaterials(MaterialNum);
		fread(pmdMaterials.data(), pmdMaterials.size() * sizeof(PMDMaterial), 1, fp);

		std::vector<Material> Materials(MaterialNum);

		std::vector<ID3D12Resource*> textureResources(MaterialNum);
		std::vector<ID3D12Resource*> sphResources(MaterialNum);
		std::vector<ID3D12Resource*> spaResources(MaterialNum);
		std::vector<ID3D12Resource*> toonResources(MaterialNum);
		{
			std::vector<PMDMaterial> pmdMaterials(MaterialNum);
			fread(pmdMaterials.data(), pmdMaterials.size() * sizeof(PMDMaterial), 1, fp);

			// コピー.
			for (int i = 0; i < pmdMaterials.size(); ++i) {
				Materials[i].IndicesNum = pmdMaterials[i].IndiceNum;
				Materials[i].Material.Diffuse = pmdMaterials[i].Diffuse;
				Materials[i].Material.Alpha = pmdMaterials[i].Alpha;
				Materials[i].Material.Specular = pmdMaterials[i].Specular;
				Materials[i].Material.Specularity = pmdMaterials[i].Specularity;
				Materials[i].Material.Ambient = pmdMaterials[i].Ambient;
				Materials[i].Additional.ToonIdx = pmdMaterials[i].ToonIdx;
			}

			auto materialBuffSize = sizeof(MaterialForHlsl);
			materialBuffSize = (materialBuffSize + 0xff) & ~0xff;

			ID3D12Resource* materialBuff = nullptr;
		}

		fclose(fp);

		ID3D12Resource* idxBuff = nullptr;
		heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		resDesc = CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(indices[0]));
		//設定は、バッファのサイズ以外頂点バッファの設定を使いまわして
		//OKだと思います。
		result = m_pDevice12->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
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

		ID3DBlob* _vsBlob = nullptr;
		ID3DBlob* _psBlob = nullptr;

		ID3DBlob* errorBlob = nullptr;
		result = D3DCompileFromFile(L"Data\\Shader\\Basic\\BasicVertexShader.hlsl",
			nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"BasicVS", "vs_5_0",
			D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
			0, &_vsBlob, &errorBlob);
		if (FAILED(result)) {
			if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
				::OutputDebugStringA("ファイルが見当たりません");
			}
			else {
				std::string errstr;
				errstr.resize(errorBlob->GetBufferSize());
				std::copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
				errstr += "\n";
				OutputDebugStringA(errstr.c_str());
			}
			exit(1);//行儀悪いかな…
		}
		result = D3DCompileFromFile(L"Data\\Shader\\Basic\\BasicPixelShader.hlsl",
			nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"BasicPS", "ps_5_0",
			D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
			0, &_psBlob, &errorBlob);
		if (FAILED(result)) {
			if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
				::OutputDebugStringA("ファイルが見当たりません");
			}
			else {
				std::string errstr;
				errstr.resize(errorBlob->GetBufferSize());
				std::copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
				errstr += "\n";
				OutputDebugStringA(errstr.c_str());
			}
			exit(1);//行儀悪いかな…
		}
		D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
			{ "POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
			{ "NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
			{ "TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
			{ "BONE_NO",0,DXGI_FORMAT_R16G16_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
			{ "WEIGHT",0,DXGI_FORMAT_R8_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
			//{ "EDGE_FLG",0,DXGI_FORMAT_R8_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};
		gpipeline.pRootSignature = nullptr;
		gpipeline.VS.pShaderBytecode = _vsBlob->GetBufferPointer();
		gpipeline.VS.BytecodeLength = _vsBlob->GetBufferSize();
		gpipeline.PS.pShaderBytecode = _psBlob->GetBufferPointer();
		gpipeline.PS.BytecodeLength = _psBlob->GetBufferSize();

		gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;//中身は0xffffffff

		//ハルシェーダ、ドメインシェーダ、ジオメトリシェーダは設定しない
		gpipeline.HS.BytecodeLength = 0;
		gpipeline.HS.pShaderBytecode = nullptr;
		gpipeline.DS.BytecodeLength = 0;
		gpipeline.DS.pShaderBytecode = nullptr;
		gpipeline.GS.BytecodeLength = 0;
		gpipeline.GS.pShaderBytecode = nullptr;

		//ラスタライザ(RS)
		//gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		gpipeline.RasterizerState.FrontCounterClockwise = false;
		gpipeline.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		gpipeline.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		gpipeline.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		gpipeline.RasterizerState.DepthClipEnable = true;
		gpipeline.RasterizerState.MultisampleEnable = false;
		gpipeline.RasterizerState.AntialiasedLineEnable = false;
		gpipeline.RasterizerState.ForcedSampleCount = 0;
		gpipeline.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;//カリングしない
		gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//三角形

		//OutputMerger部分
		//レンダーターゲット
		gpipeline.NumRenderTargets = 1;//注)このターゲット数と設定するフォーマット数は
		gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;//一致させておく事

		//深度ステンシル
		gpipeline.DepthStencilState.DepthEnable = true;//深度
		gpipeline.DepthStencilState.StencilEnable = false;//あとで
		gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		gpipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		gpipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		//ブレンド設定
		//gpipeline.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		gpipeline.BlendState.AlphaToCoverageEnable = false;
		gpipeline.BlendState.IndependentBlendEnable = false;
		gpipeline.BlendState.RenderTarget->BlendEnable = true;
		gpipeline.BlendState.RenderTarget->SrcBlend = D3D12_BLEND_SRC_ALPHA;
		gpipeline.BlendState.RenderTarget->DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		gpipeline.BlendState.RenderTarget->BlendOp = D3D12_BLEND_OP_ADD;

		gpipeline.NodeMask = 0;
		gpipeline.SampleDesc.Count = 1;
		gpipeline.SampleDesc.Quality = 0;
		gpipeline.SampleMask = 0xffffffff;//全部対象
		gpipeline.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

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

		D3D12_DESCRIPTOR_RANGE descTblRange[1] = {};//テクスチャと定数の２つ
		descTblRange[0].NumDescriptors = 1;//定数ひとつ
		descTblRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;//種別は定数
		descTblRange[0].BaseShaderRegister = 0;//0番スロットから
		descTblRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		D3D12_ROOT_PARAMETER rootparam = {};
		rootparam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootparam.DescriptorTable.pDescriptorRanges = &descTblRange[0];//デスクリプタレンジのアドレス
		rootparam.DescriptorTable.NumDescriptorRanges = 1;//デスクリプタレンジ数
		rootparam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//全てのシェーダから見える


		rootSignatureDesc.pParameters = &rootparam;//ルートパラメータの先頭アドレス
		rootSignatureDesc.NumParameters = 1;//ルートパラメータ数

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

		rootSignatureDesc.pStaticSamplers = &samplerDesc;
		rootSignatureDesc.NumStaticSamplers = 1;

		ID3DBlob* rootSigBlob = nullptr;
		result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &errorBlob);
		result = m_pDevice12->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&rootsignature));
		rootSigBlob->Release();

		gpipeline.pRootSignature = rootsignature;
		ID3D12PipelineState* _pipelinestate = nullptr;
		result = m_pDevice12->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(&_pipelinestate));

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
		scissorrect.right = scissorrect.left + WND_WF;//切り抜き右座標
		scissorrect.bottom = scissorrect.top + WND_HF;//切り抜き下座標

		//シェーダ側に渡すための基本的な行列データ
		struct MatricesData {
			XMMATRIX world;
			XMMATRIX viewproj;
		};

		//定数バッファ作成
		XMMATRIX worldMat = XMMatrixIdentity();
		XMFLOAT3 eye(0, 10, -15);
		XMFLOAT3 target(0, 10, 0);
		XMFLOAT3 up(0, 1, 0);
		auto viewMat = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
		auto projMat = XMMatrixPerspectiveFovLH(XM_PIDIV2,//画角は90°
			static_cast<float>(WND_WF) / static_cast<float>(WND_HF),//アス比
			1.0f,//近い方
			100.0f//遠い方
		);
		heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		resDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(MatricesData) + 0xff) & ~0xff);
		ID3D12Resource* constBuff = nullptr;
		result = m_pDevice12->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&constBuff)
		);

		MatricesData* mapMatrix;//マップ先を示すポインタ
		result = constBuff->Map(0, nullptr, (void**)&mapMatrix);//マップ
		//行列の内容をコピー
		mapMatrix->world = worldMat;
		mapMatrix->viewproj = viewMat * projMat;

		ID3D12DescriptorHeap* basicDescHeap = nullptr;
		D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
		descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;//シェーダから見えるように
		descHeapDesc.NodeMask = 0;//マスクは0
		descHeapDesc.NumDescriptors = 1;//CBV1つ
		descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;//デスクリプタヒープ種別
		result = m_pDevice12->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&basicDescHeap));//生成

		////デスクリプタの先頭ハンドルを取得しておく
		auto basicHeapHandle = basicDescHeap->GetCPUDescriptorHandleForHeapStart();
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = constBuff->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = static_cast<UINT>(constBuff->GetDesc().Width);
		//定数バッファビューの作成
		m_pDevice12->CreateConstantBufferView(&cbvDesc, basicHeapHandle);

		//constBuff->Unmap(0, nullptr);


		MSG msg = {};
		unsigned int frame = 0;
		float angle = 0.0f;
		while (true) {
			worldMat = XMMatrixRotationY(angle);
			mapMatrix->world = worldMat;
			mapMatrix->viewproj = viewMat * projMat;
			angle += 0.005f;

			if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			//もうアプリケーションが終わるって時にmessageがWM_QUITになる
			if (msg.message == WM_QUIT) {
				break;
			}


			//DirectX処理
			//バックバッファのインデックスを取得
			auto bbIdx = m_pSwapChain->GetCurrentBackBufferIndex();

			D3D12_RESOURCE_BARRIER BarrierDesc = {};
			BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			BarrierDesc.Transition.pResource = _backBuffers[bbIdx];
			BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
			BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

			m_pCmdList->SetPipelineState(_pipelinestate);

			//レンダーターゲットを指定
			auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
			rtvH.ptr += bbIdx * m_pDevice12->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			auto dsvH = dsvHeap->GetCPUDescriptorHandleForHeapStart();

			m_pCmdList->ResourceBarrier(1, &BarrierDesc);
			m_pCmdList->OMSetRenderTargets(1, &rtvH, false, &dsvH);
			float clearColor[] = { 1.0f,1.0f,1.0f,1.0f };//白色
			m_pCmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);
			m_pCmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

			m_pCmdList->RSSetViewports(1, &viewport);
			m_pCmdList->RSSetScissorRects(1, &scissorrect);

			m_pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			m_pCmdList->IASetVertexBuffers(0, 1, &vbView);
			m_pCmdList->IASetIndexBuffer(&ibView);

			m_pCmdList->SetGraphicsRootSignature(rootsignature);
			m_pCmdList->SetDescriptorHeaps(1, &basicDescHeap);
			m_pCmdList->SetGraphicsRootDescriptorTable(0, basicDescHeap->GetGPUDescriptorHandleForHeapStart());

			m_pCmdList->DrawIndexedInstanced(IndicesNum, 1, 0, 0, 0);

			BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

			m_pCmdList->ResourceBarrier(1, &BarrierDesc);

			//命令のクローズ
			m_pCmdList->Close();



			//コマンドリストの実行
			ID3D12CommandList* cmdlists[] = { m_pCmdList };
			m_pCmdQueue->ExecuteCommandLists(1, cmdlists);
			////待ち
			m_pCmdQueue->Signal(_fence, ++_fenceVal);

			while (_fence->GetCompletedValue() != _fenceVal) {
				;
			}


			//フリップ
			m_pSwapChain->Present(1, 0);
			m_pCmdAllocator->Reset();//キューをクリア
			m_pCmdList->Reset(m_pCmdAllocator, nullptr);//再びコマンドリストをためる準備


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

// DXGIの生成.
void CDirectX12::CreateDXGIFactory()
{
#ifdef _DEBUG
	MyAssert::IsFailed(
		_T("DXGIの生成"),
		&CreateDXGIFactory2,
		DXGI_CREATE_FACTORY_DEBUG,			// デバッグモード.
		IID_PPV_ARGS(&m_pDxgiFactory));		// (Out)DXGI.
#else // _DEBUG
	MyAssert::IsFailed(
		_T("DXGIの生成"),
		&CreateDXGIFactory1,
		IID_PPV_ARGS(&m_pDxgiFactory));
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
			IID_PPV_ARGS(&m_pDevice12)) == S_OK)// (Out)Direct12.
		{
			// フィーチャーレベル.
			FeatureLevel = Lv;
			break;
		}
	}
}

void CDirectX12::CreateCommandObject()
{
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
}

// スワップチェーンの作成.
void CDirectX12::CreateSwapChain()
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
		&IDXGIFactory2::CreateSwapChainForHwnd, m_pDxgiFactory,
		m_pCmdQueue,									// コマンドキュー.
		m_hWnd,											// ウィンドウハンドル.
		&SwapChainDesc,									// スワップチェーン設定.
		nullptr,										// ひとまずnullotrでよい.TODO : なにこれ
		nullptr,										// これもnulltrでよう
		(IDXGISwapChain1**)&m_pSwapChain);				// (Out)スワップチェーン.
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
