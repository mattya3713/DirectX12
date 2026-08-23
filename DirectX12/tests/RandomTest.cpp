// MyRand単体テスト(スタンドアロン. ゲーム本体には含まれない).
// ビルド例: cl /nologo /EHsc /std:c++20 RandomTest.cpp /Fe:RandomTest.exe
//
// 確認内容:
// 1. GetRandomPercentage(int)を範囲を変えて呼び分けても、毎回指定範囲内の値が返ること
//    (distributionをstaticにしていた旧実装では初回のMin/Maxに固定されるバグがあった)
// 2. GetRandomPercentage(float)も同様に範囲を守ること
// 3. Boss::Moveと同じ重み抽選(0-99を5分割)で全パターンが出ること
// 4. GetRandomValue(vector版)が要素のどれかを返し、空ベクタは0を返すこと

#include <iostream>
#include <vector>

#include "../SourceCode/99_Utility/Math/Random/Random.h"

namespace {

	// 指定範囲[Min,Max]の抽選をCount回行い、範囲外が出ないかと両端が出たかを見る.
	void ExerciseIntRange(int Min, int Max, int Count, bool* pInRange, bool* pHitMin, bool* pHitMax)
	{
		*pInRange = true;
		*pHitMin = false;
		*pHitMax = false;

		for (int i = 0; i < Count; ++i)
		{
			const int value = MyRand::GetRandomPercentage(Min, Max);
			if (value < Min || value > Max) { *pInRange = false; }
			if (value == Min) { *pHitMin = true; }
			if (value == Max) { *pHitMax = true; }
		}
	}

	// float版([Min,Max)の半開区間. ただし生成結果のfloat丸めでMaxちょうどに張り付くことがあるため
	// 範囲判定はMax込みで見る).
	void ExerciseFloatRange(float Min, float Max, int Count, bool* pInRange, bool* pAboveMin)
	{
		*pInRange = true;
		*pAboveMin = false;

		for (int i = 0; i < Count; ++i)
		{
			const float value = MyRand::GetRandomPercentage(Min, Max);
			if (value < Min || value > Max) { *pInRange = false; }
			if (value > Min) { *pAboveMin = true; }
		}
	}

} // namespace

