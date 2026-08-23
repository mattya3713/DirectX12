#pragma once

#include <cassert>
#include <concepts>
#include <functional>
#include <limits>
#include <memory>
#include <unordered_map>
#include <vector>

/**********************************************************************************
* @author    : Coder 玄武(閃斬 Production Loop).
* @date      : 2026/08/23.
* @brief     : シングルスレッド用の汎用オブジェクトプール.
**********************************************************************************/

template<typename T>
concept Poolable = requires(T t) { t.Reset(); };

template<typename T>
class ObjectPool final
{
public:
	ObjectPool() = default;
	~ObjectPool() = default;

	ObjectPool(const ObjectPool&)            = delete;
	ObjectPool& operator=(const ObjectPool&) = delete;
	ObjectPool(ObjectPool&&)                 = delete;
	ObjectPool& operator=(ObjectPool&&)      = delete;

	// 再利用時の初期化はReset()へ任せる.
	template<typename... Args>
	T* Acquire(Args&&... Args_)
	{
		T* p_object = nullptr;

		if (m_Free.empty())
		{
			m_Owned.push_back(std::make_unique<T>(std::forward<Args>(Args_)...));
			p_object = m_Owned.back().get();
			m_OwnedIndices.emplace(p_object, m_Owned.size() - 1);
			m_ActivePositions.push_back(INACTIVE_POSITION);
			++m_CreatedCount;
		}
		else
		{
			p_object = m_Free.back();
			m_Free.pop_back();
		}

		if constexpr (Poolable<T>)
		{
			p_object->Reset();
		}

		const std::size_t owned_index = m_OwnedIndices.at(p_object);
		m_ActivePositions[owned_index] = m_Active.size();
		m_Active.push_back(p_object);
		return p_object;
	}

	// プールへ返却する.
	void Release(T* pObject)
	{
		if (!pObject) { return; }

		const auto owned_it = m_OwnedIndices.find(pObject);
		if (owned_it == m_OwnedIndices.end())
		{
			assert(false && "ObjectPool: 所有していないポインタがReleaseされました");
			return;
		}

		const std::size_t owned_index = owned_it->second;
		const std::size_t active_position = m_ActivePositions[owned_index];
		if (active_position == INACTIVE_POSITION)
		{
			assert(false && "ObjectPool: 同じポインタが二重にReleaseされました");
			return;
		}

		T* p_moved = m_Active.back();
		m_Active[active_position] = p_moved;
		m_Active.pop_back();
		m_ActivePositions[m_OwnedIndices.at(p_moved)] = active_position;
		m_ActivePositions[owned_index] = INACTIVE_POSITION;
		m_Free.push_back(pObject);
	}

	// プール返却デリータ付きshared_ptrとして取得する(shared_ptrの最後の参照が
	// 切れた時点でプールへ返却される. プールはこのshared_ptrより長生きすること).
	// 新規生成時に必要なコンストラクタ引数を渡せる(再利用時は未使用).
	template<typename... Args>
	std::shared_ptr<T> AcquireShared(Args&&... Args_)
	{
		return std::shared_ptr<T>(Acquire(std::forward<Args>(Args_)...), [this](T* pObject) { Release(pObject); });
	}

	// 使用中のオブジェクトだけを走査する(Update/Draw用).
	// NOTE: 走査中にAcquire/Releaseしないこと(イテレータが無効になる).
	template<typename Func>
	void ForEachActive(Func&& Func_)
	{
		for (T* p_object : m_Active)
		{
			Func_(*p_object);
		}
	}

	// 使用中のオブジェクトだけを走査する(const版).
	template<typename Func>
	void ForEachActive(Func&& Func_) const
	{
		for (const T* p_object : m_Active)
		{
			Func_(*p_object);
		}
	}

	// デバッグ/テスト用の統計.
	size_t GetActiveCount() const noexcept { return m_Active.size(); }
	size_t GetFreeCount() const noexcept { return m_Free.size(); }
	size_t GetCreatedCount() const noexcept { return m_CreatedCount; } // 累積新規生成数(再利用では増えない).
	void ClearStats() noexcept { m_CreatedCount = 0; }

private:
	static constexpr std::size_t INACTIVE_POSITION = (std::numeric_limits<std::size_t>::max)();

	std::vector<std::unique_ptr<T>> m_Owned;  // 全オブジェクトの所有(破棄はここで一括).
	std::vector<T*>                 m_Active; // 使用中.
	std::vector<T*>                 m_Free;   // 再利用待ち.
	std::unordered_map<T*, std::size_t> m_OwnedIndices; // ポインタから固定スロットを引く.
	std::vector<std::size_t> m_ActivePositions; // 固定スロットごとのActive内位置.
	size_t                          m_CreatedCount = 0; // 累積新規生成数.
};
