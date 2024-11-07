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
	// �f�o�b�O���C���[���I��.
	EnableDebuglayer();

#endif _DEBUG

	try {
		
		// DXGI�̐���.
		CreateDXGIFactory(
			m_pDxgiFactory);
	
		// �R�}���h�ނ̐���.
		CreateCommandObject(
			m_pCmdAllocator,
			m_pCmdList,
			m_pCmdQueue);
		
		// �X���b�v�`�F�[���̐���.
		CreateSwapChain(
			m_pSwapChain);

		// �����_�[�^�[�Q�b�g�̍쐬.
		CreateRenderTarget(
			m_pRenderTargetViewHeap,
			m_pBackBuffer);

		// �[�x�o�b�t�@�̍쐬.
		CreateDepthDesc(
			m_pDepthBuffer, 
			m_pDepthHeap);
		
		// �t�F���X�̕\��.
		CreateFance(
			m_pFence);

		// TODO : ���u���@
		// �e�N�X�`�����[�h�e�[�u���̍쐬.
		CreateTextureLoadTable();

		char signature[3];
		PMDHeader pmdheader = {};

		//string strModelPath = "Model/�������J.pmd";
		std::string strModelPath = "Model/�����~�N.pmd";
		FILE* fp;
		auto err = fopen_s(&fp, "Data\\Model\\�����~�N.pmd", "rb");
		if (fp == nullptr) {
			return -1;
		}
		fread(signature, sizeof(signature), 1, fp);
		fread(&pmdheader, sizeof(pmdheader), 1, fp);

		unsigned int vertNum;//���_��
		fread(&vertNum, sizeof(vertNum), 1, fp);

		std::vector<PMDVertex> vertices(vertNum);//�o�b�t�@�m��
		for (unsigned int i = 0; i < vertNum; i++)
		{
			fread(&vertices[i], PmdVertexSize, 1, fp);
		}

		unsigned int IndicesNum;//�C���f�b�N�X��
		fread(&IndicesNum, sizeof(IndicesNum), 1, fp);//

		//UPLOAD(�m�ۂ͉\)
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
		vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();//�o�b�t�@�̉��z�A�h���X
		vbView.SizeInBytes = static_cast<UINT>(vertices.size() * sizeof(PMDVertex));//�S�o�C�g��
		vbView.StrideInBytes = sizeof(PMDVertex);//1���_������̃o�C�g��

		std::vector<unsigned short> indices(IndicesNum);
		fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);//��C�ɓǂݍ���

		unsigned int MaterialNum;//�}�e���A����
		fread(&MaterialNum, sizeof(MaterialNum), 1, fp);
		std::vector<Material> Materials(MaterialNum);

		std::vector<ID3D12Resource*> textureResources(MaterialNum);
		std::vector<ID3D12Resource*> sphResources(MaterialNum);
		std::vector<ID3D12Resource*> spaResources(MaterialNum);
		std::vector<ID3D12Resource*> toonResources(MaterialNum);
		{
			std::vector<PMDMaterial> pmdMaterials(MaterialNum);
			fread(pmdMaterials.data(), pmdMaterials.size() * sizeof(PMDMaterial), 1, fp);
			//�R�s�[
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
				//�g�D�[�����\�[�X�̓ǂݍ���
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
				if (count(texFileName.begin(), texFileName.end(), '*') > 0) {//�X�v���b�^������
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
				//���f���ƃe�N�X�`���p�X����A�v���P�[�V��������̃e�N�X�`���p�X�𓾂�
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
		//�ݒ�́A�o�b�t�@�̃T�C�Y�ȊO���_�o�b�t�@�̐ݒ���g���܂킵��
		//OK���Ǝv���܂��B
		heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		resDesc = CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(indices[0]));
		result = m_pDevice12->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&idxBuff));

		//������o�b�t�@�ɃC���f�b�N�X�f�[�^���R�s�[
		unsigned short* mappedIdx = nullptr;
		idxBuff->Map(0, nullptr, (void**)&mappedIdx);
		std::copy(indices.begin(), indices.end(), mappedIdx);
		idxBuff->Unmap(0, nullptr);

		//�C���f�b�N�X�o�b�t�@�r���[���쐬
		D3D12_INDEX_BUFFER_VIEW ibView = {};
		ibView.BufferLocation = idxBuff->GetGPUVirtualAddress();
		ibView.Format = DXGI_FORMAT_R16_UINT;
		ibView.SizeInBytes = static_cast<UINT>(indices.size() * sizeof(indices[0]));

		//�}�e���A���o�b�t�@���쐬
		auto MaterialBuffSize = sizeof(MaterialForHlsl);
		MaterialBuffSize = (MaterialBuffSize + 0xff) & ~0xff;
		heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		resDesc = CD3DX12_RESOURCE_DESC::Buffer(MaterialBuffSize * MaterialNum);//�ܑ̂Ȃ����ǎd���Ȃ��ł���
		ID3D12Resource* MaterialBuff = nullptr;
		result = m_pDevice12->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&MaterialBuff)
		);

		//�}�b�v�}�e���A���ɃR�s�[
		char* mapMaterial = nullptr;
		result = MaterialBuff->Map(0, nullptr, (void**)&mapMaterial);
		for (auto& m : Materials) {
			*((MaterialForHlsl*)mapMaterial) = m.Material;//�f�[�^�R�s�[
			mapMaterial += MaterialBuffSize;//���̃A���C�����g�ʒu�܂Ői�߂�
		}
		MaterialBuff->Unmap(0, nullptr);


		ID3D12DescriptorHeap* MaterialDescHeap = nullptr;
		D3D12_DESCRIPTOR_HEAP_DESC MaterialDescHeapDesc = {};
		MaterialDescHeapDesc.NumDescriptors = MaterialNum * 5;//�}�e���A�����Ԃ�(�萔1�A�e�N�X�`��3��)
		MaterialDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		MaterialDescHeapDesc.NodeMask = 0;

		MaterialDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;//�f�X�N���v�^�q�[�v���
		result = m_pDevice12->CreateDescriptorHeap(&MaterialDescHeapDesc, IID_PPV_ARGS(&MaterialDescHeap));//����

		D3D12_CONSTANT_BUFFER_VIEW_DESC matCBVDesc = {};
		matCBVDesc.BufferLocation = MaterialBuff->GetGPUVirtualAddress();
		matCBVDesc.SizeInBytes = static_cast<UINT>(MaterialBuffSize);

		////�ʏ�e�N�X�`���r���[�쐬
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;//��q
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2D�e�N�X�`��
		srvDesc.Texture2D.MipLevels = 1;//�~�b�v�}�b�v�͎g�p���Ȃ��̂�1

		auto matDescHeapH = MaterialDescHeap->GetCPUDescriptorHandleForHeapStart();
		auto incSize = m_pDevice12->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		for (size_t i = 0; i < MaterialNum; ++i) {
			//�}�e���A���Œ�o�b�t�@�r���[
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

		// �O���t�B�b�N�p�C�v���C���X�e�[�g�̐ݒ�.
		CreateGraphicPipeline(m_pPipelineState);

		D3D12_VIEWPORT viewport = {};
		viewport.Width =  WND_W;//�o�͐�̕�(�s�N�Z����)
		viewport.Height = WND_H;//�o�͐�̍���(�s�N�Z����)
		viewport.TopLeftX = 0;//�o�͐�̍�����WX
		viewport.TopLeftY = 0;//�o�͐�̍�����WY
		viewport.MaxDepth = 1.0f;//�[�x�ő�l
		viewport.MinDepth = 0.0f;//�[�x�ŏ��l


		D3D12_RECT scissorrect = {};
		scissorrect.top = 0;//�؂蔲������W
		scissorrect.left = 0;//�؂蔲�������W
		scissorrect.right = scissorrect.left + WND_W;//�؂蔲���E���W
		scissorrect.bottom = scissorrect.top + WND_H;//�؂蔲�������W

		//�V�F�[�_���ɓn�����߂̊�{�I�Ȋ��f�[�^
		struct SceneData {
			DirectX::XMMATRIX world;//���[���h�s��
			DirectX::XMMATRIX view;//�r���[�v���W�F�N�V�����s��
			DirectX::XMMATRIX proj;//
			DirectX::XMFLOAT3 eye;//���_���W
		};

		//�萔�o�b�t�@�쐬
		DirectX::XMMATRIX worldMat = DirectX::XMMatrixIdentity();
		DirectX::XMFLOAT3 eye(0, 15, -15);
		DirectX::XMFLOAT3 target(0, 15, 0);
		DirectX::XMFLOAT3 up(0, 1, 0);
		auto viewMat = DirectX::XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
		auto projMat = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4,//��p��45��
			static_cast<float>(WND_W) / static_cast<float>(WND_H),//�A�X��
			1.0f,//�߂���
			100.0f//������
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

		SceneData* mapScene = nullptr;//�}�b�v��������|�C���^
		result = constBuff->Map(0, nullptr, (void**)&mapScene);//�}�b�v
		//�s��̓��e���R�s�[
		mapScene->world = worldMat;
		mapScene->view = viewMat;
		mapScene->proj = projMat;
		mapScene->eye = eye;
		ID3D12DescriptorHeap* basicDescHeap = nullptr;
		D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
		descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;//�V�F�[�_���猩����悤��
		descHeapDesc.NodeMask = 0;//�}�X�N��0
		descHeapDesc.NumDescriptors = 1;//
		descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;//�f�X�N���v�^�q�[�v���
		result = m_pDevice12->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&basicDescHeap));//����

		////�f�X�N���v�^�̐擪�n���h�����擾���Ă���
		auto basicHeapHandle = basicDescHeap->GetCPUDescriptorHandleForHeapStart();

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = constBuff->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = static_cast<UINT>(constBuff->GetDesc().Width);
		//�萔�o�b�t�@�r���[�̍쐬
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
			//�����A�v���P�[�V�������I�����Ď���message��WM_QUIT�ɂȂ�
			if (msg.message == WM_QUIT) {
				break;
			}

			//DirectX����
			//�o�b�N�o�b�t�@�̃C���f�b�N�X���擾
			auto bbIdx = m_pSwapChain->GetCurrentBackBufferIndex();

			auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_pBackBuffer[bbIdx].Get(),
				D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
			m_pCmdList->ResourceBarrier(1, &barrier);

			m_pCmdList->SetPipelineState(m_pPipelineState.Get());


			//�����_�[�^�[�Q�b�g���w��
			auto rtvH = m_pRenderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();
			rtvH.ptr += static_cast<ULONG_PTR>(bbIdx * m_pDevice12->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));

			m_pCmdList->OMSetRenderTargets(1, &rtvH, false, &dsvH);
			m_pCmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
			//��ʃN���A

			float clearColor[] = { 1.0f,1.0f,1.0f,1.0f };//���F
			m_pCmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

			m_pCmdList->RSSetViewports(1, &viewport);
			m_pCmdList->RSSetScissorRects(1, &scissorrect);

			m_pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			m_pCmdList->IASetVertexBuffers(0, 1, &vbView);
			m_pCmdList->IASetIndexBuffer(&ibView);

			m_pCmdList->SetGraphicsRootSignature(m_pRootSignature.Get());

			//WVP�ϊ��s��
			m_pCmdList->SetDescriptorHeaps(1, &basicDescHeap);
			m_pCmdList->SetGraphicsRootDescriptorTable(0, basicDescHeap->GetGPUDescriptorHandleForHeapStart());

			//�}�e���A��
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

			//���߂̃N���[�Y
			m_pCmdList->Close();



			//�R�}���h���X�g�̎��s
			ID3D12CommandList* cmdlists[] = { m_pCmdList.Get() };
			m_pCmdQueue->ExecuteCommandLists(1, cmdlists);
			////�҂�
			++m_FenceValue;
			m_pCmdQueue->Signal(m_pFence.Get(), m_FenceValue);

			if (m_pFence->GetCompletedValue() != m_FenceValue) {
				auto event = CreateEvent(nullptr, false, false, nullptr);
				m_pFence->SetEventOnCompletion(m_FenceValue, event);
				WaitForSingleObjectEx(event, INFINITE, false);
				CloseHandle(event);
			}


			//�t���b�v
			m_pSwapChain->Present(0, 0);
			m_pCmdAllocator->Reset();//�L���[���N���A
			m_pCmdList->Reset(m_pCmdAllocator.Get(), m_pPipelineState.Get());//�ĂуR�}���h���X�g�����߂鏀��

		}
	}
	catch(const std::runtime_error& Msg) {

		// �G���[���b�Z�[�W��\��.
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

// DXGI�̐���.
void CDirectX12::CreateDXGIFactory(MyComPtr<IDXGIFactory6>& DxgiFactory)
{
#ifdef _DEBUG
	MyAssert::IsFailed(
		_T("DXGI�̐���"),
		&CreateDXGIFactory2,
		DXGI_CREATE_FACTORY_DEBUG,			// �f�o�b�O���[�h.
		IID_PPV_ARGS(DxgiFactory.ReleaseAndGetAddressOf()));		// (Out)DXGI.
#else // _DEBUG
	MyAssert::IsFailed(
		_T("DXGI�̐���"),
		&CreateDXGIFactory1,
		IID_PPV_ARGS(m_pDxgiFactory.ReleaseAndGetAddressOf()));
#endif

	// �t�B�[�`�����x����.
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
		// DirectX12�����̉�.
		if (D3D12CreateDevice(
			FindAdapter(L"NVIDIA"),				// �O���{��I��.
			Lv,									// �t�B�[�`���[���x��.
			IID_PPV_ARGS(m_pDevice12.ReleaseAndGetAddressOf())) == S_OK)// (Out)Direct12.
		{
			// �t�B�[�`���[���x��.
			FeatureLevel = Lv;
			break;
		}
	}
}

