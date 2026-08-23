#include "PlaytestRecorder.h"

#if _DEBUG

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "ImGui/imgui.h"

#include "00_Game/00_GameLoop/Time/Time.h"
#include "00_Game/30_Camera/00_Base/CameraBase.h"
#include "00_Game/30_Camera/99_Manager/CameraManager.h"
#include "00_Game/50_Input/Input.h"
#include "00_Game/50_Input/VirtualPad.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/Boss.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/Player.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/PlayerStateID.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/State/BossStateID.h"
#include "99_Utility/Debug/Imgui/ImGuiManager.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"

namespace {

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

	// 現在時刻から保存ファイル名を生成する(PlaytestLogs/Playtest_yyyymmdd_hhmmss.csv).
	std::string MakeSavePath()
	{
		const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		std::tm local_time{};
		localtime_s(&local_time, &now);

		std::ostringstream stream;
		stream << "PlaytestLogs/Playtest_"
			<< std::put_time(&local_time, "%Y%m%d_%H%M%S") << ".csv";
		return stream.str();
	}

} // namespace

PlaytestRecorder& PlaytestRecorder::Instance()
{
	static PlaytestRecorder instance;
	return instance;
}

void PlaytestRecorder::Toggle()
{
	if (!m_IsRecording)
	{
		m_IsRecording  = true;
		m_Frame        = 0;
		m_ElapsedTime  = 0.0f;
		m_PrevPlayerHp = -1.0f;
		m_PrevBossHp   = -1.0f;
		m_PrevPlayerState = "?";
		m_PrevBossState   = "?";
		m_CsvLines.clear();
		m_Events.clear();
		m_CsvLines.emplace_back("frame,time,pstate,bstate,php,bhp,combo,ult,timescale,move_x,move_y,attack,dodge,parry,cam_x,cam_y,cam_z");
		m_Events.emplace_back("RECORD START");
	}
	else
	{
		m_IsRecording = false;
		m_Events.emplace_back("RECORD STOP");
		SaveToCsv();
	}
}

void PlaytestRecorder::Tick()
{
	if (!m_IsRecording) { return; }

	Player* p_player = ServiceLocator::Get<Player>();
	Boss* p_boss = ServiceLocator::Get<Boss>();
	VirtualPad* p_pad = ServiceLocator::Get<VirtualPad>();
	CameraManager* p_camera_manager = ServiceLocator::Get<CameraManager>();

	const float delta_time = GameTime::GetDeltaTime();
	m_ElapsedTime += delta_time;
	++m_Frame;

	// --- 各種状態の取得(全て読み取り専用. ゲームロジックには介入しない) ---
	const auto player_state = p_player ? p_player->GetCurrentStateID() : PlayerState::eID::None;
	const auto boss_state   = p_boss ? p_boss->GetCurrentStateID() : BossState::eID::None;
	const float player_hp = p_player ? p_player->GetHealth().GetHP() : -1.0f;
	const float boss_hp   = p_boss ? p_boss->GetHealth().GetHP() : -1.0f;

	DirectX::XMFLOAT2 move{ 0.0f, 0.0f };
	bool attack = false, dodge = false, parry = false;
	if (p_pad)
	{
		move    = p_pad->GetAxisInput(VirtualPad::eGameAxisAction::Move);
		attack  = p_pad->IsActionDown(VirtualPad::eGameAction::Attack);
		dodge   = p_pad->IsActionDown(VirtualPad::eGameAction::Dodge);
		parry   = p_pad->IsActionDown(VirtualPad::eGameAction::Parry);
	}

	DirectX::XMFLOAT3 cam_pos{ 0.0f, 0.0f, 0.0f };
	if (p_camera_manager)
	{
		if (CameraBase* p_active = p_camera_manager->GetActive()) { cam_pos = p_active->GetPosition(); }
	}

	// --- イベント検出(State遷移・HP変化) ---
	const std::string player_state_name = p_player ? ToString(player_state) : "-";
	const std::string boss_state_name   = p_boss ? ToString(boss_state) : "-";

	if (m_PrevPlayerState != "?" && m_PrevPlayerState != player_state_name)
	{
		m_Events.push_back("[" + std::to_string(m_Frame) + "] P " + m_PrevPlayerState + " -> " + player_state_name);
	}
	if (m_PrevBossState != "?" && m_PrevBossState != boss_state_name)
	{
		m_Events.push_back("[" + std::to_string(m_Frame) + "] B " + m_PrevBossState + " -> " + boss_state_name);
	}
	if (m_PrevPlayerHp >= 0.0f && player_hp < m_PrevPlayerHp)
	{
		m_Events.push_back("[" + std::to_string(m_Frame) + "] P HP -" + std::to_string(m_PrevPlayerHp - player_hp));
	}
	if (m_PrevBossHp >= 0.0f && boss_hp < m_PrevBossHp)
	{
		m_Events.push_back("[" + std::to_string(m_Frame) + "] B HP -" + std::to_string(m_PrevBossHp - boss_hp));
	}
	m_PrevPlayerState = player_state_name;
	m_PrevBossState   = boss_state_name;
	m_PrevPlayerHp    = player_hp;
	m_PrevBossHp      = boss_hp;

	// --- CSV行(1フレーム1行) ---
	std::ostringstream row;
	row << m_Frame << ','
		<< std::fixed << std::setprecision(3) << m_ElapsedTime << ','
		<< player_state_name << ',' << boss_state_name << ','
		<< std::setprecision(1) << player_hp << ',' << boss_hp << ','
		<< (p_player ? p_player->GetCombo() : -1) << ','
		<< std::fixed << std::setprecision(2) << (p_player ? p_player->GetCurrentUltValue() : -1.0f) << ','
		<< GameTime::GetTimeScale() << ','
		<< std::setprecision(3) << move.x << ',' << move.y << ','
		<< (attack ? 1 : 0) << ',' << (dodge ? 1 : 0) << ',' << (parry ? 1 : 0) << ','
		<< cam_pos.x << ',' << cam_pos.y << ',' << cam_pos.z;
	m_CsvLines.push_back(row.str());
}

