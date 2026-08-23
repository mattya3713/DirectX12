#pragma once

#include <cassert>
#include <memory>
#include <vector>

/**********************************************************************************
* @author    : 朱雀 (Suzaku / 閃斬 Production Loop Coder).
* @date      : 2026/08/23.
* @brief     : 所有権付きインスタンスのフリーリスト再利用プール.
*            : ObjectPool(玄武)との違い: 二重返却・所有権外ポインタをassertせず
*            : 「falseを返す」ことで検出する(ゲームループ内で落ちたくない用途向け).
*            : 容量超過(MaxActive)時はAcquireがnullptrを返す.
*            : スレッドセーフではない.
**********************************************************************************/

template<typename T>
class ReusableSlotPool final
{
public:
	explicit ReusableSlotPool(size_t MaxActive = static_cast<size_t>(-1)) noexcept
		: m_MaxActive(MaxActive)
	{
	}

	ReusableSlotPool(const ReusableSlotPool&)            = delete;
	ReusableSlotPool& operator=(const ReusableSlotPool&) = delete;

	// 空きスロットがまだあるか.
	bool HasFreeCapacity() const noexcept { return m_Active.size() < m_MaxActive; }

	// 空きがあれば再利用、無ければCreateNew()で新規生成して登録する.
	// CreateNewはunique_ptr<T>を返す関数オブジェクト(再利用時は呼ばれない).
	// 容量超過・生成失敗はnullptr.
	template<typename F>
	T* Acquire(F&& CreateNew)
	{
		if (!HasFreeCapacity()) { return nullptr; }

		T* p_instance = nullptr;

		if (!m_Free.empty())
		{
			p_instance = m_Free.back();
			m_Free.pop_back();
			++m_ReusedCount;
		}
		else
		{
			std::unique_ptr<T> owned = CreateNew();
			if (!owned) { return nullptr; }
			p_instance = owned.get();
			m_Owned.push_back(std::move(owned));
			++m_CreatedCount;
		}

		m_Active.push_back(p_instance);
		return p_instance;
	}

	// 使用済みインスタンスを返却する(false=二重返却または所有権外ポインタ. 状態は変更しない).
	bool Return(T* pInstance) noexcept
	{
		if (!pInstance) { return false; }

		const auto it = std::find(m_Active.begin(), m_Active.end(), pInstance);
		if (it == m_Active.end()) { return false; }

		m_Active.erase(it);
		m_Free.push_back(pInstance);
		++m_ReturnedCount;
		return true;
	}

	// プールが所有しているか(所有権外ポインタの検出用).
	bool Owns(const T* pInstance) const noexcept
	{
		for (const T* p_active : m_Active)
		{
			if (p_active == pInstance) { return true; }
		}
		for (const T* p_free : m_Free)
		{
			if (p_free == pInstance) { return true; }
		}
		return false;
	}

	size_t ActiveCount() const noexcept { return m_Active.size(); }
	size_t FreeCount() const noexcept { return m_Free.size(); }
	size_t CreatedCount() const noexcept { return m_CreatedCount; }  // 累積新規生成数.
	size_t ReusedCount() const noexcept { return m_ReusedCount; }    // 累積再利用数.
	size_t ReturnedCount() const noexcept { return m_ReturnedCount; } // 累積返却数.

private:
	std::vector<std::unique_ptr<T>> m_Owned;  // 全インスタンスの所有(破棄はここで一括).
	std::vector<T*>                 m_Active; // 使用中.
	std::vector<T*>                 m_Free;   // 再利用待ち.

	size_t m_MaxActive      = 0;
	size_t m_CreatedCount   = 0;
	size_t m_ReusedCount    = 0;
	size_t m_ReturnedCount  = 0;
};
