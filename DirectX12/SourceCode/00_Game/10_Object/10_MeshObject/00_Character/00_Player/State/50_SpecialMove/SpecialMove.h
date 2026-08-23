#pragma once

#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/PlayerStateBase.h"

namespace PlayerState {

	/**********************************************************************************
	* @author    : Coder 玄武(閃斬 Production Loop).
	* @date      : 2026-08-23.
	* @brief     : 【仮実装】必殺技ステート(special_move1〜3を順に再生するだけの骨格).
	*            : 撃破シーケンス基盤の成立判定(ゲージMAX中にBossへヒット)のための
	*            : 暫定トリガーであり、正式な必殺技システムは別Featureで上書きする.
	**********************************************************************************/

	class SpecialMove final : public PlayerStateBase
	{
	public:
		explicit SpecialMove(Player* pOwner) noexcept;
		~SpecialMove() override = default;

		PlayerState::eID GetStateID() const override;

		void Enter() override;
		void Update() override;
		void Exit() override;

	private:
		float m_ElapsedTime = 0.0f; // Enterからの経過時間(秒).
		int   m_ClipPhase   = 0;    // 再生中のクリップ段階(0〜2).
	};

} // namespace PlayerState
