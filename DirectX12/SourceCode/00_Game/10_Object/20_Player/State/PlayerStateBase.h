#pragma once

#include "99_Utility/StateMachine/StateBase.h"
#include "00_Game/10_Object/20_Player/State/PlayerStateID.h"

class Player;

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/10.
* @brief     : Playerのステートの基底クラス. StateBase<Player>を継承しPlayer独自の
*            : アクセサを追加する.
**********************************************************************************/

class PlayerStateBase : public StateBase<Player>
{
public:
	explicit PlayerStateBase(Player* pOwner) noexcept;
	~PlayerStateBase() override = default;

	// ステートIDの取得.
	virtual PlayerState::eID GetStateID() const = 0;

	// LateUpdateの既定動作(MoveVecの向いている方向へラープ回転する).
	// MoveVecが向きを持たない(≒静止中)ステートでは何もしない.
	void LateUpdate() override;

protected:
	// オーナー(プレイヤー)の取得.
	Player* GetPlayer() const noexcept { return m_pOwner; }

	// AnimationClipTable(AnimationEditorで保存したもの)から名前でクリップを引き、
	// 見つかればPlayerのメッシュへ適用する(未登録なら何もしない).
	void ApplyNamedClip(const char* ClipName) const;
};