// �R�}���h�ނ̐���.
void CDirectX12::CreateCommandObject(
	MyComPtr<ID3D12CommandAllocator>&	CmdAllocator,
	MyComPtr<ID3D12GraphicsCommandList>&CmdList,
	MyComPtr<ID3D12CommandQueue>&		CmdQueue)
{
	MyAssert::IsFailed(
		_T("�R�}���h���X�g�A���P�[�^�[�̐���"),
		&ID3D12Device::CreateCommandAllocator, m_pDevice12.Get(),
		D3D12_COMMAND_LIST_TYPE_DIRECT,			// �쐬����R�}���h�A���P�[�^�̎��.
		IID_PPV_ARGS(CmdAllocator.ReleaseAndGetAddressOf()));		// (Out) �R�}���h�A���P�[�^.

	MyAssert::IsFailed(
		_T("�R�}���h���X�g�̐���"),
		&ID3D12Device::CreateCommandList, m_pDevice12.Get(),
		0,									// �P���GPU����̏ꍇ��0.
		D3D12_COMMAND_LIST_TYPE_DIRECT,		// �쐬����R�}���h ���X�g�̎��.
		CmdAllocator.Get(),				// �A���P�[�^�ւ̃|�C���^.
		nullptr,							// �_�~�[�̏����p�C�v���C�����ݒ肳���?
		IID_PPV_ARGS(CmdList.ReleaseAndGetAddressOf()));				// (Out) �R�}���h���X�g.

	// �R�}���h�L���[�\���̂̍쐬.
	D3D12_COMMAND_QUEUE_DESC CmdQueueDesc = {};
	CmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;				// �^�C���A�E�g�Ȃ�.
	CmdQueueDesc.NodeMask = 0;										// �A�_�v�^�[��������g��Ȃ��Ƃ���0�ł���.
	CmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;	// �v���C�I���e�B�͓��Ɏw��Ȃ�.
	CmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;				// �R�}���h���X�g�ƍ��킹��.

	MyAssert::IsFailed(
		_T("�L���[�̍쐬"),
		&ID3D12Device::CreateCommandQueue, m_pDevice12.Get(),
		&CmdQueueDesc,
		IID_PPV_ARGS(CmdQueue.ReleaseAndGetAddressOf()));
}

