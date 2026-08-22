#pragma once

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/10.
* @brief     : Playerのステートを識別するID.
*            : 実装済みのステートのみを列挙する(未実装のIDを先に生やさない).
**********************************************************************************/

namespace PlayerState {

	enum class eID
	{
		None = 0, // 未初期化、または無効なステートID.

		Idle, // 待機.
		Run,  // 走り.

		AttackCombo_0, // 攻撃1段目.
		AttackCombo_1, // 攻撃2段目.
		AttackCombo_2, // 攻撃3段目(コンボ入力継続で1段目へループ).
		Parry,         // パリィ.

		DodgeExecute, // 回避.

		KnockBack, // 被弾ノックバック(吹き飛び→着地でIdleへ戻る).

		_Max
	};

} // namespace PlayerState
