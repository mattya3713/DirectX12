// Standalone verification for CombatTuning::ClampToValidRange (直接入力検証タスク).
// JSON読込値の不正値・極端値が妥当範囲へクランプされることを検証する.
#include <cassert>
#include <cstdio>
#include <fstream>
#include <filesystem>

#include "00_Game/60_Combat/CombatTuning.h"

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
	// 1. 極端値・負値のクランプ.
	{
		CombatTuningData tuning{};
		tuning.ComboRushDistance  = -100.0f;
		tuning.Attack0Amount      = 99999.0f;
		tuning.HitStopScale       = -1.0f;
		tuning.DodgeDistance      = -5.0f;
		tuning.JumpHeight         = 5000.0f;

		CombatTuning::ClampToValidRange(tuning);

		Check(tuning.ComboRushDistance ==  0.0f,   "clamp min: ComboRushDistance");
		Check(tuning.Attack0Amount     == 200.0f,  "clamp max: Attack0Amount");
		Check(tuning.HitStopScale      ==  0.0f,   "clamp min: HitStopScale");
		Check(tuning.DodgeDistance     ==  1.0f,   "clamp min: DodgeDistance(1.0)");
		Check(tuning.JumpHeight        == 10.0f,   "clamp max: JumpHeight(10.0)");
	}

	// 2. 妥当値は変化しない.
	{
		CombatTuningData tuning{};
		tuning.Attack2Amount = 40.0f;
		tuning.ParrySlowScale = 0.25f;

		CombatTuning::ClampToValidRange(tuning);

		Check(tuning.Attack2Amount == 40.0f, "valid value unchanged: Attack2Amount");
		Check(tuning.ParrySlowScale == 0.25f, "valid value unchanged: ParrySlowScale");
	}

	// 3. 不正JSON(型不一致)読込でクラッシュせずfalse/現行値維持.
	{
		const char* kBadFile = "tuning_test_bad.json";
		std::ofstream out(kBadFile);
		out << "{ \"Attack0Amount\": \"not_a_number\" }";
		out.close();

		CombatTuningData before = CombatTuning::Get();
		const bool loaded = CombatTuning::Load(kBadFile);

		Check(!loaded, "invalid type json -> Load returns false");
		Check(CombatTuning::Get().Attack0Amount == before.Attack0Amount, "current values preserved on failure");

		std::filesystem::remove(kBadFile);
	}

	// 4. 範囲外JSON読込でクランプされる(Load経由).
	{
		const char* kExtremeFile = "tuning_test_extreme.json";
		std::ofstream out(kExtremeFile);
		out << "{ \"Attack1Amount\": -30.0, \"Boss1Windup\": 999.0 }";
		out.close();

		const float prev_attack2 = CombatTuning::Get().Attack2Amount;
		const bool loaded = CombatTuning::Load(kExtremeFile);

		Check(loaded, "extreme range json -> Load succeeds");
		Check(CombatTuning::Get().Attack1Amount == 0.0f, "clamped via load: Attack1Amount(-30->0)");
		Check(CombatTuning::Get().Boss1Windup == 5.0f, "clamped via load: Boss1Windup(999->5)");
		Check(CombatTuning::Get().Attack2Amount == prev_attack2, "unlisted field unchanged");

		CombatTuning::ResetToDefaults();
		std::filesystem::remove(kExtremeFile);
	}

	std::printf("\nResult: PASS=%d FAIL=%d\n", s_PassCount, s_FailCount);
	return (s_FailCount == 0) ? 0 : 1;
}
