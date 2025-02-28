#pragma once

#include<d3d12.h>
#include<DirectXMath.h>
#include<vector>
#include<string>
#include"PMXStructHeader.h"	// PMX�p�\���̂܂Ƃ�.

// �O���錾.
class CDirectX12;
class CPMXRenderer;

/**************************************************
*	PMX���f���N���X.
*	�S���F���e ����
**/

class CPMXActor
{
	friend CPMXRenderer;

public:
	CPMXActor(const char* filepath, CPMXRenderer& renderer);
	~CPMXActor();
	///�N���[���͒��_����у}�e���A���͋��ʂ̃o�b�t�@������悤�ɂ���
	CPMXActor* Clone();
	void Update();
	void Draw();

	// �A�j���[�V�����J�n.
	void PlayAnimation();

	// �A�j���[�V�����̍X�V.
	void MotionUpdate();

	/*******************************************
	* @brief	VMD�̃��[�h.
	* @param	�t�@�C���p�X.
	* @param	���[�r�[��.
	*******************************************/
	void LoadVMDFile(const char* FilePath, const char* Name);

private:

	struct Transform {
		// �����Ɏ����Ă�XMMATRIX�����o��16�o�C�g�A���C�����g�ł��邽��.
		// Transform��new����ۂɂ�16�o�C�g���E�Ɋm�ۂ���.
		void* operator new(size_t size);
		DirectX::XMMATRIX world;
	};

	// �ǂݍ��񂾃}�e���A�������ƂɃ}�e���A���o�b�t�@���쐬.
	void CreateMaterialData();
	
	// �}�e���A�����e�N�X�`���̃r���[���쐬.
	void CreateMaterialAndTextureView();

	// ���W�ϊ��p�r���[�̐���.
	void CreateTransformView();

	// PMD�t�@�C���̃��[�h.
	void LoadPMXFile(const char* FilePath);
	
	// PMX�w�b�^�[�ǂݍ���.
	void ReadPMXHeader(FILE* fp, PMX::Header* Header);

	// ������̓ǂݍ���.
	void ReadString(FILE* fp, std::string& Name);

	// �p�X�̕ϊ��̊֐��|�C���^.
	void ConvertUTF16ToUTF8(const std::vector<uint8_t>& buffer, std::string& OutString); 
	void ConvertUTF8(const std::vector<uint8_t>& buffer, std::string& OutString);
	using PathConverterFunction = void(CPMXActor::*)(const std::vector<uint8_t>&, std::string&);
	PathConverterFunction PathConverter = nullptr;

	// �C���f�b�N�X��ǂݍ��ގ��̊֐���I�����邽�߂̊֐��|�C���^.
	void ReadPMXIndices1Byte(FILE* fp,const uint32_t& IndicesNum, std::vector<uint32_t>* Faces);
	void ReadPMXIndices2Byte(FILE* fp,const uint32_t& IndicesNum, std::vector<uint32_t>* Faces);
	void ReadPMXIndices4Byte(FILE* fp,const uint32_t& IndicesNum, std::vector<uint32_t>* Faces);
	using ReadIndicesFunction = void(CPMXActor::*)(FILE*, const uint32_t&, std::vector<uint32_t>*);
	ReadIndicesFunction ReadIndices = nullptr;

	/*******************************************
	* @brief	PMX�o�C�i������C���f�b�N�X���ƃC���f�b�N�X��ǂݍ���.
	* @param	�ǂݍ��݃t�@�C���|�C���^.
	* @param	�ǂݍ��񂾃C���f�b�N�X.
	*******************************************/
	void ReadPMXIndices(FILE* fp, std::vector<uint32_t>* Faces, uint32_t* IndicesNum);

	/*******************************************
	* @brief	��]���𖖒[�܂œ`�d������ċA�֐�.
	* @param	��]���������{�[���m�[�h.
	* @param    ��]�s��.
	*******************************************/
	//void RecursiveMatrixMultipy(BoneNode* node, const DirectX::XMMATRIX& mat);

	float _angle;//�e�X�g�pY����]

	float GetYFromXOnBezier(
		float x, 
		const DirectX::XMFLOAT2& a, 
		const DirectX::XMFLOAT2& b, uint8_t n = 12);

	/*******************************************
	* @brief    �w�肳�ꂽ�T�C�Y���̃f�[�^��ǂݍ��݁A4�o�C�g�̒l�Ƃ��ĕԂ�.
	* @param    �t�@�C���|�C���^�B�ǂݍ��ݑΏۂ̃t�@�C�����w���܂�.
	* @param    �ǂݍ��ރf�[�^�̃T�C�Y(1,2,4�o�C�g�̂ǂꂩ).
	* @return   �ǂݍ��񂾃f�[�^��4�o�C�g��`uint32_t`�^�ɕϊ������l
	* @throw	�m��Ȃ��T�C�Y.
	*******************************************/
	uint32_t ReadAndCastIndices(FILE* fp, uint8_t indexSize);

private:
	CPMXRenderer& m_pRenderer;
	CDirectX12& m_pDx12;

	// ���_�֘A.
	MyComPtr<ID3D12Resource>		m_pVertexBuffer;			// ���_�o�b�t�@.
	PMX::VertexForHLSL*				m_pMappedVertex;			// ���_�}�b�v.
	std::vector<PMX::VertexForHLSL>	m_VerticesForHLSL;			// GPU�ɑ��钸�_���W.
	D3D12_VERTEX_BUFFER_VIEW		m_pVertexBufferView;		// ���_�o�b�t�@�r���[.

	// �C���f�b�N�X�֘A.
	MyComPtr<ID3D12Resource>		m_pIndexBuffer;				// �C���f�b�N�X�o�b�t�@.
	std::vector<uint32_t>			m_Faces;					// �C���f�b�N�X.
	uint32_t*						m_MappedIndex;				// ���_�}�b�v.
	D3D12_INDEX_BUFFER_VIEW			m_pIndexBufferView;			// �C���f�b�N�X�o�b�t�@�r���[.


	MyComPtr<ID3D12Resource>		m_pTransformMat;			// ���W�ϊ��s��(���̓��[���h�̂�).

	// ���W�֘A.
	Transform						m_Transform;				// ���W.		
	MyComPtr<ID3D12DescriptorHeap>	m_pTransformHeap;			// ���W�ϊ��q�[�v.
	DirectX::XMMATRIX*				m_MappedMatrices;			// GPU�Ƃ݂���W.					
	MyComPtr<ID3D12Resource>		m_pTransformBuffer;			// �o�b�t�@.

	//�}�e���A���֘A.
	MyComPtr<ID3D12Resource>				m_pMaterialBuff;	// �}�e���A���o�b�t�@.
	char*									m_pMappedMaterial;	// �}�e���A���}�b�v
	MyComPtr<ID3D12DescriptorHeap>			m_pMaterialHeap;	// �}�e���A���q�[�v.
	std::vector<PMX::Material>				m_Materials;		// �}�e���A��.
	std::vector<PMX::MaterialForHLSL>		m_MaterialsForHLSL;	// GPU�ɑ���}�e���A�����.


	std::vector<MyComPtr<ID3D12Resource>>	m_pTextureResource;	// �摜���\�[�X.
	std::vector<MyComPtr<ID3D12Resource>>	m_pSphResource;		// Sph���\�[�X.
	std::vector<MyComPtr<ID3D12Resource>>	m_pToonResource;	// �g�D�[�����\�|�X.



	
};

