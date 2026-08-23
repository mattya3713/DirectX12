#pragma once

#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/State/BossStateBase.h"

namespace BossState {

	/**********************************************************************************
	* @author    : Coder 玄武(閃斬 Production Loop).
	* @date      : 2026/08/23.
	* @brief     : Bossの回転攻撃(boss_spin_attack1クリップ). その場で振り回すため
	*            : 攻撃判定の有効時間を既存攻撃より長く取っている.
	**********************************************************************************/

	class SpinAttack final : public BossStateBase
	{
	public:
		explicit SpinAttack(Boss* pOwner) noexcept;
		~SpinAttack() override = default;

		BossState::eID GetStateID() const override { return BossState::eID::SpinAttack; }

		void Enter() override;
		void Update() override;
		void Exit() override;

	private:
		float m_ElapsedTime = 0.0f; // Enterからの経過時間(秒).
	};

} // namespace BossState
