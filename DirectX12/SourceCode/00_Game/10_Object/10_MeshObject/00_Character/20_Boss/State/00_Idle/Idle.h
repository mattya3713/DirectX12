#pragma once

#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/State/BossStateBase.h"

namespace BossState {

	/**********************************************************************************
	* @author    : mattya3713.
	* @date      : 2026/08/13.
	* @brief     : Bossの待機ステート. ターゲットがAggroRange以内に入るとMoveへ遷移する.
	**********************************************************************************/

	class Idle final : public BossStateBase
	{
	public:
		explicit Idle(Boss* pOwner) noexcept;
		~Idle() override = default;

		BossState::eID GetStateID() const override { return BossState::eID::Idle; }

		void Enter() override;
		void Update() override;
	};

} // namespace BossState
