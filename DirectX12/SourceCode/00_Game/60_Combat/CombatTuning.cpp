#include "CombatTuning.h"

#include <fstream>

#include "json/json.hpp"

namespace {

	// 既定値(旧ハードコード値と同一. ResetToDefaultsで復元される).
	const CombatTuningData& Defaults() noexcept
	{
		static const CombatTuningData defaults{};
		return defaults;
	}

} // namespace

CombatTuningData& CombatTuning::Get() noexcept
{
	static CombatTuningData tuning{};
	return tuning;
}

void CombatTuning::ResetToDefaults() noexcept
{
	Get() = Defaults();
}

void from_json(const nlohmann::json& Data, CombatTuningData& Tuning)
{
	Tuning.ComboRushDistance  = Data.value("ComboRushDistance",  Defaults().ComboRushDistance);
	Tuning.ComboSpeedPerCombo = Data.value("ComboSpeedPerCombo", Defaults().ComboSpeedPerCombo);
	Tuning.ComboSpeedMaxBonus = Data.value("ComboSpeedMaxBonus", Defaults().ComboSpeedMaxBonus);
	Tuning.Attack0Amount      = Data.value("Attack0Amount",      Defaults().Attack0Amount);
	Tuning.Attack1Amount      = Data.value("Attack1Amount",      Defaults().Attack1Amount);
	Tuning.Attack2Amount      = Data.value("Attack2Amount",      Defaults().Attack2Amount);
	Tuning.ParryMaxWaitTime   = Data.value("ParryMaxWaitTime",   Defaults().ParryMaxWaitTime);
	Tuning.DodgeDistance      = Data.value("DodgeDistance",      Defaults().DodgeDistance);
	Tuning.DodgeDuration      = Data.value("DodgeDuration",      Defaults().DodgeDuration);
	Tuning.HitStopScale       = Data.value("HitStopScale",       Defaults().HitStopScale);
	Tuning.HitStopDuration    = Data.value("HitStopDuration",    Defaults().HitStopDuration);
	Tuning.ParrySlowScale     = Data.value("ParrySlowScale",     Defaults().ParrySlowScale);
	Tuning.ParrySlowDuration  = Data.value("ParrySlowDuration",  Defaults().ParrySlowDuration);
	Tuning.Boss1Windup        = Data.value("Boss1Windup",        Defaults().Boss1Windup);
	Tuning.Boss1Active        = Data.value("Boss1Active",        Defaults().Boss1Active);
	Tuning.Boss1Recovery      = Data.value("Boss1Recovery",      Defaults().Boss1Recovery);
	Tuning.Boss1Amount        = Data.value("Boss1Amount",        Defaults().Boss1Amount);
	Tuning.Boss2Windup        = Data.value("Boss2Windup",        Defaults().Boss2Windup);
	Tuning.Boss2Active        = Data.value("Boss2Active",        Defaults().Boss2Active);
	Tuning.Boss2Recovery      = Data.value("Boss2Recovery",      Defaults().Boss2Recovery);
	Tuning.Boss2Amount        = Data.value("Boss2Amount",        Defaults().Boss2Amount);
	Tuning.BeamWindup         = Data.value("BeamWindup",         Defaults().BeamWindup);
	Tuning.BeamActive         = Data.value("BeamActive",         Defaults().BeamActive);
	Tuning.BeamRecovery       = Data.value("BeamRecovery",       Defaults().BeamRecovery);
	Tuning.BeamAmount         = Data.value("BeamAmount",         Defaults().BeamAmount);
	Tuning.JumpCrouch              = Data.value("JumpCrouch",              Defaults().JumpCrouch);
	Tuning.JumpAirTime             = Data.value("JumpAirTime",             Defaults().JumpAirTime);
	Tuning.JumpHeight              = Data.value("JumpHeight",              Defaults().JumpHeight);
	Tuning.JumpLandingActiveBefore = Data.value("JumpLandingActiveBefore", Defaults().JumpLandingActiveBefore);
	Tuning.JumpLandingActiveAfter  = Data.value("JumpLandingActiveAfter",  Defaults().JumpLandingActiveAfter);
	Tuning.JumpRecovery            = Data.value("JumpRecovery",            Defaults().JumpRecovery);
	Tuning.JumpAmount              = Data.value("JumpAmount",              Defaults().JumpAmount);
	Tuning.SpinWindup         = Data.value("SpinWindup",         Defaults().SpinWindup);
	Tuning.SpinActive         = Data.value("SpinActive",         Defaults().SpinActive);
	Tuning.SpinRecovery       = Data.value("SpinRecovery",       Defaults().SpinRecovery);
	Tuning.SpinAmount         = Data.value("SpinAmount",         Defaults().SpinAmount);
}

