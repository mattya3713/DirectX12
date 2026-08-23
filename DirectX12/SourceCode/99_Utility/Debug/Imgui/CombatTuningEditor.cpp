#include "CombatTuningEditor.h"

#include <cstring>
#include <filesystem>

#include "00_Game/60_Combat/CombatTuning.h"
#include "99_Utility/Debug/Imgui/ImGuiManager.h"

namespace {
	constexpr const char* kPresetDir = "Data\\Json\\Combat";

	// Boss攻撃1種分のタイミング+威力をまとめて編集するUI.
	void DrawBossAttack(const char* Label, float* Windup, float* Active, float* Recovery, float* Amount)
	{
		if (!ImGui::CollapsingHeader(Label)) { return; }
		ImGuiManager::Tweak("予備動作(s)", *Windup, 0.0f, 5.0f);
		ImGuiManager::Tweak("判定有効(s)", *Active, 0.0f, 3.0f);
		ImGuiManager::Tweak("硬直(s)", *Recovery, 0.0f, 5.0f);
		ImGuiManager::Tweak("威力", *Amount, 0.0f, 200.0f);
	}

	// プリセット名が安全か(空文字・パス区切り・".."は任意ディレクトリ読み書きになるため拒否).
	bool IsPresetNameValid(const char* Name) noexcept
	{
		if (Name == nullptr || Name[0] == '\0') { return false; }

		const std::string name(Name);
		if (name.find("..") != std::string::npos) { return false; }
		if (name.find('/') != std::string::npos) { return false; }
		if (name.find('\\') != std::string::npos) { return false; }
		if (name.find(':') != std::string::npos) { return false; }

		return true;
	}
}

