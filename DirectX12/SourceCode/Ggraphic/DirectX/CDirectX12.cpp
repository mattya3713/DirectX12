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
	, m_DepthClearValue	(  ) 
	, m_pFence			( nullptr )
	, m_FenceValue		( 0 )
	, m_pPipelineState	( nullptr )	
	, m_pRootSignature	( nullptr )
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

		// 深度バッファの作成.
		CreateDepthDesc(
			m_pDepthBuffer, 
			m_pDepthHeap);
		
		// フェンスの表示.
		CreateFance(
			m_pFence);

		// TODO : 仮置き　
		// テクスチャロードテーブルの作成.
		CreateTextureLoadTable();

		char signature[3];
		PMDHeader pmdheader = {};

		//string strModelPath = "Model/巡音ルカ.pmd";
		std::string strModelPath = "Model/初音ミク.pmd";
		FILE* fp;
		auto err = fopen_s(&fp, "Data\\Model\\初音ミク.pmd", "rb");
		if (fp == nullptr) {
			return -1;
		}
		fread(signature, sizeof(signature), 1, fp);
		fread(&pmdheader, sizeof(pmdheader), 1, fp);

		unsigned int vertNum;//頂点数
		fread(&vertNum, sizeof(vertNum), 1, fp);

		std::vector<PMDVertex> vertices(vertNum);//バッファ確保
		for (unsigned int i = 0; i < vertNum; i++)
		{
			fread(&vertices[i], PmdVertexSize, 1, fp);
		}

		unsigned int IndicesNum;//インデックス数
		fread(&IndicesNum, sizeof(IndicesNum), 1, fp);//

		//UPLOAD(確保は可能)
		auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(vertices.size() * sizeof(PMDVertex));
		ID3D12Resource* vertBuff = nullptr;

		HRESULT result;

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
		fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);//一気に読み込み

		unsigned int MaterialNum;//マテリアル数
		fread(&MaterialNum, sizeof(MaterialNum), 1, fp);
		std::vector<Material> Materials(MaterialNum);

		std::vector<ID3D12Resource*> textureResources(MaterialNum);
		std::vector<ID3D12Resource*> sphResources(MaterialNum);
		std::vector<ID3D12Resource*> spaResources(MaterialNum);
		std::vector<ID3D12Resource*> toonResources(MaterialNum);
		{
			std::vector<PMDMaterial> pmdMaterials(MaterialNum);
			fread(pmdMaterials.data(), pmdMaterials.size() * sizeof(PMDMaterial), 1, fp);
			//コピー
			for (int i = 0; i < pmdMaterials.size(); ++i) {
				Materials[i].IndicesNum = pmdMaterials[i].IndicesNum;
				Materials[i].Material.Diffuse = pmdMaterials[i].Diffuse;
				Materials[i].Material.Alpha = pmdMaterials[i].Alpha;
				Materials[i].Material.Specular = pmdMaterials[i].Specular;
				Materials[i].Material.Specularity = pmdMaterials[i].Specularity;
				Materials[i].Material.Ambient = pmdMaterials[i].Ambient;
				Materials[i].Additional.ToonIdx = pmdMaterials[i].ToonIdx;
			}

			for (int i = 0; i < pmdMaterials.size(); ++i) {
				//トゥーンリソースの読み込み
				std::string toonFilePath = "Data/Model/toon/";
				char toonFileName[16];
				sprintf_s(toonFileName, 16, "toon%02d.bmp", pmdMaterials[i].ToonIdx + 1);
				toonFilePath += toonFileName;
				toonResources[i] = LoadTextureFromFile(toonFilePath);

				if (strlen(pmdMaterials[i].TexFilePath) == 0) {
					textureResources[i] = nullptr;
					continue;
				}

				std::string texFileName = pmdMaterials[i].TexFilePath;
				std::string sphFileName = "";
				std::string spaFileName = "";
				if (count(texFileName.begin(), texFileName.end(), '*') > 0) {//スプリッタがある
					auto namepair = MyFilePath::SplitFileName(texFileName);
					if (MyFilePath::GetExtension(namepair.first) == "sph") {
						texFileName = namepair.second;
						sphFileName = namepair.first;
					}
					else if (MyFilePath::GetExtension(namepair.first) == "spa") {
						texFileName = namepair.second;
						spaFileName = namepair.first;
					}
					else {
						texFileName = namepair.first;
						if (MyFilePath::GetExtension(namepair.second) == "sph") {
							sphFileName = namepair.second;
						}
						else if (MyFilePath::GetExtension(namepair.second) == "spa") {
							spaFileName = namepair.second;
						}
					}
				}
				else {
					if (MyFilePath::GetExtension(pmdMaterials[i].TexFilePath) == "sph") {
						sphFileName = pmdMaterials[i].TexFilePath;
						texFileName = "";
					}
					else if (MyFilePath::GetExtension(pmdMaterials[i].TexFilePath) == "spa") {
						spaFileName = pmdMaterials[i].TexFilePath;
						texFileName = "";
					}
					else {
						texFileName = pmdMaterials[i].TexFilePath;
					}
				}
				//モデルとテクスチャパスからアプリケーションからのテクスチャパスを得る
				if (texFileName != "") {
					auto texFilePath = MyFilePath::GetTexPath(strModelPath, texFileName.c_str());
					textureResources[i] = LoadTextureFromFile(texFilePath);
				}
				if (sphFileName != "") {
					auto sphFilePath = MyFilePath::GetTexPath(strModelPath, sphFileName.c_str());
					sphResources[i] = LoadTextureFromFile(sphFilePath);
				}
				if (spaFileName != "") {
					auto spaFilePath = MyFilePath::GetTexPath(strModelPath, spaFileName.c_str());
					spaResources[i] = LoadTextureFromFile(spaFilePath);
				}


			}

		}
		fclose(fp);


		auto whiteTex = CreateWhiteTexture();
		auto blackTex = CreateBlackTexture();
		auto gradTex = CreateGrayGradationTexture();


		ID3D12Resource* idxBuff = nullptr;
		//設定は、バッファのサイズ以外頂点バッファの設定を使いまわして
		//OKだと思います。
		heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		resDesc = CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(indices[0]));
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

		//マテリアルバッファを作成
		auto MaterialBuffSize = sizeof(MaterialForHlsl);
		MaterialBuffSize = (MaterialBuffSize + 0xff) & ~0xff;
		heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		resDesc = CD3DX12_RESOURCE_DESC::Buffer(MaterialBuffSize * MaterialNum);//勿体ないけど仕方ないですね
		ID3D12Resource* MaterialBuff = nullptr;
		result = m_pDevice12->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&MaterialBuff)
		);

		//マップマテリアルにコピー
		char* mapMaterial = nullptr;
		result = MaterialBuff->Map(0, nullptr, (void**)&mapMaterial);
		for (auto& m : Materials) {
			*((MaterialForHlsl*)mapMaterial) = m.Material;//データコピー
			mapMaterial += MaterialBuffSize;//次のアライメント位置まで進める
		}
		MaterialBuff->Unmap(0, nullptr);


		ID3D12DescriptorHeap* MaterialDescHeap = nullptr;
		D3D12_DESCRIPTOR_HEAP_DESC MaterialDescHeapDesc = {};
		MaterialDescHeapDesc.NumDescriptors = MaterialNum * 5;//マテリアル数ぶん(定数1つ、テクスチャ3つ)
		MaterialDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		MaterialDescHeapDesc.NodeMask = 0;

		MaterialDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;//デスクリプタヒープ種別
		result = m_pDevice12->CreateDescriptorHeap(&MaterialDescHeapDesc, IID_PPV_ARGS(&MaterialDescHeap));//生成

		D3D12_CONSTANT_BUFFER_VIEW_DESC matCBVDesc = {};
		matCBVDesc.BufferLocation = MaterialBuff->GetGPUVirtualAddress();
		matCBVDesc.SizeInBytes = static_cast<UINT>(MaterialBuffSize);

		////通常テクスチャビュー作成
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;//後述
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
		srvDesc.Texture2D.MipLevels = 1;//ミップマップは使用しないので1

		auto matDescHeapH = MaterialDescHeap->GetCPUDescriptorHandleForHeapStart();
		auto incSize = m_pDevice12->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		for (size_t i = 0; i < MaterialNum; ++i) {
			//マテリアル固定バッファビュー
			m_pDevice12->CreateConstantBufferView(&matCBVDesc, matDescHeapH);
			matDescHeapH.ptr += incSize;
			matCBVDesc.BufferLocation += MaterialBuffSize;
			if (textureResources[i] == nullptr) {
				srvDesc.Format = whiteTex->GetDesc().Format;
				m_pDevice12->CreateShaderResourceView(whiteTex, &srvDesc, matDescHeapH);
			}
			else {
				srvDesc.Format = textureResources[i]->GetDesc().Format;
				m_pDevice12->CreateShaderResourceView(textureResources[i], &srvDesc, matDescHeapH);
			}
			matDescHeapH.ptr += incSize;

			if (sphResources[i] == nullptr) {
				srvDesc.Format = whiteTex->GetDesc().Format;
				m_pDevice12->CreateShaderResourceView(whiteTex, &srvDesc, matDescHeapH);
			}
			else {
				srvDesc.Format = sphResources[i]->GetDesc().Format;
				m_pDevice12->CreateShaderResourceView(sphResources[i], &srvDesc, matDescHeapH);
			}
			matDescHeapH.ptr += incSize;

			if (spaResources[i] == nullptr) {
				srvDesc.Format = blackTex->GetDesc().Format;
				m_pDevice12->CreateShaderResourceView(blackTex, &srvDesc, matDescHeapH);
			}
			else {
				srvDesc.Format = spaResources[i]->GetDesc().Format;
				m_pDevice12->CreateShaderResourceView(spaResources[i], &srvDesc, matDescHeapH);
			}
			matDescHeapH.ptr += incSize;


			if (toonResources[i] == nullptr) {
				srvDesc.Format = gradTex->GetDesc().Format;
				m_pDevice12->CreateShaderResourceView(gradTex, &srvDesc, matDescHeapH);
			}
			else {
				srvDesc.Format = toonResources[i]->GetDesc().Format;
				m_pDevice12->CreateShaderResourceView(toonResources[i], &srvDesc, matDescHeapH);
			}
			matDescHeapH.ptr += incSize;

		}

		// グラフィックパイプラインステートの設定.
		CreateGraphicPipeline(m_pPipelineState);

		D3D12_VIEWPORT viewport = {};
		viewport.Width =  WND_W;//出力先の幅(ピクセル数)
		viewport.Height = WND_H;//出力先の高さ(ピクセル数)
		viewport.TopLeftX = 0;//出力先の左上座標X
		viewport.TopLeftY = 0;//出力先の左上座標Y
		viewport.MaxDepth = 1.0f;//深度最大値
		viewport.MinDepth = 0.0f;//深度最小値


		D3D12_RECT scissorrect = {};
		scissorrect.top = 0;//切り抜き上座標
		scissorrect.left = 0;//切り抜き左座標
		scissorrect.right = scissorrect.left + WND_W;//切り抜き右座標
		scissorrect.bottom = scissorrect.top + WND_H;//切り抜き下座標

		//シェーダ側に渡すための基本的な環境データ
		struct SceneData {
			DirectX::XMMATRIX world;//ワールド行列
			DirectX::XMMATRIX view;//ビュープロジェクション行列
			DirectX::XMMATRIX proj;//
			DirectX::XMFLOAT3 eye;//視点座標
		};

		//定数バッファ作成
		DirectX::XMMATRIX worldMat = DirectX::XMMatrixIdentity();
		DirectX::XMFLOAT3 eye(0, 15, -15);
		DirectX::XMFLOAT3 target(0, 15, 0);
		DirectX::XMFLOAT3 up(0, 1, 0);
		auto viewMat = DirectX::XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
		auto projMat = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4,//画角は45°
			static_cast<float>(WND_W) / static_cast<float>(WND_H),//アス比
			1.0f,//近い方
			100.0f//遠い方
		);
		ID3D12Resource* constBuff = nullptr;
		heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		resDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(SceneData) + 0xff) & ~0xff);
		result = m_pDevice12->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&constBuff)
		);

		SceneData* mapScene = nullptr;//マップ先を示すポインタ
		result = constBuff->Map(0, nullptr, (void**)&mapScene);//マップ
		//行列の内容をコピー
		mapScene->world = worldMat;
		mapScene->view = viewMat;
		mapScene->proj = projMat;
		mapScene->eye = eye;
		ID3D12DescriptorHeap* basicDescHeap = nullptr;
		D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
		descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;//シェーダから見えるように
		descHeapDesc.NodeMask = 0;//マスクは0
		descHeapDesc.NumDescriptors = 1;//
		descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;//デスクリプタヒープ種別
		result = m_pDevice12->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&basicDescHeap));//生成

		////デスクリプタの先頭ハンドルを取得しておく
		auto basicHeapHandle = basicDescHeap->GetCPUDescriptorHandleForHeapStart();

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = constBuff->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = static_cast<UINT>(constBuff->GetDesc().Width);
		//定数バッファビューの作成
		m_pDevice12->CreateConstantBufferView(&cbvDesc, basicHeapHandle);

		MSG msg = {};
		unsigned int frame = 0;
		float angle = 0.0f;
		auto dsvH = m_pRenderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();
		while (true) {
			worldMat = DirectX::XMMatrixRotationY(angle);
			mapScene->world = worldMat;
			mapScene->view = viewMat;
			mapScene->proj = projMat;
			angle += 0.01f;

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

			auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_pBackBuffer[bbIdx].Get(),
				D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
			m_pCmdList->ResourceBarrier(1, &barrier);

			m_pCmdList->SetPipelineState(m_pPipelineState.Get());


			//レンダーターゲットを指定
			auto rtvH = m_pRenderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();
			rtvH.ptr += static_cast<ULONG_PTR>(bbIdx * m_pDevice12->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));

			m_pCmdList->OMSetRenderTargets(1, &rtvH, false, &dsvH);
			m_pCmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
			//画面クリア

			float clearColor[] = { 1.0f,1.0f,1.0f,1.0f };//白色
			m_pCmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

			m_pCmdList->RSSetViewports(1, &viewport);
			m_pCmdList->RSSetScissorRects(1, &scissorrect);

			m_pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			m_pCmdList->IASetVertexBuffers(0, 1, &vbView);
			m_pCmdList->IASetIndexBuffer(&ibView);

			m_pCmdList->SetGraphicsRootSignature(m_pRootSignature.Get());

			//WVP変換行列
			m_pCmdList->SetDescriptorHeaps(1, &basicDescHeap);
			m_pCmdList->SetGraphicsRootDescriptorTable(0, basicDescHeap->GetGPUDescriptorHandleForHeapStart());

			//マテリアル
			m_pCmdList->SetDescriptorHeaps(1, &MaterialDescHeap);

			auto MaterialH = MaterialDescHeap->GetGPUDescriptorHandleForHeapStart();
			unsigned int idxOffset = 0;

			auto cbvsrvIncSize = m_pDevice12->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;
			for (auto& m : Materials) {
				m_pCmdList->SetGraphicsRootDescriptorTable(1, MaterialH);
				m_pCmdList->DrawIndexedInstanced(m.IndicesNum, 1, idxOffset, 0, 0);
				MaterialH.ptr += cbvsrvIncSize;
				idxOffset += m.IndicesNum;
			}

			barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_pBackBuffer[bbIdx].Get(),
				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
			m_pCmdList->ResourceBarrier(1, &barrier);

			//命令のクローズ
			m_pCmdList->Close();



			//コマンドリストの実行
			ID3D12CommandList* cmdlists[] = { m_pCmdList.Get() };
			m_pCmdQueue->ExecuteCommandLists(1, cmdlists);
			////待ち
			++m_FenceValue;
			m_pCmdQueue->Signal(m_pFence.Get(), m_FenceValue);

			if (m_pFence->GetCompletedValue() != m_FenceValue) {
				auto event = CreateEvent(nullptr, false, false, nullptr);
				m_pFence->SetEventOnCompletion(m_FenceValue, event);
				WaitForSingleObjectEx(event, INFINITE, false);
				CloseHandle(event);
			}


			//フリップ
			m_pSwapChain->Present(0, 0);
			m_pCmdAllocator->Reset();//キューをクリア
			m_pCmdList->Reset(m_pCmdAllocator.Get(), m_pPipelineState.Get());//再びコマンドリストをためる準備

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

