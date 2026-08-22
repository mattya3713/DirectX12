#pragma once

#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/State/BossStateBase.h"

namespace BossState {

	/**********************************************************************************
	* @author    : mattya3713.
	* @date      : 2026/08/23.
	* @brief     : Bossの2つ目の攻撃ステート(boss_attack2クリップ). 構造はAttackと同じ
	*            : 予備動作→攻撃判定有効化→硬直だが、予備動作が長い代わりに高威力.
	*            : 選択はMove::Update()がランダムに行う(このState自身は選択しない).
	**********************************************************************************/

	class Attack2 final : public BossStateBase
	{
	public:
		explicit Attack2(Boss* pOwner) noexcept;
		~Attack2() override = default;

		BossState::eID GetStateID() const override { return BossState::eID::Attack2; }

		void Enter() override;
		void Update() override;
		void Exit() override;

	private:
		float m_ElapsedTime = 0.0f; // Enterからの経過時間(秒).
	};

} // namespace BossState
