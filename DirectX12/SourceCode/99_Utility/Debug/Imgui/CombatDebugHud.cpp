#include "CombatDebugHud.h"

#if _DEBUG

#include "ImGui/imgui.h"

#include "00_Game/00_GameLoop/Time/Time.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/Boss.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/Player.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/PlayerStateID.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/State/BossStateID.h"

namespace {

	// PlayerState::eIDの表示名.
	const char* ToString(PlayerState::eID Id)
	{
		switch (Id)
		{
		case PlayerState::eID::Idle:          return "Idle";
		case PlayerState::eID::Run:           return "Run";
		case PlayerState::eID::AttackCombo_0: return "AttackCombo_0";
		case PlayerState::eID::AttackCombo_1: return "AttackCombo_1";
		case PlayerState::eID::AttackCombo_2: return "AttackCombo_2";
		case PlayerState::eID::Parry:         return "Parry";
		case PlayerState::eID::DodgeExecute:  return "DodgeExecute";
		case PlayerState::eID::KnockBack:     return "KnockBack";
		default:                              return "(Unknown)";
		}
	}

	// BossState::eIDの表示名.
	const char* ToString(BossState::eID Id)
	{
		switch (Id)
		{
		case BossState::eID::Idle:          return "Idle";
		case BossState::eID::Move:          return "Move";
		case BossState::eID::Attack:        return "Attack";
		case BossState::eID::Attack2:       return "Attack2";
		case BossState::eID::BeamAttack:    return "BeamAttack";
		case BossState::eID::JumpAttack:    return "JumpAttack";
		case BossState::eID::SpinAttack:    return "SpinAttack";
		case BossState::eID::ParryReaction: return "ParryReaction";
		case BossState::eID::Dead:          return "Dead";
		default:                            return "(Unknown)";
		}
	}

	// コライダー3種のON/OFFを1行で表示する.
	void DrawColliderRow(bool Attack, bool Damage, bool HasParry, bool Parry)
	{
		ImGui::Text("  Collider: Atk[%s] Dmg[%s]%s",
			Attack ? "ON" : "--",
			Damage ? "ON" : "--",
			HasParry ? (Parry ? " Parry[ON]" : " Parry[--]") : "");
	}

} // namespace

void CombatDebugHud::Draw(const Player* p_player, const Boss* p_boss)
{
	if (!ImGui::Begin("Combat Debug")) { ImGui::End(); return; }

	if (p_player)
	{
		ImGui::TextUnformatted("== Player ==");
		ImGui::Text("  HP      : %.0f / %.0f", p_player->GetHealth().GetHP(), p_player->GetHealth().GetMaxHP());
		ImGui::Text("  State   : %s", ToString(p_player->GetCurrentStateID()));
		ImGui::Text("  Combo   : %d", p_player->GetCombo());
		ImGui::Text("  Ult     : %.1f / %.1f", p_player->GetCurrentUltValue(), p_player->GetMaxUltValue());
		DrawColliderRow(
			p_player->IsAttackColliderActive(),
			p_player->IsDamageColliderActive(),
			true, p_player->IsParryColliderActive());
		ImGui::Separator();
	}

	if (p_boss)
	{
		ImGui::TextUnformatted("== Boss ==");
		ImGui::Text("  HP      : %.0f / %.0f", p_boss->GetHealth().GetHP(), p_boss->GetHealth().GetMaxHP());
		ImGui::Text("  State   : %s", ToString(p_boss->GetCurrentStateID()));
		DrawColliderRow(
			p_boss->IsAttackColliderActive(),
			p_boss->IsDamageColliderActive(),
			false, false);
		ImGui::Separator();
	}

	ImGui::Text("TimeScale: %.2f%s",
		GameTime::GetTimeScale(),
		GameTime::IsPaused() ? " (PAUSED)" : "");

	ImGui::End();
}

#endif // _DEBUG
