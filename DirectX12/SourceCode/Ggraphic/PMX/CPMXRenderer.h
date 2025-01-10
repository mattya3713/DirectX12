#pragma once

#include<d3d12.h>
#include<vector>
#include<memory>

// �O���錾.
class CDirectX12;
class CPMXActor;

/**************************************************
*	PMX�p�`��p�C�v���C���N���X.
*	�S���F���e ����
**/

class CPMXRenderer
{
	friend CPMXActor;
public:
	CPMXRenderer(CDirectX12& dx12);
	~CPMXRenderer();
	void Update();
	void Draw();

	// PMD�p�̃p�C�v���C���X�e�[�g���擾.
	ID3D12PipelineState* GetPipelineState();

	// PMD�p�̃��[�g�������擾.
	ID3D12RootSignature* GetRootSignature();

private:
	// �e�N�X�`���̔ėp�f�ނ��쐬.
	ID3D12Resource* CreateDefaultTexture(size_t Width, size_t Height);
	// ���e�N�X�`���̐���.
	ID3D12Resource* CreateWhiteTexture();
	// ���e�N�X�`���̐���.
	ID3D12Resource* CreateBlackTexture();
	// �O���[�e�N�X�`���̐���.
	ID3D12Resource* CreateGrayGradationTexture();

	// �p�C�v���C��������.
	void CreateGraphicsPipelineForPMD();
	// ���[�g�V�O�l�`��������.
	void CreateRootSignature();

	/*******************************************
	* @brief	�V�F�[�_�[�̃R���p�C��.
	* @param	�t�@�C���p�X.
	* @param	�G���g���[�|�C���g.
	* @param	�o�͌`��.
	* @param	�V�F�[�_�[�u���u(����o�C�i��).
	*******************************************/
	HRESULT CompileShaderFromFile(
		const std::wstring& FilePath,
		LPCSTR EntryPoint,
		LPCSTR Target,
		ID3DBlob** ShaderBlob);

private:
	CDirectX12& m_pDx12;

	MyComPtr<ID3D12PipelineState>	m_pPipelineState;		// �p�C�v���C��.
	MyComPtr<ID3D12RootSignature>	m_pRootSignature;		// ���[�g�V�O�l�`��.

	//PMD�p���ʃe�N�X�`��.
	MyComPtr<ID3D12Resource>		m_pWhiteTex;			// ���F�̃e�N�X�`��.
	MyComPtr<ID3D12Resource>		m_pBlackTex;			// ���F�̃e�N�X�`��.
	MyComPtr<ID3D12Resource>		m_pGradTex;				// ��<->���O���f�[�V�����̃e�N�X�`��.

};

