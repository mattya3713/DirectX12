#pragma once

#include<d3d12.h>
#include<DirectXMath.h>
#include<vector>
#include<string>

// 前方宣言.
class CDirectX12;
class CPMXRenderer;

class CPMXActor
{
	friend CPMXRenderer;
private:

	// PMXヘッダー構造体.
	struct PMXHeader
	{
		float Version;				// バージョン.
		uint8_t Encoding;			// テキストエンコーディング（0: UTF16, 1: UTF8）.
		uint8_t AdditionalUV;		// 追加UV数.
		uint8_t VertexIndexSize;	// 頂点インデックスサイズ.
		uint8_t TextureIndexSize;	// テクスチャインデックスサイズ.
		uint8_t MaterialIndexSize;	// マテリアルインデックスサイズ.
		uint8_t BoneIndexSize;		// ボーンインデックスサイズ.
		uint8_t MorphIndexSize;		// モーフインデックスサイズ.
		uint8_t RigidBodyIndexSize; // 剛体インデックスサイズ.

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
		{}
	};

	// PMXモデル情報.
	struct PMXModelInfo {
		std::string ModelName;			// モデル名.
		std::string ModelNameEnglish;	// モデル名(英語).
		std::string ModelComment;		// コメント.
		std::string ModelCommentEnglish;// コメント(英語).
	};

	// TODO : パディングの対処法を考える.
	// PMDマテリアル構造体.
#pragma pack(1)
	struct PMDMaterial {
		DirectX::XMFLOAT3 Diffuse;  // ディフューズ色			: 12Byte.
		float	 Alpha;				// α値					:  4Byte.
		float    Specularity;		// スペキュラの強さ		:  4Byte.
		DirectX::XMFLOAT3 Specular; // スペキュラ色			: 12Byte.
		DirectX::XMFLOAT3 Ambient;  // アンビエント色			: 12Byte.
		uint8_t  ToonIdx;			// トゥーン番号			:  1Byte.
		uint8_t  EdgeFlg;			// Material毎の輪郭線ﾌﾗｸﾞ	:  1Byte.
//		uint16_t Padding;			// パディング				:  2Byte.
		uint32_t IndicesNum;		// 割り当たるインデックス数	:  4Byte.
		char     TexFilePath[20];	// テクスチャファイル名	: 20Byte.
									// 合計					: 70Byte(パディングなし).
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
	
	// PMDボーン構造体.
	struct PMDBone
	{
		unsigned char	BoneName[20];	// ボーン名.
		unsigned short	ParentNo;		// 親ボーン名.
		unsigned short	NextNo;			// 先端のボーン番号.
		unsigned char	TypeNo;			// ボーンの種類.
		unsigned short	IKBoneNo;		// IKボーン番号.
		DirectX::XMFLOAT3 Pos;			// ボーンの基準座標.

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

#include <DirectXMath.h>

	// BDEF1 ボーンウェイト (1ボーンの場合).
	struct BDEF1Weight {
		uint16_t BoneIndex;   // ウェイト1.0の単一ボーン(参照Index).

		BDEF1Weight(uint16_t BoneIndex)
			: BoneIndex(BoneIndex)
		{}  // 初期化.
	};

	// BDEF2 ボーンウェイト (2ボーンの場合).
	struct BDEF2Weight {
		uint16_t BoneIndex1;  // ボーン1の参照Index.
		uint16_t BoneIndex2;  // ボーン2の参照Index.
		float Weight1;		  // ボーン1のウェイト値(0～1.0), ボーン2のウェイト値は 1.0-ボーン1ウェイト.

		BDEF2Weight(uint16_t Bone1, uint16_t Bone2, float Weight)
			: BoneIndex1	(Bone1)
			, BoneIndex2	(Bone2)
			, Weight1		(Weight) 
		{}
	};

	// BDEF4 ボーンウェイト (4ボーンの場合).
	struct BDEF4Weight {
		uint16_t BoneIndex[4];  // ボーンインデックス (4ボーン).
		float Weight[4];		// ボーンウェイト (それぞれのウェイト).

