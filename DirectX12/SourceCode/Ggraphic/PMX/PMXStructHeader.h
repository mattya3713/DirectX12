#pragma once

/**************************************************
*	PMX�p�\����.
*	�S���F���e ����
**/

namespace PMX {

	// PMX�t�@�C�����ǂ����̎��ʎq.
	static constexpr std::array<unsigned char, 4> SIGNATURE{ 0x50, 0x4d, 0x58, 0x20 };

	// �w�b�_�[�\����.
	struct Header
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

		Header()
			: Signature			{ }
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

	// �w�b�_�[�T�C�Y.
	static constexpr size_t HEADER_SIZE = 17;

	// ���f�����.
	struct PMXModelInfo {
		std::string ModelName;			// ���f����.
		std::string ModelNameEnglish;	// ���f����(�p��).
		std::string ModelComment;		// �R�����g.
		std::string ModelCommentEnglish;// �R�����g(�p��).
	};

	// ���_���.
	struct Vertex
	{
		DirectX::XMFLOAT3					Position;		// ���W.
		DirectX::XMFLOAT3					Normal;			// �@��.
		DirectX::XMFLOAT2					UV;				// UV.	
		std::array<DirectX::XMFLOAT4, 4>	AdditionalUV;	// �ǉ���UV.
		std::array<uint32_t, 4>				BoneIndices;	// �{�[���C���f�b�N�X.
		std::array<float, 4>				BoneWeights;	// �{�[���E�F�C�g.
		DirectX::XMFLOAT3					SDEF_C;			// SDEF_C�l.
		DirectX::XMFLOAT3					SDEF_R0;		// SDEF_R0�l.
		DirectX::XMFLOAT3					SDEF_R1;		// SDEF_R1�l.
		float								Edge;			// �G�b�W�T�C�Y.
	};

	// GPU�p���_���.
	struct VertexForHLSL {
		DirectX::XMFLOAT3 Position;		// �ʒu.
		DirectX::XMFLOAT3 Normal;		// �@��.
		DirectX::XMFLOAT2 UV;			// UV���W.

		VertexForHLSL()
			: Position	( 0.0f, 0.0f, 0.0f )
			, Normal	( 0.0f, 0.0f, 0.0f )
			, UV		( 0.0f, 0.0f )
		{}
	};

	// GPU�p���_�o�b�t�@�̃T�C�Y.
	static constexpr size_t GPU_VERTEX_SIZE = sizeof(VertexForHLSL);

	// �ʏ��.
	struct Face {
		std::array<uint32_t, 3> Index; // 3���_�C���f�b�N�X.

		Face()
			: Index{}
		{}
	};

	// GPU�p���_�o�b�t�@�̃T�C�Y.
	static constexpr size_t GPU_INDEX_SIZE = sizeof(Face);

	// PMX�e�N�X�`�����.
	struct TexturePath {
		std::string			Path;	// �e�e�N�X�`���̃p�X.

		explicit TexturePath(const std::string& path = "")
			: Path(path) {}
	};

	// PMX�}�e���A�����.
	struct Material {
		std::string			Name;					// �}�e���A����.
		std::string			EnglishName;			// �}�e���A����(�p��).
		DirectX::XMFLOAT4	Diffuse;				// �f�B�t���[�Y�F (RGBA).
		DirectX::XMFLOAT3	Specular;				// �X�y�L�����F.
		float				Specularity;			// �X�y�L�����W��.
		DirectX::XMFLOAT3	Ambient;				// �A���r�G���g�F.
		uint8_t				DrawMode;				// �`�惂�[�h.
		DirectX::XMFLOAT4	EdgeColor;				// �G�b�W�J���[.
		float				EdgeSize;				// �G�b�W�T�C�Y.
		uint8_t				SphereMode;				// �X�t�B�A���[�h.
		uint8_t				SphereTextureIndex;		// �X�t�B�A�e�N�X�`���C���f�b�N�X.
		uint8_t				ToonFlag;				// �g�D�[���t���O (0: �Ǝ�, 1: ����).
		uint8_t				ToonTextureIndex;		// �g�D�[���e�N�X�`���C���f�b�N�X.
		uint32_t			TextureIndex;			// �e�N�X�`���C���f�b�N�X.
		uint32_t			NumFaceCount;			// �}�e���A���Ɋ��蓖�Ă���ʐ�.
		std::string			Memo;					// �������.

