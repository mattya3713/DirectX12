#pragma once

#include<d3d12.h>
#include<DirectXMath.h>
#include<vector>
#include<string>
#include"PMX/PMXStructHeader.h"	// TransformConstantBuffer(b1)を共用する.
#include"Model/ModelData.h"		// フォーマットを問わないモデルデータ.
#include"XSkeletonData.h"		// ボーン階層・アニメーションクリップ.

// 前方宣言.
class DirectX12;
class PMXRenderer;

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/12.
* @brief     : .xファイル(XParser経由)をPMXRendererの既存パイプラインで描画するクラス.
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

	void Update();
	void Draw();

	// ワールド行列を設定する(移動・回転・拡縮の反映用).
	void SetWorldMatrix(const DirectX::XMMATRIX& World) noexcept { if (m_pMappedTransformCB) { m_pMappedTransformCB->World = World; } }

	// 名前でアニメーションクリップを再生する(見つからない場合は何もしない).
	void PlayAnimation(const std::string& ClipName);

	void StopAnimation() noexcept { m_CurrentClipIndex = -1; }

#if _DEBUG
	// バインドポーズでのY軸方向の高さ(Scaleを掛ける前. モデルサイズ検知用).
	float GetLocalHeight() const noexcept { return m_LocalHeight; }
#endif

	// 読み込まれているクリップ名の一覧(ImGui等での一覧表示用).
	const std::vector<XSkeleton::AnimationClip>& GetClips() const noexcept { return m_Skeleton.Clips; }
	int GetCurrentClipIndex() const noexcept { return m_CurrentClipIndex; }

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

	// FinalMatrix全ボーン分書き込む.
	void UpdateBoneMatrices();

private:
	PMXRenderer& m_Renderer;
	DirectX12&   m_Dx12;

	MyComPtr<ID3D12Resource> m_pVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;

	MyComPtr<ID3D12Resource> m_pIndexBuffer;
	D3D12_INDEX_BUFFER_VIEW  m_IndexBufferView;

	MyComPtr<ID3D12Resource>      m_pTransformConstantBuffer;
	PMX::TransformConstantBuffer* m_pMappedTransformCB = nullptr;

	MyComPtr<ID3D12Resource>  m_pBoneTransformBuffer; // StructuredBuffer<float4x4>(t3). ボーン数ぶん.
	DirectX::XMMATRIX*        m_pMappedBoneTransforms = nullptr;

	MyComPtr<ID3D12DescriptorHeap> m_pCbvSrvUavHeap;
	UINT m_CbvSrvUavDescriptorSize = 0;

	Model::ModelData        m_ModelData;
	XSkeleton::SkeletalData m_Skeleton;

#if _DEBUG
	float m_LocalHeight = 0.0f; // バインドポーズでの高さ(Y方向 max-min. モデルサイズ検知用).
#endif

	std::vector<MyComPtr<ID3D12Resource>> m_pTextureResource; // ベーステクスチャ(MaterialIndexに対応).

	int   m_CurrentClipIndex = -1; // 再生中のクリップ(m_Skeleton.Clipsへのインデックス. -1=未再生=バインドポーズ).
	float m_CurrentTime      = 0.0f; // 現在のクリップ内再生時刻(ファイル依存の時間軸. 秒ではない).
};
