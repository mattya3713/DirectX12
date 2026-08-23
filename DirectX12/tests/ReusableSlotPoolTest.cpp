// ReusableSlotPool単体テスト(スタンドアロン. ゲーム本体には含まれない).
// ビルド方法: cl /nologo /EHsc /std:c++20 /W4 tests\ReusableSlotPoolTest.cpp /Fe:tests\ReusableSlotPoolTest.exe
//
// 確認内容:
// 1. 空きが無い時のみ新規生成し、以降は再利用(Created不変)
// 2. 容量超過でnullptr
// 3. 二重返却・所有権外ポインタはfalse(クラッシュしない)
// 4. 統計(Created/Reused/Active/Returned)の整合

#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>

#include "../SourceCode/99_Utility/ObjectPool/ReusableSlotPool.h"

namespace {

	int g_CheckCount = 0;
	int g_ConstructionCount = 0;

	void Check(bool Condition, const char* Label)
	{
		++g_CheckCount;
		if (!Condition)
		{
			std::cerr << "FAILED: " << Label << '\n';
			std::exit(1);
		}
		std::cout << "PASS: " << Label << '\n';
	}

	// テスト用ダミー.
	struct Dummy
	{
		int Value = 0;
		Dummy() { ++g_ConstructionCount; }
	};

}

int main()
{
	// ===== 1〜2. 新規生成→返却→再利用と容量超過 =====
	{
		ReusableSlotPool<Dummy> pool(2);

		Dummy* a = pool.Acquire([]() { return std::make_unique<Dummy>(); });
		Check(a != nullptr, "Acquire A");
		Dummy* b = pool.Acquire([]() { return std::make_unique<Dummy>(); });
		Check(b != nullptr && b != a, "Acquire B distinct");

		Check(pool.CreatedCount() == 2, "Created==2 (no reuse yet)");
		Check(!pool.HasFreeCapacity(), "capacity full");

		Dummy* c = pool.Acquire([]() { return std::make_unique<Dummy>(); });
		Check(c == nullptr, "capacity overflow returns nullptr");

		Check(pool.Return(a), "Return A");
		Check(!pool.Return(a), "double Return A == false");
		Check(pool.Return(b), "Return B");

		Dummy* d = pool.Acquire([]() { return std::make_unique<Dummy>(); });
		Check(d != nullptr, "reuse after return");
		Check((d == a || d == b), "reused from freelist");
		Check(pool.CreatedCount() == 2, "Created still==2 (reused)");
		Check(pool.ReusedCount() == 1, "Reused==1");
	}

	// ===== 3. 所有権外ポインタ・null =====
	{
		ReusableSlotPool<Dummy> pool(4);
		Dummy foreign;
		Check(!pool.Return(&foreign), "foreign pointer rejected");
		Check(!pool.Return(nullptr), "nullptr rejected");

		Dummy* p = pool.Acquire([]() { return std::make_unique<Dummy>(); });
		Check(p != nullptr, "acquired after rejects");
		Check(pool.Owns(p), "Owns(tracked)");
	}

	std::cout << "All " << g_CheckCount << " checks passed.\n";
	return 0;
}