		BDEF4Weight(uint16_t bone0, uint16_t bone1, uint16_t bone2, uint16_t bone3,
			float weight0, float weight1, float weight2, float weight3)
		{
			BoneIndex[0] = bone0;
			BoneIndex[1] = bone1;
			BoneIndex[2] = bone2;
			BoneIndex[3] = bone3;
			Weight[0] = weight0;
			Weight[1] = weight1;
			Weight[2] = weight2;
			Weight[3] = weight3;
		}
	};

	// SDEF ボーンウェイト (SDEF方式).
	struct SDEFWeight {
		uint16_t BoneIndex1;    // ボーンインデックス1.
		uint16_t BoneIndex2;    // ボーンインデックス2.
		float Weight1;			// ボーン1のウェイト.
		DirectX::XMFLOAT3 C;	// SDEF補正用C.
		DirectX::XMFLOAT3 R0;	// SDEF補正用R0.
		DirectX::XMFLOAT3 R1;	// SDEF補正用R1.

		SDEFWeight(int bone1, int bone2, float weight1,
			const DirectX::XMFLOAT3& c, const DirectX::XMFLOAT3& r0, const DirectX::XMFLOAT3& r1)
			: BoneIndex1(bone1), BoneIndex2(bone2), Weight1(weight1), C(c), R0(r0), R1(r1) {}
	};

	// ボーンウェイトを格納する構造体.
	struct PMXBoneWeight {
		uint8_t WeightType;  // ウェイトタイプ (BDEF1, BDEF2, BDEF4, SDEF).

		union {
			BDEF1Weight BDEF1;
			BDEF2Weight BDEF2;
			BDEF4Weight BDEF4;
			SDEFWeight SDEF;
		};
		
		// コンストラクタ.
		PMXBoneWeight()		// デフォルトコンストラクタ (BDEF1で初期化)
			: WeightType(0)
			, BDEF1(0)
		{}

		// BDEF1用コンストラクタ.
		PMXBoneWeight(int boneIndex) 
			: WeightType(0), 
			BDEF1(boneIndex)
		{}

		// BDEF2用コンストラクタ.
		PMXBoneWeight (int bone1, int bone2, float weight1) 
			: WeightType(1)
			, BDEF2(bone1, bone2, weight1)
		{}

		// BDEF4用コンストラクタ.
		PMXBoneWeight(int bone0, int bone1, int bone2, int bone3,
			float weight0, float weight1, float weight2, float weight3)
			: WeightType(2)
			, BDEF4(bone0, bone1, bone2, bone3, weight0, weight1, weight2, weight3) 
		{}

		// SDEF用コンストラクタ.
		PMXBoneWeight(int bone1, int bone2, float weight1,
			const DirectX::XMFLOAT3& c, const DirectX::XMFLOAT3& r0, const DirectX::XMFLOAT3& r1)
			: WeightType(3)
			, SDEF(bone1, bone2, weight1, c, r0, r1)
		{}
	};

	// PMX頂点構造体.
	struct PMXVertex {
		DirectX::XMFLOAT3               Position;		// 頂点位置.
		DirectX::XMFLOAT3               Normal;			// 頂点法線.
		DirectX::XMFLOAT2               UV;				// 頂点UV座標.
		std::vector<DirectX::XMFLOAT4>  AdditionalUV;	// 追加UV座標(最大4つまで).
		PMXBoneWeight					BoneWeight;		// ボーンウェイト.
		float							Edge;			// エッジ倍率.

		PMXVertex()
			: Position		( 0.0f, 0.0f, 0.0f )
			, Normal		( 0.0f, 0.0f, 0.0f )
			, UV			( 0.0f, 0.0f )
			, AdditionalUV	{ }
			, BoneWeight	{ }
			, Edge			( 0.0f )
		{}
	};

	// PMX面構造体.
	struct PMXFace {
		std::array<uint32_t, 3> Index; // 3頂点インデックス.

		PMXFace()
			: Index	{}
		{}
	};


	// PMXテクスチャ構造体.
	struct TexturePath {
		uint32_t					TextureCount;	// テクスチャの数.
		std::vector<std::string>	TexturePaths;	// 各テクスチャのパス.