bool PlaytestRecorder::SaveToCsv()
{
	const std::string path = MakeSavePath();

	// 保存先ディレクトリは実行時に生成されるため初回保存前に作る(ofstreamはディレクトリを作らない).
	std::error_code error;
	std::filesystem::create_directories(std::filesystem::path(path).parent_path(), error);
	if (error) { return false; }

	std::ofstream file(path);
	if (!file.is_open())
	{
		m_Events.emplace_back("SAVE FAILED: " + path);
		return false;
	}

	for (const std::string& line : m_CsvLines)
	{
		file << line << '\n';
	}

	m_LastSavePath = path;
	m_Events.push_back("SAVED: " + path);
	return true;
}

void PlaytestRecorder::DrawImGui()
{
	ImGui::SetNextWindowPos(ImVec2(20.0f, 480.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin(IMGUI_JP("Playtest Recorder"))) { ImGui::End(); return; }

	ImGui::TextColored(
		m_IsRecording ? ImVec4(1.0f, 0.3f, 0.3f, 1.0f) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
		m_IsRecording ? "[REC] %d frames (%.1fs)" : "[IDLE] (F9 to record)",
		m_Frame, m_ElapsedTime);

	if (ImGui::Button(m_IsRecording ? IMGUI_JP("停止して保存") : IMGUI_JP("記録開始")))
	{
		Toggle();
	}

	if (!m_LastSavePath.empty())
	{
		ImGui::TextUnformatted(IMGUI_JP("保存先: "));
		ImGui::SameLine();
		ImGui::TextUnformatted(m_LastSavePath.c_str());
	}

	ImGui::Separator();

	// イベント履歴(直近100件. State遷移とHP変化を追える).
	ImGui::BeginChild("events", ImVec2(0.0f, 200.0f), ImGuiWindowFlags_HorizontalScrollbar);
	for (auto it = m_Events.rbegin(); it != m_Events.rend(); ++it)
	{
		ImGui::TextUnformatted(it->c_str());
	}
	ImGui::EndChild();

	ImGui::End();
}

#endif // _DEBUG
