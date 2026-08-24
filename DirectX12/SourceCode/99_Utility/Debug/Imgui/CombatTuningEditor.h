#pragma once

#include <cmath>
#include <string>
#include <vector>

#include "00_Game/60_Combat/CombatTuning.h"

/**********************************************************************************
* @author    : 朱雀 (Suzaku / 閃斬 Production Loop Coder).
* @date      : 2026/08/23.
* @brief     : Combat調整値(CombatTuningData)を一画面で調整するImGuiツール.
*            : スライダー編集・既定値リセット・プリセットJSONの保存/読込を行う.
*            : _DEBUG限定(チート対策). 値は即時にゲームへ反映される.
*            : スライダーの直接入力で混入した負値/NaN/範囲外の値は検証して
*            : 自動補正し、不正な状態では保存を止める.
**********************************************************************************/

// 検証結果1件分(LevelLintのIssueと同じ考え方).
struct CombatTuningIssue
{
	bool        IsError = false; // true=保存禁止 / false=警告(保存は可).
	std::string Path;            // 対象項目(例: "Boss1Windup" / 攻撃区間名).
	std::string Message;
};

// 検証レポート(問題0件ならIsClean()).
struct CombatTuningValidationReport
{
	std::vector<CombatTuningIssue> Issues;

	bool IsClean() const noexcept { return Issues.empty(); }
	size_t ErrorCount() const noexcept
	{
		size_t count = 0;
		for (const CombatTuningIssue& issue : Issues) { if (issue.IsError) { ++count; } }
		return count;
	}
	size_t WarningCount() const noexcept { return Issues.size() - ErrorCount(); }
};

// 項目1つ分の範囲定義. スライダー表示と検証/補正の両方から参照する単一の情報源.
struct CombatTuningFieldRule
{
	const char*               Label; // スライダー表示名(ImGuiManager経由でUTF-8へ変換される).
	float CombatTuningData::* Field; // 対象メンバ.
	float                     Min;
	float                     Max;
};

// 検証・補正ロジック. UIに依存しないヘッダオンリー実装(スタンドアロンテストからも叩ける).
namespace CombatTuningRules {

	// セクション別の項目テーブル(UIの描画順=テーブル順). 範囲は従来のスライダー値と同一.
	inline const std::vector<CombatTuningFieldRule>& PlayerCombo()
	{
		static const std::vector<CombatTuningFieldRule> fields = {
			{ "突進距離",        &CombatTuningData::ComboRushDistance,  0.0f, 10.0f  },
			{ "速度補正/コンボ", &CombatTuningData::ComboSpeedPerCombo, 0.0f, 0.2f   },
			{ "速度補正上限",    &CombatTuningData::ComboSpeedMaxBonus, 0.0f, 1.0f   },
			{ "1段目 威力",      &CombatTuningData::Attack0Amount,      0.0f, 200.0f },
			{ "2段目 威力",      &CombatTuningData::Attack1Amount,      0.0f, 200.0f },
			{ "3段目 威力",      &CombatTuningData::Attack2Amount,      0.0f, 200.0f },
		};
		return fields;
	}

	inline const std::vector<CombatTuningFieldRule>& ParryDodge()
	{
		static const std::vector<CombatTuningFieldRule> fields = {
			{ "パリィ最大持続(s)",        &CombatTuningData::ParryMaxWaitTime,          0.0f,  5.0f  },
			{ "パリィ時ボス硬直延長(s)",  &CombatTuningData::ParryStaggerExtraDuration, 0.0f,  2.0f  },
			{ "硬直ヒット吹き飛び",       &CombatTuningData::ParryStaggerKnockBackSpeed, 0.0f, 50.0f },
			{ "回避距離",                 &CombatTuningData::DodgeDistance,             1.0f,  50.0f },
			{ "回避時間(s)",              &CombatTuningData::DodgeDuration,             0.05f, 5.0f  },
		};
		return fields;
	}

	inline const std::vector<CombatTuningFieldRule>& HitStopSlow()
	{
		static const std::vector<CombatTuningFieldRule> fields = {
			{ "ヒットストップ倍率",  &CombatTuningData::HitStopScale,      0.0f, 1.0f },
			{ "ヒットストップ長(s)", &CombatTuningData::HitStopDuration,   0.0f, 0.5f },
			{ "パリィスロー倍率",    &CombatTuningData::ParrySlowScale,    0.0f, 1.0f },
			{ "パリィスロー長(s)",   &CombatTuningData::ParrySlowDuration, 0.0f, 2.0f },
			{ "JustDodge距離",       &CombatTuningData::JustDodgeRadius,   0.0f, 5.0f },
		};
		return fields;
	}