		Material()
			: Name				("")                        
			, EnglishName		("")						
			, Diffuse			( 0.0f, 0.0f, 0.0f, 0.0f )  
			, Specular			( 0.0f, 0.0f, 0.0f )		
			, Specularity		( 0.0f )					
			, Ambient			( 0.0f, 0.0f, 0.0f )        
			, DrawMode			( 0 )                       
			, EdgeColor			( 0.0f, 0.0f, 0.0f, 0.0f )	
			, EdgeSize			( 0.0f )                    
			, SphereMode		( 0 )						
			, SphereTextureIndex( 0 )						
			, ToonFlag			( 0 )                       
			, ToonTextureIndex	( 0 )						
			, TextureIndex		( 0 )						
			, NumFaceCount		( 0 )						
			, Memo				("")
		{}
	};

	struct MaterialForHLSL
	{
		DirectX::XMFLOAT4	Diffuse;		// �f�B�t���[�Y�F (RGBA).
		DirectX::XMFLOAT3	Specular;		// �X�y�L�����F.
		float				SpecularPower;	// �X�y�L�����W��.
		DirectX::XMFLOAT3	Ambient;		// �A���r�G���g�F.
	};	

	// GPU�p�}�e���A���o�b�t�@�̃T�C�Y.
	static constexpr size_t GPU_MATERIAL_SIZE = sizeof(MaterialForHLSL);

	// PMX�{�[���t���O.
	enum BoneFlags : uint16_t
	{
		TargetShowMode = 0x0001,
		AllowRotate = 0x0002,
		AllowTranslate = 0x0004,
		Visible = 0x0008,
		AllowControl = 0x0010,
		IK = 0x0020,
		AppendLocal = 0x0080,
		AppendRotate = 0x0100,
		AppendTranslate = 0x0200,
		FixedAxis = 0x0400,
		LocalAxis = 0x800,
		DeformAfterPhysics = 0x1000,
		DeformOuterParent = 0x2000,
	};

	// IK�̏��.
	struct IKLink
	{
		uint32_t	IKBoneIndex;
		uint8_t		EnableLimit;

		DirectX::XMFLOAT3 LimitMin;
		DirectX::XMFLOAT3 LimitMax;
	};

	// �{�[���\����.
	struct Bone
	{
		std::string Name;                // �{�[���̖��O.
		std::string EnglishName;         // �{�[���̖��O(�p��).
		DirectX::XMFLOAT3 Position;      // �{�[���̈ʒu.
		uint32_t ParentBoneIndex;        // �e�{�[���̃C���f�b�N�X.
		uint32_t DeformDepth;            // �{�[���̕ό`�[�x.
		BoneFlags BoneFlag;              // �{�[���̃t���O.
		DirectX::XMFLOAT3 PositionOffset;// �{�[���̈ʒu�I�t�Z�b�g.
		uint32_t LinkBoneIndex;          // �����N�����{�[���̃C���f�b�N�X.
		uint32_t AppendBoneIndex;        // �Y�t�{�[���̃C���f�b�N�X.
		float AppendWeight;              // �Y�t�{�[���̃E�F�C�g.
		DirectX::XMFLOAT3 FixedAxis;     // �Œ莲.
		DirectX::XMFLOAT3 LocalXAxis;    // ���[�J��X��.
		DirectX::XMFLOAT3 LocalZAxis;    // ���[�J��Z��.
		uint32_t KeyValue;               // �{�[���̃L�[�l.
		uint32_t IKTargetBoneIndex;      // IK�^�[�Q�b�g�{�[���̃C���f�b�N�X.
		uint32_t IKIterationCount;       // IK������.
		float IKLimit;                   // IK�����p�x.
		std::vector<IKLink> IKLinks;     // IK�����N�̃��X�g.

		// �R���X�g���N�^
		Bone()
			: Name					("")
			, EnglishName			("")
			, Position				( 0.0f, 0.0f, 0.0f )
			, ParentBoneIndex		( 0 )
			, DeformDepth			( 0 )
			, BoneFlag				( BoneFlags::TargetShowMode )
			, PositionOffset		( 0.0f, 0.0f, 0.0f )
			, LinkBoneIndex			( 0 )
			, AppendBoneIndex		( 0 )
			, AppendWeight			( 0.0f )
			, FixedAxis				( 0.0f, 0.0f, 0.0f )
			, LocalXAxis			( 1.0f, 0.0f, 0.0f )
			, LocalZAxis			( 0.0f, 0.0f, 1.0f )
			, KeyValue				( 0 )
			, IKTargetBoneIndex		( 0 )
			, IKIterationCount		( 0 )
			, IKLimit				( 0.0f )
			, IKLinks				{}
		{}
	}; 
	
