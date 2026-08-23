#include "SpecialMove.h"

#include <algorithm>

#include "00_Game/00_GameLoop/Time/Time.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/Player.h"

namespace {
	// 【仮実装】各クリップ1.2秒ずつ順に再生し、全期間攻撃判定を有効にする仮値.
	constexpr float CLIP_DURATION      = 1.2f;
	constexpr int   CLIP_COUNT         = 3;
	constexpr float ATTACK_TOTAL_TIME  = CLIP_DURATION * CLIP_COUNT;
}

namespace PlayerState {

	SpecialMove::SpecialMove(Player* pOwner) noexcept
		: PlayerStateBase(pOwner)
	{
	}

	PlayerState::eID SpecialMove::GetStateID() const
	{
		return PlayerState::eID::SpecialMove;
	}

	void SpecialMove::Enter()
	{
		m_ElapsedTime = 0.0f;
		m_ClipPhase   = 0;

		ApplyNamedClip("player_special_move1");

		// 撃破シーケンスの成立判定はMainScene側が「ヒットしたか」を見るため、
		// ここでは攻撃判定を全期間有効にしておく(威力は既存Attack設定を流用しない仮値).
		GetPlayer()->SetAttackColliderActive(true);
	}

	void SpecialMove::Update()
	{
		m_ElapsedTime += GameTime::GetDeltaTime();

		// special_move1→2→3を順に切り替える.
		const int next_phase = std::min(static_cast<int>(m_ElapsedTime / CLIP_DURATION), CLIP_COUNT - 1);
		if (next_phase != m_ClipPhase)
		{
			m_ClipPhase = next_phase;
			switch (m_ClipPhase)
			{
			case 1: ApplyNamedClip("player_special_move2"); break;
			case 2: ApplyNamedClip("player_special_move3"); break;
			default: break;
			}
		}

		if (m_ElapsedTime >= ATTACK_TOTAL_TIME)
		{
			GetPlayer()->ChangeState(PlayerState::eID::Idle);
		}
	}

	void SpecialMove::Exit()
	{
		GetPlayer()->SetAttackColliderActive(false);
	}

} // namespace PlayerState