void CombatTuningEditor::Draw()
{
	ImGui::SetNextWindowPos(ImVec2(520.0f, 20.0f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(420.0f, 640.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin(IMGUI_JP("Combat Tuning Editor"))) { ImGui::End(); return; }

	// ----- プリセット操作 -----
	if (ImGui::Button(IMGUI_JP("既定値へリセット"))) {
		CombatTuning::ResetToDefaults();
		m_WasLoadedFromMissingFile = false;
	}

	ImGui::SameLine();
	if (ImGui::Button(IMGUI_JP("保存")) && m_PresetName[0] != '\0' && IsPresetNameValid(m_PresetName)) {
		std::string name = m_PresetName;
		if (name.find(".json") == std::string::npos) { name += ".json"; }
		const std::filesystem::path path = std::filesystem::path(kPresetDir) / name;
		std::filesystem::create_directories(path.parent_path());
		CombatTuning::Save(path);
	}

	ImGui::SameLine();
	if (ImGui::Button(IMGUI_JP("読込")) && m_PresetName[0] != '\0' && IsPresetNameValid(m_PresetName)) {
		std::string name = m_PresetName;
		if (name.find(".json") == std::string::npos) { name += ".json"; }
		m_WasLoadedFromMissingFile = !CombatTuning::Load(std::filesystem::path(kPresetDir) / name);
	}
	if (m_WasLoadedFromMissingFile) {
		ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), IMGUI_JP("プリセットが見つかりません"));
	}

	// プリセット名の不正文字チェック(保存/読込ボタン押下時に表示).
	if (m_PresetName[0] != '\0' && !IsPresetNameValid(m_PresetName)) {
		ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), IMGUI_JP("プリセット名に使えない文字があります(\\ / : ..)"));
	}

	ImGui::InputText(IMGUI_JP("プリセット名"), m_PresetName, sizeof(m_PresetName));

	ImGui::Separator();

	// ----- Player攻撃(コンボ) -----
	if (ImGui::CollapsingHeader(IMGUI_JP("Player: コンボ"), ImGuiTreeNodeFlags_DefaultOpen))
	{
		CombatTuningData& t = CombatTuning::Get();
		ImGuiManager::Tweak("突進距離",        t.ComboRushDistance, 0.0f, 10.0f);
		ImGuiManager::Tweak("速度補正/コンボ", t.ComboSpeedPerCombo, 0.0f, 0.2f);
		ImGuiManager::Tweak("速度補正上限",    t.ComboSpeedMaxBonus, 0.0f, 1.0f);
		ImGuiManager::Tweak("1段目 威力",      t.Attack0Amount, 0.0f, 200.0f);
		ImGuiManager::Tweak("2段目 威力",      t.Attack1Amount, 0.0f, 200.0f);
		ImGuiManager::Tweak("3段目 威力",      t.Attack2Amount, 0.0f, 200.0f);
	}

	// ----- Player防御 -----
	if (ImGui::CollapsingHeader(IMGUI_JP("Player: パリィ/回避")))
	{
		CombatTuningData& t = CombatTuning::Get();
		ImGuiManager::Tweak("パリィ最大持続(s)", t.ParryMaxWaitTime, 0.0f, 5.0f);
		ImGuiManager::Tweak("回避距離",          t.DodgeDistance, 1.0f, 50.0f);
		ImGuiManager::Tweak("回避時間(s)",       t.DodgeDuration, 0.05f, 5.0f);
	}

	// ----- 演出 -----
	if (ImGui::CollapsingHeader(IMGUI_JP("ヒットストップ/スロー")))
	{
		CombatTuningData& t = CombatTuning::Get();
		ImGuiManager::Tweak("ヒットストップ倍率", t.HitStopScale, 0.0f, 1.0f);
		ImGuiManager::Tweak("ヒットストップ長(s)", t.HitStopDuration, 0.0f, 0.5f);
		ImGuiManager::Tweak("パリィスロー倍率",   t.ParrySlowScale, 0.0f, 1.0f);
		ImGuiManager::Tweak("パリィスロー長(s)",  t.ParrySlowDuration, 0.0f, 2.0f);
	}

	// ----- Boss攻撃 -----
	DrawBossAttack(IMGUI_JP("Boss: 攻撃1(boss_attack1)"),
		&CombatTuning::Get().Boss1Windup, &CombatTuning::Get().Boss1Active,
		&CombatTuning::Get().Boss1Recovery, &CombatTuning::Get().Boss1Amount);

	DrawBossAttack(IMGUI_JP("Boss: 攻撃2(boss_attack2)"),
		&CombatTuning::Get().Boss2Windup, &CombatTuning::Get().Boss2Active,
		&CombatTuning::Get().Boss2Recovery, &CombatTuning::Get().Boss2Amount);

	DrawBossAttack(IMGUI_JP("Boss: ビーム"),
		&CombatTuning::Get().BeamWindup, &CombatTuning::Get().BeamActive,
		&CombatTuning::Get().BeamRecovery, &CombatTuning::Get().BeamAmount);

	if (ImGui::CollapsingHeader(IMGUI_JP("Boss: 跳躍攻撃")))
	{
		CombatTuningData& t = CombatTuning::Get();
		ImGuiManager::Tweak("しゃがみ込み(s)",  t.JumpCrouch, 0.0f, 3.0f);
		ImGuiManager::Tweak("滞空時間(s)",      t.JumpAirTime, 0.1f, 3.0f);
		ImGuiManager::Tweak("跳躍高度",         t.JumpHeight, 0.1f, 10.0f);
		ImGuiManager::Tweak("着地前判定(s)",    t.JumpLandingActiveBefore, 0.0f, 1.0f);
		ImGuiManager::Tweak("着地後判定(s)",    t.JumpLandingActiveAfter, 0.0f, 1.0f);
		ImGuiManager::Tweak("着地後硬直(s)",    t.JumpRecovery, 0.0f, 3.0f);
		ImGuiManager::Tweak("威力",             t.JumpAmount, 0.0f, 200.0f);
	}

	DrawBossAttack(IMGUI_JP("Boss: 回転攻撃"),
		&CombatTuning::Get().SpinWindup, &CombatTuning::Get().SpinActive,
		&CombatTuning::Get().SpinRecovery, &CombatTuning::Get().SpinAmount);

	ImGui::End();
}
