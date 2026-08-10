#pragma once

#include "00_Game/10_Object/10_MeshObject/00_Character/10_Enemy/Enemy.h"

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/10.
* @brief     : ボスキャラクラス. まだ骨格のみ(フェーズ・専用攻撃パターン等は未実装).
**********************************************************************************/

class Boss final : public Enemy
{
public:
	Boss();
	~Boss() override;
};
