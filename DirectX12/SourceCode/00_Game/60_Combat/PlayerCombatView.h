#pragma once

#include <DirectXMath.h>

class Player;

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/13.
* @brief     : CombatCoordinatorへPlayerの限定操作だけを公開する非所有View.
*            : Playerへ用途別インターフェースの多重継承を増やさず、操作権限だけを
*            : 限定するため、継承ではなくViewとして分離している.
**********************************************************************************/

class PlayerCombatView final
{
public:
	explicit PlayerCombatView(Player& Player) noexcept;
	~PlayerCombatView() = default;

	// 戦闘演出計算に使う現在位置を取得する.
	DirectX::XMFLOAT3 GetPosition() const noexcept;

	// パリィ成立時の位置・向き補間を開始する.
	void EnterParryReaction(const DirectX::XMFLOAT3& TargetPosition, float TargetYawDeg, float Duration) const noexcept;

private:
	Player* m_pPlayer; // 非所有.
};
