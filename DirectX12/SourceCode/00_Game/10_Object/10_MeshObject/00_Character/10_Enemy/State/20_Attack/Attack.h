#pragma once

#include "00_Game/10_Object/10_MeshObject/00_Character/10_Enemy/State/EnemyStateBase.h"

namespace EnemyState {

	/**********************************************************************************
	* @author    : mattya3713.
	* @date      : 2026/08/11.
	* @brief     : Enemyの攻撃ステート. 予備動作→攻撃判定有効化→硬直の順に進み、
	*            : 終わったらChase(ターゲットを見失っていればIdle)へ戻る.
	**********************************************************************************/

	class Attack final : public EnemyStateBase
	{
	public:
		explicit Attack(Enemy* pOwner) noexcept;
		~Attack() override = default;

		EnemyState::eID GetStateID() const override { return EnemyState::eID::Attack; }

		void Enter() override;
		void Update() override;
		void Exit() override;

	private:
		float m_ElapsedTime = 0.0f; // Enterからの経過時間(秒).
	};

} // namespace EnemyState
