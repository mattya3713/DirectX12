#pragma once

#include<d3d12.h>
#include<DirectXMath.h>
#include<vector>
#include<string>

// �O���錾.
class CDirectX12;
class CPMDRenderer;

class CPMDActor
{
	friend CPMDRenderer;
private:

	// PMD�w�b�_�[�\����.
	struct PMDHeader
	{
		float Version;			// �o�[�W����.
		char ModelName[20];		// ���f���̖��O.
		char ModelComment[256];	// ���f���̃R�����g.
	};

	// TODO : �p�f�B���O�̑Ώ��@���l����.
	//PMD�}�e���A���\����
#pragma pack(1)
	struct PMDMaterial {
		DirectX::XMFLOAT3 Diffuse;  // �f�B�t���[�Y�F				: 12Byte.
		float	 Alpha;				// ���l						:  4Byte.
		float    Specularity;		// �X�y�L�����̋���			:  4Byte.
		DirectX::XMFLOAT3 Specular; // �X�y�L�����F				: 12Byte.
		DirectX::XMFLOAT3 Ambient;  // �A���r�G���g�F				: 12Byte.
		uint8_t  ToonIdx;			// �g�D�[���ԍ�				:  1Byte.
		uint8_t  EdgeFlg;			// Material���̗֊s���t���O	:  1Byte.
//		uint16_t Padding;			// �p�f�B���O					:  2Byte.
		uint32_t IndicesNum;		// ���蓖����C���f�b�N�X��		:  4Byte.
		char     TexFilePath[20];	// �e�N�X�`���t�@�C����		: 20Byte.
	};								// ���v						: 70Byte(�p�f�B���O�Ȃ�).
#pragma pack()

	// PMD���_�\����.
	struct PMDVertex
	{
		DirectX::XMFLOAT3 Pos;		// ���_���W		: 12Byte.
		DirectX::XMFLOAT3 Normal;	// �@���x�N�g��	: 12Byte.
		DirectX::XMFLOAT2 uv;		// uv���W		:  8Byte.
		uint16_t BoneNo[2];			// �{�[���ԍ�		:  4Byte.
		uint8_t  BoneWeight;		// �{�[���e���x	:  1Byte.
		uint8_t  EdgeFlg;			// �֊s���t���O	:  1Byte.
		uint16_t Padding;			// �p�f�B���O		:  2Byte.
	};								// ���v			: 40Byte.


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

	struct Transform {
		//�����Ɏ����Ă�XMMATRIX�����o��16�o�C�g�A���C�����g�ł��邽��
		//Transform��new����ۂɂ�16�o�C�g���E�Ɋm�ۂ���
		void* operator new(size_t size);
		DirectX::XMMATRIX world;
	};

	
	//�ǂݍ��񂾃}�e���A�������ƂɃ}�e���A���o�b�t�@���쐬
	HRESULT CreateMaterialData();
	
	//�}�e���A�����e�N�X�`���̃r���[���쐬
	HRESULT CreateMaterialAndTextureView();

	//���W�ϊ��p�r���[�̐���
	HRESULT CreateTransformView();

	//PMD�t�@�C���̃��[�h
	HRESULT LoadPMDFile(const char* path);

	float _angle;//�e�X�g�pY����]
public:
	CPMDActor(const char* filepath,CPMDRenderer& renderer);
	~CPMDActor();
	///�N���[���͒��_����у}�e���A���͋��ʂ̃o�b�t�@������悤�ɂ���
	CPMDActor* Clone();
	void Update();
	void Draw();

private:
	CPMDRenderer& m_pRenderer;
	CDirectX12& m_pDx12;

	//���_�֘A
	MyComPtr<ID3D12Resource>		m_pVertexBuffer;			// ���_�o�b�t�@.
	MyComPtr<ID3D12Resource>		m_pIndexBuffer;				// �C���f�b�N�X�o�b�t�@.
	D3D12_VERTEX_BUFFER_VIEW		m_pVertexBufferView;		// ���_�o�b�t�@�r���[.
	D3D12_INDEX_BUFFER_VIEW			m_pIndexBufferView;			// �C���f�b�N�X�o�b�t�@�r���[.

	MyComPtr<ID3D12Resource>		m_pTransformMat;			// ���W�ϊ��s��(���̓��[���h�̂�).
	MyComPtr<ID3D12DescriptorHeap>	m_pTransformHeap;			// ���W�ϊ��q�[�v.

	Transform						m_Transform;							
	Transform*						m_MappedTransform;					
	MyComPtr<ID3D12Resource>		m_pTransformBuff;		

	//�}�e���A���֘A
	std::vector<MyComPtr<Material>>			m_pMaterial;			// �}�e���A��.
	MyComPtr<ID3D12Resource>				m_pMaterialBuff;	// �}�e���A���o�b�t�@.
	std::vector<MyComPtr<ID3D12Resource>>	m_pTextureResource;	// �摜���\�[�X.
	std::vector<MyComPtr<ID3D12Resource>>	m_pSphResource;		// Sph���\�[�X.
	std::vector<MyComPtr<ID3D12Resource>>	m_pSpaResource;		// Spa���\�[�X.
	std::vector<MyComPtr<ID3D12Resource>>	m_pToonResource;	// �g�D�[�����\�|�X.

	MyComPtr<ID3D12DescriptorHeap> m_pMaterialHeap;				// �}�e���A���q�[�v(5�Ԃ�)
};

