#pragma once

/**********************************************************************************
* @author    : Coder 玄武(閃斬 Production Loop).
* @date      : 2026/08/23.
* @brief     : 開発用戦闘状態表示(Combat Debug HUD. _DEBUG限定).
*            : Player/BossのHP・現在State・Playerのコンボ数/必殺ゲージ・
*            : GameTimeのTimeScale・Attack/Damage/Parryコライダーの有効状態を
*            : ImGui一覧表示する。Releaseビルドでは完全に無効(何も残らない).
**********************************************************************************/

class Player;
class Boss;

#if _DEBUG

class CombatDebugHud final
{
public:
	// 毎フレーム呼ぶ(null許容. 無い対象は表示を省略する).
	static void Draw(const Player* pPlayer, const Boss* pBoss);
};

#endif // _DEBUG