// �X���b�v�`�F�[���̍쐬.
void CDirectX12::CreateSwapChain(MyComPtr<IDXGISwapChain4>& SwapChain)
{
	// �X���b�v �`�F�[���\���̂̐ݒ�.
	DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
	SwapChainDesc.Width = WND_W;									//  ��ʂ̕�.
	SwapChainDesc.Height = WND_H;									//  ��ʂ̍���.
	SwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;				//  �\���`��.
	SwapChainDesc.Stereo = false;									//  �S��ʃ��[�h���ǂ���.
	SwapChainDesc.SampleDesc.Count = 1;								//  �s�N�Z��������̃}���`�T���v���̐�.
	SwapChainDesc.SampleDesc.Quality = 0;							//  �i�����x��(0~1).
	SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	//  �ޯ��ޯ̧�̃�������.
	SwapChainDesc.BufferCount = 2;									//  �ޯ��ޯ̧�̐�.
	SwapChainDesc.Scaling = DXGI_SCALING_STRETCH;					//  �ޯ��ޯ̧�̻��ނ����ޯĂƓ������Ȃ��ꍇ�̻��ޕύX�̓���.
	SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;		//  �د�ߌ�͑f�����j��.
	SwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;			//  �ܯ������,�ޯ��ޯ̧�̓��ߐ��̓���
	SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;	//  �ܯ�����ݓ���̵�߼��(����޳�ٽ��؂�ւ��\Ӱ��).

	MyAssert::IsFailed(
		_T("�X���b�v�`�F�[���̍쐬"),
		&IDXGIFactory2::CreateSwapChainForHwnd, m_pDxgiFactory.Get(),
		m_pCmdQueue.Get(),								// �R�}���h�L���[.
		m_hWnd,											// �E�B���h�E�n���h��.
		&SwapChainDesc,									// �X���b�v�`�F�[���ݒ�.
		nullptr,										// �ЂƂ܂�nullotr�ł悢.TODO : �Ȃɂ���
		nullptr,										// �����nulltr�ł悤
		(IDXGISwapChain1**)SwapChain.ReleaseAndGetAddressOf());	// (Out)�X���b�v�`�F�[��.
}

