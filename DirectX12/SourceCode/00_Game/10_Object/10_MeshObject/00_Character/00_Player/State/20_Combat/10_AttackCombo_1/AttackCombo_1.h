#pragma once

#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/20_Combat/Combat.h"

namespace PlayerState {

	/**********************************************************************************
	* @author    : mattya3713.
	* @date      : 2026/08/11.
	* @brief     : 攻撃コンボ2段目.
	**********************************************************************************/

	class AttackCombo_1 final : public Combat
	{
	public:
		explicit AttackCombo_1(Player* pOwner) noexcept;
		~AttackCombo_1() override = default;

		PlayerState::eID GetStateID() const override { return PlayerState::eID::AttackCombo_1; }
		std::string GetSettingsFileName() const override { return "Data\\Json\\Player\\AttackCombo\\AttackCombo_1.json"; }

		void Enter() override;
		void Update() override;
	};

} // namespace PlayerState