void CDirectX12::BeginDraw()
{
}

void CDirectX12::Draw()
{
}

void CDirectX12::EndDraw()
{
	
}

MyComPtr<IDXGISwapChain4> CDirectX12::GetSwapChain()
{
	return m_pSwapChain;
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
			_T("%d個目のスワップチェーン内のバッファーとビューを関連づける", i + 1),
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
	DepthResourceDesc.Width = WND_WF;									// 幅と高さはレンダーターゲットと同じ.
	DepthResourceDesc.Height = WND_HF;									// 上に同じ.
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
	LoadLambdaTable["sph"] =
		LoadLambdaTable["spa"] =
		LoadLambdaTable["bmp"] =
		LoadLambdaTable["png"] =
		LoadLambdaTable["jpg"] =
		[](const std::wstring& path, DirectX::TexMetadata* meta, DirectX::ScratchImage& img)->HRESULT {
		return LoadFromWICFile(path.c_str(), DirectX::WIC_FLAGS_NONE, meta, img);
		};

	LoadLambdaTable["tga"] = [](const std::wstring& path, DirectX::TexMetadata* meta, DirectX::ScratchImage& img)->HRESULT {
		return LoadFromTGAFile(path.c_str(), meta, img);
		};

	LoadLambdaTable["dds"] = [](const std::wstring& path, DirectX::TexMetadata* meta, DirectX::ScratchImage& img)->HRESULT {
		return LoadFromDDSFile(path.c_str(), DirectX::DDS_FLAGS_NONE, meta, img);
		};

}

