#pragma once

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/13.
* @brief     : Bossのステートを識別するID.
*            : 実装済みのステートのみを列挙する(未実装のIDを先に生やさない).
**********************************************************************************/

namespace BossState {

	enum class eID
	{
		None = 0, // 未初期化、または無効なステートID.

		Idle,          // 待機(ターゲットがAggroRange内に入るまで).
		Move,          // 接近(ターゲットへ直進. 将来は攻撃選択の起点にもなる想定).
		Attack,        // 攻撃(足止めして攻撃判定を出す).
		Attack2,       // 攻撃その2(boss_attack2. 予備動作が長い大振り高威力. Moveがランダムで選択する).
		ParryReaction, // パリィされた直後のリアクション(Boss::EnterParryReaction経由でのみ入る. 汎用ChangeStateのswitchには無い).
		Dead,          // 死亡(HPが0になった後. その場に残り続ける).

		_Max
	};

} // namespace BossState