// �����_�[�^�[�Q�b�g�̍쐬.
void CDirectX12::CreateRenderTarget(
	MyComPtr<ID3D12DescriptorHeap>&			RenderTargetViewHeap,
	std::vector<MyComPtr<ID3D12Resource>>&	BackBuffer)
{
	// �f�B�X�N���v�^�q�[�v�\���̂̍쐬.
	D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
	HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;		// RTV�p�q�[�v.
	HeapDesc.NumDescriptors = 2;						// 2�̃f�B�X�N���v�^.
	HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;	// �q�[�v�̃I�v�V����(���ɂȂ���ݒ�).
	HeapDesc.NodeMask = 0;								// �P��A�_�v�^.					

	MyAssert::IsFailed(
		_T("�f�B�X�N���v�^�q�[�v�̍쐬"),
		&ID3D12Device::CreateDescriptorHeap, m_pDevice12.Get(),
		&HeapDesc,														// �f�B�X�N���v�^�q�[�v�\���̂�o�^.
		IID_PPV_ARGS(RenderTargetViewHeap.ReleaseAndGetAddressOf()));// (Out)�f�B�X�N���v�^�q�[�v.

	// �X���b�v�`�F�[���\����.
	DXGI_SWAP_CHAIN_DESC SwcDesc = {};
	MyAssert::IsFailed(
		_T("�X���b�v�`�F�[���\���̂��擾."),
		&IDXGISwapChain4::GetDesc, m_pSwapChain.Get(),
		&SwcDesc);

	// �ި������˰�߂̐擪�A�h���X�����o��.
	D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandle = RenderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();

	// �o�b�N�o�b�t�@���q�[�v�̐����錾.
	m_pBackBuffer.resize(SwcDesc.BufferCount);

	// SRGB�����_�[�^�[�Q�b�g�r���[�ݒ�.
	D3D12_RENDER_TARGET_VIEW_DESC RTVDesc = {};
	RTVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	// �o�b�N�o�t�@�̐���.
	for (int i = 0; i < static_cast<int>(SwcDesc.BufferCount); ++i)
	{
		MyAssert::IsFailed(
			_T("%d�ڂ̃X���b�v�`�F�[�����̃o�b�t�@�[�ƃr���[���֘A�Â���", i + 1),
			&IDXGISwapChain4::GetBuffer, m_pSwapChain.Get(),
			static_cast<UINT>(i),
			IID_PPV_ARGS(m_pBackBuffer[i].GetAddressOf()));

		RTVDesc.Format = m_pBackBuffer[i]->GetDesc().Format;

		// �����_�[�^�[�Q�b�g�r���[�𐶐�����.
		m_pDevice12->CreateRenderTargetView(
			BackBuffer[i].Get(),
			&RTVDesc,
			DescriptorHandle);

		// �|�C���^�����炷.
		DescriptorHandle.ptr += m_pDevice12->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	DXGI_SWAP_CHAIN_DESC1 Desc = {};
	MyAssert::IsFailed(
		_T("��ʕ����擾"),
		&IDXGISwapChain4::GetDesc1, m_pSwapChain.Get(),
		&Desc);

	m_pViewport.reset(new CD3DX12_VIEWPORT(BackBuffer[0].Get()));
	m_pScissorRect.reset(new CD3DX12_RECT(0, 0, Desc.Width, Desc.Height));

}

// �[�x�o�b�t�@�쐬.
void CDirectX12::CreateDepthDesc(
	MyComPtr<ID3D12Resource>&		DepthBuffer,
	MyComPtr<ID3D12DescriptorHeap>&	DepthHeap)
{
	// �[�x�o�b�t�@�̎d�l.
	D3D12_RESOURCE_DESC DepthResourceDesc = {};
	DepthResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;	// 2�����̃e�N�X�`���f�[�^�Ƃ���.
	DepthResourceDesc.Width = WND_WF;									// ���ƍ����̓����_�[�^�[�Q�b�g�Ɠ���.
	DepthResourceDesc.Height = WND_HF;									// ��ɓ���.
	DepthResourceDesc.DepthOrArraySize = 1;								// �e�N�X�`���z��ł��Ȃ���3D�e�N�X�`���ł��Ȃ�.
	DepthResourceDesc.Format = DXGI_FORMAT_D32_FLOAT;					// �[�x�l�������ݗp�t�H�[�}�b�g.
	DepthResourceDesc.SampleDesc.Count = 1;								// �T���v����1�s�N�Z��������1��.
	DepthResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;	// ���̃o�b�t�@�͐[�x�X�e���V���Ƃ��Ďg�p���܂�.
	DepthResourceDesc.MipLevels = 1;
	DepthResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	DepthResourceDesc.Alignment = 0;

	// �f�v�X�p�q�[�v�v���p�e�B.
	D3D12_HEAP_PROPERTIES DepthHeapProperty = {};
	DepthHeapProperty.Type = D3D12_HEAP_TYPE_DEFAULT;					// DEFAULT��������UNKNOWN�ł悵.
	DepthHeapProperty.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	DepthHeapProperty.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	// ���̃N���A�o�����[���d�v�ȈӖ�������.
	m_DepthClearValue.DepthStencil.Depth = 1.0f;		// �[���P(�ő�l)�ŃN���A.
	m_DepthClearValue.Format = DXGI_FORMAT_D32_FLOAT;	// 32bit�[�x�l�Ƃ��ăN���A.

	MyAssert::IsFailed(
		_T("�[�x�o�b�t�@���\�[�X���쐬"),
		&ID3D12Device::CreateCommittedResource, m_pDevice12.Get(),
		&DepthHeapProperty,							// �q�[�v�v���p�e�B�̐ݒ�.
		D3D12_HEAP_FLAG_NONE,						// �q�[�v�̃I�v�V����(���ɂȂ���ݒ�).
		&DepthResourceDesc,							// ���\�[�X�̎d�l.
		D3D12_RESOURCE_STATE_DEPTH_WRITE,			// ���\�[�X�̏������.
		&m_DepthClearValue,							// �[�x�o�b�t�@���N���A���邽�߂̐ݒ�.
		IID_PPV_ARGS(DepthBuffer.ReleaseAndGetAddressOf())); // (Out)�[�x�o�b�t�@.

	// �[�x�X�e���V���r���[�p�̃f�X�N���v�^�q�[�v���쐬
	D3D12_DESCRIPTOR_HEAP_DESC DsvHeapDesc = {};
	DsvHeapDesc.NumDescriptors = 1;                   // �[�x�r���[1��.
	DsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;// �f�X�N���v�^�q�[�v�̃^�C�v.

	MyAssert::IsFailed(
		_T("�[�x�X�e���V���r���[�p�̃f�X�N���v�^�q�[�v���쐬"),
		&ID3D12Device::CreateDescriptorHeap, m_pDevice12.Get(),
		&DsvHeapDesc,											// �q�[�v�̐ݒ�.
		IID_PPV_ARGS(DepthHeap.ReleaseAndGetAddressOf()));	// (Out)�f�X�N���v�^�q�[�v.
	
	// �[�x�r���[�쐬.
	D3D12_DEPTH_STENCIL_VIEW_DESC DsvDesc = {};
	DsvDesc.Format = DXGI_FORMAT_D32_FLOAT;					// �f�v�X�t�H�[�}�b�g.
	DsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;	// 2D�e�N�X�`��.
	DsvDesc.Flags = D3D12_DSV_FLAG_NONE;					// �t���O�Ȃ�.

	m_pDevice12->CreateDepthStencilView(
		m_pDepthBuffer.Get(),								// �[�x�o�b�t�@.
		&DsvDesc,											// �[�x�r���[�̐ݒ�.
		DepthHeap->GetCPUDescriptorHandleForHeapStart());// �q�[�v���̈ʒu.
}

// �t�F���X�̍쐬.
void CDirectX12::CreateFance(MyComPtr<ID3D12Fence>& Fence)
{
	MyAssert::IsFailed(
		_T("�t�F���X�̐���"),
		&ID3D12Device::CreateFence, m_pDevice12.Get(),
		m_FenceValue,									// �������q.
		D3D12_FENCE_FLAG_NONE,							// �t�F���X�̃I�v�V����.
		IID_PPV_ARGS(Fence.ReleaseAndGetAddressOf()));// (Out) �t�F���X.
}

// �e�N�X�`�����[�h�e�[�u���̍쐬.
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
	resDesc.Width = 4;//��
	resDesc.Height = 256;//����
	resDesc.DepthOrArraySize = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;//
	resDesc.MipLevels = 1;//
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;//���C�A�E�g�ɂ��Ă͌��肵�Ȃ�
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;//�Ƃ��Ƀt���O�Ȃ�

	D3D12_HEAP_PROPERTIES texHeapProp = {};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;//����Ȑݒ�Ȃ̂�default�ł�upload�ł��Ȃ�
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;//���C�g�o�b�N��
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;//�]����L0�܂�CPU�����璼��
	texHeapProp.CreationNodeMask = 0;//�P��A�_�v�^�̂���0
	texHeapProp.VisibleNodeMask = 0;//�P��A�_�v�^�̂���0

	ID3D12Resource* gradBuff = nullptr;
	auto result = m_pDevice12->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,//���Ɏw��Ȃ�
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&gradBuff)
	);
	if (FAILED(result)) {
		return nullptr;
	}

	//�オ�����ĉ��������e�N�X�`���f�[�^���쐬
	std::vector<unsigned int> data(4 * 256);
	auto it = data.begin();
	unsigned int c = 0xff;
	for (; it != data.end(); it += 4) {
		auto col = (0xff << 24) | RGB(c, c, c);//RGBA���t���т��Ă��邽��RGB�}�N����0xff<<24��p���ĕ\���B
		//auto col = (0xff << 24) | (c<<16)|(c<<8)|c;//����ł�OK
		std::fill(it, it + 4, col);
		--c;
	}

	result = gradBuff->WriteToSubresource(0, nullptr, data.data(), 4 * sizeof(unsigned int), sizeof(unsigned int) * static_cast<UINT>(data.size()));
	return gradBuff;
}

