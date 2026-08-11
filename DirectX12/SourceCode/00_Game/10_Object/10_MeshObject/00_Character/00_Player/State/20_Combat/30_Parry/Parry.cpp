#include "Parry.h"

#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/Player.h"
#include "99_System/GameLoop/Time/Time.h"

namespace {
	constexpr float PARRY_MAX_WAIT_TIME = 1.5f; // 構え続けられる最大時間(秒).
}

namespace PlayerState {

Parry::Parry(Player* pOwner) noexcept
	: Combat(pOwner)
{
}

void Parry::Enter()
{
	Combat::Enter();

	m_ElapsedTime = 0.0f;
	GetPlayer()->SetDamageColliderActive(false); // 構え中は通常のダメージを受けない.

	ApplyNamedClip("Parry");
}

void Parry::Update()
{
	Combat::Update();

	m_ElapsedTime += GameTime::GetDeltaTime();
	if (m_ElapsedTime >= PARRY_MAX_WAIT_TIME)
	{
		GetPlayer()->ChangeState(PlayerState::eID::Idle);
	}
}

void Parry::Exit()
{
	GetPlayer()->SetDamageColliderActive(true);

	Combat::Exit();
}

} // namespace PlayerState
