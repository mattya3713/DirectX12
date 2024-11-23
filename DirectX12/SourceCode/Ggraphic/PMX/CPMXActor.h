#pragma once

#include<d3d12.h>
#include<DirectXMath.h>
#include<vector>
#include<string>

// �O���錾.
class CDirectX12;
class CPMXRenderer;

class CPMXActor
{
	friend CPMXRenderer;
private:

	// PMX�w�b�_�[�\����.
	struct PMXHeader
	{
		float Version;				// �o�[�W����.
		uint8_t Encoding;			// �e�L�X�g�G���R�[�f�B���O�i0: UTF16, 1: UTF8�j.
		uint8_t AdditionalUV;		// �ǉ�UV��.
		uint8_t VertexIndexSize;	// ���_�C���f�b�N�X�T�C�Y.
		uint8_t TextureIndexSize;	// �e�N�X�`���C���f�b�N�X�T�C�Y.
		uint8_t MaterialIndexSize;	// �}�e���A���C���f�b�N�X�T�C�Y.
		uint8_t BoneIndexSize;		// �{�[���C���f�b�N�X�T�C�Y.
		uint8_t MorphIndexSize;		// ���[�t�C���f�b�N�X�T�C�Y.
		uint8_t RigidBodyIndexSize; // ���̃C���f�b�N�X�T�C�Y.
		std::string ModelName;      // ���f�����i�f�R�[�h��̕�����j.
		std::string ModelComment;   // ���f���R�����g�i�f�R�[�h��̕�����j.

		PMXHeader()
			: Version			( 0.0f )
			, Encoding			( 0 )
			, AdditionalUV		( 0 )
			, VertexIndexSize	( 0 )
			, TextureIndexSize	( 0 )
			, MaterialIndexSize	( 0 )
			, BoneIndexSize		( 0 )
			, MorphIndexSize	( 0 )
			, RigidBodyIndexSize( 0 )
			, ModelName			("")
			, ModelComment		("")
		{}
	};

	// PMX���f�����.
	struct PMXModelInfo {
		std::string ModelName;			// ���f����.
		std::string ModelNameEnglish;	// ���f����(�p��).
		std::string ModelComment;		// �R�����g.
		std::string ModelCommentEnglish;// �R�����g(�p��).
	};

	// TODO : �p�f�B���O�̑Ώ��@���l����.
	// PMD�}�e���A���\����.
#pragma pack(1)
	struct PMDMaterial {
		DirectX::XMFLOAT3 Diffuse;  // �f�B�t���[�Y�F			: 12Byte.
		float	 Alpha;				// ���l					:  4Byte.
		float    Specularity;		// �X�y�L�����̋���		:  4Byte.
		DirectX::XMFLOAT3 Specular; // �X�y�L�����F			: 12Byte.
		DirectX::XMFLOAT3 Ambient;  // �A���r�G���g�F			: 12Byte.
		uint8_t  ToonIdx;			// �g�D�[���ԍ�			:  1Byte.
		uint8_t  EdgeFlg;			// Material���̗֊s���׸�	:  1Byte.
//		uint16_t Padding;			// �p�f�B���O				:  2Byte.
		uint32_t IndicesNum;		// ���蓖����C���f�b�N�X��	:  4Byte.
		char     TexFilePath[20];	// �e�N�X�`���t�@�C����	: 20Byte.
									// ���v					: 70Byte(�p�f�B���O�Ȃ�).
		PMDMaterial()
			: Diffuse		(0.0f, 0.0f, 0.0f)
			, Alpha			(1.0f)
			, Specularity	(0.0f)
			, Specular		(0.0f, 0.0f, 0.0f)
			, Ambient		(0.0f, 0.0f, 0.0f)
			, ToonIdx		(0)
			, EdgeFlg		(0)
			, IndicesNum	(0)
			, TexFilePath	{}
		{}
	};
	
	// PMD�{�[���\����.
	struct PMDBone
	{
		unsigned char	BoneName[20];	// �{�[����.
		unsigned short	ParentNo;		// �e�{�[����.
		unsigned short	NextNo;			// ��[�̃{�[���ԍ�.
		unsigned char	TypeNo;			// �{�[���̎��.
		unsigned short	IKBoneNo;		// IK�{�[���ԍ�.
		DirectX::XMFLOAT3 Pos;			// �{�[���̊���W.

