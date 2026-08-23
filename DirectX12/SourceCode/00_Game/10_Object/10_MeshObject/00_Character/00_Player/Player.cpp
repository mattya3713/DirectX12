#include "Player.h"

#include <cmath>

#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/00_Idle/Idle.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/10_Run/Run.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/20_Combat/00_AttackCombo_0/AttackCombo_0.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/20_Combat/10_AttackCombo_1/AttackCombo_1.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/20_Combat/20_AttackCombo_2/AttackCombo_2.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/20_Combat/30_Parry/Parry.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/30_Dodge/00_DodgeExecute/DodgeExecute.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/40_KnockBack/KnockBack.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/50_SpecialMove/SpecialMove.h"
#include "00_Game/40_Collision/CollisionDetector.h"
#include "00_Game/00_GameLoop/Time/Time.h"
#include "00_Game/30_Camera/00_Base/CameraBase.h"
#include "00_Game/30_Camera/99_Manager/CameraManager.h"
#include "99_Utility/ObjectPool/ObjectPool.h"
#include "99_Utility/Debug/Log/DebugLog.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"

namespace {
	// 状態切替が最も頻発するIdle/Runだけプーリングする(ObjectPoolの組込み動作確認兼.
	// 本格導入(全State/パーティクル等)は別タスク). プール返却デリータ付きshared_ptrで
	// StateMachine側の所有権インターフェイスは変更しない.
	// NOTE: Idle/RunはChangeState呼び出し直後にreturnするため、自身がプールへ返却された
	//       直後にメンバへ触れることがない(StateMachineの破棄タイミングと整合).
	ObjectPool<PlayerState::Idle>& GetIdleStatePool()
	{
		static ObjectPool<PlayerState::Idle> pool;
		return pool;
	}

	ObjectPool<PlayerState::Run>& GetRunStatePool()
	{
		static ObjectPool<PlayerState::Run> pool;
		return pool;
	}
}

namespace {
	constexpr float KNOCKBACK_HORIZONTAL_SPEED = 6.0f; // ノックバックの水平初速(KnockBack State側の定数と合わせる).
	constexpr float KNOCKBACK_VERTICAL_SPEED   = 3.0f; // ノックバックの垂直初速.

	// コンボフロー用の仮値(バランスは後で調整する).
	constexpr float kUltGainPerHit     = 5.0f;  // 攻撃1ヒットあたりの必殺ゲージ獲得量.
	constexpr float kShakeBaseIntensity = 0.05f; // 揺れ幅の基本値.
	constexpr float kShakePerCombo      = 0.004f; // コンボ数1あたりの揺れ幅増加.
	constexpr float kShakeMaxIntensity  = 0.15f;  // 揺れ幅の上限(過剰防止).
}

Player::Player()
	: m_StateMachine  { this }
	, m_ParryCollider { &GetTransform() }
{
	m_DamageCollider.SetMyMask(eCollisionGroup::PlayerDamage);
	m_DamageCollider.SetTargetMask(eCollisionGroup::EnemyAttack);

	m_AttackCollider.SetMyMask(eCollisionGroup::PlayerAttack);
	m_AttackCollider.SetTargetMask(eCollisionGroup::EnemyDamage);

	// パリィ判定(m_DamageColliderと同じ位置・大きさだが別物. Parry中のみ有効化する).
	m_ParryCollider.SetMyMask(eCollisionGroup::PlayerParry);
	m_ParryCollider.SetTargetMask(eCollisionGroup::EnemyAttack);
	m_ParryCollider.SetRadius(0.5f);
	m_ParryCollider.SetHeight(2.0f);
	m_ParryCollider.SetPositionOffset({ 0.0f, 1.0f, 0.0f });
	m_ParryCollider.SetActive(false);

	// 実体判定(Boss/Enemy実体との押し出し専用).
	SetBodyCollisionMasks(eCollisionGroup::PlayerBody, eCollisionGroup::EnemyBody);

	if (CollisionDetector* p_detector = ServiceLocator::Get<CollisionDetector>())
	{
		p_detector->RegisterCollider(m_ParryCollider);
	}

	ChangeState(PlayerState::eID::Idle);
}

Player::~Player()
{
	if (CollisionDetector* p_detector = ServiceLocator::Get<CollisionDetector>())
	{
		p_detector->UnregisterCollider(&m_ParryCollider);
	}
}

void Player::Update()
{
	m_StateMachine.Update();
	m_StateMachine.LateUpdate();

	Character::Update();

	if (m_spMesh)
	{
		Transform drawn_transform = GetTransform();
		drawn_transform.Rotation.y += DirectX::XMConvertToRadians(m_ModelFrontOffsetDeg);
		m_spMesh->SetWorldTransform(drawn_transform);
	}

	ProcessAttackHits(); // 攻撃ヒット時のコンボ/ゲージ加算と画面揺れ(コンボフロー用).
}

void Player::Draw()
{

	Character::Draw();
}

#if _DEBUG
void Player::DrawDebugColliders() const
{
	Character::DrawDebugColliders();
	DrawColliderDebug(m_ParryCollider, { 0.1f, 1.0f, 0.2f }); // パリィ判定=緑.
}
#endif

