#pragma once

#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/20_Combat/Combat.h"

namespace PlayerState {

	/**********************************************************************************
	* @author    : mattya3713.
	* @date      : 2026/08/11.
	* @brief     : パリィ. 一定時間、通常の被弾判定を無効化する(無敵)構え.
	*            : Senzanはパリィ成功/失敗をEnemy側の攻撃コライダーのマスク(Parry_Suc/
	*            : Parry_Fai/Parry_Noc)で判定していたが、Enemy/Bossの攻撃がまだ実装されて
	*            : いないため今回は判定できない. 構え→時間経過でIdleに戻るだけの
	*            : 骨組みとして実装し、成功/失敗判定はEnemy/Bossの攻撃実装時に追加する想定.
	*            : SenzanのParry::Enter()はCombat::Enter()を呼んでおらずm_CurrentTimeが
	*            : リセットされない状態依存のバグに見えたため、こちらでは正しく呼ぶ.
	**********************************************************************************/

	class Parry final : public Combat
	{
	public:
		explicit Parry(Player* pOwner) noexcept;
		~Parry() override = default;

		PlayerState::eID GetStateID() const override { return PlayerState::eID::Parry; }
		// JSON設定は使わない(ColliderWindowsも持たない. SenzanのParryも未使用).

		void Enter() override;
		void Update() override;
		void Exit() override;

	private:
		float m_ElapsedTime = 0.0f; // 構えてからの経過時間.
	};

} // namespace PlayerState