ID3D12Resource* CDirectX12::CreateGrayGradationTexture()
{
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resDesc.Width = 4;//幅
	resDesc.Height = 256;//高さ
	resDesc.DepthOrArraySize = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;//
	resDesc.MipLevels = 1;//
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;//レイアウトについては決定しない
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;//とくにフラグなし

	D3D12_HEAP_PROPERTIES texHeapProp = {};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;//特殊な設定なのでdefaultでもuploadでもなく
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;//ライトバックで
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;//転送がL0つまりCPU側から直で
	texHeapProp.CreationNodeMask = 0;//単一アダプタのため0
	texHeapProp.VisibleNodeMask = 0;//単一アダプタのため0

	ID3D12Resource* gradBuff = nullptr;
	auto result = m_pDevice12->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,//特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&gradBuff)
	);
	if (FAILED(result)) {
		return nullptr;
	}

	//上が白くて下が黒いテクスチャデータを作成
	std::vector<unsigned int> data(4 * 256);
	auto it = data.begin();
	unsigned int c = 0xff;
	for (; it != data.end(); it += 4) {
		auto col = (0xff << 24) | RGB(c, c, c);//RGBAが逆並びしているためRGBマクロと0xff<<24を用いて表す。
		//auto col = (0xff << 24) | (c<<16)|(c<<8)|c;//これでもOK
		std::fill(it, it + 4, col);
		--c;
	}

	result = gradBuff->WriteToSubresource(0, nullptr, data.data(), 4 * sizeof(unsigned int), sizeof(unsigned int) * static_cast<UINT>(data.size()));
	return gradBuff;
}