		TexturePath()
			: TextureCount	()
			, TexturePaths	{}
		{}
	};

#pragma pack(1)
	struct PMXMaterial {
		std::string			Name;					// マテリアル名.
		std::string			EnglishName;			// マテリアル名(英語).
		DirectX::XMFLOAT4	Diffuse;				// ディフューズ色 (RGBA).
		DirectX::XMFLOAT3	Specular;				// スペキュラ色.
		float				Specularity;			// スペキュラ係数.
		DirectX::XMFLOAT3	Ambient;				// アンビエント色.
		DirectX::XMFLOAT4	EdgeColor;				// エッジカラー.
		float				EdgeSize;				// エッジサイズ.
		uint8_t				SphereMode;				// スフィアモード.
		uint8_t				ToonFlag;				// トゥーンフラグ (0: 独自, 1: 共通).
		uint8_t				SphereTextureIndex;		// スフィアテクスチャインデックス.
		uint8_t				ToonTextureIndex;		// トゥーンテクスチャインデックス.
		uint32_t			TextureIndex;			// テクスチャインデックス.
		uint32_t			FaceCount;				// マテリアルに割り当てられる面数.
		char				Memo[256];				// メモ情報 (PMX独自).

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

	// シェーダ側に投げられるマテリアルデータ.
	struct MaterialForHlsl {
		DirectX::XMFLOAT3	Diffuse;	// ディフューズ色.		
		float				Alpha;		// α値.		
		DirectX::XMFLOAT3	Specular;	// スペキュラの強.		
		float				Specularity;// スペキュラ色.		
		DirectX::XMFLOAT3	Ambient;	// アンビエント色.		

		MaterialForHlsl()
			: Diffuse		(0.0f, 0.0f, 0.0f)
			, Alpha			(0.0f)
			, Specular		(0.0f, 0.0f, 0.0f)
			, Specularity	(0.0f)
			, Ambient		(0.0f, 0.0f, 0.0f)
		{}
	};

	// それ以外のマテリアルデータ.
	struct AdditionalMaterial {
		std::string TexPath;	// テクスチャファイルパス.
		int			ToonIdx;	// トゥーン番号.
		bool		EdgeFlg;	// マテリアル毎の輪郭線フラグ.

		AdditionalMaterial()
			: TexPath		{}
			, ToonIdx		(0)
			, EdgeFlg		(false)
		{}
	};

	// まとめたもの.
	struct Material {
		unsigned int IndicesNum;		// インデックス数.
		MaterialForHlsl Materialhlsl;	// シェーダ側に投げられるマテリアルデータ.
		AdditionalMaterial Additional;	// それ以外のマテリアルデータ.
		
		Material()
			: IndicesNum	(0)
			, Materialhlsl	{}
			, Additional	{}
		{}
	};

	struct Transform {
		// 内部に持ってるXMMATRIXメンバが16バイトアライメントであるため.
		// Transformをnewする際には16バイト境界に確保する.
		void* operator new(size_t size);
		DirectX::XMMATRIX world;
	};

	struct BoneNode {
		int						BoneIndex;	// ボーンインデックス.
		DirectX::XMFLOAT3		StartPos;	// ボーン基準点(回転中心).
		std::vector<BoneNode*>	Children;	// 子ノード.

		BoneNode()
			: BoneIndex (0)
			, StartPos	(0.0f, 0.0f, 0.0f)
			, Children	{}
		{}
	};

	// モーションデータ.
	struct VMDKeyFrame {
		char BoneName[15];				// ボーン名.
		unsigned int FrameNo;			// フレーム番号(読込時は現在のフレーム位置を0とした相対位置).
		DirectX::XMFLOAT3 Location;		// 位置.
		DirectX::XMFLOAT4 Quaternion;	// 回転.
		unsigned char Bezier[64];		// [4][4][4]  ベジェ補完パラメータ.

		VMDKeyFrame()
			: BoneName	{}
			, FrameNo	(0)
			, Location	{}
			, Quaternion{}
			, Bezier    {}
		{}
	};
	
	// キーフレーム構造体.
	struct KeyFrame {
		unsigned int FrameNo;			// フレーム№(アニメーション開始からの経過時間).
		DirectX::XMVECTOR Quaternion;	// クォータニオン.
		DirectX::XMFLOAT2 p1, p2;		// ベジェの中間コントロールポイント.

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

