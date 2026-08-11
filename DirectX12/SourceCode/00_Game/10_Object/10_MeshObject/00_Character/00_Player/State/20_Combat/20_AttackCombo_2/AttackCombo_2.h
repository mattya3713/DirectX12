#pragma once

#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/20_Combat/Combat.h"

namespace PlayerState {

	/**********************************************************************************
	* @author    : mattya3713.
	* @date      : 2026/08/11.
	* @brief     : 攻撃コンボ3段目. コンボ入力が続く限り1段目へループする.
	**********************************************************************************/

	class AttackCombo_2 final : public Combat
	{
	public:
		explicit AttackCombo_2(Player* pOwner) noexcept;
		~AttackCombo_2() override = default;

		PlayerState::eID GetStateID() const override { return PlayerState::eID::AttackCombo_2; }
		std::string GetSettingsFileName() const override { return "Data\\Json\\Player\\AttackCombo\\AttackCombo_2.json"; }

		void Enter() override;
		void Update() override;
	};

} // namespace PlayerState
