#pragma once

#include<d3d12.h>
#include<DirectXMath.h>
#include<vector>
#include<string>
#include"PMXStructHeader.h"	// PMX用構造体まとめ.

// 前方宣言.
class DirectX12;
class PMXRenderer;

/**************************************************
*	PMXモデルクラス.
*	担当：淵脇 未来
**/

class PMXActor
{
	friend PMXRenderer;

public:
	PMXActor(const char* filepath, PMXRenderer& renderer);
	~PMXActor();
	///クローンは頂点およびマテリアルは共通のバッファを見るようにする
	PMXActor* Clone();
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

	struct Transform {
		// 内部に持ってるXMMATRIXメンバが16バイトアライメントであるため.
		// Transformをnewする際には16バイト境界に確保する.
		void* operator new(size_t size);
		DirectX::XMMATRIX world;
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
	void ReadPMXHeader(FILE* fp, PMX::Header* Header);

	// 文字列の読み込み.
	void ReadString(FILE* fp, std::string& Name);

	// パスの変換の関数ポインタ.
	void ConvertUTF16ToUTF8(const std::vector<uint8_t>& buffer, std::string& OutString); 
	void ConvertUTF8(const std::vector<uint8_t>& buffer, std::string& OutString);
	using PathConverterFunction = void(PMXActor::*)(const std::vector<uint8_t>&, std::string&);
	PathConverterFunction PathConverter = nullptr;

	// インデックスを読み込む時の関数を選択するための関数ポインタ.
	void ReadPMXIndices1Byte(FILE* fp,const uint32_t& IndicesNum, std::vector<uint32_t>* Faces);
	void ReadPMXIndices2Byte(FILE* fp,const uint32_t& IndicesNum, std::vector<uint32_t>* Faces);
	void ReadPMXIndices4Byte(FILE* fp,const uint32_t& IndicesNum, std::vector<uint32_t>* Faces);
	using ReadIndicesFunction = void(PMXActor::*)(FILE*, const uint32_t&, std::vector<uint32_t>*);
	ReadIndicesFunction ReadIndices = nullptr;

	/*******************************************
	* @brief	PMXバイナリからインデックス数とインデックスを読み込む.
	* @param	読み込みファイルポインタ.
	* @param	読み込んだインデックス.
	*******************************************/
	void ReadPMXIndices(FILE* fp, std::vector<uint32_t>* Faces, uint32_t* IndicesNum);

	/*******************************************
	* @brief	回転情報を末端まで伝播させる再帰関数.
	* @param	回転させたいボーンノード.
	* @param    回転行列.
	*******************************************/
	//void RecursiveMatrixMultipy(BoneNode* node, const DirectX::XMMATRIX& mat);

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

private:
	PMXRenderer& m_pRenderer;
	DirectX12& m_pDx12;

	// 頂点関連.
	MyComPtr<ID3D12Resource>		m_pVertexBuffer;			// 頂点バッファ.
	PMX::VertexForHLSL*				m_pMappedVertex;			// 頂点マップ.
	std::vector<PMX::VertexForHLSL>	m_VerticesForHLSL;			// GPUに送る頂点座標.
	D3D12_VERTEX_BUFFER_VIEW		m_pVertexBufferView;		// 頂点バッファビュー.

	// インデックス関連.
	MyComPtr<ID3D12Resource>		m_pIndexBuffer;				// インデックスバッファ.
	std::vector<uint32_t>			m_Faces;					// インデックス.
	uint32_t*						m_MappedIndex;				// 頂点マップ.
	D3D12_INDEX_BUFFER_VIEW			m_pIndexBufferView;			// インデックスバッファビュー.


	MyComPtr<ID3D12Resource>		m_pTransformMat;			// 座標変換行列(今はワールドのみ).

	// 座標関連.
	Transform						m_Transform;				// 座標.		
	MyComPtr<ID3D12DescriptorHeap>	m_pTransformHeap;			// 座標変換ヒープ.
	DirectX::XMMATRIX*				m_MappedMatrices;			// GPUとみる座標.					
	MyComPtr<ID3D12Resource>		m_pTransformBuffer;			// バッファ.

	//マテリアル関連.
	MyComPtr<ID3D12Resource>				m_pMaterialBuff;	// マテリアルバッファ.
	char*									m_pMappedMaterial;	// マテリアルマップ
	MyComPtr<ID3D12DescriptorHeap>			m_pMaterialHeap;	// マテリアルヒープ.
	std::vector<PMX::Material>				m_Materials;		// マテリアル.
	std::vector<PMX::MaterialForHLSL>		m_MaterialsForHLSL;	// GPUに送るマテリアル情報.


	std::vector<MyComPtr<ID3D12Resource>>	m_pTextureResource;	// 画像リソース.
	std::vector<MyComPtr<ID3D12Resource>>	m_pSphResource;		// Sphリソース.
	std::vector<MyComPtr<ID3D12Resource>>	m_pToonResource;	// トゥーンリソ－ス.



	
};

