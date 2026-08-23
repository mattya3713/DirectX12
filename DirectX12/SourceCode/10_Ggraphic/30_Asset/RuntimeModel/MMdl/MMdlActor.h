#pragma once

#include<d3d12.h>
#include<DirectXMath.h>
#include<filesystem>
#include<vector>
#include<string>
#include"90_Legacy/PMX/PMXStructHeader.h"	// TransformConstantBuffer(b1)を共用する.
#include"30_Asset/Parser/ModelData.h"		// フォーマットを問わないモデルデータ.
#include"MMdlSkeletonData.h"		// ボーン階層・アニメーションクリップ.
#include"10_Ggraphic/30_Asset/RuntimeFormat/RuntimeFormat.h"
#include"10_Ggraphic/30_Asset/RuntimeModel/MMdl/MMdlResource.h"

// 前方宣言.
class DirectX12;
class MmdlRenderer;

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/12.
* @brief     : mmdlランタイム形式をMmdlRendererの既存パイプラインで描画するクラス.
**********************************************************************************/

class MmdlActor
{
public:
	MmdlActor(const char* FilePath, MmdlRenderer& Renderer);
	MmdlActor(const std::filesystem::path& FilePath, MmdlRenderer& Renderer);
	MmdlActor(std::shared_ptr<MmdlResource> Resource, MmdlRenderer& Renderer);
	~MmdlActor();

	MmdlActor(const MmdlActor&)            = delete;
	MmdlActor& operator=(const MmdlActor&) = delete;
	MmdlActor(MmdlActor&&)                 = delete;
	MmdlActor& operator=(MmdlActor&&)      = delete;

	void Update();
	void Draw();

	// ワールド行列を設定する(移動・回転・拡縮の反映用).
	// NOTE: 描画後にTransformを戻してはいけない. TransformCBは永続マップされており、
	//       書き込みは即GPUから参照される(戻すと次フレームの描画に一瞬前フレームの行列が残る).
	void SetWorldMatrix(const DirectX::XMMATRIX& World) noexcept { if (m_pMappedTransformCB) { m_pMappedTransformCB->World = World; } }

	// 名前でアニメーションクリップを再生する(見つからない場合は何もしない).
	void PlayAnimation(const std::string& ClipName);
	// 外部から指定したActionFrameで姿勢を固定する.
	void SetCurrentFrame(float ActionFrame) noexcept
	{
		if (m_CurrentClipIndex < 0) { return; }
		m_CurrentTime = ActionFrame / 30.0f * static_cast<float>(m_Skeleton.TicksPerSecond);
		m_IsExternallyDriven = true;
	}

	void StopAnimation() noexcept { m_CurrentClipIndex = -1; }

	// バインドポーズでのY軸方向の高さ(Scaleを掛ける前. モデルサイズ検知用).
	float GetLocalHeight() const noexcept { return m_LocalHeight; }

	// 読み込まれているクリップ名の一覧(ImGui等での一覧表示用).
	const std::vector<XSkeleton::AnimationClip>& GetClips() const noexcept { return m_Skeleton.Clips; }
	int GetCurrentClipIndex() const noexcept { return m_CurrentClipIndex; }
	// 現在のアニメーション内再生位置を秒で取得する(クリップ未再生なら0).
	float GetCurrentAnimationSeconds() const noexcept
	{
		if (m_CurrentClipIndex < 0 || m_Skeleton.TicksPerSecond <= 0) { return 0.0f; }
		return m_CurrentTime / static_cast<float>(m_Skeleton.TicksPerSecond);
	}

private:
	// ルートパラメータのインデックス(PMXActorと同じシェーダー・ルートシグネチャを使うため揃える).
	enum RootParamIndex
	{
		RP_SCENE_CBV = 0,
		RP_TRANSFORM_CBV,
		RP_MATERIAL_TABLE_CBV_SRV,
		RP_BONE_SRV,
		RP_SHADOW_SRV, // シャドウマップ(ピクセルシェーダーt3. MmdlRendererが管理する共有深度バッファ).
		RP_COUNT
	};

private:
	// GPUリソースの作成と初期データ転送を担当.
	void CreateResources();

	// テクスチャがなければレンダラーの白テクスチャを返す.
	MyComPtr<ID3D12Resource> LoadTexture(const std::string& Path);

	// FinalMatrix全ボーン分書き込む.
	void UpdateBoneMatrices();
	void LoadRuntimeModel(const std::filesystem::path& FilePath);

private:
	MmdlRenderer& m_Renderer;
	DirectX12&   m_Dx12;
	std::shared_ptr<MmdlResource> m_spResource;

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

	float m_LocalHeight = 0.0f; // バインドポーズでの高さ(Y方向 max-min. モデルサイズ検知用).

	std::vector<MyComPtr<ID3D12Resource>> m_pTextureResource; // ベーステクスチャ(MaterialIndexに対応).
	std::vector<MyComPtr<ID3D12Resource>> m_pToonResource; // トゥーンテクスチャ(MaterialIndexに対応).
	std::vector<MyComPtr<ID3D12Resource>> m_pSphereResource; // スフィアテクスチャ(MaterialIndexに対応).
	std::vector<RuntimeFormat::SkinSubmeshRole> m_SkinSubmeshRoles; // モデル固有の合成役割.
	float m_FrontCompositeOpacity = 1.0f; // オフスクリーン結果の不透明度.
	float m_FrontCompositeMaxDistance = 0.0f; // 髪を奥へ緩和する距離.

	int   m_CurrentClipIndex = -1; // 再生中のクリップ(m_Skeleton.Clipsへのインデックス. -1=未再生=バインドポーズ).
	float m_CurrentTime      = 0.0f; // 現在のクリップ内再生時刻(ファイル依存の時間軸. 秒ではない).
	bool  m_IsExternallyDriven = false; // 外部指定フレームで再生を停止中.
	float m_PlaybackSpeed    = 1.0f; // 再生速度倍率(コンボフロー用. 1.0=等速).

	// クロスフェード用(PlayAnimation切替時に前ポーズへ線形補間で滑らかにつなぐ).
	static constexpr float kBlendDuration = 0.2f;                       // ブレンド時間(秒).
	std::vector<DirectX::XMMATRIX> m_BlendSourceLocals;                 // ブレンド開始時の各ボーンのローカル変換.
	std::vector<DirectX::XMMATRIX> m_LastLocalTransforms;               // 前フレームの各ボーンのローカル変換(スナップショット取得用).
	bool  m_IsBlending   = false;                                       // ブレンド中か.
	float m_BlendElapsed = 0.0f;                                        // ブレンド経過時間(秒).

public:
	// アニメーション再生速度の倍率を設定する(0以下は1.0へ丸める).
	void SetPlaybackSpeed(float Speed) noexcept { m_PlaybackSpeed = (Speed > 0.0f) ? Speed : 1.0f; }
	float GetPlaybackSpeed() const noexcept { return m_PlaybackSpeed; }
};
