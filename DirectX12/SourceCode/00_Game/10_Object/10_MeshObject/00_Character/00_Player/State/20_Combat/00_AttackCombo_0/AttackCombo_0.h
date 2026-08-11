#pragma once

#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/20_Combat/Combat.h"

namespace PlayerState {

	/**********************************************************************************
	* @author    : mattya3713.
	* @date      : 2026/08/11.
	* @brief     : 攻撃コンボ1段目.
	**********************************************************************************/

	class AttackCombo_0 final : public Combat
	{
	public:
		explicit AttackCombo_0(Player* pOwner) noexcept;
		~AttackCombo_0() override = default;

		PlayerState::eID GetStateID() const override { return PlayerState::eID::AttackCombo_0; }
		std::string GetSettingsFileName() const override { return "Data\\Json\\Player\\AttackCombo\\AttackCombo_0.json"; }

		void Enter() override;
		void Update() override;
	};

} // namespace PlayerState
