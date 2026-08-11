#pragma once

#include "00_Game/10_Object/10_MeshObject/00_Character/10_Enemy/State/EnemyStateBase.h"

namespace EnemyState {

	/**********************************************************************************
	* @author    : mattya3713.
	* @date      : 2026/08/11.
	* @brief     : Enemyの追跡ステート. ターゲットへ直進する. AttackRange以内でAttackへ、
	*            : LoseRangeを超えたらIdleへ戻る.
	**********************************************************************************/

	class Chase final : public EnemyStateBase
	{
	public:
		explicit Chase(Enemy* pOwner) noexcept;
		~Chase() override = default;

		EnemyState::eID GetStateID() const override { return EnemyState::eID::Chase; }

		void Update() override;
	};

} // namespace EnemyState
