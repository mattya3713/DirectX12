#include "MainScene.h"

#include <cassert>

#include "10_Ggraphic/DirectX/DirectX12.h"
#include "10_Ggraphic/PMX/PMXActor.h"
#include "10_Ggraphic/PMX/PMXRenderer.h"
#include "10_Ggraphic/PMX/PMXMesh.h"
#include "10_Ggraphic/X/XActor.h"
#include "10_Ggraphic/X/XMesh.h"
#include "00_Game/30_Camera/99_Manager/CameraManager.h"
#include "00_Game/30_Camera/30_Debug/DebugCamera.h"
#include "00_Game/30_Camera/00_Base/CameraBase.h"
#include "00_Game/50_Input/Input.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/Player.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/Boss.h"
#include "00_Game/60_Combat/CombatCoordinator.h"
#include "99_Utility/Debug/Imgui/ImGuiManager.h"
#include "99_Utility/Debug/Imgui/ModelPreviewPanel.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"
#include "99_Utility/String/String.h"
#include "00_Game/00_Scene/SceneManager.h"

MainScene::MainScene() = default;

MainScene::~MainScene()
{
	if (CombatCoordinator* p_combat_coordinator = ServiceLocator::Get<CombatCoordinator>()) {
		p_combat_coordinator->Clear();
	}
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

	try {
		m_pPMXRenderer = std::make_shared<PMXRenderer>(*p_dx12);
	}
	catch (const std::runtime_error& Msg) {
		std::wstring w_str = MyString::StringToWString(Msg.what());
		_ASSERT_EXPR(false, w_str.c_str());
	}

	try {
		m_upPlayer = std::make_unique<Player>();
		m_upPlayer->AttachMesh(std::make_shared<XMesh>("Data/Model/X/Player/player.X", *m_pPMXRenderer));
		Transform player_transform;
		player_transform.Position = { 0.0f, 0.0f, 0.0f };
		player_transform.Scale = { 1.36f, 1.36f, 1.36f }; // モデルサイズ検知パネルで実測し、当たり判定の高さ(2.0)に合わせて調整済み.
		m_upPlayer->SetTransform(player_transform);

		m_upBoss = std::make_unique<Boss>();
		m_upBoss->AttachMesh(std::make_shared<XMesh>("Data/Model/X/Boss/boss.X", *m_pPMXRenderer));
		Transform boss_transform;
		boss_transform.Position = { 0.0f, 0.0f, 8.0f };
		boss_transform.Scale = { 1.05f, 1.05f, 1.05f }; // モデルサイズ検知パネルで実測し、当たり判定の高さ(2.0)に合わせて調整済み.
		m_upBoss->SetTransform(boss_transform);

		if (CombatCoordinator* p_combat_coordinator = ServiceLocator::Get<CombatCoordinator>()) {
			p_combat_coordinator->Initialize(PlayerCombatView{ *m_upPlayer }, BossCombatView{ *m_upBoss });
		}
	}
	catch (const std::runtime_error& Msg) {
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

#if _DEBUG
	// 実行中にPlayer/BossのScaleを調整できるデバッグパネル(モデルサイズ調整用).
	ImGui::Begin("Actor Scale (Debug)", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

	if (m_upPlayer) {
		Transform transform = m_upPlayer->GetTransform();
		float scale = transform.Scale.x;
		ImGuiManager::Input("Player Scale", scale, true, 0.1f, 1.0f);
		if (scale > 0.0f) {
			transform.Scale = { scale, scale, scale };
			m_upPlayer->SetTransform(transform);
		}
	}

	if (m_upBoss) {
		Transform transform = m_upBoss->GetTransform();
		float scale = transform.Scale.x;
		ImGuiManager::Input("Boss Scale", scale, true, 0.1f, 1.0f);
		if (scale > 0.0f) {
			transform.Scale = { scale, scale, scale };
			m_upBoss->SetTransform(transform);
		}
	}

	ImGui::End();
#endif

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

	if (m_upBoss && m_upPlayer) {
		// ロックオン等は無く、Playerの位置をそのままEnemyのターゲットとして毎フレーム渡す.
		m_upBoss->SetTargetPos(m_upPlayer->GetPosition());
	}

	if (m_upPlayer) {
		m_upPlayer->Update();
	}

	if (m_upBoss) {
		m_upBoss->Update();
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

	if (m_upPlayer) {
		m_upPlayer->Draw();
	}

	if (m_upBoss) {
		m_upBoss->Draw();
	}
}
