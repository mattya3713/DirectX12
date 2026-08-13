#pragma once

#include <DirectXMath.h>

class Boss;

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/13.
* @brief     : CombatCoordinatorへBossの限定操作だけを公開する非所有View.
*            : Bossへ用途別インターフェースの多重継承を増やさず、操作権限だけを
*            : 限定するため、継承ではなくViewとして分離している.
**********************************************************************************/

class BossCombatView final
{
public:
	explicit BossCombatView(Boss& Boss) noexcept;
	~BossCombatView() = default;

	// 戦闘演出計算に使う現在位置を取得する.
	DirectX::XMFLOAT3 GetPosition() const noexcept;

	// パリィ成立時の位置・向き補間を開始する.
	void EnterParryReaction(const DirectX::XMFLOAT3& TargetPosition, float TargetYawDeg, float Duration) const;

private:
	Boss* m_pBoss; // 非所有.
};