void Player::OnDamaged(const HitEvent& Event)
{
	// 被弾すると勢い(コンボ)はリセットされるが、必殺ゲージは保持する
	// (「今の勢い」と「溜めた資源」を区別する設計).
	ResetCombo(PlayerAccess::ComboEconomyKey{});
	// 吹き飛び方向を求める(接触点から離れる水平方向が最も確実.
	// Normalの向きは衝突判定の引数順に依存するため、フォールバック扱いにする).
	DirectX::XMFLOAT3 direction{ 0.0f, 0.0f, 1.0f };
	{
		const DirectX::XMFLOAT3& my_pos = GetPosition();
		const float dir_x = my_pos.x - Event.ContactPoint.x;
		const float dir_z = my_pos.z - Event.ContactPoint.z;
		const float length_sq = dir_x * dir_x + dir_z * dir_z;

		if (length_sq > 1e-6f) {
			const float inv_length = 1.0f / std::sqrtf(length_sq);
			direction = { dir_x * inv_length, 0.0f, dir_z * inv_length };
		}
		else {
			// 接触点が自分の中心と一致した場合は法線(自分→相手方向)の逆へ吹き飛ぶ.
			const float normal_x = -Event.Normal.x;
			const float normal_z = -Event.Normal.z;
			const float length = std::sqrtf(normal_x * normal_x + normal_z * normal_z);
			if (length > 1e-6f) {
				direction = { normal_x / length, 0.0f, normal_z / length };
			}
		}
	}

	m_KnockBackVelocity = {
		direction.x * KNOCKBACK_HORIZONTAL_SPEED,
		KNOCKBACK_VERTICAL_SPEED,
		direction.z * KNOCKBACK_HORIZONTAL_SPEED };

	ChangeState(PlayerState::eID::KnockBack);
}

void Player::ProcessAttackHits()
{
	// 攻撃コライダーが相手被弾判定と重なった情報(=実際に攻撃が当たった瞬間)を処理する.
	for (const CollisionInfo& info : m_AttackCollider.GetCollisionEvents())
	{
		if (!info.IsHit || info.AttackActivationId == 0) { continue; }

		// 同一スイング中の重なりで複数回加算しないよう、有効化IDで重複を排除する.
		const auto it = m_ProcessedHitAttackIds.find(info.OtherCollider);
		if (it != m_ProcessedHitAttackIds.end() && it->second >= info.AttackActivationId) { continue; }
		m_ProcessedHitAttackIds[info.OtherCollider] = info.AttackActivationId;

		AddCombo(1, PlayerAccess::ComboEconomyKey{});
		AddUltValue(kUltGainPerHit, PlayerAccess::ComboEconomyKey{});

		PlayEffectAtWorldPos("hit", info.ContactPoint); // ヒットパーティクル(コンボフローとは独立の演出).

		if (DebugLog* p_debug_log = ServiceLocator::Get<DebugLog>()) {
			p_debug_log->LogInfo("Attack Hit! Combo=" + std::to_string(GetCombo())
				+ " Ult=" + std::to_string(GetCurrentUltValue()));
		}

		// 画面揺れ(コンボ数に応じて強くなる. 上限付き).
		if (CameraManager* p_camera_manager = ServiceLocator::Get<CameraManager>())
		{
			if (CameraBase* p_camera = p_camera_manager->GetActive())
			{
				const float intensity = std::min(kShakeBaseIntensity + GetCombo() * kShakePerCombo, kShakeMaxIntensity);
				p_camera->Shake(intensity, 0.12f);
			}
		}
	}
}

void Player::ChangeState(PlayerState::eID Id)
{
	switch (Id)
	{
	case PlayerState::eID::Idle:
		m_StateMachine.ChangeState(GetIdleStatePool().AcquireShared(this));
		break;

	case PlayerState::eID::Run:
		m_StateMachine.ChangeState(GetRunStatePool().AcquireShared(this));
		break;

	case PlayerState::eID::AttackCombo_0:
		m_StateMachine.ChangeState(std::make_shared<PlayerState::AttackCombo_0>(this));
		break;

	case PlayerState::eID::AttackCombo_1:
		m_StateMachine.ChangeState(std::make_shared<PlayerState::AttackCombo_1>(this));
		break;

	case PlayerState::eID::AttackCombo_2:
		m_StateMachine.ChangeState(std::make_shared<PlayerState::AttackCombo_2>(this));
		break;

	case PlayerState::eID::Parry:
		m_StateMachine.ChangeState(std::make_shared<PlayerState::Parry>(this));
		break;

	case PlayerState::eID::DodgeExecute:
		m_StateMachine.ChangeState(std::make_shared<PlayerState::DodgeExecute>(this));
		break;

	case PlayerState::eID::KnockBack:
		m_StateMachine.ChangeState(std::make_shared<PlayerState::KnockBack>(this));
		break;

	// 【仮実装】必殺技(骨格のみ. 正式実装は別Feature).
	case PlayerState::eID::SpecialMove:
		m_StateMachine.ChangeState(std::make_shared<PlayerState::SpecialMove>(this));
		break;

	default:
		return;
	}

	m_CurrentStateID = Id;
}
