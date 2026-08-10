#pragma once

// 前方宣言(鍵を持てる具体クラス).
namespace PlayerState {
	class Idle;
	class Run;
	class AttackCombo_0;
	class AttackCombo_1;
	class AttackCombo_2;
	class Parry;
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
		MovementKey() {}
	};

	// コンボ数・必殺ゲージを変更してよいクラス(攻撃系Stateのみ).
	class ComboEconomyKey
	{
		friend class PlayerState::AttackCombo_0;
		friend class PlayerState::AttackCombo_1;
		friend class PlayerState::AttackCombo_2;
		friend class PlayerState::Parry;
		ComboEconomyKey() {}
	};

} // namespace PlayerAccess