	enum class MorphType : uint8_t
	{
		Group,
		Position,
		Bone,
		UV,
		AddUV1,
		AddUV2,
		AddUV3,
		AddUV4,
		Material,
		Flip,
		Impluse,
	};

	struct Morph
	{
		std::string Name;					// ���[�t�̖��O.
		std::string EnglishName;			// ���[�t�̉p�ꖼ.

		uint8_t ControlPanel;				// ����p�l���̏��.
		MorphType MorphType;				// ���[�t���.

		// ���_�̈ʒu��ω������郂�[�t.
		struct PositionMorph
		{
			uint32_t VertexIndex;			// �ω����钸�_�̃C���f�b�N�X.
			DirectX::XMFLOAT3 Position;		// ���_�̐V�����ʒu.
		};

		// UV���W��ω������郂�[�t.
		struct UVMorph
		{
			uint32_t VertexIndex;			// �ω����钸�_�̃C���f�b�N�X.
			DirectX::XMFLOAT4 UV;			// �V����UV���W.
		};

		// �{�[���̈ʒu���]��ω������郂�[�t.
		struct BoneMorph
		{
			uint32_t BoneIndex;				// �ω�����{�[���̃C���f�b�N�X.
			DirectX::XMFLOAT3 Position;		// �{�[���̐V�����ʒu.
			DirectX::XMFLOAT4 Quaternion;	// �{�[���̐V������](�N�H�[�^�j�I��).
		};

		// �}�e���A��(�F�A�Ɩ�)��ω������郂�[�t.
		struct MaterialMorph
		{
			// ���Z�^�C�v.
			enum OpType : uint8_t   
			{
				Mul,	// ��Z.
				Add,	// ���Z.
			};

			uint32_t MaterialIndex;					// �ω�����}�e���A���̃C���f�b�N�X.
			OpType OpType;							// ���Z�^�C�v.
			DirectX::XMFLOAT4 Diffuse;				// �g�U�F.
			DirectX::XMFLOAT3 Specular;				// ���ʔ��ːF.
			float SpecularPower;					// ���ʔ��ˋ��x.
			DirectX::XMFLOAT3 Ambient;				// �����F.
			DirectX::XMFLOAT4 EdgeColor;			// �G�b�W�F.
			float EdgeSize;							// �G�b�W�T�C�Y.
			DirectX::XMFLOAT4 TextureFactor;        // �e�N�X�`���t�@�N�^�[.
			DirectX::XMFLOAT4 SphereTextureFactor;  // ����e�N�X�`���t�@�N�^�[.
			DirectX::XMFLOAT4 ToonTextureFactor;    // �g�D�[���e�N�X�`���t�@�N�^�[.
		};

		// �O���[�v���[�t(�������[�t���O���[�v�Ƃ��Ĉ���).
		struct GroupMorph
		{
			uint32_t MorphIndex;					// �O���[�v������Ă��郂�[�t�̃C���f�b�N�X.
			float Weight;							// ���[�t�̏d��(�e���x).
		};

		// �t���b�v���[�t(���[�t�𔽓]������).
		struct FlipMorph
		{
			uint32_t MorphIndex;					// ���]���郂�[�t�̃C���f�b�N�X.
			float Weight;							// ���[�t�̏d��(���]�̉e���x)
		};

		// �C���p���X���[�t(���̂ւ̃C���p���X���Z).
		struct ImpulseMorph
		{
			uint32_t RigidBodyIndex;				// �C���p���X�������鍄�̂̃C���f�b�N�X.
			uint8_t LocalFlag;						// ���[�J���t���O(0:OFF, 1:ON).
			DirectX::XMFLOAT3 TranslateVelocity;	// �ʒu���x(�����x�̂悤�Ȃ���).
			DirectX::XMFLOAT3 RotateTorque;			// ��]�g���N.
		};

		std::vector<PositionMorph>	PositionMorphs;	// ���_�ʒu���[�t�̃��X�g.
		std::vector<UVMorph>		UVMorphs;       // UV���[�t�̃��X�g.
		std::vector<BoneMorph>		BoneMorphs;     // �{�[�����[�t�̃��X�g.
		std::vector<MaterialMorph>	MaterialMorphs; // �}�e���A�����[�t�̃��X�g.
		std::vector<GroupMorph>		GroupMorphs;    // �O���[�v���[�t�̃��X�g.
		std::vector<FlipMorph>		FlipMorphs;     // �t���b�v���[�t�̃��X�g.
		std::vector<ImpulseMorph>	ImpulseMorphs;  // �C���p���X���[�t�̃��X�g.
	};



} // namespace PMX