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

using namespace DirectX;

/**************************************************
*	DirectX12�Z�b�g�A�b�v.
**/
class CDirectX12
{
public:

	// ���_�\����.
	struct VerTex
	{
		XMFLOAT3 Pos;	// xyz���W.
		XMFLOAT2 uv;	// uv���W.
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
		XMFLOAT3 Pos;			// ���_���W		: 12Byte.
		XMFLOAT3 Normal;        // �@���x�N�g��	: 12Byte.
		XMFLOAT2 uv;            // uv���W		:  8Byte.
		uint16_t BoneNo[2];		// �{�[���ԍ�	:  4Byte.
		uint8_t  BoneWeight;    // �{�[���e���x	:  1Byte.
		uint8_t  EdgeFlg;       // �֊s���t���O :  1Byte.
		uint16_t Padding;		// �p�f�B���O	:  2Byte.
	}; // 40Byte.
	
	//PMD�}�e���A���\����
	struct PMDMaterial {
		XMFLOAT3 Diffuse;       // �f�B�t���[�Y�F			: 12Byte.
		float	 Alpha;         // ���l						:  4Byte.
		float    Specularity;   // �X�y�L�����̋���			:  4Byte.
		XMFLOAT3 Specular;      // �X�y�L�����F				: 12Byte.
		XMFLOAT3 Ambient;       // �A���r�G���g�F			: 12Byte.
		uint8_t  ToonIdx;		// �g�D�[���ԍ�				:  1Byte.
		uint8_t  EdgeFlg;		// Material���̗֊s���t���O	:  1Byte.
		uint16_t Padding;       // �p�f�B���O				:  2Byte.
		uint32_t IndiceNum;		// ���蓖����C���f�b�N�X��	:  4Byte.
		char     TexFilePath[20];// �e�N�X�`���t�@�C����	: 20Byte.
	};// 72Byte.

	// �V�F�[�_���ɓ�������}�e���A���f�[�^.
	struct MaterialForHlsl {
		XMFLOAT3 Diffuse;		// �f�B�t���[�Y�F.		
		float	 Alpha;			// ���l.		
		XMFLOAT3 Specular;		// �X�y�L�����̋�.		
		float	 Specularity;	// �X�y�L�����F.		
		XMFLOAT3 Ambient;		// �A���r�G���g�F.		
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

	//�f�o�C�X�R���e�L�X�g���擾.
	ID3D12Device* GetDevice() const { return m_pDevice12; }
	

private:// ����Ă����񂾂�˂�.

	// DXGI�̐���.
	void CreateDXGIFactory();

	// �R�}���h�ނ̐���.
	void CreateCommandObject();

	// �X���b�v�`�F�[���̍쐬.
	void CreateSwapChain();

	// 

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
	* @param	�������镶����.
	*******************************************/
	void ShaderCompileError(const HRESULT& Result, ID3DBlob* ErrorMsg);

private:
	HWND m_hWnd;	// �E�B���h�E�n���h��.

	ID3D12Device*		m_pDevice12;	// DirectX12�̃f�o�C�X�R���e�L�X�g.
	IDXGIFactory6*		m_pDxgiFactory;	// �f�B�X�v���C�ɏo�͂��邽�߂�API.
	IDXGISwapChain4*	m_pSwapChain;	// �X���b�v�`�F�[��.

	ID3D12CommandAllocator*		m_pCmdAllocator;// �R�}���h�A���P�[�^(���߂����߂Ă����������̈�).	
	ID3D12GraphicsCommandList*	m_pCmdList;		// �R�}���h���X�g.
	ID3D12CommandQueue*			m_pCmdQueue;	// �R�}���h�L���[.

	XMFLOAT3					m_Vertex[3];		// ���_.
};