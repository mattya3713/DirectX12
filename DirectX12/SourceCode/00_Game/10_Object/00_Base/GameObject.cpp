#include "GameObject.h"

#include <cmath>

#include "00_Game/00_GameLoop/Time/Time.h"

GameObject::GameObject()
	: m_Transform {}
{
}

GameObject::~GameObject()
{
}

void GameObject::Update()
{
	// 既定では何もしない. 派生クラスでオーバーライドする.
}

void GameObject::Draw()
{
	// 既定では何もしない. 派生クラスでオーバーライドする.
}

void GameObject::RotateToTarget(float TargetAngleDeg, float SpeedDegPerSec) noexcept
{
	const float target_rad  = DirectX::XMConvertToRadians(TargetAngleDeg);
	const float current_rad = m_Transform.Rotation.y;

	// 角度差を[-π, π]へ正規化し、最短経路で回転する.
	float diff_rad = std::fmodf(target_rad - current_rad + DirectX::XM_PI, DirectX::XM_2PI);
	if (diff_rad < 0.0f) { diff_rad += DirectX::XM_2PI; }
	diff_rad -= DirectX::XM_PI;

	const float max_step_rad = DirectX::XMConvertToRadians(SpeedDegPerSec) * GameTime::GetDeltaTime();

	if (std::fabs(diff_rad) <= max_step_rad) {
		m_Transform.Rotation.y = target_rad;
	}
	else {
		m_Transform.Rotation.y += (diff_rad > 0.0f ? max_step_rad : -max_step_rad);
	}
}
