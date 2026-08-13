#pragma once

#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/State/BossStateBase.h"

namespace BossState {

	/**********************************************************************************
	* @author    : mattya3713.
	* @date      : 2026/08/13.
	* @brief     : Bossの接近ステート. ターゲットへ直進する. AttackRange以内でAttackへ、
	*            : LoseRangeを超えたらIdleへ戻る(Enemy::Chaseと同じ役割.
	*            : 将来ここが複数攻撃パターンの選択役も兼ねる想定のためMoveと命名している).
	**********************************************************************************/

	class Move final : public BossStateBase
	{
	public:
		explicit Move(Boss* pOwner) noexcept;
		~Move() override = default;

		BossState::eID GetStateID() const override { return BossState::eID::Move; }

		void Update() override;
	};

} // namespace BossState
