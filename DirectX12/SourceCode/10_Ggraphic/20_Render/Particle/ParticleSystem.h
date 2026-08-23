#pragma once

#include <DirectXMath.h>

#include "99_Utility/ObjectPool/ObjectPool.h"

/**********************************************************************************
* @author    : Coder 青龍(せいりゅう).
* @date      : 2026/08/23.
* @brief     : オリジナルパーティクルシステム(CPU更新+ビルボード描画の最小構成).
*            : パーティクル実体はObjectPool経由で生成/破棄される
*            : (毎フレームのnew/deleteは発生しない). ServiceLocator経由で利用する.
*            : 描画パイプライン(PSO等)はプロセス単位で1つ(cpp内の静的状態).
**********************************************************************************/

class ParticleSystem final
{
public:
	ParticleSystem() = default;
	~ParticleSystem() = default;

	ParticleSystem(const ParticleSystem&)            = delete;
	ParticleSystem& operator=(const ParticleSystem&) = delete;

	// パイプライン(ルートシグネチャ/PSO/頂点バッファ)を構築する(MainScene::Createで呼ぶ).
	bool Initialize(struct ID3D12Device* pDevice);

	// 全パーティクルの寿命・速度を更新する(毎フレーム. 一時停止中は呼ばない).
	void Update(float DeltaTime);

	// アクティブカメラ基準でビルボード描画する(MainScene::Drawのキャラ描画後に呼ぶ).
	void Draw();

	// ヒットエフェクト用の既定emit(現在パラメータで白い粒子が放射状に散る).
	void SpawnHitEffect(const DirectX::XMFLOAT3& Position);

	// 全パーティクルを消す(EditorのStop/Reset用).
	void ClearParticles();

public: // Editor用のパラメータアクセス.
	// 発生パラメータ(Editorで編集・JSONプリセット保存対象).
	struct EmitterParams
	{
		int               Count        = 12;
		float             SpeedMin     = 2.0f;
		float             SpeedMax     = 5.0f;
		float             LifeTimeMin  = 0.25f;
		float             LifeTimeMax  = 0.5f;
		float             Size         = 0.08f;
		float             GravityScale = 0.5f;
		DirectX::XMFLOAT4 Color        = { 1.0f, 1.0f, 1.0f, 1.0f };
		std::string       TextureName; // 将来のテクスチャ適用用(v1は保存のみ. 描画未対応).

		void Reset()
		{
			*this = EmitterParams{};
		}
	};

	const EmitterParams& GetEmitterParams() const noexcept { return m_Params; }
	void SetEmitterParams(const EmitterParams& Params) noexcept { m_Params = Params; }

private:
	// プール管理されるパーティクル実体.
	struct Particle
	{
		DirectX::XMFLOAT3 Position{ 0.0f, 0.0f, 0.0f };
		DirectX::XMFLOAT3 Velocity{ 0.0f, 0.0f, 0.0f };
		DirectX::XMFLOAT4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
		float             LifeTime     = 0.0f; // 残り寿命(0以下=死んでいる).
		float             MaxLifeTime  = 1.0f; // 初期寿命(フェード計算用).
		float             Size         = 0.1f;
		float             GravityScale = 0.0f;

		// ObjectPoolのAcquire時に自動呼び出しされる初期化(ObjectPool側の規約).
		void Reset()
		{
			Position     = { 0.0f, 0.0f, 0.0f };
			Velocity     = { 0.0f, 0.0f, 0.0f };
			Color        = { 1.0f, 1.0f, 1.0f, 1.0f };
			LifeTime     = 0.0f;
			MaxLifeTime  = 1.0f;
			Size         = 0.1f;
			GravityScale = 0.0f;
		}
	};

private:
	void Emit(const DirectX::XMFLOAT3& Position, const EmitterParams& Params);

	static constexpr size_t kMaxParticles = 1024; // 同時存在上限.

	ObjectPool<Particle> m_Pool;   // パーティクル実体のプール(new/deleteなし).
	EmitterParams        m_Params; // 現在の発生パラメータ(Editorから変更される).
};