ID3D12Resource* CDirectX12::CreateWhiteTexture() {
	D3D12_HEAP_PROPERTIES texHeapProp = {};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;//特殊な設定なのでdefaultでもuploadでもなく
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;//ライトバックで
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;//転送がL0つまりCPU側から直で
	texHeapProp.CreationNodeMask = 0;//単一アダプタのため0
	texHeapProp.VisibleNodeMask = 0;//単一アダプタのため0

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resDesc.Width = 4;//幅
	resDesc.Height = 4;//高さ
	resDesc.DepthOrArraySize = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;//
	resDesc.MipLevels = 1;//
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;//レイアウトについては決定しない
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;//とくにフラグなし

	ID3D12Resource* whiteBuff = nullptr;
	auto result = m_pDevice12->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,//特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&whiteBuff)
	);
	if (FAILED(result)) {
		return nullptr;
	}
	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0xff);

	result = whiteBuff->WriteToSubresource(0, nullptr, data.data(), 4 * 4, static_cast<UINT>(data.size()));
	return whiteBuff;
}

ID3D12Resource* CDirectX12::CreateBlackTexture() {
	D3D12_HEAP_PROPERTIES texHeapProp = {};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;//特殊な設定なのでdefaultでもuploadでもなく
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;//ライトバックで
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;//転送がL0つまりCPU側から直で
	texHeapProp.CreationNodeMask = 0;//単一アダプタのため0
	texHeapProp.VisibleNodeMask = 0;//単一アダプタのため0

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resDesc.Width = 4;//幅
	resDesc.Height = 4;//高さ
	resDesc.DepthOrArraySize = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;//
	resDesc.MipLevels = 1;//
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;//レイアウトについては決定しない
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;//とくにフラグなし

	ID3D12Resource* blackBuff = nullptr;
	auto result = m_pDevice12->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,//特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&blackBuff)
	);
	if (FAILED(result)) {
		return nullptr;
	}
	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0x00);

	result = blackBuff->WriteToSubresource(0, nullptr, data.data(), 4 * 4, static_cast<UINT>(data.size()));
	return blackBuff;
}

