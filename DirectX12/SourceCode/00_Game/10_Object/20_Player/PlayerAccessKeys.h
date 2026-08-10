#pragma once

// 前方宣言(鍵を持てる具体クラス).
namespace PlayerState {
	class Idle;
	class Run;
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

} // namespace PlayerAccess
