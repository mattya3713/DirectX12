// Standalone verification for MyRand (task_random_bug acceptance criteria).
// static distribution固定バグが再発していないか、異なる範囲の連続呼び出しで検証する.
#include <cassert>
#include <cstdio>
#include <string>

#include "99_Utility/Math/Random/Random.h"

namespace {

	int s_PassCount = 0;
	int s_FailCount = 0;

	void Check(bool Condition, const char* Label)
	{
		if (Condition)
		{
			++s_PassCount;
			std::printf("[PASS] %s\n", Label);
		}
		else
		{
			++s_FailCount;
			std::printf("[FAIL] %s\n", Label);
		}
	}

}

int main()
{
	// 1. int版: 範囲内のみ返す(異なる範囲を連続呼び出し).
	{
		bool all_in_range = true;
		for (int i = 0; i < 1000; ++i)
		{
			const int v1 = MyRand::GetRandomPercentage(0, 100);
			if (v1 < 0 || v1 > 100) { all_in_range = false; break; }

			const int v2 = MyRand::GetRandomPercentage(0, 1);
			if (v2 != 0 && v2 != 1) { all_in_range = false; std::printf("  out of range value: %d\n", v2); break; }
		}
		Check(all_in_range, "int: (0,100)->(0,1) 連続呼び出しでも範囲が正しい");
	}

	// 2. float版: 範囲内のみ返す.
	{
		bool all_in_range = true;
		for (int i = 0; i < 1000; ++i)
		{
			const float v = MyRand::GetRandomPercentage(-5.0f, 5.0f);
			if (v < -5.0f || v > 5.0f) { all_in_range = false; break; }
		}
		Check(all_in_range, "float: (-5,5) で範囲外なし");
	}

	// 3. (0,1)の50/50抽選が機能する(旧バグでは初回範囲に固定され偏る).
	{
		int zeros = 0;
		constexpr int kTrials = 2000;
		for (int i = 0; i < kTrials; ++i)
		{
			if (MyRand::GetRandomPercentage(0, 1) == 0) { ++zeros; }
		}
		const double ratio = static_cast<double>(zeros) / kTrials;
		Check(ratio > 0.40 && ratio < 0.60, "(0,1) 50/50抽選の比率が妥当");
		std::printf("  zero ratio: %.3f\n", ratio);
	}

	// 4. 同一Min/Maxの繰り返しで範囲外が出ない(回帰).
	{
		bool ok = true;
		for (int i = 0; i < 500; ++i)
		{
			const int v = MyRand::GetRandomPercentage(7, 13);
			if (v < 7 || v > 13) { ok = false; break; }
		}
		Check(ok, "int: (7,13) 固定範囲で回帰なし");
	}

	// 5. GetRandomValue: ベクタの要素のみ返す.
	{
		const std::vector<int> values = { 10, 20, 30 };
		bool ok = true;
		for (int i = 0; i < 500; ++i)
		{
			const int v = MyRand::GetRandomValue(values);
			if (v != 10 && v != 20 && v != 30) { ok = false; break; }
		}
		Check(ok, "GetRandomValue: ベクタ要素のみ");
	}

	// 6. GetRandomValue: 空ベクタは0を返す(new/deleteクラッシュ・UB防止ガード).
	{
		const std::vector<int> empty;
		Check(MyRand::GetRandomValue(empty) == 0, "GetRandomValue: 空ベクタで0");
	}

	std::printf("\nResult: PASS=%d FAIL=%d\n", s_PassCount, s_FailCount);
	return (s_FailCount == 0) ? 0 : 1;
}
