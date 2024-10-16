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
		SwapChainDesc.Width = WND_W;									// ��ʂ̕�.
		SwapChainDesc.Height = WND_H;									// ��ʂ̍���.
		SwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;				// �\���`��.
		SwapChainDesc.Stereo = false;									// �S��ʃ��[�h���ǂ���.
		SwapChainDesc.SampleDesc.Count = 1;								// �s�N�Z��������̃}���`�T���v���̐�.
		SwapChainDesc.SampleDesc.Quality = 0;							// �i�����x��(0~1).
		SwapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;				// �ޯ��ޯ̧�̃�������.
		SwapChainDesc.BufferCount = 2;									// �ޯ��ޯ̧�̐�.
		SwapChainDesc.Scaling = DXGI_SCALING_STRETCH;					// �ޯ��ޯ̧�̻��ނ����ޯĂƓ������Ȃ��ꍇ�̻��ޕύX�̓���.
		SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;		// �د�ߌ�͑f�����j��.
		SwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;			// �ܯ������,�ޯ��ޯ̧�̓��ߐ��̓���
		SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;	// �ܯ�����ݓ���̵�߼��(����޳�ٽ��؂�ւ��\Ӱ��).

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


		// �ި������˰�߂̐擪�A�h���X�����o��.
		D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandle = RTVHeaps->GetCPUDescriptorHandleForHeapStart();

		//SRGB�����_�[�^�[�Q�b�g�r���[�ݒ�
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		//��������ƐF���͂����ԃ}�V�ɂȂ邪�A�o�b�N�o�b�t�@�Ƃ�
		//�t�H�[�}�b�g�̐H���Ⴂ�ɂ��DebugLayer�ɃG���[���o�͂����
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

		// �o�b�N�o�b�t�@���q�[�v�̐����錾.
		std::vector<ID3D12Resource*> BackBaffer(swcDesc.BufferCount);

		// �o�b�N�o�t�@�̐���.
		for (int i = 0; i < static_cast<int>(swcDesc.BufferCount); ++i)
		{
			MyAssert::IsFailed(
				_T("%d�ڂ̃X���b�v�`�F�[�����̃o�b�t�@�[�ƃr���[���֘A�Â���"),
				&IDXGISwapChain4::GetBuffer, m_pSwapChain,
				static_cast<UINT>(i),
				IID_PPV_ARGS(&BackBaffer[i]));

			// �����_�[�^�[�Q�b�g�r���[�𐶐�����.
			m_pDevice12->CreateRenderTargetView(
				BackBaffer[i],
				&rtvDesc,
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


		char Signature[3];
		PMDHeader Pmdheader = {};
		FILE* fp;
			
		auto err = fopen_s(&fp, "Data\\Model\\�����~�N.pmd", "rb");

		if (fp == nullptr) {
			char strerr[256];
			strerror_s(strerr, 256, err);
			return -1;
		}

		// �V�O�l�N�`��.
		fread(Signature, sizeof(Signature), 1, fp);
		// PMD�w�b�_�[.
		fread(&Pmdheader, sizeof(Pmdheader), 1, fp);

		// ���_��.
		unsigned int VertNum = 0;
		fread(&VertNum, sizeof(VertNum), 1, fp);

		std::string result = "VertNum : " + std::to_string(VertNum);

		OutputDebugStringA(result.c_str());

		std::vector<PMDVertex> Vertices(VertNum);//�o�b�t�@�m��
		for (auto i = 0; i < VertNum; i++)
		{
			fread(&Vertices[i], PmdVertexSize, 1, fp);
		}

		// �C���f�b�N�X��.
		unsigned int IndicesNum;
		fread(&IndicesNum, sizeof(IndicesNum), 1, fp);

		auto HeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto ResDesc = CD3DX12_RESOURCE_DESC::Buffer(Vertices.size() * sizeof(PMDVertex));

		//UPLOAD(�m�ۂ͉\)
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
		vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();//�o�b�t�@�̉��z�A�h���X
		vbView.SizeInBytes = static_cast<UINT>(Vertices.size() * sizeof(PMDVertex));//�S�o�C�g��
		vbView.StrideInBytes = sizeof(PMDVertex);//1���_������̃o�C�g��

		std::vector<unsigned short> indices(IndicesNum);

		fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);
		fclose(fp);

		ID3D12Resource* idxBuff = nullptr;
		HeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		ResDesc = CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(indices[0]));
		//�ݒ�́A�o�b�t�@�̃T�C�Y�ȊO���_�o�b�t�@�̐ݒ���g���܂킵��
		//OK���Ǝv���܂��B
		m_pDevice12->CreateCommittedResource(
			&HeapProp,
			D3D12_HEAP_FLAG_NONE,
			&ResDesc,
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

		// �u���u���쐬(�ėp�I�ȃf�[�^�̉��\���^).
		ID3DBlob* VSBlob = nullptr;
		ID3DBlob* PSBlob = nullptr;

		// �V�F�[�_�[�̃G���[�n���h��.
		// MEMO : �ڍׂɃG���[���o��̂�MyAssert�ł͂Ȃ�Blob�ŃG���[���擾����.
		ID3DBlob* ErrorBlob = nullptr;
		HRESULT Result = S_OK;

		Result = D3DCompileFromFile(
			_T("Data\\Shader\\Basic\\BasicVertexShader.hlsl"),	// �t�@�C����.
			nullptr, 											// �V�F�[�_�[�}�N���I�u�W�F�N�g.
			D3D_COMPILE_STANDARD_FILE_INCLUDE,					// �C���N���[�h�I�u�W�F�N�g
			"BasicVS", 											// �G���g���[�|�C���g.
			"vs_5_0",											// �ǂ̃V�F�[�_�[��.
			D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,	// �V�F�[�_�R���p�C���I�v�V����.
			0, 													// �G�t�F�N�g�R���p�C���I�v�V����.
			&VSBlob,											// (Out)�V�F�[�_�[�󂯎��.
			&ErrorBlob);										// (Out)�G���[�p�|�C���^.

		// �G���[�`�F�b�N.
		ShaderCompileError(Result, ErrorBlob);

		MyAssert::IsFailed(
			_T("�s�N�Z���V�F�[�_�[�̃R���p�C��"),
			&D3DCompileFromFile,
			_T("Data\\Shader\\Basic\\BasicPixelShader.hlsl"),		// �t�@�C����.
			nullptr,												// �V�F�[�_�[�}�N���I�u�W�F�N�g.
			D3D_COMPILE_STANDARD_FILE_INCLUDE,						// �C���N���[�h�I�u�W�F�N�g.
			"BasicPS", 												// �G���g���[�|�C���g.
			"ps_5_0",												// �ǂ̃V�F�[�_�[��.
			D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,		// �V�F�[�_�R���p�C���I�v�V����.
			0, 														// �G�t�F�N�g�R���p�C���I�v�V����.
			&PSBlob, 												// (Out)�V�F�[�_�[�󂯎��.
			&ErrorBlob);											// (Out)�G���[�p�|�C���^.

		// �G���[�`�F�b�N.
		ShaderCompileError(Result, ErrorBlob);

		// ���_���C�A�E�g��ݒ�.
		D3D12_INPUT_ELEMENT_DESC InputLayout[] =
		{
			{	"POSITION",									// �Z�}���e�B�N�X.
				0,											// �����Z�}���e�B�N�X���̎��Ɏg���C���f�b�N�X.
				DXGI_FORMAT_R32G32B32_FLOAT,				// �t�H�[�}�b�g(�v�f���ƃr�b�g���Ō^��\��).
				0,											// ���̓X���b�g�C���f�b�N�X.
				D3D12_APPEND_ALIGNED_ELEMENT,				// �f�[�^�̃I�t�Z�b�g�ʒu.
				D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,	// 1�̓��̓X���b�g�̓��̓f�[�^�N���X�����ʂ���^(D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA�ł悢).
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

		// �O���t�B�b�N�p�C�v���C���X�e�[�g.
		D3D12_GRAPHICS_PIPELINE_STATE_DESC GraphicsPipelineState = {};
		GraphicsPipelineState.pRootSignature = nullptr;						// ���[�g�V�O�l�N�`���̃|�C���^.
		GraphicsPipelineState.VS.pShaderBytecode = VSBlob->GetBufferPointer();	// �o�[�e�b�N�X�V�F�[�_�ւ̃|�C���^.
		GraphicsPipelineState.VS.BytecodeLength = VSBlob->GetBufferSize();		// �o�[�e�b�N�X�V�F�[�_�[�̃T�C�Y.
		GraphicsPipelineState.PS.pShaderBytecode = PSBlob->GetBufferPointer();	// �s�N�Z���V�F�[�_�̃|�C���^.
		GraphicsPipelineState.PS.BytecodeLength = PSBlob->GetBufferSize();		// �s�N�Z���V�F�[�_�̃T�C�Y.

		GraphicsPipelineState.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;			// �u�����h��Ԃ̃T���v���}�X�N.

		GraphicsPipelineState.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		GraphicsPipelineState.BlendState.RenderTarget->BlendEnable = true;
		GraphicsPipelineState.BlendState.RenderTarget->SrcBlend = D3D12_BLEND_SRC_ALPHA;
		GraphicsPipelineState.BlendState.RenderTarget->DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		GraphicsPipelineState.BlendState.RenderTarget->BlendOp = D3D12_BLEND_OP_ADD;

		// �A���t�@�̃u�����h�X�e�[�g.
		// TODO : �{�����̎��͂��̍\���̂��������ăA���t�@�̐ݒ���s��.
		//D3D12_RENDER_TARGET_BLEND_DESC RenderTargetBlendDesc = {};

		////�ЂƂ܂����Z���Z�⃿�u�����f�B���O�͎g�p���Ȃ�
		//RenderTargetBlendDesc.BlendEnable = false;
		//RenderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		////�ЂƂ܂��_�����Z�͎g�p���Ȃ�
		//RenderTargetBlendDesc.LogicOpEnable = false;

		//GraphicsPipelineState.BlendState.RenderTarget[0] = RenderTargetBlendDesc;


		GraphicsPipelineState.RasterizerState.MultisampleEnable = false;				// �܂��A���`�F���͎g��Ȃ�.
		GraphicsPipelineState.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;	// �J�����O���Ȃ�.
		GraphicsPipelineState.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;// ���g��h��Ԃ�.
		GraphicsPipelineState.RasterizerState.DepthClipEnable = true;					// �[�x�����̃N���b�s���O�͗L����.

		//�c��
		GraphicsPipelineState.RasterizerState.FrontCounterClockwise = false;
		GraphicsPipelineState.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		GraphicsPipelineState.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		GraphicsPipelineState.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		GraphicsPipelineState.RasterizerState.AntialiasedLineEnable = false;
		GraphicsPipelineState.RasterizerState.ForcedSampleCount = 0;
		GraphicsPipelineState.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;



		GraphicsPipelineState.DepthStencilState.DepthEnable = false;
		GraphicsPipelineState.DepthStencilState.StencilEnable = false;

		GraphicsPipelineState.InputLayout.pInputElementDescs = InputLayout;//���C�A�E�g�擪�A�h���X
		GraphicsPipelineState.InputLayout.NumElements = _countof(InputLayout);//���C�A�E�g�z��

		GraphicsPipelineState.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;//�X�g���b�v���̃J�b�g�Ȃ�
		GraphicsPipelineState.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//�O�p�`�ō\��

		GraphicsPipelineState.NumRenderTargets = 1;//���͂P�̂�
		GraphicsPipelineState.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;//0�`1�ɐ��K�����ꂽRGBA

		GraphicsPipelineState.SampleDesc.Count = 1;//�T���v�����O��1�s�N�Z���ɂ��P
		GraphicsPipelineState.SampleDesc.Quality = 0;//�N�I���e�B�͍Œ�

		ID3D12RootSignature* Rootsignature = nullptr;
		D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc = {};
		RootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		D3D12_DESCRIPTOR_RANGE descTblRange[2] = {};//�e�N�X�`���ƒ萔�̂Q��
		descTblRange[0].NumDescriptors = 1;//�e�N�X�`���ЂƂ�
		descTblRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;//��ʂ̓e�N�X�`��
		descTblRange[0].BaseShaderRegister = 0;//0�ԃX���b�g����
		descTblRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		descTblRange[1].NumDescriptors = 1;//�萔�ЂƂ�
		descTblRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;//��ʂ͒萔
		descTblRange[1].BaseShaderRegister = 0;//0�ԃX���b�g����
		descTblRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		D3D12_ROOT_PARAMETER rootparam = {};
		rootparam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootparam.DescriptorTable.pDescriptorRanges = &descTblRange[0];//�f�X�N���v�^�����W�̃A�h���X
		rootparam.DescriptorTable.NumDescriptorRanges = 2;//�f�X�N���v�^�����W��
		rootparam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//�S�ẴV�F�[�_���猩����

		RootSignatureDesc.pParameters = &rootparam;//���[�g�p�����[�^�̐擪�A�h���X
		RootSignatureDesc.NumParameters = 1;//���[�g�p�����[�^��

		D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//���J��Ԃ�
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//�c�J��Ԃ�
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//���s�J��Ԃ�
		samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;//�{�[�_�[�̎��͍�
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;//��Ԃ��Ȃ�(�j�A���X�g�l�C�o�[)
		samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;//�~�b�v�}�b�v�ő�l
		samplerDesc.MinLOD = 0.0f;//�~�b�v�}�b�v�ŏ��l
		samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;//�I�[�o�[�T���v�����O�̍ۃ��T���v�����O���Ȃ��H
		samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//�s�N�Z���V�F�[�_����̂݉�

		RootSignatureDesc.pStaticSamplers = &samplerDesc;
		RootSignatureDesc.NumStaticSamplers = 1;

		ID3DBlob* rootSigBlob = nullptr;
		// MEMO : �ڍׂɃG���[���o��̂�MyAssert�ł͂Ȃ�Blob�ŃG���[���擾����.
		// ���[�g�������V���A����.
		Result = D3D12SerializeRootSignature(
			&RootSignatureDesc,				// ���[�g����.
			D3D_ROOT_SIGNATURE_VERSION_1_0,	// �o�[�W�����w��.
			&rootSigBlob,					// (Out)���[�g�����u���u.
			&ErrorBlob);					// (Out)�G���[�o��.

		// �G���[�`�F�b�N.
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
			_T("�O���t�B�b�N�p�C�v���C���̍쐬"),
			&ID3D12Device::CreateGraphicsPipelineState, m_pDevice12,
			&GraphicsPipelineState,
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

		//WIC�e�N�X�`���̃��[�h
		TexMetadata metadata = {};
		ScratchImage scratchImg = {};

		// �摜�̓ǂݏo��.
		Result = LoadFromWICFile(L"Data\\Image\\textest200x200.png", WIC_FLAGS_NONE, &metadata, scratchImg);

		const Image* Image = scratchImg.GetImage(0, 0, 0);//���f�[�^���o

		// WriteToSubresource�œ]������p�̃q�[�v�ݒ�
		D3D12_HEAP_PROPERTIES texHeapProp = {};
		texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;//����Ȑݒ�Ȃ̂�default�ł�upload�ł��Ȃ�
		texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;//���C�g�o�b�N��
		texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;//�]����L0�܂�CPU�����璼��
		texHeapProp.CreationNodeMask = 0;//�P��A�_�v�^�̂���0
		texHeapProp.VisibleNodeMask = 0;//�P��A�_�v�^�̂���0

		//ResourceDesc = {};
		//ResourceDesc.Format = metadata.format;//RGBA�t�H�[�}�b�gsrvDesc.Format �ƍ��킹�Ȃ��Ĉʂ͂����Ȃ�
		//ResourceDesc.Width = static_cast<UINT>(metadata.width);//��
		//ResourceDesc.Height = static_cast<UINT>(metadata.height);//����
		//ResourceDesc.DepthOrArraySize = static_cast<uint16_t>(metadata.arraySize);//2D�Ŕz��ł��Ȃ��̂łP
		//ResourceDesc.SampleDesc.Count = 1;//�ʏ�e�N�X�`���Ȃ̂ŃA���`�F�����Ȃ�
		//ResourceDesc.SampleDesc.Quality = 0;//
		//ResourceDesc.MipLevels = static_cast<uint16_t>(metadata.mipLevels);//�~�b�v�}�b�v���Ȃ��̂Ń~�b�v���͂P��
		//ResourceDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);//2D�e�N�X�`���p
		//ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;//���C�A�E�g�ɂ��Ă͌��肵�Ȃ�
		//ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;//�Ƃ��Ƀt���O�Ȃ�

		ID3D12Resource* texbuff = nullptr;
		//MyAssert::IsFailed(
		//	_T(""),
		//	&ID3D12Device::CreateCommittedResource, m_pDevice12,
		//	&texHeapProp,
		//	D3D12_HEAP_FLAG_NONE,//���Ɏw��Ȃ�
		//	&ResourceDesc,
		//	D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,//�e�N�X�`���p(�s�N�Z���V�F�[�_���猩��p)
		//	nullptr,
		//	IID_PPV_ARGS(&texbuff)
		//);

		//MyAssert::IsFailed(
		//	_T(""),
		//	&ID3D12Resource::WriteToSubresource, texbuff,
		//	0,
		//	nullptr,//�S�̈�փR�s�[
		//	Image->pixels,//���f�[�^�A�h���X
		//	static_cast<UINT>(Image->rowPitch),//1���C���T�C�Y
		//	static_cast<UINT>(Image->slicePitch)//�S�T�C�Y
		//);

		

		ID3D12Resource* constBuff = nullptr;
		HeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		ResDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(XMMATRIX) + 0xff) & ~0xff);
		

		XMMATRIX* mapMatrix;//�}�b�v��������|�C���^

		//�萔�o�b�t�@�쐬
		XMMATRIX worldMat = XMMatrixIdentity();
		XMFLOAT3 eye(0, 10, -15);
		XMFLOAT3 target(0, 10, 0);
		XMFLOAT3 up(0, 1, 0);
		auto viewMat = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
		auto projMat = XMMatrixPerspectiveFovLH(XM_PIDIV2,//��p��90��
			WND_WF / WND_HF,//�A�X��
			1.0f,//�߂���
			100.0f//������
		);


		//�ʏ�e�N�X�`���r���[�쐬
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = metadata.format;//DXGI_FORMAT_R8G8B8A8_UNORM;//RGBA(0.0f�`1.0f�ɐ��K��)
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;//��q
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2D�e�N�X�`��
		srvDesc.Texture2D.MipLevels = 1;//�~�b�v�}�b�v�͎g�p���Ȃ��̂�1

		ID3D12DescriptorHeap* basicDescHeap = nullptr;
		D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
		descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;//�V�F�[�_���猩����悤��
		descHeapDesc.NodeMask = 0;//�}�X�N��0
		descHeapDesc.NumDescriptors = 1;//CBV1��
		descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;//�f�X�N���v�^�q�[�v���

		m_pDevice12->CreateCommittedResource(
			&HeapProp,
			D3D12_HEAP_FLAG_NONE,
			&ResDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&constBuff)
		);
		m_pDevice12->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&basicDescHeap));//����

		////�f�X�N���v�^�̐擪�n���h�����擾���Ă���
		auto basicHeapHandle = basicDescHeap->GetCPUDescriptorHandleForHeapStart();
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = constBuff->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = static_cast<UINT>(constBuff->GetDesc().Width);
		//�萔�o�b�t�@�r���[�̍쐬
		m_pDevice12->CreateConstantBufferView(&cbvDesc, basicHeapHandle);
		static unsigned int frame = 0;

		while (true)
		{
			// ���݂̃o�b�N�o�b�t�@���擾
			auto BBIndex = m_pSwapChain->GetCurrentBackBufferIndex();

			// �o�b�N�o�b�t�@�������_�[�^�[�Q�b�g�ɑJ��
			{
				const CD3DX12_RESOURCE_BARRIER Barrier = CD3DX12_RESOURCE_BARRIER::Transition(
					BackBaffer[BBIndex],
					D3D12_RESOURCE_STATE_PRESENT,
					D3D12_RESOURCE_STATE_RENDER_TARGET
				);

				m_pCmdList->ResourceBarrier(1, &Barrier);
			}

			m_pCmdList->SetPipelineState(_pipelinestate);

			// �����_�[�^�[�Q�b�g���w��.
			auto rtvH = RTVHeaps->GetCPUDescriptorHandleForHeapStart();
			rtvH.ptr += BBIndex * m_pDevice12->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			m_pCmdList->OMSetRenderTargets(1, &rtvH, false, nullptr);

			// ��ʃN���A.
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

			// �`����s
			m_pCmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);

			// �o�b�N�o�b�t�@��\����ԂɑJ��
			{
				const CD3DX12_RESOURCE_BARRIER Barrier = CD3DX12_RESOURCE_BARRIER::Transition(
					BackBaffer[BBIndex],
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					D3D12_RESOURCE_STATE_PRESENT
				);

				m_pCmdList->ResourceBarrier(1, &Barrier);
			}
		
			// ���߂̃N���[�Y
			m_pCmdList->Close();

			// �R�}���h���X�g�̎��s
			ID3D12CommandList* cmdlists[] = { m_pCmdList };
			m_pCmdQueue->ExecuteCommandLists(1, cmdlists);

			// �҂�
			m_pCmdQueue->Signal(Fence, ++FenceValue);
			if (Fence->GetCompletedValue() != FenceValue) {
				auto event = CreateEvent(nullptr, false, false, nullptr);
				Fence->SetEventOnCompletion(FenceValue, event);
				WaitForSingleObject(event, INFINITE);
				CloseHandle(event);
			}

			// �t���b�v
			m_pSwapChain->Present(1, 0);

			m_pCmdAllocator->Reset(); // �L���[���N���A
			m_pCmdList->Reset(m_pCmdAllocator, _pipelinestate); // �ĂуR�}���h���X�g�����߂鏀��

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

// ErroeBlob�ɓ������G���[���o��.
void CDirectX12::ShaderCompileError(const HRESULT& Result, ID3DBlob* ErrorMsg)
{
	if (FAILED(Result)) {
		std::wstring errstr;

		if (Result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
			errstr = L"�t�@�C������������܂���";
		}
		else {
			if (ErrorMsg) {
				// ErrorMsg ������̏ꍇ.
				errstr.resize(ErrorMsg->GetBufferSize());
				std::copy_n(static_cast<const char*>(ErrorMsg->GetBufferPointer()), ErrorMsg->GetBufferSize(), errstr.begin());
				errstr += L"\n";
			}
			else {
				// ErrorMsg ���Ȃ��̏ꍇ.
				errstr = L"ErrorMsg is null";
			}
		}

		// �G���[���b�Z�[�W���A�T�[�V�����ŕ\��.
		_ASSERT_EXPR(false, errstr.c_str());
	}
}
