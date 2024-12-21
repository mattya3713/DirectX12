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
public:

	// モデルの頂点サイズ.
	static constexpr size_t PMXVertexSize = 60;

private:

	// PMXヘッダー構造体.
	struct PMXHeader
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

	// PMXモデル情報.
	struct PMXModelInfo {
		std::string ModelName;			// モデル名.
		std::string ModelNameEnglish;	// モデル名(英語).
		std::string ModelComment;		// コメント.
		std::string ModelCommentEnglish;// コメント(英語).
	};

	// TODO : パディングの対処法を考える.
	// PMXマテリアル構造体.
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

	struct PMXVertex
	{
		DirectX::XMFLOAT3					Position;		// 座標.
		DirectX::XMFLOAT3					Normal;			// 法線.
		DirectX::XMFLOAT2					UV;				// UV.	
		std::array<DirectX::XMFLOAT4, 4>	AdditionalUV;	// 追加のUV.
		std::array<uint32_t, 4>				BoneIndices;	// ボーンインデックス.
		std::array<float   , 4>				BoneWeights;	// ボーンウェイト.
		DirectX::XMFLOAT3					SDEF_C;			// SDEF_C値.
		DirectX::XMFLOAT3					SDEF_R0;		// SDEF_R0値.
		DirectX::XMFLOAT3					SDEF_R1;		// SDEF_R1値.
		float								Edge;			// エッジサイズ.
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
		DirectX::XMFLOAT4	Diffuse;	// ディフューズ色.	
		DirectX::XMFLOAT3	Specular;	// スペキュラの強.		
		float				Specularity;// スペキュラ色.		
		DirectX::XMFLOAT3	Ambient;	// アンビエント色.		

		MaterialForHlsl()
			: Diffuse		(0.0f, 0.0f, 0.0f, 0.0f)
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

	// 読み込んだマテリアルをもとにマテリアルバッファを作成.
	void CreateMaterialData();
	
	// マテリアル＆テクスチャのビューを作成.
	void CreateMaterialAndTextureView();

	// 座標変換用ビューの生成.
	void CreateTransformView();

	// PMDファイルのロード.
	void LoadPMXFile(const char* FilePath);
	
	// PMXヘッター読み込み.
	void ReadPMXHeader(FILE* fp, PMXHeader* Header);

	// インデックスを読み込む時の関数を選択するための関数ポインタ.
	void ReadPMXIndices1Byte(FILE* fp,const uint32_t& IndicesNum, std::vector<PMXFace>* Faces);
	void ReadPMXIndices2Byte(FILE* fp,const uint32_t& IndicesNum, std::vector<PMXFace>* Faces);
	void ReadPMXIndices4Byte(FILE* fp,const uint32_t& IndicesNum, std::vector<PMXFace>* Faces);
	using ReadIndicesFunction = void(CPMXActor::*)(FILE*, const uint32_t&, std::vector<PMXFace>*);
	ReadIndicesFunction ReadIndices = nullptr;

	/*******************************************
	* @brief	PMXバイナリからインデックス数とインデックスを読み込む.
	* @param	読み込みファイルポインタ.
	* @param	読み込んだインデックス.
	*******************************************/
	void ReadPMXIndices(FILE* fp,std::vector<PMXFace>* Faces);

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
	* @brief    指定されたサイズ分のデータを読み込み、4バイトの値として返す.
	* @param    ファイルポインタ。読み込み対象のファイルを指します.
	* @param    読み込むデータのサイズ(1,2,4バイトのどれか).
	* @return   読み込んだデータを4バイトの`uint32_t`型に変換した値
	* @throw	知らないサイズ.
	*******************************************/
	uint32_t ReadAndCastIndices(FILE* fp, uint8_t indexSize);

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
	std::vector<std::shared_ptr<Material>>	m_pMaterials;		// マテリアル.
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

