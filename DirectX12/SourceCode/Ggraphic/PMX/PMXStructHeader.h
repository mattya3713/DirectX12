#pragma once

/**************************************************
*	PMX用構造体.
*	担当：淵脇 未来
**/

namespace PMX {

	// PMXファイルかどうかの識別子.
	static constexpr std::array<unsigned char, 4> SIGNATURE{ 0x50, 0x4d, 0x58, 0x20 };

	// ヘッダー構造体.
	struct Header
	{
		std::array<uint8_t, 4>	Signature;		// シグネチャ.
		float				Version;			// バージョン.
		uint8_t				NextDataSize;		// 後続データ列のサイズ(PMX 2.0の場合は8).
		uint8_t				Encoding;			// テキストエンコーディング(0: UTF16, 1: UTF8).
		uint8_t				AdditionalUV;		// 追加UV数.
		uint8_t				VertexIndexSize;	// 頂点インデックスサイズ.
		uint8_t				TextureIndexSize;	// テクスチャインデックスサイズ.
		uint8_t				MaterialIndexSize;	// マテリアルインデックスサイズ.
		uint8_t				BoneIndexSize;		// ボーンインデックスサイズ.
		uint8_t				MorphIndexSize;		// モーフインデックスサイズ.
		uint8_t				RigidBodyIndexSize;	// 剛体インデックスサイズ.

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

	// ヘッダーサイズ.
	static constexpr size_t HEADER_SIZE = 17;

	// モデル情報.
	struct PMXModelInfo {
		std::string ModelName;			// モデル名.
		std::string ModelNameEnglish;	// モデル名(英語).
		std::string ModelComment;		// コメント.
		std::string ModelCommentEnglish;// コメント(英語).
	};

	// 頂点情報.
	struct Vertex
	{
		DirectX::XMFLOAT3					Position;		// 座標.
		DirectX::XMFLOAT3					Normal;			// 法線.
		DirectX::XMFLOAT2					UV;				// UV.	
		std::array<DirectX::XMFLOAT4, 4>	AdditionalUV;	// 追加のUV.
		std::array<uint32_t, 4>				BoneIndices;	// ボーンインデックス.
		std::array<float, 4>				BoneWeights;	// ボーンウェイト.
		DirectX::XMFLOAT3					SDEF_C;			// SDEF_C値.
		DirectX::XMFLOAT3					SDEF_R0;		// SDEF_R0値.
		DirectX::XMFLOAT3					SDEF_R1;		// SDEF_R1値.
		float								Edge;			// エッジサイズ.
	};

	// GPU用頂点情報.
	struct VertexForHLSL {
		DirectX::XMFLOAT3 Position;		// 位置.
		DirectX::XMFLOAT3 Normal;		// 法線.
		DirectX::XMFLOAT2 UV;			// UV座標.

		VertexForHLSL()
			: Position	( 0.0f, 0.0f, 0.0f )
			, Normal	( 0.0f, 0.0f, 0.0f )
			, UV		( 0.0f, 0.0f )
		{}
	};

	// GPU用頂点バッファのサイズ.
	static constexpr size_t GPU_VERTEX_SIZE = sizeof(VertexForHLSL);

	// 面情報.
	struct Face {
		std::array<uint32_t, 3> Index; // 3頂点インデックス.

		Face()
			: Index{}
		{}
	};

	// GPU用頂点バッファのサイズ(インデックス配列はまとまりではないため).
	static constexpr size_t GPU_INDEX_SIZE = sizeof(uint32_t);

	// PMXテクスチャ情報.
	struct TexturePath {
		std::string			Path;	// 各テクスチャのパス.

		explicit TexturePath(const std::string& path = "")
			: Path(path) {}
	};

	// PMXモデルデフォルトの共通トゥーン素材のパス.
	static constexpr char COMMON_TOON_PATH[] = "Data\\Model\\PMX\\toon\\toon%02d.bmp";

	// PMXマテリアル情報.
	struct Material {
		std::string			Name;					// マテリアル名.
		std::string			EnglishName;			// マテリアル名(英語).
		DirectX::XMFLOAT4	Diffuse;				// ディフューズ色 (RGBA).
		DirectX::XMFLOAT3	Specular;				// スペキュラ色.
		float				Specularity;			// スペキュラ係数.
		DirectX::XMFLOAT3	Ambient;				// アンビエント色.
		uint8_t				DrawMode;				// 描画モード.
		DirectX::XMFLOAT4	EdgeColor;				// エッジカラー.
		float				EdgeSize;				// エッジサイズ.
		uint8_t				SphereMode;				// スフィアモード.
		uint8_t				SphereTextureIndex;		// スフィアテクスチャインデックス.
		uint8_t				ToonFlag;				// トゥーンフラグ (0: 独自, 1: 共通).
		uint8_t				ToonTextureIndex;		// トゥーンテクスチャインデックス.
		uint32_t			TextureIndex;			// テクスチャインデックス.
		uint32_t			NumFaceCount;			// マテリアルに割り当てられる面数.
		std::string			Memo;					// メモ情報.

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
		DirectX::XMFLOAT4	Diffuse;		// ディフューズ色 (RGBA).
		DirectX::XMFLOAT3	Specular;		// スペキュラ色.
		float				SpecularPower;	// スペキュラ係数.
		DirectX::XMFLOAT3	Ambient;		// アンビエント色.
		float				UseSphereMap;	// スフィアマップを使うかどうか.
	};

