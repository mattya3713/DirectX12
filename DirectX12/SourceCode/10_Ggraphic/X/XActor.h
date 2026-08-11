#pragma once

#include<d3d12.h>
#include<DirectXMath.h>
#include<vector>
#include<string>
#include"PMX/PMXStructHeader.h"	// TransformConstantBuffer(b1)を共用する.
#include"Model/ModelData.h"		// フォーマットを問わないモデルデータ.

// 前方宣言.
class DirectX12;
class PMXRenderer;

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/12.
* @brief     : .xファイル(XParser経由)をPMXRendererの既存パイプラインで描画するクラス.
*            : ボーン・アニメーションを持たない静的メッシュ専用(PMXActorからボーン/VMD
*            : 関連を除いた最小構成). 頂点レイアウト(Model::Vertex)・ルートシグネチャ・
*            : シェーダーはPMXActorと共通のため、専用のXRendererは用意していない.
**********************************************************************************/

class XActor
{
public:
	XActor(const char* FilePath, PMXRenderer& Renderer);
	~XActor();

	XActor(const XActor&)            = delete;
	XActor& operator=(const XActor&) = delete;
	XActor(XActor&&)                 = delete;
	XActor& operator=(XActor&&)      = delete;

	void Draw();

	// ワールド行列を設定する(移動・回転・拡縮の反映用).
	void SetWorldMatrix(const DirectX::XMMATRIX& World) noexcept { if (m_pMappedTransformCB) { m_pMappedTransformCB->World = World; } }

private:
	// ルートパラメータのインデックス(PMXActorと同じシェーダー・ルートシグネチャを使うため揃える).
	enum RootParamIndex
	{
		RP_SCENE_CBV = 0,
		RP_TRANSFORM_CBV,
		RP_MATERIAL_TABLE_CBV_SRV,
		RP_BONE_SRV,
		RP_COUNT
	};

private:
	// GPUリソースの作成と初期データ転送を担当.
	void CreateResources();

	// テクスチャがなければレンダラーの白テクスチャを返す.
	MyComPtr<ID3D12Resource> LoadTexture(const std::string& Path);

private:
	PMXRenderer& m_Renderer;
	DirectX12&   m_Dx12;

	MyComPtr<ID3D12Resource> m_pVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;

	MyComPtr<ID3D12Resource> m_pIndexBuffer;
	D3D12_INDEX_BUFFER_VIEW  m_IndexBufferView;

	MyComPtr<ID3D12Resource>      m_pTransformConstantBuffer;
	PMX::TransformConstantBuffer* m_pMappedTransformCB = nullptr;

	// ボーンStructuredBuffer(t3)用のダミー(常に1要素・単位行列). 全頂点のBoneWeightsが0のため
	// シェーダー側で実際に参照されることは無いが、ルートシグネチャの都合上有効なSRVが必要.
	MyComPtr<ID3D12Resource> m_pDummyBoneBuffer;

	MyComPtr<ID3D12DescriptorHeap> m_pCbvSrvUavHeap;
	UINT m_CbvSrvUavDescriptorSize = 0;

	Model::ModelData m_ModelData;

	std::vector<MyComPtr<ID3D12Resource>> m_pTextureResource; // ベーステクスチャ(MaterialIndexに対応).
};
