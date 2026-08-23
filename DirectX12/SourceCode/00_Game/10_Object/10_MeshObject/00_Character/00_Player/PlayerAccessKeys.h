#pragma once

// 前方宣言(鍵を持てる具体クラス).
namespace PlayerState {
	class Idle;
	class Run;
	class Combat;
	class AttackCombo_0;
	class AttackCombo_1;
	class AttackCombo_2;
	class Parry;
	class KnockBack;
}

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/10.
* @brief     : Playerへの操作を、意味ごとの「鍵」クラス経由に限定する.
*            : CharacterAccessKeys.hと同じ形(鍵クラスのコンストラクタはprivateで、
*            : friend登録されたクラスだけが生成できる).
* @pattern   : Passkey(Attorney-Client).
**********************************************************************************/

namespace PlayerAccess {

	// Player::SetMoveVec()を呼んでよいクラス(移動を制御するStateのみ).
	class MovementKey
	{
		friend class PlayerState::Idle;
		friend class PlayerState::Run;
		friend class PlayerState::KnockBack; // 吹き飛び方向を向かせるため.
		friend class PlayerState::AttackCombo_0; // 突進方向へ向かせるため.
		friend class PlayerState::AttackCombo_1;
		friend class PlayerState::AttackCombo_2;
		friend class PlayerState::Combat; // 共通の突進方向確定処理のため.
		MovementKey() {}
	};

	// コンボ数・必殺ゲージを変更してよいクラス(攻撃系Stateのみ).
	class ComboEconomyKey
	{
		friend class PlayerState::AttackCombo_0;
		friend class PlayerState::AttackCombo_1;
		friend class PlayerState::AttackCombo_2;
		friend class PlayerState::Parry;
		friend class Player; // 攻撃ヒット時の加算・被弾時のリセットを行うため.
		ComboEconomyKey() {}
	};

} // namespace PlayerAccess
