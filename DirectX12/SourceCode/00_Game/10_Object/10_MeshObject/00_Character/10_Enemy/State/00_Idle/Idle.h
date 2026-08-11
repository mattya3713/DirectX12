#pragma once

#include "00_Game/10_Object/10_MeshObject/00_Character/10_Enemy/State/EnemyStateBase.h"

namespace EnemyState {

	/**********************************************************************************
	* @author    : mattya3713.
	* @date      : 2026/08/11.
	* @brief     : Enemyの待機ステート. ターゲットがAggroRange以内に入るとChaseへ遷移する.
	**********************************************************************************/

	class Idle final : public EnemyStateBase
	{
	public:
		explicit Idle(Enemy* pOwner) noexcept;
		~Idle() override = default;

		EnemyState::eID GetStateID() const override { return EnemyState::eID::Idle; }

		void Update() override;
	};

} // namespace EnemyState