// テクスチャ読み込み.
ID3D12Resource* CDirectX12::LoadTextureFromFile(std::string& TexPath)
{
	auto it = _resourceTable.find(TexPath);
	if (it != _resourceTable.end()) {
		// テーブルに内にあったらロードするのではなくマップ内の.
		// リソースを返す.
		return _resourceTable[TexPath];
	}

	// WICテクスチャのロード.
	DirectX::TexMetadata MetaData = {};
	DirectX::ScratchImage ScratchImg = {};

	// テクスチャのファイルパス.
	auto wTexPath = MyString::StringToWString(TexPath);

	// 拡張子を取得.
	auto Extension = MyFilePath::GetExtension(TexPath);

	auto Result = LoadLambdaTable[Extension](wTexPath,
		&MetaData,
		ScratchImg);
	if (FAILED(Result)) {
		return nullptr;
	}

	// 生データ抽出.
	auto Img = ScratchImg.GetImage(0, 0, 0);	

	// WriteToSubresourceで転送する用のヒープ設定.
	D3D12_HEAP_PROPERTIES TexHeapProp = {};
	TexHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;							//　特殊な設定なのでdefaultでもuploadでもなく.
	TexHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;	//　ライトバックで.
	TexHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;			//　転送がL0つまりCPU側から直で.
	TexHeapProp.CreationNodeMask = 0;									//　単一アダプタのため0.
	TexHeapProp.VisibleNodeMask = 0;									//　単一アダプタのため0.

	D3D12_RESOURCE_DESC ResourceDesc = {};
	ResourceDesc.Format = MetaData.format;
	ResourceDesc.Width = static_cast<UINT>(MetaData.width);		// 幅.
	ResourceDesc.Height = static_cast<UINT>(MetaData.height);	// 高さ.
	ResourceDesc.DepthOrArraySize = static_cast<UINT16>(MetaData.arraySize);
	ResourceDesc.SampleDesc.Count = 1;													// 通常テクスチャなのでアンチェリしない.
	ResourceDesc.SampleDesc.Quality = 0;//
	ResourceDesc.MipLevels = static_cast<UINT16>(MetaData.mipLevels);					// ミップマップしないのでミップ数は１つ.
	ResourceDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(MetaData.dimension);
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;									// レイアウトについては決定しない.
	ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;										// とくにフラグなし.

	ID3D12Resource* Texbuff = nullptr;
	MyAssert::IsFailed(
		_T("テクスチャバッファの作成"),
		&ID3D12Device::CreateCommittedResource, m_pDevice12.Get(),
		&TexHeapProp,
		D3D12_HEAP_FLAG_NONE,//特に指定なし
		&ResourceDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&Texbuff)
	);

	MyAssert::IsFailed(
		_T("テクスチャバッファの作成"),
		&ID3D12Resource::WriteToSubresource, Texbuff,
		0,
		nullptr,								// 全領域へコピー.
		Img->pixels,							// 元データアドレス.
		static_cast<UINT>(Img->rowPitch),		// 1ラインサイズ.
		static_cast<UINT>(Img->slicePitch)		// 全サイズ.
	);

	_resourceTable[TexPath] = Texbuff;
	return Texbuff;
}

