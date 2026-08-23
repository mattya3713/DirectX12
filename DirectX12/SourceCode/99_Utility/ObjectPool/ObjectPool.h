#pragma once

#include <cassert>
#include <concepts>
#include <functional>
#include <memory>
#include <vector>

/**********************************************************************************
* @author    : Coder 玄武(閃斬 Production Loop).
* @date      : 2026/08/23.
* @brief     : 汎用オブジェクトプーリング(ジャンル非依存).
*            : 弾幕・パーティクル・大量スポーン等を毎回new/deleteせず再利用する.
*            : Tは再初期化メソッドReset()を持つことを推奨(持つ場合のみ自動呼び出し.
*            : 持たない型でも使用可能だが、Acquire時に前回の状態が残るため注意).
*            : シングルスレッド専用(スレッドセーフは保証しない).
*            : 所有権はプールがunique_ptrで持ち、利用者には生ポインタまたは
*            : プール返却デリータ付きshared_ptr(AcquireShared())を渡す.
**********************************************************************************/

template<typename T>
concept Poolable = requires(T t) { t.Reset(); };

template<typename T>
class ObjectPool final
{
public:
	ObjectPool() = default;
	~ObjectPool() = default;

	// スマートポインタの使い分け規約に合わせコピー/ムーブ禁止.
	ObjectPool(const ObjectPool&)            = delete;
	ObjectPool& operator=(const ObjectPool&) = delete;
	ObjectPool(ObjectPool&&)                 = delete;
	ObjectPool& operator=(ObjectPool&&)      = delete;

	// 空きが無ければ新規生成し、あれば再利用して返す(Reset()が定義されていれば呼ぶ).
	// Argsは新規生成時のコンストラクタ引数(再利用時には渡されない. 再初期化はReset()の責務).
	template<typename... Args>
	T* Acquire(Args&&... Args_)
	{
		T* p_object = nullptr;

		if (m_Free.empty())
		{
			m_Owned.push_back(std::make_unique<T>(std::forward<Args>(Args_)...));
			p_object = m_Owned.back().get();
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

		m_Active.push_back(p_object);
		return p_object;
	}

	// プールへ返却する(二重返却はアサート).
	void Release(T* pObject)
	{
		if (!pObject) { return; }

		const auto it = std::find(m_Active.begin(), m_Active.end(), pObject);
		if (it == m_Active.end())
		{
			assert(false && "ObjectPool: 所有していないポインタがReleaseされました(二重返却/他プールからの返却)");
			return;
		}

		// 順序維持不要のためswap&popでO(1)削除.
		*it = m_Active.back();
		m_Active.pop_back();
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
	std::vector<std::unique_ptr<T>> m_Owned;  // 全オブジェクトの所有(破棄はここで一括).
	std::vector<T*>                 m_Active; // 使用中.
	std::vector<T*>                 m_Free;   // 再利用待ち.
	size_t                          m_CreatedCount = 0; // 累積新規生成数.
};
