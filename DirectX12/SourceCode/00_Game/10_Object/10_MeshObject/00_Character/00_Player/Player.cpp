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
#include "00_Game/40_Collision/CollisionDetector.h"
#include "00_Game/00_GameLoop/Time/Time.h"
#include "99_Utility/Debug/Log/DebugLog.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"

namespace {
	constexpr float KNOCKBACK_HORIZONTAL_SPEED = 6.0f; // ノックバックの水平初速(KnockBack State側の定数と合わせる).
	constexpr float KNOCKBACK_VERTICAL_SPEED   = 3.0f; // ノックバックの垂直初速.
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

	if (m_pMesh)
	{
		Transform drawn_transform = GetTransform();
		drawn_transform.Rotation.y += DirectX::XMConvertToRadians(m_ModelFrontOffsetDeg);
		m_pMesh->SetWorldTransform(drawn_transform);
	}
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
	// TODO(一時デバッグ): ノックバック実機確認用. 確認後に削除する.
	if (DebugLog* p_log = ServiceLocator::Get<DebugLog>()) {
		p_log->LogInfo("OnDamaged fired: amount=" + std::to_string(Event.AttackAmount));
	}

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

void Player::ChangeState(PlayerState::eID Id)
{
	switch (Id)
	{
	case PlayerState::eID::Idle:
		m_StateMachine.ChangeState(std::make_shared<PlayerState::Idle>(this));
		break;

	case PlayerState::eID::Run:
		m_StateMachine.ChangeState(std::make_shared<PlayerState::Run>(this));
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

	default:
		return;
	}

	m_CurrentStateID = Id;
}
