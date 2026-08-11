#pragma once

#include "00_Game/10_Object/10_MeshObject/00_Character/10_Enemy/State/EnemyStateBase.h"

namespace EnemyState {

	/**********************************************************************************
	* @author    : mattya3713.
	* @date      : 2026/08/11.
	* @brief     : Enemyの死亡ステート. 攻撃/被弾判定を止めてその場に残り続ける
	*            : (消滅・リスポーン・死亡演出は未実装).
	**********************************************************************************/

	class Dead final : public EnemyStateBase
	{
	public:
		explicit Dead(Enemy* pOwner) noexcept;
		~Dead() override = default;

		EnemyState::eID GetStateID() const override { return EnemyState::eID::Dead; }

		void Enter() override;
	};

} // namespace EnemyState
