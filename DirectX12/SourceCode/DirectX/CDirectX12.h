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
		uint16_t BoneNo[2];		// �{�[���ԍ�		:  4Byte.
		uint8_t  BoneWeight;    // �{�[���e���x	:  1Byte.
		uint8_t  EdgeFlg;       // �֊s���t���O   :  1Byte.
		uint16_t Dummy;			// 
	}; // 38Byte.

public:
	CDirectX12();
	~CDirectX12();

	//DirectX12�\�z.
	bool Create(HWND hWnd);
	void UpDate();

	//�f�o�C�X�R���e�L�X�g���擾.
	ID3D12Device* GetDevice() const { return m_pDevice12; }
	

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
	ID3D12Device*		m_pDevice12;	// DirectX12�̃f�o�C�X�R���e�L�X�g.
	IDXGIFactory6*		m_pDxgiFactory;	// �f�B�X�v���C�ɏo�͂��邽�߂�API.
	IDXGISwapChain4*	m_pSwapChain;	// �X���b�v�`�F�[��.

	ID3D12CommandAllocator*		m_pCmdAllocator;// �R�}���h�A���P�[�^(���߂����߂Ă����������̈�).	
	ID3D12GraphicsCommandList*	m_pCmdList;		// �R�}���h���X�g.
	ID3D12CommandQueue*			m_pCmdQueue;	// �R�}���h�L���[.

	XMFLOAT3					m_Vertex[3];		// ���_.
};