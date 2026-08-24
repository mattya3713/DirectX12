// パリィ成立報酬(Boss硬直延長)用調整値の単体テスト(スタンドアロン実行. ゲーム本体には含まれない).
// ビルド法: tests\run_parry_stagger_reward_test.cmd を参照(cl直叩き).
//
// 確認内容:
// 1. 既定値が仕様ガイド内(硬直延長は+0.3〜0.5秒. 吹き飛び初速は正の値)
// 2. 変更後のResetToDefaultsで既定値へ戻る
// 3. ClampToValidRangeが負値/極端値を丸める(JSON手編集対策)
// 4. to_json→from_jsonの往復で値が保存される(Combat Tuning Editorのプリセット互換)
// 5. 新キーを含まない旧プリセットJSONからの読込では既定値で補完される

#include <cstdlib>
#include <iostream>

#include "00_Game/60_Combat/CombatTuning.h"

namespace {

	int g_CheckCount = 0; // 実施した確認の本数.

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

}

int main()
{
	CombatTuningData& tuning = CombatTuning::Get();

	// 1. 既定値.
	CombatTuning::ResetToDefaults();
	Check(tuning.ParryStaggerExtraDuration >= 0.3f && tuning.ParryStaggerExtraDuration <= 0.5f,
		"default extra duration is within spec guide (+0.3..0.5s)");
	Check(tuning.ParryStaggerKnockBackSpeed > 0.0f, "default knockback speed is positive");

	// 2. リセット.
	tuning.ParryStaggerExtraDuration  = 9.9f;
	tuning.ParryStaggerKnockBackSpeed = 0.0f;
	CombatTuning::ResetToDefaults();
	Check(tuning.ParryStaggerExtraDuration  == 0.4f,  "ResetToDefaults restores extra duration");
	Check(tuning.ParryStaggerKnockBackSpeed == 10.0f, "ResetToDefaults restores knockback speed");

	// 3. クランプ.
	tuning.ParryStaggerExtraDuration  = -1.0f;
	tuning.ParryStaggerKnockBackSpeed = 9999.0f;
	CombatTuning::ClampToValidRange(tuning);
	Check(tuning.ParryStaggerExtraDuration  == 0.0f,  "clamp floors negative extra duration");
	Check(tuning.ParryStaggerKnockBackSpeed == 50.0f, "clamp caps extreme knockback speed");

	// 4. 往復保存.
	tuning.ParryStaggerExtraDuration  = 0.5f;
	tuning.ParryStaggerKnockBackSpeed = 12.5f;
	const nlohmann::json data = tuning;
	CombatTuningData restored{};
	from_json(data, restored);
	Check(restored.ParryStaggerExtraDuration  == 0.5f,  "json roundtrip keeps extra duration");
	Check(restored.ParryStaggerKnockBackSpeed == 12.5f, "json roundtrip keeps knockback speed");
	Check(data.contains("ParryStaggerExtraDuration") && data.contains("ParryStaggerKnockBackSpeed"),
		"serialized json exposes the two new keys by name");

	// 5. 旧プリセット互換.
	const nlohmann::json legacy = { { "ParryMaxWaitTime", 2.0f } };
	CombatTuningData compat{};
	from_json(legacy, compat);
	Check(compat.ParryStaggerExtraDuration  == CombatTuningData{}.ParryStaggerExtraDuration,
		"legacy preset without new key falls back to default extra duration");
	Check(compat.ParryStaggerKnockBackSpeed == CombatTuningData{}.ParryStaggerKnockBackSpeed,
		"legacy preset without new key falls back to default knockback speed");

	std::cout << "All " << g_CheckCount << " checks passed.\n";
	return 0;
}