		PMDBone()
			: BoneName		{}
			, ParentNo		(0)
			, NextNo		(0)
			, TypeNo		(0)
			, IKBoneNo		(0)
			, Pos			(0.0f, 0.0f, 0.0f)
		{}
	};

#pragma pack()

	// PMD���_�\����.
	struct PMDVertex
	{
		DirectX::XMFLOAT3 Pos;		// ���_���W		: 12Byte.
		DirectX::XMFLOAT3 Normal;	// �@���x�N�g��	: 12Byte.
		DirectX::XMFLOAT2 UV;		// uv���W		:  8Byte.
		uint16_t BoneNo[2];			// �{�[���ԍ�		:  4Byte.
		uint8_t  BoneWeight;		// �{�[���e���x	:  1Byte.
		uint8_t  EdgeFlg;			// �֊s���t���O	:  1Byte.
		uint16_t Padding;			// �p�f�B���O		:  2Byte.
									// ���v			: 40Byte.
		PMDVertex()
			: Pos			(0.0f, 0.0f, 0.0f)
			, Normal		(0.0f, 0.0f, 0.0f)
			, UV			(0.0f, 0.0f)
			, BoneNo		{}
			, BoneWeight	(0)
			, EdgeFlg		(0)
			, Padding		(0)
		{}
	};								

	
	// �V�F�[�_���ɓ�������}�e���A���f�[�^.
	struct MaterialForHlsl {
		DirectX::XMFLOAT3	Diffuse;	// �f�B�t���[�Y�F.		
		float				Alpha;		// ���l.		
		DirectX::XMFLOAT3	Specular;	// �X�y�L�����̋�.		
		float				Specularity;// �X�y�L�����F.		
		DirectX::XMFLOAT3	Ambient;	// �A���r�G���g�F.		

		MaterialForHlsl()
			: Diffuse		(0.0f, 0.0f, 0.0f)
			, Alpha			(0.0f)
			, Specular		(0.0f, 0.0f, 0.0f)
			, Specularity	(0.0f)
			, Ambient		(0.0f, 0.0f, 0.0f)
		{}
	};

	// ����ȊO�̃}�e���A���f�[�^.
	struct AdditionalMaterial {
		std::string TexPath;	// �e�N�X�`���t�@�C���p�X.
		int			ToonIdx;	// �g�D�[���ԍ�.
		bool		EdgeFlg;	// �}�e���A�����̗֊s���t���O.

		AdditionalMaterial()
			: TexPath		{}
			, ToonIdx		(0)
			, EdgeFlg		(false)
		{}
	};

	// �܂Ƃ߂�����.
	struct Material {
		unsigned int IndicesNum;		// �C���f�b�N�X��.
		MaterialForHlsl Materialhlsl;	// �V�F�[�_���ɓ�������}�e���A���f�[�^.
		AdditionalMaterial Additional;	// ����ȊO�̃}�e���A���f�[�^.
		
		Material()
			: IndicesNum	(0)
			, Materialhlsl	{}
			, Additional	{}
		{}
	};

	struct Transform {
		// �����Ɏ����Ă�XMMATRIX�����o��16�o�C�g�A���C�����g�ł��邽��.
		// Transform��new����ۂɂ�16�o�C�g���E�Ɋm�ۂ���.
		void* operator new(size_t size);
		DirectX::XMMATRIX world;
	};

	struct BoneNode {
		int						BoneIndex;	// �{�[���C���f�b�N�X.
		DirectX::XMFLOAT3		StartPos;	// �{�[����_(��]���S).
		std::vector<BoneNode*>	Children;	// �q�m�[�h.

		BoneNode()
			: BoneIndex (0)
			, StartPos	(0.0f, 0.0f, 0.0f)
			, Children	{}
		{}
	};

	// ���[�V�����f�[�^.
	struct VMDKeyFrame {
		char BoneName[15];				// �{�[����.
		unsigned int FrameNo;			// �t���[���ԍ�(�Ǎ����͌��݂̃t���[���ʒu��0�Ƃ������Έʒu).
		DirectX::XMFLOAT3 Location;		// �ʒu.
		DirectX::XMFLOAT4 Quaternion;	// ��].
		unsigned char Bezier[64];		// [4][4][4]  �x�W�F�⊮�p�����[�^.

		VMDKeyFrame()
			: BoneName	{}
			, FrameNo	(0)
			, Location	{}
			, Quaternion{}
			, Bezier    {}
		{}
	};
	
