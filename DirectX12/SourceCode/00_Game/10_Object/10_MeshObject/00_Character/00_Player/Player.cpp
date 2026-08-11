#include "Player.h"

#include <cmath>

#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/00_Idle/Idle.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/10_Run/Run.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/20_Combat/00_AttackCombo_0/AttackCombo_0.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/20_Combat/10_AttackCombo_1/AttackCombo_1.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/20_Combat/20_AttackCombo_2/AttackCombo_2.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/20_Combat/30_Parry/Parry.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/30_Dodge/00_DodgeExecute/DodgeExecute.h"
#include "99_System/GameLoop/Time/Time.h"

Player::Player()
	: m_StateMachine { this }
{
	m_DamageCollider.SetMyMask(eCollisionGroup::PlayerDamage);
	m_DamageCollider.SetTargetMask(eCollisionGroup::EnemyAttack);

	m_AttackCollider.SetMyMask(eCollisionGroup::PlayerAttack);
	m_AttackCollider.SetTargetMask(eCollisionGroup::EnemyDamage);

	ChangeState(PlayerState::eID::Idle);
}

void Player::Update()
{
	m_StateMachine.Update();
	m_StateMachine.LateUpdate();

	Character::Update(); // Transform確定後にメッシュへ反映しつつ、被弾判定も処理する.
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

	default:
		return;
	}

	m_CurrentStateID = Id;
}

void Player::RotateToTarget(float TargetAngleDeg, float SpeedDegPerSec) noexcept
{
	const float target_rad  = DirectX::XMConvertToRadians(TargetAngleDeg);
	const float current_rad = m_Transform.Rotation.y;

	// 角度差を[-π, π]へ正規化し、最短経路で回転する.
	float diff_rad = std::fmodf(target_rad - current_rad + DirectX::XM_PI, DirectX::XM_2PI);
	if (diff_rad < 0.0f) { diff_rad += DirectX::XM_2PI; }
	diff_rad -= DirectX::XM_PI;

	const float max_step_rad = DirectX::XMConvertToRadians(SpeedDegPerSec) * GameTime::GetDeltaTime();

	if (std::fabs(diff_rad) <= max_step_rad)
	{
		m_Transform.Rotation.y = target_rad;
	}
	else
	{
		m_Transform.Rotation.y += (diff_rad > 0.0f ? max_step_rad : -max_step_rad);
	}
}
