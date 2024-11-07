#pragma once

#include<d3d12.h>
#include<DirectXMath.h>
#include<vector>
#include<string>

// 前方宣言.
class CDirectX12;
class PMDRenderer;

class PMDActor
{
	friend PMDRenderer;
private:

	// シェーダ側に投げられるマテリアルデータ.
	struct MaterialForHlsl {
		DirectX::XMFLOAT3 Diffuse;		// ディフューズ色.		
		float	 Alpha;					// α値.		
		DirectX::XMFLOAT3 Specular;		// スペキュラの強.		
		float	 Specularity;			// スペキュラ色.		
		DirectX::XMFLOAT3 Ambient;		// アンビエント色.		
	};

	// それ以外のマテリアルデータ.
	struct AdditionalMaterial {
		std::string TexPath;	// テクスチャファイルパス.
		int			ToonIdx;	// トゥーン番号.
		bool		EdgeFlg;	// マテリアル毎の輪郭線フラグ.
	};

	// まとめたもの.
	struct Material {
		unsigned int IndicesNum;	//インデックス数.
		MaterialForHlsl Material;
		AdditionalMaterial Additional;
	};

	struct Transform {
		//内部に持ってるXMMATRIXメンバが16バイトアライメントであるため
		//Transformをnewする際には16バイト境界に確保する
		void* operator new(size_t size);
		DirectX::XMMATRIX world;
	};

	
	//読み込んだマテリアルをもとにマテリアルバッファを作成
	HRESULT CreateMaterialData();
	
	//マテリアル＆テクスチャのビューを作成
	HRESULT CreateMaterialAndTextureView();

	//座標変換用ビューの生成
	HRESULT CreateTransformView();

	//PMDファイルのロード
	HRESULT LoadPMDFile(const char* path);

	float _angle;//テスト用Y軸回転
public:
	PMDActor(const char* filepath,PMDRenderer& renderer);
	~PMDActor();
	///クローンは頂点およびマテリアルは共通のバッファを見るようにする
	PMDActor* Clone();
	void Update();
	void Draw();

private:
	PMDRenderer& m_pRenderer;
	CDirectX12& m_pDx12;

	//頂点関連
	MyComPtr<ID3D12Resource>		m_pVertexBuffer;			// 頂点バッファ.
	MyComPtr<ID3D12Resource>		m_pIndexBuffer;				// インデックスバッファ.
	D3D12_VERTEX_BUFFER_VIEW		m_pVertexBufferView;		// 頂点バッファビュー.
	D3D12_INDEX_BUFFER_VIEW			m_pIndexBufferView;			// インデックスバッファビュー.

	MyComPtr<ID3D12Resource>		m_pTransformMat;			// 座標変換行列(今はワールドのみ).
	MyComPtr<ID3D12DescriptorHeap>	m_pTransformHeap;			// 座標変換ヒープ.

	Transform						m_Transform;							
	Transform*						m_MappedTransform;					
	MyComPtr<ID3D12Resource>		m_pTransformBuff;		

	//マテリアル関連
	std::vector<MyComPtr<Material>>			m_pMaterial;			// マテリアル.
	MyComPtr<ID3D12Resource>				m_pMaterialBuff;	// マテリアルバッファ.
	std::vector<MyComPtr<ID3D12Resource>>	m_pTextureResource;	// 画像リソース.
	std::vector<MyComPtr<ID3D12Resource>>	m_pSphResource;		// Sphリソース.
	std::vector<MyComPtr<ID3D12Resource>>	m_pSpaResource;		// Spaリソース.
	std::vector<MyComPtr<ID3D12Resource>>	m_pToonResource;	// トゥーンリソ－ス.

	MyComPtr<ID3D12DescriptorHeap> m_pMaterialHeap;				// マテリアルヒープ(5個ぶん)
};