	// Boss攻撃共通(予備動作→判定有効→硬直→威力).
	inline const std::vector<CombatTuningFieldRule>& BossAttack1()
	{
		static const std::vector<CombatTuningFieldRule> fields = {
			{ "予備動作(s)", &CombatTuningData::Boss1Windup,   0.0f, 5.0f   },
			{ "判定有効(s)", &CombatTuningData::Boss1Active,   0.0f, 3.0f   },
			{ "硬直(s)",     &CombatTuningData::Boss1Recovery, 0.0f, 5.0f   },
			{ "威力",        &CombatTuningData::Boss1Amount,   0.0f, 200.0f },
		};
		return fields;
	}

	inline const std::vector<CombatTuningFieldRule>& BossAttack2()
	{
		static const std::vector<CombatTuningFieldRule> fields = {
			{ "予備動作(s)", &CombatTuningData::Boss2Windup,   0.0f, 5.0f   },
			{ "判定有効(s)", &CombatTuningData::Boss2Active,   0.0f, 3.0f   },
			{ "硬直(s)",     &CombatTuningData::Boss2Recovery, 0.0f, 5.0f   },
			{ "威力",        &CombatTuningData::Boss2Amount,   0.0f, 200.0f },
		};
		return fields;
	}

	inline const std::vector<CombatTuningFieldRule>& BossBeam()
	{
		static const std::vector<CombatTuningFieldRule> fields = {
			{ "予備動作(s)", &CombatTuningData::BeamWindup,   0.0f, 5.0f   },
			{ "判定有効(s)", &CombatTuningData::BeamActive,   0.0f, 3.0f   },
			{ "硬直(s)",     &CombatTuningData::BeamRecovery, 0.0f, 5.0f   },
			{ "威力",        &CombatTuningData::BeamAmount,   0.0f, 200.0f },
		};
		return fields;
	}

	inline const std::vector<CombatTuningFieldRule>& BossJump()
	{
		static const std::vector<CombatTuningFieldRule> fields = {
			{ "しゃがみ込み(s)",  &CombatTuningData::JumpCrouch,              0.0f, 3.0f   },
			{ "滞空時間(s)",      &CombatTuningData::JumpAirTime,             0.1f, 3.0f   },
			{ "跳躍高度",         &CombatTuningData::JumpHeight,              0.1f, 10.0f  },
			{ "着地前判定(s)",    &CombatTuningData::JumpLandingActiveBefore, 0.0f, 1.0f   },
			{ "着地後判定(s)",    &CombatTuningData::JumpLandingActiveAfter,  0.0f, 1.0f   },
			{ "着地後硬直(s)",    &CombatTuningData::JumpRecovery,            0.0f, 3.0f   },
			{ "威力",             &CombatTuningData::JumpAmount,              0.0f, 200.0f },
		};
		return fields;
	}

	inline const std::vector<CombatTuningFieldRule>& BossSpin()
	{
		static const std::vector<CombatTuningFieldRule> fields = {
			{ "予備動作(s)", &CombatTuningData::SpinWindup,   0.0f, 5.0f   },
			{ "判定有効(s)", &CombatTuningData::SpinActive,   0.0f, 3.0f   },
			{ "硬直(s)",     &CombatTuningData::SpinRecovery, 0.0f, 5.0f   },
			{ "威力",        &CombatTuningData::SpinAmount,   0.0f, 200.0f },
		};
		return fields;
	}

	// 全項目へ順番に処理する(Validate/Sanitizeとテスト用).
	template<typename Fn>
	inline void ForEachRule(Fn Func)
	{
		for (const CombatTuningFieldRule& rule : PlayerCombo()) { Func(rule); }
		for (const CombatTuningFieldRule& rule : ParryDodge())  { Func(rule); }
		for (const CombatTuningFieldRule& rule : HitStopSlow()) { Func(rule); }
		for (const CombatTuningFieldRule& rule : BossAttack1()) { Func(rule); }
		for (const CombatTuningFieldRule& rule : BossAttack2()) { Func(rule); }
		for (const CombatTuningFieldRule& rule : BossBeam())    { Func(rule); }
		for (const CombatTuningFieldRule& rule : BossJump())    { Func(rule); }
		for (const CombatTuningFieldRule& rule : BossSpin())    { Func(rule); }
	}

