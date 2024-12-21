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
public:

	// ���f���̒��_�T�C�Y.
	static constexpr size_t PMXVertexSize = 60;

private:

	// PMX�w�b�_�[�\����.
	struct PMXHeader
	{
		std::array<uint8_t, 4>	Signature;		// �V�O�l�`��.
		float				Version;			// �o�[�W����.
		uint8_t				NextDataSize;		// �㑱�f�[�^��̃T�C�Y(PMX 2.0�̏ꍇ��8).
		uint8_t				Encoding;			// �e�L�X�g�G���R�[�f�B���O(0: UTF16, 1: UTF8).
		uint8_t				AdditionalUV;		// �ǉ�UV��.
		uint8_t				VertexIndexSize;	// ���_�C���f�b�N�X�T�C�Y.
		uint8_t				TextureIndexSize;	// �e�N�X�`���C���f�b�N�X�T�C�Y.
		uint8_t				MaterialIndexSize;	// �}�e���A���C���f�b�N�X�T�C�Y.
		uint8_t				BoneIndexSize;		// �{�[���C���f�b�N�X�T�C�Y.
		uint8_t				MorphIndexSize;		// ���[�t�C���f�b�N�X�T�C�Y.
		uint8_t				RigidBodyIndexSize;	// ���̃C���f�b�N�X�T�C�Y.

		PMXHeader()
			: Signature			{}
			, Version			( 0.0f )
			, Encoding			( 0 )
			, NextDataSize		( 0 )
			, AdditionalUV		( 0 )
			, VertexIndexSize	( 0 )
			, TextureIndexSize	( 0 )
			, MaterialIndexSize	( 0 )
			, BoneIndexSize		( 0 )
			, MorphIndexSize	( 0 )
			, RigidBodyIndexSize( 0 )
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
	// PMX�}�e���A���\����.
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

	struct PMXVertex
	{
		DirectX::XMFLOAT3					Position;		// ���W.
		DirectX::XMFLOAT3					Normal;			// �@��.
		DirectX::XMFLOAT2					UV;				// UV.	
		std::array<DirectX::XMFLOAT4, 4>	AdditionalUV;	// �ǉ���UV.
		std::array<uint32_t, 4>				BoneIndices;	// �{�[���C���f�b�N�X.
		std::array<float   , 4>				BoneWeights;	// �{�[���E�F�C�g.
		DirectX::XMFLOAT3					SDEF_C;			// SDEF_C�l.
		DirectX::XMFLOAT3					SDEF_R0;		// SDEF_R0�l.
		DirectX::XMFLOAT3					SDEF_R1;		// SDEF_R1�l.
		float								Edge;			// �G�b�W�T�C�Y.
	};


	// PMX�ʍ\����.
	struct PMXFace {
		std::array<uint32_t, 3> Index; // 3���_�C���f�b�N�X.

		PMXFace()
			: Index	{}
		{}
	};


	// PMX�e�N�X�`���\����.
	struct TexturePath {
		uint32_t					TextureCount;	// �e�N�X�`���̐�.
		std::vector<std::string>	TexturePaths;	// �e�e�N�X�`���̃p�X.

		TexturePath()
			: TextureCount	()
			, TexturePaths	{}
		{}
	};

#pragma pack(1)
	struct PMXMaterial {
		std::string			Name;					// �}�e���A����.
		std::string			EnglishName;			// �}�e���A����(�p��).
		DirectX::XMFLOAT4	Diffuse;				// �f�B�t���[�Y�F (RGBA).
		DirectX::XMFLOAT3	Specular;				// �X�y�L�����F.
		float				Specularity;			// �X�y�L�����W��.
		DirectX::XMFLOAT3	Ambient;				// �A���r�G���g�F.
		DirectX::XMFLOAT4	EdgeColor;				// �G�b�W�J���[.
		float				EdgeSize;				// �G�b�W�T�C�Y.
		uint8_t				SphereMode;				// �X�t�B�A���[�h.
		uint8_t				ToonFlag;				// �g�D�[���t���O (0: �Ǝ�, 1: ����).
		uint8_t				SphereTextureIndex;		// �X�t�B�A�e�N�X�`���C���f�b�N�X.
		uint8_t				ToonTextureIndex;		// �g�D�[���e�N�X�`���C���f�b�N�X.
		uint32_t			TextureIndex;			// �e�N�X�`���C���f�b�N�X.
		uint32_t			FaceCount;				// �}�e���A���Ɋ��蓖�Ă���ʐ�.
		char				Memo[256];				// ������� (PMX�Ǝ�).

		PMXMaterial()
			: Name				("")
			, EnglishName		("")
			, Diffuse			( 0.0f, 0.0f, 0.0f, 0.0f )
			, Specular			( 0.0f, 0.0f, 0.0f )
			, Specularity		( 0.0f )
			, Ambient			( 0.0f, 0.0f, 0.0f )
			, EdgeColor			( 0.0f, 0.0f, 0.0f, 0.0f )
			, EdgeSize			( 0.0f )
			, SphereMode		( 0 )
			, ToonFlag			( 0 )
			, TextureIndex		( 0 )
			, SphereTextureIndex( 0 )
			, ToonTextureIndex	( 0 )
			, FaceCount			( 0 )
			, Memo				("")
		{}
	};
#pragma pack()

	// �V�F�[�_���ɓ�������}�e���A���f�[�^.
	struct MaterialForHlsl {
		DirectX::XMFLOAT4	Diffuse;	// �f�B�t���[�Y�F.	
		DirectX::XMFLOAT3	Specular;	// �X�y�L�����̋�.		
		float				Specularity;// �X�y�L�����F.		
		DirectX::XMFLOAT3	Ambient;	// �A���r�G���g�F.		

		MaterialForHlsl()
			: Diffuse		(0.0f, 0.0f, 0.0f, 0.0f)
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

	// �ǂݍ��񂾃}�e���A�������ƂɃ}�e���A���o�b�t�@���쐬.
	void CreateMaterialData();
	
	// �}�e���A�����e�N�X�`���̃r���[���쐬.
	void CreateMaterialAndTextureView();

	// ���W�ϊ��p�r���[�̐���.
	void CreateTransformView();

	// PMD�t�@�C���̃��[�h.
	void LoadPMXFile(const char* FilePath);
	
	// PMX�w�b�^�[�ǂݍ���.
	void ReadPMXHeader(FILE* fp, PMXHeader* Header);

	// �C���f�b�N�X��ǂݍ��ގ��̊֐���I�����邽�߂̊֐��|�C���^.
	void ReadPMXIndices1Byte(FILE* fp,const uint32_t& IndicesNum, std::vector<PMXFace>* Faces);
	void ReadPMXIndices2Byte(FILE* fp,const uint32_t& IndicesNum, std::vector<PMXFace>* Faces);
	void ReadPMXIndices4Byte(FILE* fp,const uint32_t& IndicesNum, std::vector<PMXFace>* Faces);
	using ReadIndicesFunction = void(CPMXActor::*)(FILE*, const uint32_t&, std::vector<PMXFace>*);
	ReadIndicesFunction ReadIndices = nullptr;

	/*******************************************
	* @brief	PMX�o�C�i������C���f�b�N�X���ƃC���f�b�N�X��ǂݍ���.
	* @param	�ǂݍ��݃t�@�C���|�C���^.
	* @param	�ǂݍ��񂾃C���f�b�N�X.
	*******************************************/
	void ReadPMXIndices(FILE* fp,std::vector<PMXFace>* Faces);

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

	/*******************************************
	* @brief    �w�肳�ꂽ�T�C�Y���̃f�[�^��ǂݍ��݁A4�o�C�g�̒l�Ƃ��ĕԂ�.
	* @param    �t�@�C���|�C���^�B�ǂݍ��ݑΏۂ̃t�@�C�����w���܂�.
	* @param    �ǂݍ��ރf�[�^�̃T�C�Y(1,2,4�o�C�g�̂ǂꂩ).
	* @return   �ǂݍ��񂾃f�[�^��4�o�C�g��`uint32_t`�^�ɕϊ������l
	* @throw	�m��Ȃ��T�C�Y.
	*******************************************/
	uint32_t ReadAndCastIndices(FILE* fp, uint8_t indexSize);

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
	std::vector<std::shared_ptr<Material>>	m_pMaterials;		// �}�e���A��.
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

