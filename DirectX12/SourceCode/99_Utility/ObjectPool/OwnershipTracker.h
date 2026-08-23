#pragma once

#include <cassert>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "00_Game/00_Scene/Level/LevelData.h"

/**********************************************************************************
* @author    : 朱雀 (Suzaku / 閃斬 Production Loop Coder).
* @date      : 2026/08/23.
* @brief     : 生成したインスタンスの所有権と二重返却を検出する軽量トラッカー.
*            : ObjectPool(玄武)が生ポインタの二重返却をassertで検出するのに対し、
*            : こちらは「検出してfalseを返す」用途で使用する(ゲームループ内で
*            : assert落ちさせず呼び出し側へ結果を返したい場合向け).
*            : スレッドセーフではない.
**********************************************************************************/

template<typename T>
class OwnershipTracker final
{
public:
	OwnershipTracker() = default;

	OwnershipTracker(const OwnershipTracker&)            = delete;
	OwnershipTracker& operator=(const OwnershipTracker&) = delete;

	// 所有を登録する(false=既に登録済み=二重登録).
	bool Track(T* pInstance)
	{
		if (!pInstance) { return false; }
		return m_Active.insert(pInstance).second;
	}

	// 所有を解除する(false=所有していない=他プール由来または二重解除).
	bool Untrack(T* pInstance)
	{
		return m_Active.erase(pInstance) > 0;
	}

	// 所有しているか(所有権外ポインタの検出用).
	bool IsTracked(const T* pInstance) const noexcept
	{
		return m_Active.count(const_cast<T*>(pInstance)) > 0;
	}

	size_t ActiveCount() const noexcept { return m_Active.size(); }

private:
	std::unordered_set<T*> m_Active; // 所有中のインスタンス.
};
