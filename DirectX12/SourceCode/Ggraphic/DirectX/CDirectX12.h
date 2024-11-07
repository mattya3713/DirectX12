#pragma once

//�x���ɂ��ẴR�[�h���͂𖳌��ɂ���.4005:�Ē�`.
#pragma warning(disable:4005)

//�w�b�_�Ǎ�.
#include <D3D12.h>
#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <DirectXTex.h>

// XXX : �̂̃w�b�_�[��include����Ă��܂����ߒ��p�X.
#include "d3dcompiler.h"	

//���C�u�����ǂݍ���.
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

// ���f���̒��_�T�C�Y.
constexpr size_t PmdVertexSize = 38;

/**************************************************
*	DirectX12�Z�b�g�A�b�v.
**/
class CDirectX12
{
public:

	// ���_�\����.
	struct VerTex
	{
		DirectX::XMFLOAT3 Pos;	// xyz���W.
		DirectX::XMFLOAT2 uv;	// uv���W.
	};



	// �V�F�[�_���ɓ�������}�e���A���f�[�^.
	struct MaterialForHlsl {
		DirectX::XMFLOAT3 Diffuse;		// �f�B�t���[�Y�F.		
		float	 Alpha;					// ���l.		
		DirectX::XMFLOAT3 Specular;		// �X�y�L�����̋�.		
		float	 Specularity;			// �X�y�L�����F.		
		DirectX::XMFLOAT3 Ambient;		// �A���r�G���g�F.		
	};

	// ����ȊO�̃}�e���A���f�[�^.
	struct AdditionalMaterial {
		std::string TexPath;	// �e�N�X�`���t�@�C���p�X.
		int			ToonIdx;	// �g�D�[���ԍ�.
		bool		EdgeFlg;	// �}�e���A�����̗֊s���t���O.
	};

	// �܂Ƃ߂�����.
	struct Material {
		unsigned int IndicesNum;	//�C���f�b�N�X��.
		MaterialForHlsl Material;
		AdditionalMaterial Additional;
	};

public:
	CDirectX12();
	~CDirectX12();

	//DirectX12�\�z.
	bool Create(HWND hWnd);
	void UpDate();

	void BeginDraw();
	void EndDraw();

	// �X���b�v�`�F�[���擾.
	const MyComPtr<IDXGISwapChain4> GetSwapChain();

	// DirextX12�f�o�C�X�擾.
	const MyComPtr<ID3D12Device> GetDevice();

	// �R�}���h���X�g�擾.
	const MyComPtr<ID3D12GraphicsCommandList> GetCommandList();

	// �e�N�X�`�����擾.
	MyComPtr<ID3D12Resource> GetTextureByPath(const char* texpath);



private:// ����Ă����񂾂�˂�.

	// DXGI�̐���.
	void CreateDXGIFactory(MyComPtr<IDXGIFactory6>& DxgiFactory);

	// �R�}���h�ނ̐���.
	void CreateCommandObject(
		MyComPtr<ID3D12CommandAllocator>&	CmdAllocator,
		MyComPtr<ID3D12GraphicsCommandList>&CmdList,
		MyComPtr<ID3D12CommandQueue>&		CmdQueue);

	// �X���b�v�`�F�[���̍쐬.
	void CreateSwapChain(MyComPtr<IDXGISwapChain4>& SwapChain);

	// �����_�[�^�[�Q�b�g�̍쐬.
	void CreateRenderTarget(
		MyComPtr<ID3D12DescriptorHeap>&			RenderTargetViewHeap,
		std::vector<MyComPtr<ID3D12Resource>>&	BackBuffer);

	// �[�x�o�b�t�@�̍쐬.
	void CreateDepthDesc(
		MyComPtr<ID3D12Resource>&		DepthBuffer,
		MyComPtr<ID3D12DescriptorHeap>&	DepthHeap);

	// �t�F���X�̍쐬.
	void CreateFance(MyComPtr<ID3D12Fence>& Fence);



private:
	/*******************************************
	* @brief	�A�_�v�^�[��������.
	* @param	�������镶����.
	* @return   �������A�_�v�^�[��Ԃ�.
	*******************************************/
	IDXGIAdapter* FindAdapter(std::wstring FindWord);

	/*******************************************
	* @brief	�f�o�b�O���C���[���N��.
	*******************************************/
	void EnableDebuglayer();

	/*******************************************
	* @brief	�e�N�X�`��������e�N�X�`���o�b�t�@�쐬�A���g���R�s�[����.
	* @param	�t�@�C���p�X.
	* @param	���\�[�X�̃|�C���^��Ԃ�.
	*******************************************/
	ID3D12Resource* CreateTextureFromFile(const char* Texpath);

	/*******************************************
	* @brief	 �e�N�X�`�����[�h�e�[�u���̍쐬.
	*******************************************/
	void CreateTextureLoadTable();


private:
	HWND m_hWnd;	// �E�B���h�E�n���h��.

	// DXGI.
	MyComPtr<IDXGIFactory6>					m_pDxgiFactory;			// �f�B�X�v���C�ɏo�͂��邽�߂�API.
	MyComPtr<IDXGISwapChain4>				m_pSwapChain;			// �X���b�v�`�F�[��.

	// DirectX12.
	MyComPtr<ID3D12Device>					m_pDevice12;			// DirectX12�̃f�o�C�X�R���e�L�X�g.
	MyComPtr<ID3D12CommandAllocator>		m_pCmdAllocator;		// �R�}���h�A���P�[�^(���߂����߂Ă����������̈�).	
	MyComPtr<ID3D12GraphicsCommandList>		m_pCmdList;				// �R�}���h���X�g.
	MyComPtr<ID3D12CommandQueue>			m_pCmdQueue;			// �R�}���h�L���[.

	// �����_�[�^�[�Q�b�g.
	MyComPtr<ID3D12DescriptorHeap>			m_pRenderTargetViewHeap;// �����_�[�^�[�Q�b�g�r���[.
	std::vector<MyComPtr<ID3D12Resource>>	m_pBackBuffer;			// �o�b�N�o�b�t�@.

	// �[�x�o�b�t�@.
	MyComPtr<ID3D12Resource>				m_pDepthBuffer;			// �[�x�o�b�t�@.
	MyComPtr<ID3D12DescriptorHeap>			m_pDepthHeap;			// �[�x�X�e���V���r���[�̃f�X�N���v�^�q�[�v. 
	D3D12_CLEAR_VALUE						m_DepthClearValue;		// �[�x�̃N���A�l.

	// �t�F���X��.
	MyComPtr<ID3D12Fence>					m_pFence;				// �����҂���.
	UINT64									m_FenceValue;			// �����J�E���^�[.

	// �`�����̐ݒ�.
	MyComPtr<ID3D12PipelineState>			m_pPipelineState;		// �p�C�v���C��.
	MyComPtr<ID3D12RootSignature>			m_pRootSignature;		// ���[�g�V�O�l�`��.
	std::unique_ptr<D3D12_VIEWPORT>			m_pViewport;			// �r���[�|�[�g.
	std::unique_ptr<D3D12_RECT>				m_pScissorRect;			// �V�U�[��`.

	using LoadLambda_t = std::function<HRESULT(const std::wstring& Path, DirectX::TexMetadata*, DirectX::ScratchImage&)>;
	std::map<std::string, LoadLambda_t>		m_LoadLambdaTable;

	// �t�@�C�����p�X�ƃ��\�[�X�̃}�b�v�e�[�u��.
	std::map<std::string, MyComPtr<ID3D12Resource>>	m_ResourceTable;

};
