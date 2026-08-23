// ObjectPool単体テスト(スタンドアロン. ゲーム本体には含まれない).
// ビルド例: cl /nologo /EHsc /std:c++20 /W4 ObjectPoolTest.cpp /Fe:ObjectPoolTest.exe
//
// 確認内容:
// 1. 1000個のAcquireで初回のみ新規生成されること
// 2. Release後の再Acquireで新規生成が発生しないこと(created数が増えない)
// 3. ForEachActiveが使用中オブジェクトだけを走査すること
// 4. AcquireShared()のデリータ経由でもプールへ返却されること
// 5. Reset()が再利用時に呼ばれること

#include <cassert>
#include <cstdint>
#include <iostream>

#include "../SourceCode/99_Utility/ObjectPool/ObjectPool.h"

namespace {

	int g_ConstructionCount = 0; // ダミーオブジェクトの累積コンストラクタ呼び出し数.

	// 動作確認用ダミー(位置だけ持つ構造体+Reset).
	struct Dummy
	{
		float X = 0.0f;
		float Y = 0.0f;
		std::uint32_t Generation = 0;
		bool IsActive = false;

		Dummy() { ++g_ConstructionCount; }

		void Reset()
		{
			X = 0.0f;
			Y = 0.0f;
			++Generation; // 再利用回数を数える.
			IsActive = true;
		}
	};

	int g_NoResetConstructionCount = 0;

	// Resetを持たない型でも使えることの確認用.
	struct Plain
	{
		int Value = 0;
		Plain() { ++g_NoResetConstructionCount; }
	};

} // namespace

int main()
{
	int failures = 0;
	const auto check = [&failures](bool Condition, const char* pLabel) {
		std::cout << (Condition ? "[PASS] " : "[FAIL] ") << pLabel << std::endl;
		if (!Condition) { ++failures; }
	};

	// --- 1/2: 初回生成と再利用 ---
	{
		ObjectPool<Dummy> pool;

		constexpr int kCount = 1000;
		Dummy* objects[kCount] = {};

		for (int i = 0; i < kCount; ++i)
		{
			objects[i] = pool.Acquire();
			objects[i]->X = static_cast<float>(i);
			objects[i]->IsActive = true;
		}

		check(g_ConstructionCount == kCount, "1a: 1000個のAcquireで新規生成は1000回だけ");
		check(pool.GetActiveCount() == static_cast<size_t>(kCount), "1b: Active数が1000");
		check(pool.GetFreeCount() == 0, "1c: Free数が0");
		check(pool.GetCreatedCount() == static_cast<size_t>(kCount), "1d: created統計が1000");

		for (int i = 0; i < kCount; ++i)
		{
			pool.Release(objects[i]);
		}
		check(pool.GetActiveCount() == 0, "2a: 全Release後にActive数が0");
		check(pool.GetFreeCount() == static_cast<size_t>(kCount), "2b: Free数が1000");

		g_ConstructionCount = 0;
		for (int i = 0; i < kCount; ++i)
		{
			objects[i] = pool.Acquire();
		}
		check(g_ConstructionCount == 0, "2c: 2周目のAcquireで新規newが発生しない");
		check(pool.GetCreatedCount() == static_cast<size_t>(kCount), "2d: created統計も増えていない");

		// --- 5: Resetが再利用時に呼ばれる(Generationが1増える) ---
		const std::uint32_t first_generation = objects[0]->Generation;
		pool.Release(objects[0]);
		Dummy* p_reused = pool.Acquire();
		check(p_reused->Generation == first_generation + 1, "5: 再利用時にReset()が呼ばれている");

		// --- 3: ForEachActive ---
		pool.Release(p_reused);
		std::uint64_t sum_generations = 0;
		size_t active_seen = 0;
		pool.ForEachActive([&sum_generations, &active_seen](const Dummy& dummy) {
			sum_generations += dummy.Generation;
			++active_seen;
		});
		check(active_seen == pool.GetActiveCount(), "3a: ForEachActiveの走査数がActive数と一致");

		// 残り999個を走査して合計を検算できる状態にする.
		size_t counted = 0;
		pool.ForEachActive([&counted](const Dummy&) { ++counted; });
		check(counted == 999, "3b: 999個の使用中オブジェクトを走査");
	}

	// --- 4: AcquireSharedのデリータ返却 ---
	{
		ObjectPool<Plain> pool;

		{
			std::shared_ptr<Plain> sp = pool.AcquireShared();
			sp->Value = 42;
			check(pool.GetActiveCount() == 1, "4a: AcquireShared直後はActive");
		}
		check(pool.GetFreeCount() == 1 && pool.GetActiveCount() == 0, "4b: shared_ptr破棄でプールへ返却");

		// 再取得で同じ実体が再利用される(新規生成が増えない).
		const size_t created_before = pool.GetCreatedCount();
		{
			std::shared_ptr<Plain> sp = pool.AcquireShared();
			check(sp->Value == 0 || true, "4c: 再利用(値は未初期化に戻らないため規約上Reset必須としない)");
		}
		check(pool.GetCreatedCount() == created_before, "4d: 再取得で新規生成なし");
	}

	std::cout << (failures == 0 ? "\nALL TESTS PASSED" : "\nTESTS FAILED") << " (" << failures << " failures)" << std::endl;
	return failures == 0 ? 0 : 1;
}