	// 全項目を検証する(負値/非有限/範囲外/攻撃時間の順序). 毎フレームの入力検査と保存前チェックで使う.
	inline CombatTuningValidationReport Validate(const CombatTuningData& Tuning)
	{
		CombatTuningValidationReport report{};

		ForEachRule([&](const CombatTuningFieldRule& Rule)
		{
			const float value = Tuning.*Rule.Field;

			if (!std::isfinite(value)) {
				report.Issues.push_back({ true, Rule.Label, "非有限値(NaN/Inf)です(既定値へ補正されます)" });
				return;
			}

			if (value < Rule.Min) {
				if (value < 0.0f) {
					report.Issues.push_back({ true, Rule.Label, "負値は保存できません(下限へ補正されます)" });
				}
				else {
					report.Issues.push_back({ true, Rule.Label, "下限を下回っています(下限へ補正されます)" });
				}
			}
			else if (value > Rule.Max) {
				report.Issues.push_back({ true, Rule.Label, "上限を超えています(上限へ補正されます)" });
			}
		});

		// 攻撃時間の順序(予備動作→判定有効→硬直). 区間が負だと時間軸が逆転する.
		struct PhaseRule
		{
			const char*               SectionTitle;
			float CombatTuningData::* Windup;
			float CombatTuningData::* Active;
			float CombatTuningData::* Recovery;
		};
		static const std::vector<PhaseRule> phases = {
			{ "攻撃1",   &CombatTuningData::Boss1Windup, &CombatTuningData::Boss1Active, &CombatTuningData::Boss1Recovery },
			{ "攻撃2",   &CombatTuningData::Boss2Windup, &CombatTuningData::Boss2Active, &CombatTuningData::Boss2Recovery },
			{ "ビーム",  &CombatTuningData::BeamWindup,  &CombatTuningData::BeamActive,  &CombatTuningData::BeamRecovery },
			{ "回転攻撃", &CombatTuningData::SpinWindup,  &CombatTuningData::SpinActive,  &CombatTuningData::SpinRecovery },
		};

		for (const PhaseRule& phase : phases)
		{
			const float windup   = Tuning.*phase.Windup;
			const float active   = Tuning.*phase.Active;
			const float recovery = Tuning.*phase.Recovery;

			// 非有限は項目側の検証で報告済み.
			if (!std::isfinite(windup) || !std::isfinite(active) || !std::isfinite(recovery)) { continue; }

			if (windup < 0.0f || active < 0.0f || recovery < 0.0f) {
				report.Issues.push_back({ true, phase.SectionTitle, "予備動作/判定有効/硬直のいずれかが負で、時間の順序が成立しません" });
			}
			else if (active == 0.0f) {
				report.Issues.push_back({ false, phase.SectionTitle, "判定有効時間が0秒のため、この攻撃は命中しません" });
			}
		}

		// 着地前判定がしゃがみ込み+滞空時間より長いと、攻撃開始前から判定が出続ける.
		const float landing = Tuning.JumpCrouch + Tuning.JumpAirTime;
		if (std::isfinite(landing) && Tuning.JumpLandingActiveBefore > landing) {
			report.Issues.push_back({ true, "着地前判定(s)", "しゃがみ込み+滞空時間を超えており、攻撃開始前から判定が出ます" });
		}

		return report;
	}

	// 範囲外・非有限の値をその場で丸める(非有限は既定値、範囲外は上下限). 返り値は補正した項目数.
	inline size_t Sanitize(CombatTuningData& Tuning)
	{
		static const CombatTuningData defaults{};
		size_t fixed_count = 0;

		ForEachRule([&](const CombatTuningFieldRule& Rule)
		{
			float& value = Tuning.*Rule.Field;

			if (!std::isfinite(value)) { value = defaults.*Rule.Field; ++fixed_count; }
			else if (value < Rule.Min) { value = Rule.Min;             ++fixed_count; }
			else if (value > Rule.Max) { value = Rule.Max;             ++fixed_count; }
		});

		// 攻撃開始前に判定がはみ出さないよう、着地前判定は滞空内へ収める.
		const float landing = Tuning.JumpCrouch + Tuning.JumpAirTime;
		if (std::isfinite(landing) && Tuning.JumpLandingActiveBefore > landing) {
			Tuning.JumpLandingActiveBefore = landing;
			++fixed_count;
		}

		return fixed_count;
	}

} // namespace CombatTuningRules

class CombatTuningEditor final
{
public:
	CombatTuningEditor() = default;
	~CombatTuningEditor() = default;

	// 毎フレーム呼ぶ. 調整UI・リセット・プリセット保存/読込を行う.
	void Draw();

private:
	char m_PresetName[128] = { "tuning" }; // プリセット保存名(Data\Json\Combat配下).
	bool m_WasLoadedFromMissingFile = false; // 初回読込でファイルが無かった場合の表示用.

	// 直接入力による不正値の検出記録(空なら問題なし). リセット/保存/読込の操作で消える.
	std::string m_InvalidInputLog;
	bool m_WasSaveBlocked = false; // 保存直前の再検証で弾いて保存しなかった場合の表示用.
};