ID3D12Resource* CDirectX12::CreateWhiteTexture() {
	D3D12_HEAP_PROPERTIES texHeapProp = {};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;//����Ȑݒ�Ȃ̂�default�ł�upload�ł��Ȃ�
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;//���C�g�o�b�N��
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;//�]����L0�܂�CPU�����璼��
	texHeapProp.CreationNodeMask = 0;//�P��A�_�v�^�̂���0
	texHeapProp.VisibleNodeMask = 0;//�P��A�_�v�^�̂���0

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resDesc.Width = 4;//��
	resDesc.Height = 4;//����
	resDesc.DepthOrArraySize = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;//
	resDesc.MipLevels = 1;//
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;//���C�A�E�g�ɂ��Ă͌��肵�Ȃ�
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;//�Ƃ��Ƀt���O�Ȃ�

	ID3D12Resource* whiteBuff = nullptr;
	auto result = m_pDevice12->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,//���Ɏw��Ȃ�
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
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;//����Ȑݒ�Ȃ̂�default�ł�upload�ł��Ȃ�
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;//���C�g�o�b�N��
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;//�]����L0�܂�CPU�����璼��
	texHeapProp.CreationNodeMask = 0;//�P��A�_�v�^�̂���0
	texHeapProp.VisibleNodeMask = 0;//�P��A�_�v�^�̂���0

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resDesc.Width = 4;//��
	resDesc.Height = 4;//����
	resDesc.DepthOrArraySize = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;//
	resDesc.MipLevels = 1;//
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;//���C�A�E�g�ɂ��Ă͌��肵�Ȃ�
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;//�Ƃ��Ƀt���O�Ȃ�

	ID3D12Resource* blackBuff = nullptr;
	auto result = m_pDevice12->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,//���Ɏw��Ȃ�
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

// �e�N�X�`���ǂݍ���.
ID3D12Resource* CDirectX12::LoadTextureFromFile(std::string& TexPath)
{
	auto it = _resourceTable.find(TexPath);
	if (it != _resourceTable.end()) {
		// �e�[�u���ɓ��ɂ������烍�[�h����̂ł͂Ȃ��}�b�v����.
		// ���\�[�X��Ԃ�.
		return _resourceTable[TexPath];
	}

	// WIC�e�N�X�`���̃��[�h.
	DirectX::TexMetadata MetaData = {};
	DirectX::ScratchImage ScratchImg = {};

	// �e�N�X�`���̃t�@�C���p�X.
	auto wTexPath = MyString::StringToWString(TexPath);

	// �g���q���擾.
	auto Extension = MyFilePath::GetExtension(TexPath);

	auto Result = LoadLambdaTable[Extension](wTexPath,
		&MetaData,
		ScratchImg);
	if (FAILED(Result)) {
		return nullptr;
	}

	// ���f�[�^���o.
	auto Img = ScratchImg.GetImage(0, 0, 0);	

	// WriteToSubresource�œ]������p�̃q�[�v�ݒ�.
	D3D12_HEAP_PROPERTIES TexHeapProp = {};
	TexHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;							//�@����Ȑݒ�Ȃ̂�default�ł�upload�ł��Ȃ�.
	TexHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;	//�@���C�g�o�b�N��.
	TexHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;			//�@�]����L0�܂�CPU�����璼��.
	TexHeapProp.CreationNodeMask = 0;									//�@�P��A�_�v�^�̂���0.
	TexHeapProp.VisibleNodeMask = 0;									//�@�P��A�_�v�^�̂���0.

	D3D12_RESOURCE_DESC ResourceDesc = {};
	ResourceDesc.Format = MetaData.format;
	ResourceDesc.Width = static_cast<UINT>(MetaData.width);		// ��.
	ResourceDesc.Height = static_cast<UINT>(MetaData.height);	// ����.
	ResourceDesc.DepthOrArraySize = static_cast<UINT16>(MetaData.arraySize);
	ResourceDesc.SampleDesc.Count = 1;													// �ʏ�e�N�X�`���Ȃ̂ŃA���`�F�����Ȃ�.
	ResourceDesc.SampleDesc.Quality = 0;//
	ResourceDesc.MipLevels = static_cast<UINT16>(MetaData.mipLevels);					// �~�b�v�}�b�v���Ȃ��̂Ń~�b�v���͂P��.
	ResourceDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(MetaData.dimension);
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;									// ���C�A�E�g�ɂ��Ă͌��肵�Ȃ�.
	ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;										// �Ƃ��Ƀt���O�Ȃ�.

	ID3D12Resource* Texbuff = nullptr;
	MyAssert::IsFailed(
		_T("�e�N�X�`���o�b�t�@�̍쐬"),
		&ID3D12Device::CreateCommittedResource, m_pDevice12.Get(),
		&TexHeapProp,
		D3D12_HEAP_FLAG_NONE,//���Ɏw��Ȃ�
		&ResourceDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&Texbuff)
	);

	MyAssert::IsFailed(
		_T("�e�N�X�`���o�b�t�@�̍쐬"),
		&ID3D12Resource::WriteToSubresource, Texbuff,
		0,
		nullptr,								// �S�̈�փR�s�[.
		Img->pixels,							// ���f�[�^�A�h���X.
		static_cast<UINT>(Img->rowPitch),		// 1���C���T�C�Y.
		static_cast<UINT>(Img->slicePitch)		// �S�T�C�Y.
	);

	_resourceTable[TexPath] = Texbuff;
	return Texbuff;
}

