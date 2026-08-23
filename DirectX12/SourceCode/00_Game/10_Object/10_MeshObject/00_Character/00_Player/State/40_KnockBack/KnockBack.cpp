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
	// 被弾クリップを再生する(player.msknに存在するplayer_take_damageを使用.
	// クリップはループ再生のため、着地でIdleへ戻った際はIdle側が現在フレームでポーズ固定する).
	ApplyNamedClip("player_take_damage");

	// 吹き飛び初速はPlayer::OnDamaged()が被弾時に計算して保持しているものを受け取る.
	m_Physics.SetVelocity(GetPlayer()->GetKnockBackVelocity());

	// 吹き飛び方向を向かせる(向き補正はMoveVec基準のため、水平方向をMoveVecへ設定する).
	GetPlayer()->SetMoveVec({ m_Physics.GetVelocity().x, 0.0f, m_Physics.GetVelocity().z }, PlayerAccess::MovementKey{});
}

void KnockBack::LateUpdate()
{
	const float delta_time = GameTime::GetDeltaTime();

	// 吹き飛び方向へ向きを補正する(MoveVec基準のラープ回転を流用する).
	PlayerStateBase::LateUpdate();

	// 重力・水平減衰・積分はPhysicsBodyへ委譲する.
	m_Physics.ApplyGravity(delta_time, GRAVITY);
	m_Physics.ApplyDamping(delta_time, HORIZONTAL_DAMPING_RATE);

	const DirectX::XMFLOAT3 delta = m_Physics.Integrate(delta_time);
	GetPlayer()->AddPosition(delta);

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