void to_json(nlohmann::json& Data, const CombatTuningData& Tuning)
{
	Data["ComboRushDistance"]  = Tuning.ComboRushDistance;
	Data["ComboSpeedPerCombo"] = Tuning.ComboSpeedPerCombo;
	Data["ComboSpeedMaxBonus"] = Tuning.ComboSpeedMaxBonus;
	Data["Attack0Amount"]      = Tuning.Attack0Amount;
	Data["Attack1Amount"]      = Tuning.Attack1Amount;
	Data["Attack2Amount"]      = Tuning.Attack2Amount;
	Data["ParryMaxWaitTime"]   = Tuning.ParryMaxWaitTime;
	Data["DodgeDistance"]      = Tuning.DodgeDistance;
	Data["DodgeDuration"]      = Tuning.DodgeDuration;
	Data["HitStopScale"]       = Tuning.HitStopScale;
	Data["HitStopDuration"]    = Tuning.HitStopDuration;
	Data["ParrySlowScale"]     = Tuning.ParrySlowScale;
	Data["ParrySlowDuration"]  = Tuning.ParrySlowDuration;
	Data["Boss1Windup"]        = Tuning.Boss1Windup;
	Data["Boss1Active"]        = Tuning.Boss1Active;
	Data["Boss1Recovery"]      = Tuning.Boss1Recovery;
	Data["Boss1Amount"]        = Tuning.Boss1Amount;
	Data["Boss2Windup"]        = Tuning.Boss2Windup;
	Data["Boss2Active"]        = Tuning.Boss2Active;
	Data["Boss2Recovery"]      = Tuning.Boss2Recovery;
	Data["Boss2Amount"]        = Tuning.Boss2Amount;
	Data["BeamWindup"]         = Tuning.BeamWindup;
	Data["BeamActive"]         = Tuning.BeamActive;
	Data["BeamRecovery"]       = Tuning.BeamRecovery;
	Data["BeamAmount"]         = Tuning.BeamAmount;
	Data["JumpCrouch"]              = Tuning.JumpCrouch;
	Data["JumpAirTime"]             = Tuning.JumpAirTime;
	Data["JumpHeight"]              = Tuning.JumpHeight;
	Data["JumpLandingActiveBefore"] = Tuning.JumpLandingActiveBefore;
	Data["JumpLandingActiveAfter"]  = Tuning.JumpLandingActiveAfter;
	Data["JumpRecovery"]            = Tuning.JumpRecovery;
	Data["JumpAmount"]              = Tuning.JumpAmount;
	Data["SpinWindup"]         = Tuning.SpinWindup;
	Data["SpinActive"]         = Tuning.SpinActive;
	Data["SpinRecovery"]       = Tuning.SpinRecovery;
	Data["SpinAmount"]         = Tuning.SpinAmount;
}

bool CombatTuning::Save(const std::filesystem::path& Path)
{
	nlohmann::json data = Get();
	std::ofstream file(Path);
	if (!file) { return false; }
	file << data.dump(2);
	return true;
}

bool CombatTuning::Load(const std::filesystem::path& Path)
{
	std::ifstream file(Path);
	if (!file) { return false; }

	try {
		nlohmann::json data{};
		file >> data;
		CombatTuningData loaded{};
		from_json(data, loaded); // 欠損キーは既定値で補完される.
		Get() = loaded;
		return true;
	}
	catch (const nlohmann::json::exception&) {
		return false; // 不正JSON時は現行値を壊さない.
	}
}
