// CombatTuning入力検証の単体テスト(スタンドアロン. ゲーム本体には含まれない).
// ビルド方法: cl /nologo /EHsc /std:c++20 /utf-8 /W4 /I SourceCode /I Data\Library tests\CombatTuningValidationTest.cpp SourceCode\00_Game\60_Combat\CombatTuning.cpp /Fe:tests\CombatTuningValidationTest.exe
//
// 確認内容:
// 1. 既定値は検証を通る. 項目テーブルが全フィールドを網羅している
// 2. 負値/NaN/Inf/範囲外はErrorとして検出され、Sanitizeで上下限・既定値へ丸められる
// 3. 攻撃時間の順序(予備動作→判定有効→硬直)と着地前判定の関係を検証する
// 4. 正常値は保存/再読込で壊れず、不正値を含むJSONは読込後に補正される

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>

#include "../SourceCode/00_Game/60_Combat/CombatTuning.h"
#include "../SourceCode/99_Utility/Debug/Imgui/CombatTuningEditor.h"

namespace {

	int g_CheckCount = 0;

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

	bool HasIssue(const CombatTuningValidationReport& Report, bool IsError, const std::string& PathPart)
	{
		for (const CombatTuningIssue& issue : Report.Issues)
		{
			if (issue.IsError == IsError && issue.Path.find(PathPart) != std::string::npos) { return true; }
		}
		return false;
	}

	void WriteText(const std::filesystem::path& Path, const char* Text)
	{
		std::ofstream file(Path);
		file << Text;
	}

	std::string ReadText(const std::filesystem::path& Path)
	{
		std::ifstream file(Path);
		return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
	}

} // namespace