	// GPU用マテリアルバッファのサイズ.
	static constexpr size_t GPU_MATERIAL_SIZE = sizeof(MaterialForHLSL);

	// PMXボーンフラグ.
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

	// IKの情報.
	struct IKLink
	{
		uint32_t	IKBoneIndex;
		uint8_t		EnableLimit;

		DirectX::XMFLOAT3 LimitMin;
		DirectX::XMFLOAT3 LimitMax;
	};

	// ボーン構造体.
	struct Bone
	{
		std::string Name;                // ボーンの名前.
		std::string EnglishName;         // ボーンの名前(英語).
		DirectX::XMFLOAT3 Position;      // ボーンの位置.
		uint32_t ParentBoneIndex;        // 親ボーンのインデックス.
		uint32_t DeformDepth;            // ボーンの変形深度.
		BoneFlags BoneFlag;              // ボーンのフラグ.
		DirectX::XMFLOAT3 PositionOffset;// ボーンの位置オフセット.
		uint32_t LinkBoneIndex;          // リンクされるボーンのインデックス.
		uint32_t AppendBoneIndex;        // 添付ボーンのインデックス.
		float AppendWeight;              // 添付ボーンのウェイト.
		DirectX::XMFLOAT3 FixedAxis;     // 固定軸.
		DirectX::XMFLOAT3 LocalXAxis;    // ローカルX軸.
		DirectX::XMFLOAT3 LocalZAxis;    // ローカルZ軸.
		uint32_t KeyValue;               // ボーンのキー値.
		uint32_t IKTargetBoneIndex;      // IKターゲットボーンのインデックス.
		uint32_t IKIterationCount;       // IK反復回数.
		float IKLimit;                   // IK制限角度.
		std::vector<IKLink> IKLinks;     // IKリンクのリスト.

		// コンストラクタ
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
		std::string Name;					// モーフの名前.
		std::string EnglishName;			// モーフの英語名.

		uint8_t ControlPanel;				// 制御パネルの情報.
		MorphType MorphType;				// モーフ種別.

		// 頂点の位置を変化させるモーフ.
		struct PositionMorph
		{
			uint32_t VertexIndex;			// 変化する頂点のインデックス.
			DirectX::XMFLOAT3 Position;		// 頂点の新しい位置.
		};

		// UV座標を変化させるモーフ.
		struct UVMorph
		{
			uint32_t VertexIndex;			// 変化する頂点のインデックス.
			DirectX::XMFLOAT4 UV;			// 新しいUV座標.
		};

		// ボーンの位置や回転を変化させるモーフ.
		struct BoneMorph
		{
			uint32_t BoneIndex;				// 変化するボーンのインデックス.
			DirectX::XMFLOAT3 Position;		// ボーンの新しい位置.
			DirectX::XMFLOAT4 Quaternion;	// ボーンの新しい回転(クォータニオン).
		};

		// マテリアル(色、照明)を変化させるモーフ.
		struct MaterialMorph
		{
			// 演算タイプ.
			enum OpType : uint8_t   
			{
				Mul,	// 乗算.
				Add,	// 加算.
			};

			uint32_t MaterialIndex;					// 変化するマテリアルのインデックス.
			OpType OpType;							// 演算タイプ.
			DirectX::XMFLOAT4 Diffuse;				// 拡散色.
			DirectX::XMFLOAT3 Specular;				// 鏡面反射色.
			float SpecularPower;					// 鏡面反射強度.
			DirectX::XMFLOAT3 Ambient;				// 環境光色.
			DirectX::XMFLOAT4 EdgeColor;			// エッジ色.
			float EdgeSize;							// エッジサイズ.
			DirectX::XMFLOAT4 TextureFactor;        // テクスチャファクター.
			DirectX::XMFLOAT4 SphereTextureFactor;  // 球状テクスチャファクター.
			DirectX::XMFLOAT4 ToonTextureFactor;    // トゥーンテクスチャファクター.
		};

		// グループモーフ(複数モーフをグループとして扱う).
		struct GroupMorph
		{
			uint32_t MorphIndex;					// グループ化されているモーフのインデックス.
			float Weight;							// モーフの重み(影響度).
		};

		// フリップモーフ(モーフを反転させる).
		struct FlipMorph
		{
			uint32_t MorphIndex;					// 反転するモーフのインデックス.
			float Weight;							// モーフの重み(反転の影響度)
		};

		// インパルスモーフ(剛体へのインパルス加算).
		struct ImpulseMorph
		{
			uint32_t RigidBodyIndex;				// インパルスを加える剛体のインデックス.
			uint8_t LocalFlag;						// ローカルフラグ(0:OFF, 1:ON).
			DirectX::XMFLOAT3 TranslateVelocity;	// 位置速度(加速度のようなもの).
			DirectX::XMFLOAT3 RotateTorque;			// 回転トルク.
		};

		std::vector<PositionMorph>	PositionMorphs;	// 頂点位置モーフのリスト.
		std::vector<UVMorph>		UVMorphs;       // UVモーフのリスト.
		std::vector<BoneMorph>		BoneMorphs;     // ボーンモーフのリスト.
		std::vector<MaterialMorph>	MaterialMorphs; // マテリアルモーフのリスト.
		std::vector<GroupMorph>		GroupMorphs;    // グループモーフのリスト.
		std::vector<FlipMorph>		FlipMorphs;     // フリップモーフのリスト.
		std::vector<ImpulseMorph>	ImpulseMorphs;  // インパルスモーフのリスト.
	};



} // namespace PMX