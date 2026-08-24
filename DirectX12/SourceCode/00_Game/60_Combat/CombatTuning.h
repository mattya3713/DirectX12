#pragma once

#include <filesystem>

#include "json/json.hpp"

/**********************************************************************************
* @author    : 朱雀 (Suzaku / 閃斬 Production Loop Coder).
* @date      : 2026/08/23.
* @brief     : Combat調整値の一括管理(データテーブル化). 各Stateに散らばっていた
*            : ハードコード定数(攻撃力・判定窓時間・ヒットストップ等)をここに集約し、
*            : Combat Tuning EditorからのJSON保存/読込・リセットに対応する.
*            : 既定値は各Stateの旧ハードコード値と同一のため、読み込まなくても
*            : 従来と同じ挙動になる.
**********************************************************************************/

struct CombatTuningData
{
	// ----- Player攻撃(コンボ) -----
	float ComboRushDistance   = 2.5f;  // 攻撃開始時の突進距離.
	float ComboSpeedPerCombo  = 0.01f; // コンボ数あたりの速度補正.
	float ComboSpeedMaxBonus  = 0.2f;  // 速度補正の上限.
	float Attack0Amount       = 25.0f; // AttackCombo_0 攻撃力.
	float Attack1Amount       = 30.0f; // AttackCombo_1 攻撃力.
	float Attack2Amount       = 40.0f; // AttackCombo_2 攻撃力.

	// ----- Player防御 -----
	float ParryMaxWaitTime = 1.5f;     // パリィ構えの最大持続(秒).
	float DodgeDistance    = 25.0f;    // 回避移動距離.
	float DodgeDuration    = 1.7f;     // 回避移動時間(秒).

	// ----- パリィ成立報酬(Boss硬直延長) -----
	float ParryStaggerExtraDuration  = 0.4f;  // パリィ成立後、Boss硬直をPlayer反応時間からさらに延長する時間(秒).
	float ParryStaggerKnockBackSpeed = 10.0f; // 硬直中のBossへ攻撃を命中させた時の吹き飛び初速.

	// ----- 演出(時間スケール) -----
	float HitStopScale     = 0.05f;    // ヒットストップの時間スケール.
	float HitStopDuration  = 0.08f;    // ヒットストップの長さ(秒).
	float ParrySlowScale   = 0.25f;    // パリィ成立スローモーションのスケール.
	float ParrySlowDuration = 0.4f;    // パリィ成立スローモーションの長さ(秒).

	// ----- Boss攻撃1(boss_attack1) -----
	float Boss1Windup   = 0.6f;
	float Boss1Active   = 0.3f;
	float Boss1Recovery = 0.8f;
	float Boss1Amount   = 25.0f;

	// ----- Boss攻撃2(boss_attack2. 大振り高威力) -----
	float Boss2Windup   = 0.9f;
	float Boss2Active   = 0.25f;
	float Boss2Recovery = 0.6f;
	float Boss2Amount   = 40.0f;

	// ----- Bossビーム(boss_beem1) -----
	float BeamWindup   = 1.1f;
	float BeamActive   = 0.3f;
	float BeamRecovery = 0.7f;
	float BeamAmount   = 15.0f;

	// ----- Boss跳躍攻撃(boss_jump_attack1. 空中構造のため項目が異なる) -----
	float JumpCrouch              = 0.4f;  // しゃがみ込み(予備動作).
	float JumpAirTime             = 0.9f;  // 滞空時間.
	float JumpHeight              = 1.8f;  // 最大跳躍高度.
	float JumpLandingActiveBefore = 0.1f;  // 着地の何秒前から判定を出すか.
	float JumpLandingActiveAfter  = 0.15f; // 着地後に判定が残る時間.
	float JumpRecovery            = 0.6f;  // 着地後の硬直.
	float JumpAmount              = 30.0f;

	// ----- Boss回転攻撃(boss_spin_attack1) -----
	float SpinWindup   = 0.5f;
	float SpinActive   = 0.9f;
	float SpinRecovery = 0.7f;
	float SpinAmount   = 20.0f;
};

// nlohmann相互変換(ADL. プリセットJSONの保存/読込で使用).
void from_json(const nlohmann::json& Data, CombatTuningData& Tuning);
void to_json(nlohmann::json& Data, const CombatTuningData& Tuning);

class CombatTuning final
{
public:
	CombatTuning() = delete;

	// 調整値本体(静的生存. MainSceneや各Stateから直接参照する).
	static CombatTuningData& Get() noexcept;

	// 既定値へ戻す.
	static void ResetToDefaults() noexcept;

	// 各フィールドを妥当範囲へクランプする(JSON読込値の不正値・極端値防止用.
	// 範囲はEditorのスライダー範囲と一致させる).
	static void ClampToValidRange(CombatTuningData& Tuning) noexcept;

	// プリセットとしてJSONへ保存/から読込(既定値との差分ではなく全値を書き出す).
	static bool Save(const std::filesystem::path& Path);
	static bool Load(const std::filesystem::path& Path);
};
