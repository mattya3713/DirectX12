#pragma once

#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/State/BossStateBase.h"

namespace BossState {

	/**********************************************************************************
	* @author    : mattya3713.
	* @date      : 2026/08/13.
	* @brief     : Bossの攻撃ステート. 予備動作→攻撃判定有効化→硬直の順に進み、
	*            : 終わったらMove(ターゲットを見失っていればIdle)へ戻る.
	*            : 現時点では単一パターンのみ(専用攻撃パターン群は未実装).
	**********************************************************************************/

	class Attack final : public BossStateBase
	{
	public:
		explicit Attack(Boss* pOwner) noexcept;
		~Attack() override = default;

		BossState::eID GetStateID() const override { return BossState::eID::Attack; }

		void Enter() override;
		void Update() override;
		void Exit() override;

	private:
		float m_ElapsedTime = 0.0f; // Enterからの経過時間(秒).
	};

} // namespace BossState
