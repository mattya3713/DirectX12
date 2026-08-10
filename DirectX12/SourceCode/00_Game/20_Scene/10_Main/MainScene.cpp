#include "MainScene.h"

#include <cassert>

#include "10_Ggraphic/DirectX/DirectX12.h"
#include "10_Ggraphic/PMX/PMXActor.h"
#include "10_Ggraphic/PMX/PMXRenderer.h"
#include "00_Game/30_Camera/99_Manager/CameraManager.h"
#include "00_Game/30_Camera/30_Debug/DebugCamera.h"
#include "00_Game/30_Camera/00_Base/CameraBase.h"
#include "00_Game/50_Input/Input.h"
#include "00_Game/10_Object/20_Player/Player.h"
#include "99_Utility/Debug/Imgui/ImGuiManager.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"
#include "99_Utility/String/String.h"
#include "99_System/Scene/SceneManager.h"

namespace {
	// デバッグ表示用にPlayerState::eIDを文字列化する.
	const char* ToDebugString(PlayerState::eID Id)
	{
		switch (Id)
		{
		case PlayerState::eID::Idle: return "Idle";
		case PlayerState::eID::Run:  return "Run";
		default:                     return "None";
		}
	}
}

MainScene::MainScene() = default;

MainScene::~MainScene()
{
}

void MainScene::Initialize()
{
}

void MainScene::Create()
{
	DirectX12* p_dx12 = ServiceLocator::Get<DirectX12>();

	// カメラを登録・有効化.
	if (CameraManager* p_camera_manager = ServiceLocator::Get<CameraManager>()) {
		p_camera_manager->Register("Debug", std::make_unique<DebugCamera>());
		p_camera_manager->SetActive("Debug");
	}

	m_upPlayer = std::make_unique<Player>();

	try {
		m_pPMXRenderer = std::make_shared<PMXRenderer>(*p_dx12);
		m_pPMXActor = std::make_shared<PMXActor>("Data\\Model\\PMX\\Hatune\\REM式プロセカ風初音ミクN25.pmx", *m_pPMXRenderer);
		m_pPMXActor->PlayAnimation();
	}
	catch (const std::runtime_error& Msg) {
		// エラーメッセージを表示(未捕捉のまま伝播させてabortするのを防ぐ).
		std::wstring w_str = MyString::StringToWString(Msg.what());
		_ASSERT_EXPR(false, w_str.c_str());
	}
}

void MainScene::Update()
{
#if _DEBUG
	// F1でアニメーション調整シーンへ切り替える.
	if (Input::IsKeyDown(VK_F1)) {
		if (SceneManager* p_scene_manager = ServiceLocator::Get<SceneManager>()) {
			p_scene_manager->LoadScene(SceneManager::eList::AnimationTuning);
		}
	}
#endif // _DEBUG.

	DirectX12* p_dx12 = ServiceLocator::Get<DirectX12>();

	if (CameraManager* p_camera_manager = ServiceLocator::Get<CameraManager>()) {
		p_camera_manager->Update();

		// アクティブカメラの行列をDirectX12側へ反映.
		if (CameraBase* active_camera = p_camera_manager->GetActive()) {
			p_dx12->SetCamera(
				active_camera->GetViewMatrix(),
				active_camera->GetProjMatrix(),
				active_camera->GetPosition());
		}
	}

	if (p_dx12) {
		p_dx12->Update();
	}

	if (m_pPMXActor) {
		m_pPMXActor->Update();
	}

	if (m_upPlayer) {
		m_upPlayer->Update();
	}
}

void MainScene::LateUpdate()
{
}

void MainScene::Draw()
{
	DirectX12* p_dx12 = ServiceLocator::Get<DirectX12>();
	if (!p_dx12) { return; }

	p_dx12->GetCommandList()->SetPipelineState(m_pPMXRenderer->GetPipelineState());
	p_dx12->GetCommandList()->SetGraphicsRootSignature(m_pPMXRenderer->GetRootSignature());
	p_dx12->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	if (m_pPMXActor) {
		m_pPMXActor->Draw();
	}

	if (m_upPlayer) {
		ImGui::Begin("Player", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		const DirectX::XMFLOAT3& position = m_upPlayer->GetPosition();
		ImGui::Text("Position: (%.2f, %.2f, %.2f)", position.x, position.y, position.z);
		ImGui::Text("State: %s", ToDebugString(m_upPlayer->GetCurrentStateID()));
		ImGui::End();
	}
}
