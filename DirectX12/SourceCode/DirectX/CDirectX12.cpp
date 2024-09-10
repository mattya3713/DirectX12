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
	// �f�o�b�O���C���[���I��.
	EnableDebuglayer();

#endif _DEBUG


	try {
		MyAssert::IsFailed(
			_T("DXGI�̐���"),
			&CreateDXGIFactory1,
			IID_PPV_ARGS(&m_pDxgiFactory));

		// �t�B�[�`�����x����.
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
			// DirectX12�����̉�.
			if (D3D12CreateDevice(
				FindAdapter(L"NVIDIA"),				// �O���{��I��, nullptr�Ŏ����I��.
				lv,									// �t�B�[�`���[���x��.
				IID_PPV_ARGS(&m_pDevice12)) == S_OK)// (Out)Direct12.
			{
				// �t�B�[�`���[���x��.
				FeatureLevel = lv;
				break;
			}
		}

		MyAssert::IsFailed(
			_T("�R�}���h���X�g�A���P�[�^�[�̐���"),
			&ID3D12Device::CreateCommandAllocator, m_pDevice12,
			D3D12_COMMAND_LIST_TYPE_DIRECT,			// �쐬����R�}���h�A���P�[�^�̎��.
			IID_PPV_ARGS(&m_pCmdAllocator));		// (Out) �R�}���h�A���P�[�^.

		MyAssert::IsFailed(
			_T("�R�}���h���X�g�̐���"),
			&ID3D12Device::CreateCommandList, m_pDevice12,
			0,										// �P���GPU����̏ꍇ��0.
			D3D12_COMMAND_LIST_TYPE_DIRECT,			// �쐬����R�}���h ���X�g�̎��.
			m_pCmdAllocator,						// �A���P�[�^�ւ̃|�C���^.
			nullptr,								// �_�~�[�̏����p�C�v���C�����ݒ肳���?
			IID_PPV_ARGS(&m_pCmdList));				// (Out) �R�}���h���X�g.

		// �R�}���h�L���[�\���̂̍쐬.
		D3D12_COMMAND_QUEUE_DESC CmdQueueDesc = {};
		CmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;				// �^�C���A�E�g�Ȃ�.
		CmdQueueDesc.NodeMask = 0;										// �A�_�v�^�[��������g��Ȃ��Ƃ���0�ł���.
		CmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;	// �v���C�I���e�B�͓��Ɏw��Ȃ�.
		CmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;				// �R�}���h���X�g�ƍ��킹��.

		MyAssert::IsFailed(
			_T("�L���[�̍쐬"),
			&ID3D12Device::CreateCommandQueue, m_pDevice12,
			&CmdQueueDesc,
			IID_PPV_ARGS(&m_pCmdQueue));

		// �X���b�v �`�F�[���\���̂̐ݒ�.
		DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
		SwapChainDesc.Width = WND_W;									//  ��ʂ̕�.
		SwapChainDesc.Height = WND_H;									//  ��ʂ̍���.
		SwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;				//  �\���`��.
		SwapChainDesc.Stereo = false;									//  �S��ʃ��[�h���ǂ���.
		SwapChainDesc.SampleDesc.Count = 1;										//  �s�N�Z��������̃}���`�T���v���̐�.
		SwapChainDesc.SampleDesc.Quality = 0;										//  �i�����x��(0~1).
		SwapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;					//  �ޯ��ޯ̧�̃�������.
		SwapChainDesc.BufferCount = 2;										//  �ޯ��ޯ̧�̐�.
		SwapChainDesc.Scaling = DXGI_SCALING_STRETCH;						//  �ޯ��ޯ̧�̻��ނ����ޯĂƓ������Ȃ��ꍇ�̻��ޕύX�̓���.
		SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;			//  �د�ߌ�͑f�����j��.
		SwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;				//  �ܯ������,�ޯ��ޯ̧�̓��ߐ��̓���
		SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;	//  �ܯ�����ݓ���̵�߼��(����޳�ٽ��؂�ւ��\Ӱ��).

		MyAssert::IsFailed(
			_T("�X���b�v�`�F�[���̍쐬"),
			&IDXGIFactory2::CreateSwapChainForHwnd, m_pDxgiFactory,
			m_pCmdQueue,									// �R�}���h�L���[.
			hWnd,											// �E�B���h�E�n���h��.
			&SwapChainDesc,									// �X���b�v�`�F�[���ݒ�.
			nullptr,										// �ЂƂ܂�nullotr�ł悢.TODO : �Ȃɂ���
			nullptr,										// �����nulltr�ł悤
			(IDXGISwapChain1**)&m_pSwapChain);				// (Out)�X���b�v�`�F�[��.

		// �f�B�X�N���v�^�q�[�v�\���̂̍쐬.
		D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
		HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;		// �q�[�v���̃f�B�X�N���v�^�̌����w��(RenderTargetView).
		HeapDesc.NumDescriptors = 2;						// �q�[�v���̃f�B�X�N���v�^�̐�(�\���̂Q��).	
		HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;	// �q�[�v�̃I�v�V�������w��.
		HeapDesc.NodeMask = 0;								// �f�B�X�N���v�^�q�[�v���K�p�����m�[�h(�P��A�_�v�^�̏ꍇ��0).					

		// �f�B�X�N���v�^�q�[�v.
		ID3D12DescriptorHeap* RTVHeaps = nullptr;

		MyAssert::IsFailed(
			_T("�f�B�X�N���v�^�q�[�v�̍쐬"),
			&ID3D12Device::CreateDescriptorHeap, m_pDevice12,
			&HeapDesc,										// �f�B�X�N���v�^�q�[�v�\���̂�o�^.
			IID_PPV_ARGS(&RTVHeaps));						// (Out)�f�B�X�N���v�^�q�[�v.

		// �X���b�v�`�F�[���\����.
		DXGI_SWAP_CHAIN_DESC swcDesc = {};
		MyAssert::IsFailed(
			_T("�f�B�X�N���v�^�q�[�v�̍쐬"),
			&IDXGISwapChain4::GetDesc, m_pSwapChain,
			&swcDesc);


		// �ި������˰�߂̐퓬�A�h���X�����o��.
		D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandle = RTVHeaps->GetCPUDescriptorHandleForHeapStart();

		// �o�b�N�o�b�t�@���q�[�v�̐����錾.
		std::vector<ID3D12Resource*> BackBaffer(swcDesc.BufferCount);

		// �o�b�N�o�t�@�̐���.
		for (int i = 0; i < static_cast<int>(swcDesc.BufferCount); ++i)
		{
			MyAssert::IsFailed(
				_T("%d�ڂ̃X���b�v�`�F�[�����̃o�b�t�@�[�ƃr���[���֘A�Â���", i + 1),
				&IDXGISwapChain4::GetBuffer, m_pSwapChain,
				static_cast<UINT>(i),
				IID_PPV_ARGS(&BackBaffer[i]));

			// �����_�[�^�[�Q�b�g�r���[�𐶐�����.
			m_pDevice12->CreateRenderTargetView(
				BackBaffer[i],
				nullptr,
				DescriptorHandle);

			// �|�C���^�����炷.
			DescriptorHandle.ptr += m_pDevice12->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		}

		ID3D12Fence* Fence = nullptr;
		UINT64 FenceValue = 0;
		MyAssert::IsFailed(
			_T("�t�F���X�̐���"),
			&ID3D12Device::CreateFence, m_pDevice12,
			FenceValue,									// �������q.
			D3D12_FENCE_FLAG_NONE,						// �t�F���X�̃I�v�V����.
			IID_PPV_ARGS(&Fence));						// (Out) �t�F���X.

		XMFLOAT3 vertices[] = {
		{-0.4f,-0.7f,0.0f} ,//����
		{-0.4f,0.7f,0.0f} ,//����
		{0.4f,-0.7f,0.0f} ,//�E��
		{0.4f,0.7f,0.0f} ,//�E��
		};

		// �q�[�v�̃v���p�e�B.
		D3D12_HEAP_PROPERTIES heapprop = {};								
		heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;						// �q�[�v�̎��.
		heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;	// CPU�y�[�W�v���p�e�B.
		heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;	// �������v�[��.

		// �e�N�X�`�����\�[�X.
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


		//UPLOAD(�m�ۂ͉\)
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
		vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();//�o�b�t�@�̉��z�A�h���X
		vbView.SizeInBytes = sizeof(vertices);//�S�o�C�g��
		vbView.StrideInBytes = sizeof(vertices[0]);//1���_������̃o�C�g��

		unsigned short indices[] = { 0,1,2, 2,1,3 };

		ID3D12Resource* idxBuff = nullptr;
		//�ݒ�́A�o�b�t�@�̃T�C�Y�ȊO���_�o�b�t�@�̐ݒ���g���܂킵��
		//OK���Ǝv���܂��B
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

		//������o�b�t�@�ɃC���f�b�N�X�f�[�^���R�s�[
		unsigned short* mappedIdx = nullptr;
		idxBuff->Map(0, nullptr, (void**)&mappedIdx);
		std::copy(std::begin(indices), std::end(indices), mappedIdx);
		idxBuff->Unmap(0, nullptr);

		//�C���f�b�N�X�o�b�t�@�r���[���쐬
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
		//		::OutputDebugStringA("�t�@�C������������܂���");
		//	}
		//	else {
		//		std::string errstr;
		//		errstr.resize(errorBlob->GetBufferSize());
		//		std::copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
		//		errstr += "\n";
		//		OutputDebugStringA(errstr.c_str());
		//	}
		//	exit(1);//�s�V�������ȁc
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
		//		::OutputDebugStringA("�t�@�C������������܂���");
		//	}
		//	else {
		//		std::string errstr;
		//		errstr.resize(errorBlob->GetBufferSize());
		//		std::copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
		//		errstr += "\n";
		//		OutputDebugStringA(errstr.c_str());
		//	}
		//	exit(1);//�s�V�������ȁc
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

		//�c��
		gpipeline.RasterizerState.FrontCounterClockwise = false;
		gpipeline.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		gpipeline.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		gpipeline.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		gpipeline.RasterizerState.AntialiasedLineEnable = false;
		gpipeline.RasterizerState.ForcedSampleCount = 0;
		gpipeline.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;


		gpipeline.DepthStencilState.DepthEnable = false;
		gpipeline.DepthStencilState.StencilEnable = false;

		gpipeline.InputLayout.pInputElementDescs = inputLayout;//���C�A�E�g�擪�A�h���X
		gpipeline.InputLayout.NumElements = _countof(inputLayout);//���C�A�E�g�z��

		gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;//�X�g���b�v���̃J�b�g�Ȃ�
		gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//�O�p�`�ō\��

		gpipeline.NumRenderTargets = 1;//���͂P�̂�
		gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;//0�`1�ɐ��K�����ꂽRGBA

		gpipeline.SampleDesc.Count = 1;//�T���v�����O��1�s�N�Z���ɂ��P
		gpipeline.SampleDesc.Quality = 0;//�N�I���e�B�͍Œ�

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
		viewport.Width = WND_WF;//�o�͐�̕�(�s�N�Z����)
		viewport.Height = WND_HF;//�o�͐�̍���(�s�N�Z����)
		viewport.TopLeftX = 0;//�o�͐�̍�����WX
		viewport.TopLeftY = 0;//�o�͐�̍�����WY
		viewport.MaxDepth = 1.0f;//�[�x�ő�l
		viewport.MinDepth = 0.0f;//�[�x�ŏ��l


		D3D12_RECT scissorrect = {};
		scissorrect.top = 0;//�؂蔲������W
		scissorrect.left = 0;//�؂蔲�������W
		scissorrect.right = scissorrect.left + static_cast<LONG>(WND_W);//�؂蔲���E���W
		scissorrect.bottom = scissorrect.top + static_cast<LONG>(WND_H);//�؂蔲�������W

		unsigned int frame = 0;

		while (true)
		{
			// ���݂̃o�b�N�o�b�t�@���擾.
			auto BBIndex = m_pSwapChain->GetCurrentBackBufferIndex();

			D3D12_RESOURCE_BARRIER BarrierDesc = {};
			BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;						// ���p�̂̌^.
			BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;							// �t���O.
			BarrierDesc.Transition.pResource = BackBaffer[BBIndex];							// �ޯ��ޯ̧ؿ��.
			BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;	// �J�ڂ̃T�u���\�[�X�̃C���f�b�N�X.
			BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;				// ?
			BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;			// ?

			//�����_�[�^�[�Q�b�g���w��
			auto rtvH = RTVHeaps->GetCPUDescriptorHandleForHeapStart();
			rtvH.ptr += BBIndex * m_pDevice12->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			m_pCmdList->OMSetRenderTargets(1, &rtvH, false, nullptr);

			//��ʃN���A

			float r, g, b;
			r = (float)(0xff & frame >> 16) / 255.0f;
			g = (float)(0xff & frame >> 8) / 255.0f;
			b = (float)(0xff & frame >> 0) / 255.0f;
			float clearColor[] = { r,g,b,1.0f };//���F
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

			//���߂̃N���[�Y
			m_pCmdList->Close();



			//�R�}���h���X�g�̎��s
			ID3D12CommandList* cmdlists[] = { m_pCmdList };
			m_pCmdQueue->ExecuteCommandLists(1, cmdlists);
			////�҂�
			m_pCmdQueue->Signal(Fence, ++FenceValue);

			if (Fence->GetCompletedValue() != FenceValue) {
				auto event = CreateEvent(nullptr, false, false, nullptr);
				Fence->SetEventOnCompletion(FenceValue, event);
				WaitForSingleObject(event, INFINITE);
				CloseHandle(event);
			}
			m_pCmdAllocator->Reset();//�L���[���N���A
			m_pCmdList->Reset(m_pCmdAllocator, _pipelinestate);//�ĂуR�}���h���X�g�����߂鏀��


			//�t���b�v
			m_pSwapChain->Present(1, 0);

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
	DebugLayer->Release();
}