// グラフィックパイプラインステートの設定.
void CDirectX12::CreateGraphicPipeline(
	MyComPtr<ID3D12PipelineState>& GraphicPipelineState)
{
	ID3DBlob* VSBlob = nullptr;	// 頂点シェーダーのブロブ.
	ID3DBlob* PSBlob = nullptr;	// ピクセルシェーダーのブロブ.
	ID3DBlob* ErrerBlob = nullptr;	// ピクセルシェーダーのブロブ.
	HRESULT   result = S_OK;

	// 頂点シェーダーの読み込み.
	CompileShaderFromFile(
		L"Data\\Shader\\Basic\\BasicVertexShader.hlsl",
		"BasicVS", "vs_5_0",
		&VSBlob);


	CompileShaderFromFile(
		L"Data\\Shader\\Basic\\BasicPixelShader.hlsl",
		"BasicPS", "ps_5_0",
		&PSBlob);

	// TODO : 短くできそう.
	D3D12_INPUT_ELEMENT_DESC InputLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL"	, 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT	, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONE_NO"	, 0, DXGI_FORMAT_R16G16_UINT	, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "WEIGHT"	, 0, DXGI_FORMAT_R8_UINT		, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		//{ "EDGE_FLG",0,DXGI_FORMAT_R8_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
	};


	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};
	gpipeline.pRootSignature = nullptr;
	gpipeline.VS.pShaderBytecode = VSBlob->GetBufferPointer();
	gpipeline.VS.BytecodeLength = VSBlob->GetBufferSize();
	gpipeline.PS.pShaderBytecode = PSBlob->GetBufferPointer();
	gpipeline.PS.BytecodeLength = PSBlob->GetBufferSize();

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

	gpipeline.RasterizerState.FrontCounterClockwise = false;
	gpipeline.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	gpipeline.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	gpipeline.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	gpipeline.RasterizerState.AntialiasedLineEnable = false;
	gpipeline.RasterizerState.ForcedSampleCount = 0;
	gpipeline.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;


	gpipeline.DepthStencilState.DepthEnable = true;//深度バッファを使うぞ
	gpipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;//全て書き込み
	gpipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;//小さい方を採用
	gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	gpipeline.DepthStencilState.StencilEnable = false;

	gpipeline.InputLayout.pInputElementDescs = InputLayout;//レイアウト先頭アドレス
	gpipeline.InputLayout.NumElements = _countof(InputLayout);//レイアウト配列数

	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;//ストリップ時のカットなし
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//三角形で構成

	gpipeline.NumRenderTargets = 1;//今は１つのみ
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;//0～1に正規化されたRGBA

	gpipeline.SampleDesc.Count = 1;//サンプリングは1ピクセルにつき１
	gpipeline.SampleDesc.Quality = 0;//クオリティは最低

	ID3D12RootSignature* rootsignature = nullptr;
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	D3D12_DESCRIPTOR_RANGE descTblRange[3] = {};//テクスチャと定数の２つ


	//定数ひとつ目(座標変換用)
	descTblRange[0].NumDescriptors = 1;//定数ひとつ
	descTblRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;//種別は定数
	descTblRange[0].BaseShaderRegister = 0;//0番スロットから
	descTblRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//定数ふたつめ(マテリアル用)
	descTblRange[1].NumDescriptors = 1;//デスクリプタヒープはたくさんあるが一度に使うのは１つ
	descTblRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;//種別は定数
	descTblRange[1].BaseShaderRegister = 1;//1番スロットから
	descTblRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//テクスチャ1つ目(↑のマテリアルとペア)
	descTblRange[2].NumDescriptors = 4;//テクスチャ４つ(基本とsphとspaとトゥーン)
	descTblRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;//種別はテクスチャ
	descTblRange[2].BaseShaderRegister = 0;//0番スロットから
	descTblRange[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootparam[2] = {};
	rootparam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[0].DescriptorTable.pDescriptorRanges = &descTblRange[0];//デスクリプタレンジのアドレス
	rootparam[0].DescriptorTable.NumDescriptorRanges = 1;//デスクリプタレンジ数
	rootparam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//全てのシェーダから見える

	rootparam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[1].DescriptorTable.pDescriptorRanges = &descTblRange[1];//デスクリプタレンジのアドレス
	rootparam[1].DescriptorTable.NumDescriptorRanges = 2;//デスクリプタレンジ数←ここ
	rootparam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//ピクセルシェーダから見える

	rootSignatureDesc.pParameters = rootparam;//ルートパラメータの先頭アドレス
	rootSignatureDesc.NumParameters = 2;//ルートパラメータ数

	D3D12_STATIC_SAMPLER_DESC samplerDesc[2] = {};
	samplerDesc[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//横繰り返し
	samplerDesc[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//縦繰り返し
	samplerDesc[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//奥行繰り返し
	samplerDesc[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;//ボーダーの時は黒
	samplerDesc[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;//補間しない(ニアレストネイバー)
	samplerDesc[0].MaxLOD = D3D12_FLOAT32_MAX;//ミップマップ最大値
	samplerDesc[0].MinLOD = 0.0f;//ミップマップ最小値
	samplerDesc[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;//オーバーサンプリングの際リサンプリングしない？
	samplerDesc[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//ピクセルシェーダからのみ可視
	samplerDesc[0].ShaderRegister = 0;
	samplerDesc[1] = samplerDesc[0];//変更点以外をコピー
	samplerDesc[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;//
	samplerDesc[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc[1].ShaderRegister = 1;
	rootSignatureDesc.pStaticSamplers = samplerDesc;
	rootSignatureDesc.NumStaticSamplers = 2;

	ID3DBlob* rootSigBlob = nullptr;
	result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &ErrerBlob);
	result = m_pDevice12->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&rootsignature));
	rootSigBlob->Release();

	gpipeline.pRootSignature = rootsignature;
	ID3D12PipelineState* _pipelinestate = nullptr;
	result = m_pDevice12->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(&_pipelinestate));
}

// シェーダーのコンパイル.
HRESULT CDirectX12::CompileShaderFromFile(
	const std::wstring& FilePath, 
	LPCSTR EntryPoint, 
	LPCSTR Target, 
	ID3DBlob** ShaderBlob)
{
		ID3DBlob* ErrorBlob = nullptr;
		HRESULT Result = D3DCompileFromFile(
			FilePath.c_str(),
			nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
			EntryPoint, Target,
			D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // デバッグオプション.
			0, ShaderBlob, &ErrorBlob
		);

		// コンパイルエラー時にエラーハンドリングを行う.
		ShaderCompileError(Result, ErrorBlob);

		return Result;
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
		std::wstring ErrStr;

		if (Result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
			ErrStr = L"ファイルが見当たりません";
		}
		else {
			if (ErrorMsg) {
				// ErrorMsg があるの場合.
				ErrStr.resize(ErrorMsg->GetBufferSize());
				std::copy_n(static_cast<const char*>(ErrorMsg->GetBufferPointer()), ErrorMsg->GetBufferSize(), ErrStr.begin());
				ErrStr += L"\n";
			}
			else {
				// ErrorMsg がないの場合.
				ErrStr = L"ErrorMsg is null";
			}
		}
		if (ErrorMsg) {
			ErrorMsg->Release();  // メモリ解放
		}

		std::wstring WideErrorMsg(ErrStr);
		std::string FormattedMessage = MyAssert::FormatErrorMessage(WideErrorMsg, Result);
		throw std::runtime_error(FormattedMessage);
	}
}
