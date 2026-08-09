#pragma once

#include "00_Game/05_Object/10_Character/Character.h"

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/10.
* @brief     : プレイヤークラス. まだ骨格のみ(入力・移動・FSM等は未実装).
**********************************************************************************/

class Player final : public Character
{
public:
	Player();
	~Player() override;
};
