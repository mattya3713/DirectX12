#pragma once

#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/PlayerStateBase.h"

namespace PlayerState {

	/**********************************************************************************
	* @author    : mattya3713.
	* @date      : 2026/08/10.
	* @brief     : プレイヤーの走りステート. カメラ相対の移動入力をワールド移動量へ変換する.
	*            : 移動入力が無くなるとIdleへ遷移する.
	**********************************************************************************/

	class Run final : public PlayerStateBase
	{
	public:
		explicit Run(Player* pOwner) noexcept;
		~Run() override = default;

		PlayerState::eID GetStateID() const override { return PlayerState::eID::Run; }

		void Enter() override;
		void Update() override;
		void LateUpdate() override;

	private:
		// 入力+カメラ向きから移動ベクトルを算出しPlayerへ設定する.
		void CalculateMoveVec();
	};

} // namespace PlayerState
