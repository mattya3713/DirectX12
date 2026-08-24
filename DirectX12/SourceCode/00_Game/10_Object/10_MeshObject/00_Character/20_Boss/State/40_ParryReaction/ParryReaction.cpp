#include "ParryReaction.h"

#include <algorithm>
#include <cmath>

#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/Boss.h"
#include "00_Game/00_GameLoop/Time/Time.h"
#include "99_Utility/DirectXMath/DirectXMathExpansion.h"
#include "99_Utility/Math/Easing/Easing.h"

namespace {
	constexpr float PARRY_REACTION_ROTATE_SPEED = 720.0f; // 度/秒.

	// 硬直中ヒットの吹き飛び残速の減衰率(1/秒. 初速10なら合計約1.2単位押し出される).
	constexpr float STAGGER_KNOCK_BACK_DAMPING = 8.0f;
}

namespace BossState {

ParryReaction::ParryReaction(Boss* pOwner, const DirectX::XMFLOAT3& TargetPosition, float TargetYawDeg, float Duration) noexcept
	: BossStateBase(pOwner)
	, m_TargetPosition { TargetPosition }
	, m_TargetYawDeg   { TargetYawDeg }
	, m_Duration       { Duration }
{
}

void ParryReaction::Enter()
{
	ApplyNamedClip("boss_take_damage");
	m_ElapsedTime   = 0.0f;
	m_StartPosition = GetBoss()->GetPosition();

	GetBoss()->SetAttackColliderActive(false); // パリィされた硬直中は攻撃判定を出さない.
}

void ParryReaction::Update()
{
	// 反撃不能は硬直全体の保証事項なので毎フレーム再無効化する(他系統の有効化があっても上書きする).
	GetBoss()->SetAttackColliderActive(false);

	const float delta_time = GameTime::GetDeltaTime();
	m_ElapsedTime += delta_time;

	// 硬直中に攻撃を命中させた分の吹き飛び(Boss::OnDamagedが積んだ要求を消費する).
	const DirectX::XMFLOAT3 knock_back = GetBoss()->ConsumePendingStaggerKnockBack();
	m_KnockBackVelocity.x += knock_back.x;
	m_KnockBackVelocity.z += knock_back.z;

	GetBoss()->RotateToTarget(m_TargetYawDeg, PARRY_REACTION_ROTATE_SPEED);

	const float clamped_time = std::min(m_ElapsedTime, m_Duration);
	DirectX::XMFLOAT3 position = {};
	MyEasing::UpdateEasing(MyEasing::Type::OutCubic, clamped_time, m_Duration, m_StartPosition, m_TargetPosition, position);

	// 吹き飛び残速をEasing位置へ上乗せ(SetPositionは絶対指定のため、ここで足さないと毎フレーム消える).
	position.x += m_KnockBackVelocity.x * delta_time;
	position.z += m_KnockBackVelocity.z * delta_time;
	GetBoss()->SetPosition(position);

	// 残速を指数減衰させる(フレームレート非依存).
	const float damp = std::exp(-STAGGER_KNOCK_BACK_DAMPING * delta_time);
	m_KnockBackVelocity.x *= damp;
	m_KnockBackVelocity.z *= damp;

	if (m_ElapsedTime >= m_Duration)
	{
		const bool target_in_range = DistanceSqToTargetXZ() <= GetBoss()->GetLoseRange() * GetBoss()->GetLoseRange();
		GetBoss()->ChangeState(target_in_range ? BossState::eID::Move : BossState::eID::Idle);
	}
}

} // namespace BossState
