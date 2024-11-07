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

	// PMD�w�b�_�[�\����.
	struct PMDHeader
	{
		float Version;			// �o�[�W����.
		char ModelName[20];		// ���f���̖��O.
		char ModelComment[256];	// ���f���̃R�����g.
	};

	// PMD���_�\����.
	struct PMDVertex
	{
		DirectX::XMFLOAT3 Pos;		// ���_���W		: 12Byte.
		DirectX::XMFLOAT3 Normal;	// �@���x�N�g��	: 12Byte.
		DirectX::XMFLOAT2 uv;		// uv���W		:  8Byte.
		uint16_t BoneNo[2];			// �{�[���ԍ�	:  4Byte.
		uint8_t  BoneWeight;		// �{�[���e���x	:  1Byte.
		uint8_t  EdgeFlg;			// �֊s���t���O :  1Byte.
		uint16_t Padding;			// �p�f�B���O	:  2Byte.
	};								// ���v         : 40Byte.

	// TODO : �p�f�B���O�̑Ώ��@���l����.
	//PMD�}�e���A���\����
#pragma pack(1)
	struct PMDMaterial {
		DirectX::XMFLOAT3 Diffuse;  // �f�B�t���[�Y�F			: 12Byte.
		float	 Alpha;				// ���l						:  4Byte.
		float    Specularity;		// �X�y�L�����̋���			:  4Byte.
		DirectX::XMFLOAT3 Specular; // �X�y�L�����F				: 12Byte.
		DirectX::XMFLOAT3 Ambient;  // �A���r�G���g�F			: 12Byte.
		uint8_t  ToonIdx;			// �g�D�[���ԍ�				:  1Byte.
		uint8_t  EdgeFlg;			// Material���̗֊s���t���O	:  1Byte.
//		uint16_t Padding;			// �p�f�B���O				:  2Byte.
		uint32_t IndicesNum;		// ���蓖����C���f�b�N�X��	:  4Byte.
		char     TexFilePath[20];	// �e�N�X�`���t�@�C����		: 20Byte.
	};// 70Byte(�p�f�B���O�Ȃ�).
#pragma pack()

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
	void Draw();
	void EndDraw();

	// �X���b�v�`�F�[���擾.
	const MyComPtr<IDXGISwapChain4> GetSwapChain();

	// DirextX12�f�o�C�X�擾.
	const MyComPtr<ID3D12Device> GetDevice();

	// �R�}���h���X�g�擾.
	const MyComPtr<ID3D12GraphicsCommandList> GetCommandList();

	MyComPtr<ID3D12Resource> GetTextureByPath(const char* texpath);

	//�f�o�C�X�R���e�L�X�g���擾.
	//ID3D12Device* GetDevice() const { return m_pDevice12; }


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

	// �O���t�B�b�N�p�C�v���C���X�e�[�g�̐ݒ�.
	void CreateGraphicPipeline(MyComPtr<ID3D12PipelineState>& GraphicPipelineState);

	//�e�N�X�`�����[�_�e�[�u���̍쐬.
	void CreateTextureLoadTable();

	// texture�̍쐬.
	ID3D12Resource* CreateGrayGradationTexture();
	ID3D12Resource* CreateWhiteTexture();
	ID3D12Resource* CreateBlackTexture();
	// �ǂݍ���
	ID3D12Resource* LoadTextureFromFile(std::string& texPath);


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
	* @brief	ErroeBlob�ɓ������G���[���o��.
	* @param	���������t�@�C�������邩�ǂ���.
	* @param	���̑��̃R���p�C���G���[���e.
	*******************************************/
	void ShaderCompileError(const HRESULT& Result, ID3DBlob* ErrorMsg);


	/*******************************************
	* @brief	�V�F�[�_�[�̃R���p�C��.
	* @param	�t�@�C���p�X.
	* @param	�G���g���[�|�C���g.
	* @param	.
	* @param	�V�F�[�_�[�u���u(����o�C�i��).
	*******************************************/
	HRESULT CompileShaderFromFile(
		const std::wstring& FilePath,
		LPCSTR EntryPoint,
		LPCSTR Target,
		ID3DBlob** ShaderBlob);

	/*******************************************
	* @brief	�e�N�X�`��������e�N�X�`���o�b�t�@�쐬�A���g���R�s�[����.
	* @param	�t�@�C���p�X.
	* @param	���\�[�X�̃|�C���^��Ԃ�.
	*******************************************/
	ID3D12Resource* CreateTextureFromFile(const char* Texpath);


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
	MyComPtr<ID3D12PipelineState>			m_pPipelineState;		// �`��ݒ�.
	MyComPtr<ID3D12RootSignature>			m_pRootSignature;		// ���[�g�V�O�l�`��.
	std::unique_ptr<D3D12_VIEWPORT>			m_pViewport;			// �r���[�|�[�g.
	std::unique_ptr<D3D12_RECT>				m_pScissorRect;			// �V�U�[��`.

	using LoadLambda_t = std::function<HRESULT(const std::wstring& Path, DirectX::TexMetadata*, DirectX::ScratchImage&)>;
	std::map<std::string, LoadLambda_t>		m_LoadLambdaTable;

	// �t�@�C�����p�X�ƃ��\�[�X�̃}�b�v�e�[�u��.
	std::map<std::string, MyComPtr<ID3D12Resource>>	m_ResourceTable;

};
