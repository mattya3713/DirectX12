#pragma once

// 前方宣言(鍵を持てる具体クラス).
class Player;
class Enemy;
class Boss;

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/10.
* @brief     : Characterへの操作を、意味ごとの「鍵」クラス経由に限定するための
*            : Passkey(Attorney-Client)イディオム(Senzanの`PlayerAccessKeys.h`を一般化して移植).
*            :
*            : 単純にfriendするとクラス単位でしか絞れず、相手の全private/protected
*            : メンバへ無制限にアクセスできてしまう。ここでは操作の意味ごとに鍵クラスを
*            : 分け、鍵を要求する公開関数を経由してのみ書き換えを許可する。
*            : 鍵クラス自身はコンストラクタがprivateであり、friend登録されたクラスだけが
*            : 生成できる(呼び出し側の`friend class Player;`等が無いとコンパイルが通らない).
**********************************************************************************/

namespace CharacterAccess {

	// Character::ApplyDamage()を呼んでよいクラス(攻撃を行いうるクラスのみ).
	class DamageKey
	{
		friend class Player;
		friend class Enemy;
		friend class Boss;
		DamageKey() {}
	};

} // namespace CharacterAccess
