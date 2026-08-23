#pragma once

#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/State/BossStateBase.h"

namespace BossState {

	/**********************************************************************************
	* @author    : Coder 玄武(閃斬 Production Loop).
	* @date      : 2026/08/23.
	* @brief     : Bossのジャンプ攻撃(boss_jump_attack1クリップ). 予備動作中に空中へ
	*            : 跳ね上がり(放物線移動)、着地タイミングで攻撃判定を出す.
	*            : 高度は解析式で計算するため必ず地面に正確に着地する.
	**********************************************************************************/

	class JumpAttack final : public BossStateBase
	{
	public:
		explicit JumpAttack(Boss* pOwner) noexcept;
		~JumpAttack() override = default;

		BossState::eID GetStateID() const override { return BossState::eID::JumpAttack; }

		void Enter() override;
		void Update() override;
		void Exit() override;

	private:
		float m_ElapsedTime = 0.0f; // Enterからの経過時間(秒).
		float m_PrevHeight  = 0.0f; // 前フレームの跳躍高度(フレーム間移動量の算出用).
	};

} // namespace BossState