	// �L�[�t���[���\����.
	struct KeyFrame {
		unsigned int FrameNo;			// �t���[����(�A�j���[�V�����J�n����̌o�ߎ���).
		DirectX::XMVECTOR Quaternion;	// �N�H�[�^�j�I��.
		DirectX::XMFLOAT2 p1, p2;		// �x�W�F�̒��ԃR���g���[���|�C���g.

		KeyFrame(
			unsigned int FrameNo,
			const DirectX::XMVECTOR& q,
			const DirectX::XMFLOAT2& ip1,
			const DirectX::XMFLOAT2& ip2) :
			FrameNo		(FrameNo),
			Quaternion	(q),
			p1			(ip1),
			p2			(ip2)
		{}
	};

	// ���[�V�����\����.
	struct Motion
	{
		unsigned int Frame;				// �A�j���[�V�����J�n����̃t���[��.
		DirectX::XMVECTOR Quaternion;	// �N�I�[�^�j�I��.

		Motion()
			: Frame		(0)
			, Quaternion{}
		{}
	};

	//�ǂݍ��񂾃}�e���A�������ƂɃ}�e���A���o�b�t�@���쐬
	void CreateMaterialData();
	
	//�}�e���A�����e�N�X�`���̃r���[���쐬
	void CreateMaterialAndTextureView();

	//���W�ϊ��p�r���[�̐���
	void CreateTransformView();

	//PMD�t�@�C���̃��[�h
	void LoadPMDFile(const char* FilePath);

	/*******************************************
	* @brief	��]���𖖒[�܂œ`�d������ċA�֐�.
	* @param	��]���������{�[���m�[�h.
	* @param    ��]�s��.
	*******************************************/
	void RecursiveMatrixMultipy(BoneNode* node, const DirectX::XMMATRIX& mat);

	float _angle;//�e�X�g�pY����]

	float GetYFromXOnBezier(
		float x, 
		const DirectX::XMFLOAT2& a, 
		const DirectX::XMFLOAT2& b, uint8_t n = 12);


public:
	CPMXActor(const char* filepath,CPMXRenderer& renderer);
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
	CPMXRenderer& m_pRenderer;
	CDirectX12& m_pDx12;

	//���_�֘A
	MyComPtr<ID3D12Resource>		m_pVertexBuffer;			// ���_�o�b�t�@.
	MyComPtr<ID3D12Resource>		m_pIndexBuffer;				// �C���f�b�N�X�o�b�t�@.
	D3D12_VERTEX_BUFFER_VIEW		m_pVertexBufferView;		// ���_�o�b�t�@�r���[.
	D3D12_INDEX_BUFFER_VIEW			m_pIndexBufferView;			// �C���f�b�N�X�o�b�t�@�r���[.

	MyComPtr<ID3D12Resource>		m_pTransformMat;			// ���W�ϊ��s��(���̓��[���h�̂�).
	MyComPtr<ID3D12DescriptorHeap>	m_pTransformHeap;			// ���W�ϊ��q�[�v.

	Transform						m_Transform;				// ���W.		
	DirectX::XMMATRIX*				m_MappedMatrices;			// GPU�Ƃ݂���W.					
	MyComPtr<ID3D12Resource>		m_pTransformBuff;			// �o�b�t�@.

	//�}�e���A���֘A
	std::vector<std::shared_ptr<Material>>	m_pMaterial;		// �}�e���A��.
	MyComPtr<ID3D12Resource>				m_pMaterialBuff;	// �}�e���A���o�b�t�@.
	std::vector<MyComPtr<ID3D12Resource>>	m_pTextureResource;	// �摜���\�[�X.
	std::vector<MyComPtr<ID3D12Resource>>	m_pSphResource;		// Sph���\�[�X.
	std::vector<MyComPtr<ID3D12Resource>>	m_pSpaResource;		// Spa���\�[�X.
	std::vector<MyComPtr<ID3D12Resource>>	m_pToonResource;	// �g�D�[�����\�|�X.

	MyComPtr<ID3D12DescriptorHeap> m_pMaterialHeap;				// �}�e���A���q�[�v(5�Ԃ�)

	// �{�[���֘A.
	std::vector<DirectX::XMMATRIX>	m_BoneMatrix;				// �{�[�����W.
	std::map<std::string, BoneNode> m_BoneNodeTable;			// �{�[���̊K�w.

	// �A�j���[�V�����֘A.
	std::unordered_map<std::string, std::vector<KeyFrame>> m_MotionData;	// ���[�V�����f�[�^.
	DWORD							m_StartTime;				// �A�j���[�V�����̊J�n����(�~���b).
	
};

