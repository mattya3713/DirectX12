#pragma once

#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/30_Dodge/Dodge.h"

namespace PlayerState {

	/**********************************************************************************
	* @author    : mattya3713.
	* @date      : 2026/08/11.
	* @brief     : 通常回避. イージングをブレンドした加減速のある移動で一定距離を移動する.
	*            : 無敵中に敵の攻撃判定が至近距離ですれ違ったらJustDodge成立として
	*            : ログに出す(演出・ゲームプレイ的恩恵は別タスク).
	**********************************************************************************/

	class DodgeExecute final : public Dodge
	{
	public:
		explicit DodgeExecute(Player* pOwner) noexcept;
		~DodgeExecute() override = default;

		PlayerState::eID GetStateID() const override { return PlayerState::eID::DodgeExecute; }

		void Enter() override;
		// Attack/Dodge/Parryへの割り込みは行わない(回避中は無敵かつ専用の移動制御を優先する).
		void Update() override {}
		void LateUpdate() override;

	private:
		// 無敵中のJustDodge成立判定(敵攻撃判定との至近距離すれ違いを検出する).
		void CheckJustDodge();

		float m_TraveledDistance = 0.0f; // イージングで進んだ距離(m_Distanceに達したら終了).
		bool  m_IsJustDodgeJudged = false; // この回避中にJustDodge判定済みか(1回の回避で1回だけ判定する).
	};

} // namespace PlayerState
