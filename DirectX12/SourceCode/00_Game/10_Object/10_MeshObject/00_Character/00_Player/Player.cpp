#include "Player.h"

#include <cmath>

#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/00_Idle/Idle.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/10_Run/Run.h"
#include "99_System/GameLoop/Time/Time.h"

Player::Player()
	: m_StateMachine { this }
{
	ChangeState(PlayerState::eID::Idle);
}

void Player::Update()
{
	m_StateMachine.Update();
	m_StateMachine.LateUpdate();

	MeshObject::Update(); // Transform確定後にメッシュ(見た目)へ反映する.
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
