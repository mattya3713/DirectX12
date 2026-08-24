#pragma once

#include <DirectXMath.h>
#include <map>
#include <string>

#include "00_Game/10_Object/10_MeshObject/MeshObject.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/CharacterAccessKeys.h"
#include "00_Game/40_Collision/00_Capsule/CapsuleCollider.h"
#include "00_Game/40_Collision/00_Core/HitEvent.h"
#include "99_Utility/HealthSystem/HealthSystem.h"

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/10.
* @brief     : ゲーム内キャラクターの基底クラス. MeshObjectを継承する具象クラス
*            : (見た目(PMXMesh)を持てる). HPはHealthSystemをメンバとして持つ
*            : (コンポジション. IHealthSystemを直接継承しない理由はHealthSystem.h参照).
*            : 静的な小道具やトリガー等、生命・行動を持たないGameObjectと区別するための層.
**********************************************************************************/

class Character : public MeshObject
{
public:
	Character();
	virtual ~Character();

	Character(const Character&)            = delete;
	Character& operator=(const Character&) = delete;
	Character(Character&&)                 = delete;
	Character& operator=(Character&&)      = delete;

	// MeshObject::Update()の後、自分の被弾コライダーが検出したヒットと、
	// 実体(Body)コライダーの押し出しを処理する.
	void Update() override;

#if _DEBUG
	// MeshObject::Draw()の後にモデルサイズを検査する(Debugビルドのみ).
	void Draw() override;

	// 被弾/攻撃判定コライダーをワイヤーフレームで描画する(Debugビルドのみ).
	// 描画中にRoot Signature/PSOを切り替えるため、全キャラのDraw()が終わった後に
	// MainScene側でまとめて呼ぶこと(メッシュ描画と混ぜるとPSO競合でクラッシュする).
	virtual void DrawDebugColliders() const;
#endif

public:
	// HP関連の情報取得.
	const HealthSystem& GetHealth() const noexcept { return m_Health; }
	// 最大HPと現在HPを設定する(EnemyFactory等の生成時初期化用).
	void SetHealth(float MaxHP, float CurrentHP) noexcept { m_Health.SetMaxHP(MaxHP); m_Health.SetHP(CurrentHP); }

	// デバッグ用: 現在HPを全て削って通常の死亡フローを通す(デバッグコンソールのkill_boss等から使用).
	void ApplyDebugKill() { m_Health.ApplyDamage(m_Health.GetHP()); }

public: // 攻撃判定の制御(攻撃系Stateから呼ぶ想定).
	void SetAttackColliderActive(bool IsActive) noexcept { m_AttackCollider.SetActive(IsActive); }
	void SetAttackAmount(float AttackAmount) noexcept { m_AttackCollider.SetAttackAmount(AttackAmount); }
	void SetAttackColliderOffset(const DirectX::XMFLOAT3& Offset) noexcept { m_AttackCollider.SetPositionOffset(Offset); }

#if _DEBUG
	// Combat Debug HUD用の有効状態取得(表示専用. ロジックは変更しない).
	bool IsAttackColliderActive() const noexcept { return m_AttackCollider.GetActive(); }
	bool IsDamageColliderActive() const noexcept { return m_DamageCollider.GetActive(); }
#endif

public: // 被弾判定の制御(パリィ等、一時的に無敵にしたいStateから呼ぶ想定).
	void SetDamageColliderActive(bool IsActive) noexcept { m_DamageCollider.SetActive(IsActive); }

public: // エフェクト再生(フックのみ. 中身は未実装 — Effekseer/自作パーティクル等、方式決定後に実装する).

	// 自分の位置からの相対オフセットで再生する.
	void PlayEffect(const std::string& Name, const DirectX::XMFLOAT3& Offset = { 0.0f, 0.0f, 0.0f }, float Scale = 1.0f, bool IsUI = false) {}

	// ワールド座標を指定して再生する(パーティクルシステムへ接続済み. 現在はヒットエフェクトのみ).
	void PlayEffectAtWorldPos(const std::string& Name, const DirectX::XMFLOAT3& WorldPos, float Scale = 1.0f, bool IsUI = false);

	// ワールド座標+回転を指定して再生する.
	void PlayEffectAtWorldPos(const std::string& Name, const DirectX::XMFLOAT3& WorldPos, const DirectX::XMFLOAT3& EulerRotation, float Scale = 1.0f, bool IsUI = false) {}

	// スクリーン座標を指定してUIエフェクトを再生する.
	void PlayEffectUIAtScreenPos(const std::string& Name, const DirectX::XMFLOAT2& ScreenPos, float Scale = 1.0f) {}

protected:

	// 被弾時のリアクション(派生クラスで上書き. 既定は何もしない.
	// HP減算はApplyDamage()側で済むため、ここでは見た目・挙動の反応だけを扱う).
	virtual void OnDamaged(const HitEvent& Event) {}

	// 実体(Body)コライダーのグループ設定(派生クラスが自分/相手の陣営を指定する).
	void SetBodyCollisionMasks(eCollisionGroup MyGroup, eCollisionGroup TargetGroup) noexcept
	{
		m_BodyCollider.SetMyMask(MyGroup);
		m_BodyCollider.SetTargetMask(TargetGroup);
	}

	// ダメージ/死亡コールバックの登録
	void SetOnDamage(HealthSystem::DamageCallback Callback) { m_Health.SetOnDamage(std::move(Callback)); }
	void SetOnDeath(HealthSystem::DeathCallback Callback) { m_Health.SetOnDeath(std::move(Callback)); }

	// 特定のヒットを無視するか(派生クラスで上書き. パリィ済み攻撃のダメージ二重適用防止等に使う).
	virtual bool ShouldIgnoreHit(const CollisionInfo& Info) const noexcept { (void)Info; return false; }

	// HitEventを受けてダメージを適用する(publicにはしない. ApplyDamageは必ず
	// 自分の被弾コライダーが検出したHitEvent経由でのみ呼ばれる想定).
	void ApplyDamage(const HitEvent& Event) noexcept
	{
		m_Health.ApplyDamage(Event.AttackAmount);
		OnDamaged(Event); // 被弾リアクション(HP減算後に呼ぶ. Playerのノックバック等で上書きされる).
	}

#if _DEBUG
	// コライダーをワイヤーフレームで描画する(Debugビルドのみ. 派生クラスのDrawDebugColliders()からも呼べる).
	void DrawColliderDebug(const CapsuleCollider& Collider, const DirectX::XMFLOAT3& Color) const;
#endif

private:
	// 自分の被弾コライダーが検出した衝突を1件ずつHitEventへ変換し、ApplyDamageへ渡す.
	void ProcessHits();

	// 自分の実体(Body)コライダーが検出した重なりを解消する押し出し(めり込み深さの半分を法線逆方向へ移動).
	void ProcessBodyCollisions();

protected:
	HealthSystem m_Health; // HP・ダメージ処理・コールバック.

	CapsuleCollider m_DamageCollider; // 被弾判定(常時CollisionDetectorに登録される).
	CapsuleCollider m_AttackCollider; // 攻撃判定(既定で非アクティブ. 攻撃系Stateが有効/無効を切り替える).
	CapsuleCollider m_BodyCollider;   // 実体判定(押し出し専用. ダメージ判定とは無関係. 常時アクティブ).

	// 相手コライダーごとに最後にダメージを適用した攻撃の有効化ID
	// (同一スイング中は重なっても1回しかヒットさせないための記録).
	std::map<const ColliderBase*, std::uint32_t> m_ProcessedAttackIds;
};