int main()
{
	int failures = 0;
	const auto check = [&failures](bool Condition, const char* pLabel) {
		std::cout << (Condition ? "[PASS] " : "[FAIL] ") << pLabel << std::endl;
		if (!Condition) { ++failures; }
	};

	// --- 1: 範囲を変えながら呼び分けても各呼び出しが指定範囲を守ること ---
	{
		// 先に広い範囲で呼んでおく(旧バグの再現条件. 初回のMin/Maxに固定されるのが問題だった).
		bool inRange = true, hitMin = false, hitMax = false;
		ExerciseIntRange(0, 100, 100, &inRange, &hitMin, &hitMax);
		check(inRange, "1a: ウォームアップ(0,100)が範囲内");

		constexpr int kDraws = 5000;

		// 旧バグの再現条件そのもの. 広い範囲で呼んだ後に(0,1)を呼び分ける.
		ExerciseIntRange(0, 1, kDraws, &inRange, &hitMin, &hitMax);
		check(inRange, "1b: (0,1)の全結果が0または1(旧バグでは範囲外の値も出ていた)");
		check(hitMin && hitMax, "1c: (0,1)で0と1の両方が出る");

		ExerciseIntRange(50, 60, kDraws, &inRange, &hitMin, &hitMax);
		check(inRange && hitMin && hitMax, "1d: (50,60)が範囲内かつ両端を出す");

		ExerciseIntRange(-10, -1, kDraws, &inRange, &hitMin, &hitMax);
		check(inRange && hitMin && hitMax, "1e: (-10,-1)が範囲内かつ両端を出す");

		ExerciseIntRange(1000, 2000, kDraws, &inRange, &hitMin, &hitMax);
		check(inRange && hitMin && hitMax, "1f: (1000,2000)が範囲内かつ両端を出す");
	}

	// --- 2: float版 ---
	{
		bool inRange = true, aboveMin = false;
		ExerciseFloatRange(0.0f, 100.0f, 100, &inRange, &aboveMin);
		check(inRange, "2a: ウォームアップ(0.0,100.0)が範囲内");

		constexpr int kDraws = 5000;

		ExerciseFloatRange(0.0f, 1.0f, kDraws, &inRange, &aboveMin);
		check(inRange, "2b: (0.0,1.0)の全結果が[0,1)相当の範囲内");
		check(aboveMin, "2c: (0.0,1.0)で0ちょうどに固まらない");

		ExerciseFloatRange(-5.5f, 5.5f, kDraws, &inRange, &aboveMin);
		check(inRange && aboveMin, "2d: (-5.5,5.5)が範囲内");

		ExerciseFloatRange(123.4f, 123.5f, kDraws, &inRange, &aboveMin);
		check(inRange && aboveMin, "2e: 狭い区間(123.4,123.5)でも範囲を守る");
	}

	// --- 3: Boss::Moveと同じ重み抽選(Attack40/Attack2 25/Spin20/Jump10/Beam5) ---
	{
		constexpr int kRolls = 100000;
		int counts[5] = {}; // [Attack, Attack2, Spin, Jump, Beam].

		for (int i = 0; i < kRolls; ++i)
		{
			const int roll = MyRand::GetRandomPercentage(0, 99);
			if      (roll < 40) { ++counts[0]; }
			else if (roll < 65) { ++counts[1]; }
			else if (roll < 85) { ++counts[2]; }
			else if (roll < 95) { ++counts[3]; }
			else                { ++counts[4]; }
		}

		check(counts[0] > 0 && counts[1] > 0 && counts[2] > 0 && counts[3] > 0 && counts[4] > 0,
			"3a: 5パターンすべてが発生する");

		// 出現率が重みのおよそ半分〜1.5倍に収まっているか(乱数の偏りがないことの目安).
		const double expected[5] = { 40.0, 25.0, 20.0, 10.0, 5.0 };
		bool proportionsOk = true;
		for (int i = 0; i < 5; ++i)
		{
			const double percent = counts[i] * 100.0 / kRolls;
			std::cout << "       pattern" << i << ": " << percent << "% (expected ~" << expected[i] << "%)" << std::endl;
			if (percent < expected[i] * 0.5 || percent > expected[i] * 1.5) { proportionsOk = false; }
		}
		check(proportionsOk, "3b: 各パターンの出現率が重み±50%以内");

		// 中距離牽制(<20でBeam)の分岐も両側が出ること.
		int under20 = 0;
		constexpr int kTries = 10000;
		for (int i = 0; i < kTries; ++i)
		{
			if (MyRand::GetRandomPercentage(0, 99) < 20) { ++under20; }
		}
		std::cout << "       beam-provoke rate: " << under20 * 100.0 / kTries << "%" << std::endl;
		check(under20 > 0 && under20 < kTries, "3c: <20分岐の両側が発生する");
	}

	// --- 4: GetRandomValue(vector版) ---
	{
		const std::vector<int> values = { 7, 8, 9 };
		bool allValid = true;
		bool seen[3] = {};

		for (int i = 0; i < 3000; ++i)
		{
			const int value = MyRand::GetRandomValue(values);
			if (value != 7 && value != 8 && value != 9) { allValid = false; }
			if (value == 7) { seen[0] = true; }
			if (value == 8) { seen[1] = true; }
			if (value == 9) { seen[2] = true; }
		}
		check(allValid, "4a: GetRandomValueが要素のどれかだけを返す");
		check(seen[0] && seen[1] && seen[2], "4b: 全要素が一度は選ばれる");

		const std::vector<int> empty;
		check(MyRand::GetRandomValue(empty) == 0, "4c: 空ベクタは0を返す");
	}

	std::cout << (failures == 0 ? "\nALL TESTS PASSED" : "\nTESTS FAILED") << " (" << failures << " failures)" << std::endl;
	return failures == 0 ? 0 : 1;
}
