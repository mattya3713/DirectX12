#pragma once

#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/State/BossStateBase.h"

namespace BossState {

	/**********************************************************************************
	* @author    : mattya3713.
	* @date      : 2026/08/13.
	* @brief     : Bossの死亡ステート. 攻撃/被弾判定を止めてその場に残り続ける
	*            : (消滅・撃破演出・シーンクリア通知は未実装).
	**********************************************************************************/

	class Dead final : public BossStateBase
	{
	public:
		explicit Dead(Boss* pOwner) noexcept;
		~Dead() override = default;

		BossState::eID GetStateID() const override { return BossState::eID::Dead; }

		void Enter() override;
	};

} // namespace BossState