	// モーション構造体.
	struct Motion
	{
		unsigned int Frame;				// アニメーション開始からのフレーム.
		DirectX::XMVECTOR Quaternion;	// クオータニオン.

		Motion()
			: Frame		(0)
			, Quaternion{}
		{}
	};

	//読み込んだマテリアルをもとにマテリアルバッファを作成
	void CreateMaterialData();
	
	//マテリアル＆テクスチャのビューを作成
	void CreateMaterialAndTextureView();

	//座標変換用ビューの生成
	void CreateTransformView();

	//PMDファイルのロード
	void LoadPMDFile(const char* FilePath);

	/*******************************************
	* @brief	回転情報を末端まで伝播させる再帰関数.
	* @param	回転させたいボーンノード.
	* @param    回転行列.
	*******************************************/
	void RecursiveMatrixMultipy(BoneNode* node, const DirectX::XMMATRIX& mat);

	float _angle;//テスト用Y軸回転

	float GetYFromXOnBezier(
		float x, 
		const DirectX::XMFLOAT2& a, 
		const DirectX::XMFLOAT2& b, uint8_t n = 12);

	/*******************************************
	* @brief	頂点の総数を読み込む.
	* @param	読み込むファイル.
	* @param    頂点のサイズ.
	* @retrun	頂点数.
	*******************************************/
	uint32_t ReadIndicesNum(FILE* fp, uint8_t indexSize);

public:
	CPMXActor(const char* filepath,CPMXRenderer& renderer);
	~CPMXActor();
	///クローンは頂点およびマテリアルは共通のバッファを見るようにする
	CPMXActor* Clone();
	void Update();
	void Draw();

	// アニメーション開始.
	void PlayAnimation();

	// アニメーションの更新.
	void MotionUpdate();

	/*******************************************
	* @brief	VMDのロード.
	* @param	ファイルパス.
	* @param	ムービー名.
	*******************************************/
	void LoadVMDFile(const char* FilePath, const char* Name);

private:
	CPMXRenderer& m_pRenderer;
	CDirectX12& m_pDx12;

	//頂点関連
	MyComPtr<ID3D12Resource>		m_pVertexBuffer;			// 頂点バッファ.
	MyComPtr<ID3D12Resource>		m_pIndexBuffer;				// インデックスバッファ.
	D3D12_VERTEX_BUFFER_VIEW		m_pVertexBufferView;		// 頂点バッファビュー.
	D3D12_INDEX_BUFFER_VIEW			m_pIndexBufferView;			// インデックスバッファビュー.

	MyComPtr<ID3D12Resource>		m_pTransformMat;			// 座標変換行列(今はワールドのみ).
	MyComPtr<ID3D12DescriptorHeap>	m_pTransformHeap;			// 座標変換ヒープ.

	Transform						m_Transform;				// 座標.		
	DirectX::XMMATRIX*				m_MappedMatrices;			// GPUとみる座標.					
	MyComPtr<ID3D12Resource>		m_pTransformBuff;			// バッファ.

	//マテリアル関連
	std::vector<std::shared_ptr<Material>>	m_pMaterial;		// マテリアル.
	MyComPtr<ID3D12Resource>				m_pMaterialBuff;	// マテリアルバッファ.
	std::vector<MyComPtr<ID3D12Resource>>	m_pTextureResource;	// 画像リソース.
	std::vector<MyComPtr<ID3D12Resource>>	m_pSphResource;		// Sphリソース.
	std::vector<MyComPtr<ID3D12Resource>>	m_pSpaResource;		// Spaリソース.
	std::vector<MyComPtr<ID3D12Resource>>	m_pToonResource;	// トゥーンリソ－ス.

	MyComPtr<ID3D12DescriptorHeap> m_pMaterialHeap;				// マテリアルヒープ(5個ぶん)

	// ボーン関連.
	std::vector<DirectX::XMMATRIX>	m_BoneMatrix;				// ボーン座標.
	std::map<std::string, BoneNode> m_BoneNodeTable;			// ボーンの階層.

	// アニメーション関連.
	std::unordered_map<std::string, std::vector<KeyFrame>> m_MotionData;	// モーションデータ.
	DWORD							m_StartTime;				// アニメーションの開始時間(ミリ秒).
	
};