// �O���t�B�b�N�p�C�v���C���X�e�[�g�̐ݒ�.
void CDirectX12::CreateGraphicPipeline(
	MyComPtr<ID3D12PipelineState>& GraphicPipelineState)
{
	ID3DBlob* VSBlob = nullptr;	// ���_�V�F�[�_�[�̃u���u.
	ID3DBlob* PSBlob = nullptr;	// �s�N�Z���V�F�[�_�[�̃u���u.
	ID3DBlob* ErrerBlob = nullptr;	// �s�N�Z���V�F�[�_�[�̃u���u.
	HRESULT   result = S_OK;

	// ���_�V�F�[�_�[�̓ǂݍ���.
	CompileShaderFromFile(
		L"Data\\Shader\\Basic\\BasicVertexShader.hlsl",
		"BasicVS", "vs_5_0",
		&VSBlob);


	CompileShaderFromFile(
		L"Data\\Shader\\Basic\\BasicPixelShader.hlsl",
		"BasicPS", "ps_5_0",
		&PSBlob);

	// TODO : �Z���ł�����.
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

	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;//���g��0xffffffff

	//
	gpipeline.BlendState.AlphaToCoverageEnable = false;
	gpipeline.BlendState.IndependentBlendEnable = false;

	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};

	//�ЂƂ܂����Z���Z�⃿�u�����f�B���O�͎g�p���Ȃ�
	renderTargetBlendDesc.BlendEnable = false;
	renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;


	//�ЂƂ܂��_�����Z�͎g�p���Ȃ�
	renderTargetBlendDesc.LogicOpEnable = false;

	gpipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;

	gpipeline.RasterizerState.MultisampleEnable = false;//�܂��A���`�F���͎g��Ȃ�
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;//�J�����O���Ȃ�
	gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;//���g��h��Ԃ�
	gpipeline.RasterizerState.DepthClipEnable = true;//�[�x�����̃N���b�s���O�͗L����

	gpipeline.RasterizerState.FrontCounterClockwise = false;
	gpipeline.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	gpipeline.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	gpipeline.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	gpipeline.RasterizerState.AntialiasedLineEnable = false;
	gpipeline.RasterizerState.ForcedSampleCount = 0;
	gpipeline.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;


	gpipeline.DepthStencilState.DepthEnable = true;//�[�x�o�b�t�@���g����
	gpipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;//�S�ď�������
	gpipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;//�����������̗p
	gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	gpipeline.DepthStencilState.StencilEnable = false;

	gpipeline.InputLayout.pInputElementDescs = InputLayout;//���C�A�E�g�擪�A�h���X
	gpipeline.InputLayout.NumElements = _countof(InputLayout);//���C�A�E�g�z��

	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;//�X�g���b�v���̃J�b�g�Ȃ�
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//�O�p�`�ō\��

	gpipeline.NumRenderTargets = 1;//���͂P�̂�
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;//0�`1�ɐ��K�����ꂽRGBA

	gpipeline.SampleDesc.Count = 1;//�T���v�����O��1�s�N�Z���ɂ��P
	gpipeline.SampleDesc.Quality = 0;//�N�I���e�B�͍Œ�

	ID3D12RootSignature* rootsignature = nullptr;
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	D3D12_DESCRIPTOR_RANGE descTblRange[3] = {};//�e�N�X�`���ƒ萔�̂Q��


	//�萔�ЂƂ�(���W�ϊ��p)
	descTblRange[0].NumDescriptors = 1;//�萔�ЂƂ�
	descTblRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;//��ʂ͒萔
	descTblRange[0].BaseShaderRegister = 0;//0�ԃX���b�g����
	descTblRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//�萔�ӂ���(�}�e���A���p)
	descTblRange[1].NumDescriptors = 1;//�f�X�N���v�^�q�[�v�͂������񂠂邪��x�Ɏg���̂͂P��
	descTblRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;//��ʂ͒萔
	descTblRange[1].BaseShaderRegister = 1;//1�ԃX���b�g����
	descTblRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//�e�N�X�`��1��(���̃}�e���A���ƃy�A)
	descTblRange[2].NumDescriptors = 4;//�e�N�X�`���S��(��{��sph��spa�ƃg�D�[��)
	descTblRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;//��ʂ̓e�N�X�`��
	descTblRange[2].BaseShaderRegister = 0;//0�ԃX���b�g����
	descTblRange[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootparam[2] = {};
	rootparam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[0].DescriptorTable.pDescriptorRanges = &descTblRange[0];//�f�X�N���v�^�����W�̃A�h���X
	rootparam[0].DescriptorTable.NumDescriptorRanges = 1;//�f�X�N���v�^�����W��
	rootparam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//�S�ẴV�F�[�_���猩����

	rootparam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[1].DescriptorTable.pDescriptorRanges = &descTblRange[1];//�f�X�N���v�^�����W�̃A�h���X
	rootparam[1].DescriptorTable.NumDescriptorRanges = 2;//�f�X�N���v�^�����W��������
	rootparam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//�s�N�Z���V�F�[�_���猩����

	rootSignatureDesc.pParameters = rootparam;//���[�g�p�����[�^�̐擪�A�h���X
	rootSignatureDesc.NumParameters = 2;//���[�g�p�����[�^��

	D3D12_STATIC_SAMPLER_DESC samplerDesc[2] = {};
	samplerDesc[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//���J��Ԃ�
	samplerDesc[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//�c�J��Ԃ�
	samplerDesc[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//���s�J��Ԃ�
	samplerDesc[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;//�{�[�_�[�̎��͍�
	samplerDesc[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;//��Ԃ��Ȃ�(�j�A���X�g�l�C�o�[)
	samplerDesc[0].MaxLOD = D3D12_FLOAT32_MAX;//�~�b�v�}�b�v�ő�l
	samplerDesc[0].MinLOD = 0.0f;//�~�b�v�}�b�v�ŏ��l
	samplerDesc[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;//�I�[�o�[�T���v�����O�̍ۃ��T���v�����O���Ȃ��H
	samplerDesc[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//�s�N�Z���V�F�[�_����̂݉�
	samplerDesc[0].ShaderRegister = 0;
	samplerDesc[1] = samplerDesc[0];//�ύX�_�ȊO���R�s�[
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

// �V�F�[�_�[�̃R���p�C��.
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
			D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // �f�o�b�O�I�v�V����.
			0, ShaderBlob, &ErrorBlob
		);

		// �R���p�C���G���[���ɃG���[�n���h�����O���s��.
		ShaderCompileError(Result, ErrorBlob);

		return Result;
}


// �A�_�v�^�[��������.
IDXGIAdapter* CDirectX12::FindAdapter(std::wstring FindWord)
{
	// �A�^�u�^�[(�������O���{������).
	std::vector <IDXGIAdapter*> Adapter;

	// �����ɓ���̖��O�����A�_�v�^�[������.
	IDXGIAdapter* TmpAdapter = nullptr;

	// for�ł��ׂẴA�_�v�^�[���x�N�^�[�z��ɓ����.
	for (int i = 0; m_pDxgiFactory->EnumAdapters(i, &TmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i) {
		Adapter.push_back(TmpAdapter);
	}

	// ���o�����A�_�v�^�[������������Ă���.
	for (auto Adpt : Adapter) {

		DXGI_ADAPTER_DESC Adesc = {};

		// �A�_�v�^�[�������o��.
		Adpt->GetDesc(&Adesc);

		// ���O�����o��.
		std::wstring strDesc = Adesc.Description;

		// NVIDIA�Ȃ�i�[.
		if (strDesc.find(FindWord) != std::string::npos) {
			return Adpt;
		}
	}

	return nullptr;
}

// �f�o�b�O���[�h���N��.
void CDirectX12::EnableDebuglayer()
{
	ID3D12Debug* DebugLayer = nullptr;
	
	// �f�o�b�O���C���[�C���^�[�t�F�[�X���擾.
	D3D12GetDebugInterface(IID_PPV_ARGS(&DebugLayer));

	// �f�o�b�O���C���[��L��.
	DebugLayer->EnableDebugLayer();	

	// ���.
	DebugLayer->Release();
}

// ErroeBlob�ɓ������G���[���o��.
void CDirectX12::ShaderCompileError(const HRESULT& Result, ID3DBlob* ErrorMsg)
{
	if (FAILED(Result)) {
		std::wstring ErrStr;

		if (Result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
			ErrStr = L"�t�@�C������������܂���";
		}
		else {
			if (ErrorMsg) {
				// ErrorMsg ������̏ꍇ.
				ErrStr.resize(ErrorMsg->GetBufferSize());
				std::copy_n(static_cast<const char*>(ErrorMsg->GetBufferPointer()), ErrorMsg->GetBufferSize(), ErrStr.begin());
				ErrStr += L"\n";
			}
			else {
				// ErrorMsg ���Ȃ��̏ꍇ.
				ErrStr = L"ErrorMsg is null";
			}
		}
		if (ErrorMsg) {
			ErrorMsg->Release();  // ���������
		}

		std::wstring WideErrorMsg(ErrStr);
		std::string FormattedMessage = MyAssert::FormatErrorMessage(WideErrorMsg, Result);
		throw std::runtime_error(FormattedMessage);
	}
}