int main()
{
	const float nan_value = std::numeric_limits<float>::quiet_NaN();
	const float inf_value = std::numeric_limits<float>::infinity();

	// ===== 1. 既定値と項目テーブルの網羅 =====
	{
		Check(CombatTuningRules::Validate(CombatTuningData{}).IsClean(), "defaults are clean");

		size_t rule_count = 0;
		CombatTuningRules::ForEachRule([&rule_count](const CombatTuningFieldRule&) { ++rule_count; });
		Check(rule_count == 37, "field rules cover all 37 tuning fields");
	}

	// ===== 2. 不正値の検出 =====
	{
		CombatTuningData data{};

		data.ComboRushDistance = -1.0f;
		CombatTuningValidationReport report = CombatTuningRules::Validate(data);
		Check(HasIssue(report, true, "突進距離"), "negative value is reported");
		Check(report.ErrorCount() >= 1, "negative value counts as error");

		data = CombatTuningData{};
		data.HitStopScale = nan_value;
		report = CombatTuningRules::Validate(data);
		Check(HasIssue(report, true, "ヒットストップ倍率"), "NaN is reported");

		data = CombatTuningData{};
		data.Attack0Amount = inf_value;
		report = CombatTuningRules::Validate(data);
		Check(HasIssue(report, true, "1段目 威力"), "Inf is reported");

		data = CombatTuningData{};
		data.Attack1Amount = 1.0e9f;
		report = CombatTuningRules::Validate(data);
		Check(HasIssue(report, true, "2段目 威力"), "extreme value beyond upper bound is reported");

		data = CombatTuningData{};
		data.DodgeDistance = 0.5f; // 下限1.0未満(負ではない).
		report = CombatTuningRules::Validate(data);
		Check(HasIssue(report, true, "回避距離"), "below lower bound is reported");
	}

	// ===== 3. Sanitizeによる補正 =====
	{
		CombatTuningData data{};
		data.ComboRushDistance = -1.0f;
		data.DodgeDistance     = 999.0f;
		data.ParryMaxWaitTime  = nan_value;
		data.Boss2Active       = inf_value;

		const size_t fixed_count = CombatTuningRules::Sanitize(data);
		Check(fixed_count == 4, "sanitize reports fixed count");
		Check(data.ComboRushDistance == 0.0f,  "negative clamped to lower bound");
		Check(data.DodgeDistance == 50.0f,     "above upper bound clamped to upper bound");
		Check(data.ParryMaxWaitTime == 1.5f,   "NaN restored to default");
		Check(data.Boss2Active == 0.25f,       "Inf restored to default");
		Check(CombatTuningRules::Validate(data).IsClean(), "sanitized data is clean");
	}

	// ===== 4. 攻撃時間の順序検証 =====
	{
		CombatTuningData data{};
		data.Boss1Windup = -0.5f;
		CombatTuningValidationReport report = CombatTuningRules::Validate(data);
		Check(HasIssue(report, true, "攻撃1"), "negative windup breaks attack time order");

		data = CombatTuningData{};
		data.SpinRecovery = -0.1f;
		Check(HasIssue(CombatTuningRules::Validate(data), true, "回転攻撃"), "negative recovery breaks attack time order");

		data = CombatTuningData{};
		data.BeamActive = 0.0f;
		report = CombatTuningRules::Validate(data);
		Check(!report.IsClean() && report.ErrorCount() == 0, "zero active window is warning only");
		Check(HasIssue(report, false, "ビーム"), "zero active window warns about no-hit attack");

		data = CombatTuningData{};
		data.JumpCrouch              = 0.4f;
		data.JumpAirTime             = 0.9f;
		data.JumpLandingActiveBefore = 2.0f; // しゃがみ込み+滞空(1.3s)より長い.
		report = CombatTuningRules::Validate(data);
		Check(HasIssue(report, true, "着地前判定(s)"), "landing window longer than jump timeline is reported");

		CombatTuningRules::Sanitize(data);
		Check(data.JumpLandingActiveBefore <= data.JumpCrouch + data.JumpAirTime,
			"sanitize keeps landing window within jump timeline");

		data.JumpCrouch              = nan_value;
		data.JumpAirTime             = 1.0f;
		data.JumpLandingActiveBefore = 0.9f;
		const size_t fixed_count = CombatTuningRules::Sanitize(data);
		Check(fixed_count == 1 && data.JumpCrouch == 0.4f, "non-finite crouch does not corrupt landing relation");
	}

	// ===== 5. 保存/再読込(正常値は壊れない) =====
	{
		const std::filesystem::path roundtrip_json = "CombatTuningTest_roundtrip.json";

		CombatTuning::ResetToDefaults();
		CombatTuning::Get().Attack0Amount     = 33.0f;
		CombatTuning::Get().ParryMaxWaitTime  = 2.25f;
		Check(CombatTuning::Save(roundtrip_json), "save succeeds with valid values");

		// 読込結果が保存時の値へ戻ることを、一度別値で上書きしてから確認する.
		CombatTuning::ResetToDefaults();
		CombatTuning::Get().Attack0Amount = 0.0f;
		Check(CombatTuning::Load(roundtrip_json), "load succeeds");
		Check(CombatTuning::Get().Attack0Amount    == 33.0f, "round-trip restores Attack0Amount");
		Check(CombatTuning::Get().ParryMaxWaitTime == 2.25f, "round-trip restores ParryMaxWaitTime");

		// 既存JSON形式の維持(代表キーが書き出されている).
		const std::string dumped = ReadText(roundtrip_json);
		Check(dumped.find("Boss1Windup")            != std::string::npos, "json keeps Boss1Windup key");
		Check(dumped.find("JumpLandingActiveBefore") != std::string::npos, "json keeps JumpLandingActiveBefore key");
		Check(dumped.find("ParrySlowDuration")      != std::string::npos, "json keeps ParrySlowDuration key");

		std::filesystem::remove(roundtrip_json);
	}

	// ===== 6. 不正値を含むJSONの読込 =====
	{
		const std::filesystem::path dirty_json = "CombatTuningTest_dirty.json";
		WriteText(dirty_json,
			R"({
				"ComboRushDistance": -5.0,
				"DodgeDuration": -1.0,
				"HitStopScale": 1.0e40
			})");

		CombatTuning::ResetToDefaults();
		Check(CombatTuning::Load(dirty_json), "dirty json parses successfully");

		// 読込直後は検証に引っかかり、Sanitizeで範囲内へ丸められる(エディタの毎フレーム検査と同じ流れ).
		Check(!CombatTuningRules::Validate(CombatTuning::Get()).IsClean(), "dirty json detected by validation");
		CombatTuningRules::Sanitize(CombatTuning::Get());
		Check(CombatTuningRules::Validate(CombatTuning::Get()).IsClean(), "sanitized after dirty load");
		Check(CombatTuning::Get().ComboRushDistance == 0.0f,   "dirty negative rush distance clamped");
		Check(CombatTuning::Get().DodgeDuration     == 0.05f,  "dirty dodge duration clamped to lower bound");
		Check(CombatTuning::Get().HitStopScale      == 0.05f,  "overflowed hit stop scale restored to default");

		std::filesystem::remove(dirty_json);
	}

	std::cout << "All " << g_CheckCount << " checks passed.\n";
	return 0;
}
