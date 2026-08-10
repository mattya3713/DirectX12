#pragma once

#include "00_Game/10_Object/20_Player/State/PlayerStateBase.h"

namespace PlayerState {

	/**********************************************************************************
	* @author    : mattya3713.
	* @date      : 2026/08/10.
	* @brief     : プレイヤーの待機ステート. 移動入力が入るとRunへ遷移する.
	**********************************************************************************/

	class Idle final : public PlayerStateBase
	{
	public:
		explicit Idle(Player* pOwner) noexcept;
		~Idle() override = default;

		PlayerState::eID GetStateID() const override { return PlayerState::eID::Idle; }

		void Enter() override;
		void Update() override;
	};

} // namespace PlayerState
