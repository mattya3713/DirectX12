#pragma once

#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/State/BossStateBase.h"

namespace BossState {

	/**********************************************************************************
	* @author    : Coder 玄武(閃斬 Production Loop).
	* @date      : 2026/08/23.
	* @brief     : Bossの遠距離牽制攻撃(boss_beem1クリップ). 予備動作を長めに取り
	*            : 「遠くから来る」印象を出す。既存Attack/Attack2より低威力・長射程想定.
	*            : 選択はMove::Update()が行う(中距離でも選ばれうる).
	**********************************************************************************/

	class BeamAttack final : public BossStateBase
	{
	public:
		explicit BeamAttack(Boss* pOwner) noexcept;
		~BeamAttack() override = default;

		BossState::eID GetStateID() const override { return BossState::eID::BeamAttack; }

		void Enter() override;
		void Update() override;
		void Exit() override;

	private:
		float m_ElapsedTime = 0.0f; // Enterからの経過時間(秒).
	};

} // namespace BossState
