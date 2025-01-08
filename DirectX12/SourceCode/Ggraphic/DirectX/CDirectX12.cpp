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

		// �e�N�X�`�����[�h�e�[�u���̍쐬.
		CreateTextureLoadTable();

		// �[�x�o�b�t�@�̍쐬.
		CreateDepthDesc(
			m_pDepthBuffer, 
			m_pDepthHeap);
		
		// �r���[�̐ݒ�.
		CreateSceneDesc(
			m_pMappedSceneData,
			m_pSceneConstBuff,
			m_pSceneDescHeap);

		// �t�F���X�̕\��.
		CreateFance(
			m_pFence);
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
	// DirectX����.
	// �o�b�N�o�b�t�@�̃C���f�b�N�X���擾.
	auto BBIdx = m_pSwapChain->GetCurrentBackBufferIndex();
	auto Barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_pBackBuffer[BBIdx].Get(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_pCmdList->ResourceBarrier(1, &Barrier);

	// �����_�[�^�[�Q�b�g���w��.
	auto rtvH = m_pRenderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();
	rtvH.ptr += BBIdx * m_pDevice12->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// �[�x���w��.
	auto DSVHeap = m_pDepthHeap->GetCPUDescriptorHandleForHeapStart();
	m_pCmdList->OMSetRenderTargets(1, &rtvH, false, &DSVHeap);
	m_pCmdList->ClearDepthStencilView(DSVHeap, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// ��ʃN���A.
	float ClearColor[] = { 1.0f,1.0f,1.0f,1.0f };//���F
	m_pCmdList->ClearRenderTargetView(rtvH, ClearColor, 0, nullptr);

	//�r���[�|�[�g�A�V�U�[��`�̃Z�b�g.
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

	// ���߂̃N���[�Y.
	m_pCmdList->Close();

	// �R�}���h���X�g�̎��s.
	ID3D12CommandList* cmdlists[] = { m_pCmdList.Get() };
	m_pCmdQueue->ExecuteCommandLists(1, cmdlists);
	// �҂�.
	WaitForGPU();
	// �L���[���N���A.
	m_pCmdAllocator->Reset();
	// �ĂуR�}���h���X�g�����߂鏀��.
	m_pCmdList->Reset(m_pCmdAllocator.Get(), nullptr);
}

// �X���b�v�`�F�[�����擾.
const MyComPtr<IDXGISwapChain4> CDirectX12::GetSwapChain()
{
	return m_pSwapChain;
}

// DirectX12�f�o�C�X���擾.
const MyComPtr<ID3D12Device> CDirectX12::GetDevice()
{
	return m_pDevice12;
}

// �R�}���h���X�g���擾.
const MyComPtr<ID3D12GraphicsCommandList> CDirectX12::GetCommandList()
{
	return m_pCmdList;
}

// �e�N�X�`�����擾.
MyComPtr<ID3D12Resource> CDirectX12::GetTextureByPath(const char* texpath)
{
	// ���\�[�X�e�[�u����������.
	auto [iterator, Result] = m_ResourceTable.emplace(
		texpath,
		nullptr 
	);

	if (Result) {
		// �p�X������`�������ꍇ��������.
		iterator->second = MyComPtr<ID3D12Resource>(CreateTextureFromFile(texpath));
	}

	// �}�b�v���̃��\�[�X��Ԃ�
	return iterator->second;
}

void CDirectX12::SetScene()
{	
	//���݂̃V�[��(�r���[�v���W�F�N�V����)���Z�b�g
	ID3D12DescriptorHeap* sceneheaps[] = { m_pSceneDescHeap.Get() };
	m_pCmdList->SetDescriptorHeaps(1, sceneheaps);
	m_pCmdList->SetGraphicsRootDescriptorTable(0, m_pSceneDescHeap->GetGPUDescriptorHandleForHeapStart());
}

// GPU�̊����҂�.
void CDirectX12::WaitForGPU()
{

	m_pCmdQueue->Signal(m_pFence.Get(), ++m_FenceValue);

	if (m_pFence->GetCompletedValue() < m_FenceValue) {
		auto event = CreateEvent(nullptr, false, false, nullptr);

		// event������ɍ쐬���ꂽ�����m�F.
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
			_T("�X���b�v�`�F�[�����̃o�b�t�@�[�ƃr���[���֘A�Â���"),
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
	DepthResourceDesc.Width = WND_W;									// ���ƍ����̓����_�[�^�[�Q�b�g�Ɠ���.
	DepthResourceDesc.Height = WND_H;									// ��ɓ���.
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

// �V�[���r���[�̍쐬.
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
		_T("�萔�o�b�t�@�쐬"),
		&ID3D12Device::CreateCommittedResource, m_pDevice12.Get(),
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(SceneConstBuff.ReleaseAndGetAddressOf()));

	MappedSceneData = nullptr;

	MyAssert::IsFailed(
		_T("�V�[�����̃}�b�v"),
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
		(DirectX::XM_PIDIV4,//��p��45��
		static_cast<float>(desc.Width) / static_cast<float>(desc.Height),//�A�X��
		0.1f,//�߂���
		1000.0f//������
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

	// �f�X�N���v�^�̐擪�n���h�����擾���Ă���.
	auto heapHandle = SceneDescHeap->GetCPUDescriptorHandleForHeapStart();

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = SceneConstBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = static_cast<UINT>(SceneConstBuff->GetDesc().Width);

	// �萔�o�b�t�@�r���[�̍쐬.
	m_pDevice12->CreateConstantBufferView(&cbvDesc, heapHandle);
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

// �e�N�X�`��������e�N�X�`���o�b�t�@�쐬�A���g���R�s�[����.
ID3D12Resource* CDirectX12::CreateTextureFromFile(const char* Texpath)
{
	std::string TexPath = Texpath;
	//�e�N�X�`���̃��[�h
	DirectX::TexMetadata	Metadata = {};
	DirectX::ScratchImage	ScratchImg = {};

	//�e�N�X�`���̃t�@�C���p�X. 
	std::wstring wTexPath = MyString::StringToWString(TexPath);

	//�g���q���擾.
	auto Extension = MyFilePath::GetExtension(TexPath);

	HRESULT Result = m_LoadLambdaTable[Extension](wTexPath,
		&Metadata,
		ScratchImg);

	if (FAILED(Result)) {

		// �G���[���b�Z�[�W���쐬.
		std::string_view ErrorMessage = MyAssert::HResultToJapanese(Result);

		// ���b�Z�[�W�{�b�N�X��\��.
		MessageBoxA(nullptr, std::string(ErrorMessage).c_str(), "Texture Load Error", MB_OK | MB_ICONERROR);

		return nullptr;
	}

	auto Image = ScratchImg.GetImage(0, 0, 0);//���f�[�^���o

	//WriteToSubresource�œ]������p�̃q�[�v�ݒ�
	auto TexHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);

	// �e�N�X�`�����\�[�X�̃f�B�X�N���^.
	D3D12_RESOURCE_DESC ResDesc = {};
	ResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 2D �e�N�X�`��.
	ResDesc.Alignment = 0;                                 // �f�t�H���g�A���C�����g.
	ResDesc.Width = Metadata.width;                        // �e�N�X�`���̕�.
	ResDesc.Height = static_cast<UINT>(Metadata.height);   // �e�N�X�`���̍���.
	ResDesc.DepthOrArraySize = static_cast<UINT16>(Metadata.arraySize); // �z��̃T�C�Y.
	ResDesc.MipLevels = static_cast<UINT16>(Metadata.mipLevels);        // MIP ���x��.
	ResDesc.Format = Metadata.format;                     // �e�N�X�`���t�H�[�}�b�g.
	ResDesc.SampleDesc.Count = 1;                         // �T���v�����O (�}���`�T���v���͂��Ȃ�).
	ResDesc.SampleDesc.Quality = 0;                       // �T���v�����O�i�� (�f�t�H���g).
	ResDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;        // ���C�A�E�g (�h���C�o�[�ɔC����).
	ResDesc.Flags = D3D12_RESOURCE_FLAG_NONE;             // ����ȃt���O�Ȃ�.

	ID3D12Resource* Texbuff = nullptr;
	Result = m_pDevice12->CreateCommittedResource(
		&TexHeapProp,
		D3D12_HEAP_FLAG_NONE,//���Ɏw��Ȃ�
		&ResDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&Texbuff)
	);

	if (FAILED(Result)) {
		return nullptr;
	}

	Result = Texbuff->WriteToSubresource(0,
		nullptr,			// �S�̈�փR�s�[.
		Image->pixels,		// ���f�[�^�A�h���X.
		static_cast<UINT>(Image->rowPitch),	// 1���C���T�C�Y.
		static_cast<UINT>(Image->slicePitch)// �S�T�C�Y.
	);

	if (FAILED(Result)) {
		return nullptr;
	}

	return Texbuff;
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