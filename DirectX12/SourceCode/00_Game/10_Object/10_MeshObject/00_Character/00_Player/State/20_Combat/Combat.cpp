#include "Combat.h"

#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/Player.h"
#include "00_Game/50_Input/VirtualPad.h"
#include "99_Utility/FileManager/FileManager.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"

namespace PlayerState {

Combat::Combat(Player* pOwner) noexcept
	: PlayerStateBase(pOwner)
{
}

void Combat::Enter()
{
	m_CurrentTime     = 0.0f;
	m_IsComboAccepted = false;
	m_ColliderWindows.clear();

	GetPlayer()->SetAttackColliderActive(false);

	LoadSettings();
}

void Combat::Update()
{
	m_CurrentTime = GetPlayer()->GetCurrentAnimationSeconds();
	ProcessColliderWindows();
}

void Combat::Exit()
{
	for (ColliderWindow& window : m_ColliderWindows)
	{
		window.IsAct = false;
		window.IsEnd = false;
	}

	GetPlayer()->SetAttackColliderActive(false);
}

void Combat::LoadSettings()
{
	const std::string file_name = GetSettingsFileName();
	if (file_name.empty()) { return; }

	const nlohmann::json data = FileManager::JsonLoad(file_name);
	if (data.empty()) { return; }

	m_ComboStartTime    = data.value("ComboStartTime", m_ComboStartTime);
	m_MinComboTransTime = data.value("MinComboTransTime", m_MinComboTransTime);
	m_ComboEndTime      = data.value("ComboEndTime", m_ComboEndTime);

	if (data.contains("ColliderWindows"))
	{
		for (const nlohmann::json& window : data["ColliderWindows"])
		{
			AddColliderWindow(window.value("start", 0.0f), window.value("duration", 0.1f));
		}
	}
}

void Combat::AddColliderWindow(float Start, float Duration)
{
	m_ColliderWindows.push_back(ColliderWindow{ Start, Duration, false, false });
}

void Combat::ProcessColliderWindows()
{
	for (ColliderWindow& window : m_ColliderWindows)
	{
		if (window.IsEnd) { continue; }

		if (window.IsAct)
		{
			if (m_CurrentTime >= window.Start + window.Duration)
			{
				GetPlayer()->SetAttackColliderActive(false);
				window.IsAct = false;
				window.IsEnd = true;
			}
		}
		else if (m_CurrentTime >= window.Start)
		{
			GetPlayer()->SetAttackColliderActive(true);
			window.IsAct = true;
		}
	}
}

bool Combat::UpdateComboInput()
{
	VirtualPad* p_pad = ServiceLocator::Get<VirtualPad>();
	if (p_pad && !m_IsComboAccepted
		&& m_CurrentTime >= m_ComboStartTime && m_CurrentTime <= m_ComboEndTime
		&& p_pad->IsActionPress(VirtualPad::eGameAction::Attack))
	{
		m_IsComboAccepted = true;
	}

	return m_IsComboAccepted && m_CurrentTime >= m_MinComboTransTime;
}

} // namespace PlayerState
