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

			//�����_�[�^�[�Q�b�g���w��.
			auto rtvH = RTVHeaps->GetCPUDescriptorHandleForHeapStart();
			rtvH.ptr += static_cast<ULONG_PTR>(BBIndex * m_pDevice12->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));

			m_pCmdList->OMSetRenderTargets(
				1,			// �����_�[�^�[�Q�b�g��(1�ł悢).
				&rtvH,		// �����_�[�^�[�Q�b�g�n���h���̐擪�A�h���X.
				false,		// �������ɘA�����Ă��邩.
				nullptr);	// �n���h��(nullptr)�ł悢.

			// ��ʃN���A.
			float CrearColor[] = { 0.f, 0.5f, 0.f, 1.f };
			m_pCmdList->ClearRenderTargetView(
				rtvH,
				CrearColor,
				0,
				nullptr);

			BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

			m_pCmdList->ResourceBarrier(1, &BarrierDesc);

			// ���߂��I������.
			m_pCmdList->Close();

			//	�R�}���h���X�g�����s����.
			ID3D12CommandList* CmdList[] = { m_pCmdList };
			m_pCmdQueue->ExecuteCommandLists(
				1,			// ���s����R�}���h���X�g��. 
				CmdList);	// �R�}���h���X�g�̐擪�A�h���X.

			m_pCmdQueue->ExecuteCommandLists(1, CmdList);
			m_pCmdQueue->Signal(Fence, ++FenceValue);


			if (Fence->GetCompletedValue() != FenceValue) {
				auto event = CreateEvent(nullptr, false, false, nullptr);
				Fence->SetEventOnCompletion(FenceValue, event);
				WaitForSingleObject(event, INFINITE);
				CloseHandle(event);
			}
			m_pCmdAllocator->Reset();				// �L���[���N���A.
			m_pCmdList->Reset(m_pCmdAllocator, nullptr);// �ĂуR�}���h���X�g�����߂鏀��.


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
