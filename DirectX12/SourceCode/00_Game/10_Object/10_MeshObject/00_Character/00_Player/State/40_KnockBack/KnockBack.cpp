#include "KnockBack.h"

#include <cmath>

#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/Player.h"
#include "00_Game/00_GameLoop/Time/Time.h"

namespace {
	constexpr float KNOCKBACK_HORIZONTAL_SPEED = 6.0f; // 水平初速(単位/秒). 演出バランスは後で調整する.
	constexpr float KNOCKBACK_VERTICAL_SPEED   = 3.0f; // 垂直初速(上方向、単位/秒).
	constexpr float GRAVITY                    = 20.0f; // 重力加速度(単位/秒^2).
	constexpr float HORIZONTAL_DAMPING_RATE    = 1.5f;  // 水平速度の減衰率(指数/秒. フレームレート非依存).
	constexpr float GROUND_Y                   = 0.0f;  // 地面の高さ(着地判定).
}

namespace PlayerState {

KnockBack::KnockBack(Player* pOwner) noexcept
	: PlayerStateBase(pOwner)
{
}

void KnockBack::Enter()
{
	// 吹き飛び初速はPlayer::OnDamaged()が被弾時に計算して保持しているものを受け取る.
	m_Velocity = GetPlayer()->GetKnockBackVelocity();

	// 吹き飛び方向を向かせる(向き補正はMoveVec基準のため、水平方向をMoveVecへ設定する).
	GetPlayer()->SetMoveVec({ m_Velocity.x, 0.0f, m_Velocity.z }, PlayerAccess::MovementKey{});
}

void KnockBack::LateUpdate()
{
	const float delta_time = GameTime::GetDeltaTime();

	// 吹き飛び方向へ向きを補正する(MoveVec基準のラープ回転を流用する).
	PlayerStateBase::LateUpdate();

	// 重力による落下.
	m_Velocity.y -= GRAVITY * delta_time;

	// 水平速度の減衰(指数減衰. フレームレート非依存のためdtベース).
	const float damping = std::exp(-HORIZONTAL_DAMPING_RATE * delta_time);
	m_Velocity.x *= damping;
	m_Velocity.z *= damping;

	GetPlayer()->AddPosition({ m_Velocity.x * delta_time, m_Velocity.y * delta_time, m_Velocity.z * delta_time });

	// 地面に着地したらIdleへ戻る(めり込んだ分は地面へ戻す).
	if (GetPlayer()->GetPosition().y <= GROUND_Y)
	{
		Transform landed_transform = GetPlayer()->GetTransform();
		landed_transform.Position.y = GROUND_Y;
		GetPlayer()->SetTransform(landed_transform);

		GetPlayer()->ChangeState(PlayerState::eID::Idle);
	}
}

} // namespace PlayerState
