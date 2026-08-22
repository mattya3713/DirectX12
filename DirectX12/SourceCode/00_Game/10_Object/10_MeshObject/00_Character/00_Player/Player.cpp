#include "Player.h"

#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/00_Idle/Idle.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/10_Run/Run.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/20_Combat/00_AttackCombo_0/AttackCombo_0.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/20_Combat/10_AttackCombo_1/AttackCombo_1.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/20_Combat/20_AttackCombo_2/AttackCombo_2.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/20_Combat/30_Parry/Parry.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/30_Dodge/00_DodgeExecute/DodgeExecute.h"
#include "00_Game/40_Collision/CollisionDetector.h"
#include "00_Game/00_GameLoop/Time/Time.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"

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

	Character::Update(); // Transform確定後にメッシュへ反映しつつ、被弾判定も処理する.
}

void Player::Draw()
{
	if (m_pMesh)
	{
		Transform rotated_transform = GetTransform();
		rotated_transform.Rotation.y += DirectX::XM_PI; // モデルが180度反転した状態で作られているため、描画時だけ正面を合わせる.
		m_pMesh->SetWorldTransform(rotated_transform);
	}

	Character::Draw();

	if (m_pMesh)
	{
		m_pMesh->SetWorldTransform(GetTransform()); // 当たり判定計算等に影響しないよう、次フレームのUpdateまでに実際のTransformへ戻す.
	}
}

#if _DEBUG
void Player::DrawDebugColliders() const
{
	Character::DrawDebugColliders();
	DrawColliderDebug(m_ParryCollider, { 0.1f, 1.0f, 0.2f }); // パリィ判定=緑.
}
#endif

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

	default:
		return;
	}

	m_CurrentStateID = Id;
}
