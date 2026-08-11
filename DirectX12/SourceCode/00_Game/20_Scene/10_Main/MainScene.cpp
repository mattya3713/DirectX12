#include "MainScene.h"

#include <cassert>

#include "10_Ggraphic/DirectX/DirectX12.h"
#include "10_Ggraphic/PMX/PMXActor.h"
#include "10_Ggraphic/PMX/PMXRenderer.h"
#include "10_Ggraphic/PMX/PMXMesh.h"
#include "10_Ggraphic/X/XActor.h"
#include "00_Game/30_Camera/99_Manager/CameraManager.h"
#include "00_Game/30_Camera/30_Debug/DebugCamera.h"
#include "00_Game/30_Camera/00_Base/CameraBase.h"
#include "00_Game/50_Input/Input.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/Player.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/10_Enemy/Enemy.h"
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
		case PlayerState::eID::Idle:          return "Idle";
		case PlayerState::eID::Run:           return "Run";
		case PlayerState::eID::AttackCombo_0: return "AttackCombo_0";
		case PlayerState::eID::AttackCombo_1: return "AttackCombo_1";
		case PlayerState::eID::AttackCombo_2: return "AttackCombo_2";
		case PlayerState::eID::Parry:         return "Parry";
		case PlayerState::eID::DodgeExecute:  return "DodgeExecute";
		default:                              return "None";
		}
	}

	// デバッグ表示用にEnemyState::eIDを文字列化する.
	const char* ToDebugString(EnemyState::eID Id)
	{
		switch (Id)
		{
		case EnemyState::eID::Idle:   return "Idle";
		case EnemyState::eID::Chase:  return "Chase";
		case EnemyState::eID::Attack: return "Attack";
		case EnemyState::eID::Dead:   return "Dead";
		default:                      return "None";
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

	try {
		m_pPMXRenderer = std::make_shared<PMXRenderer>(*p_dx12);
		m_pPMXActor = std::make_shared<PMXActor>("Data\\Model\\PMX\\Hatune\\REM式プロセカ風初音ミクN25.pmx", *m_pPMXRenderer);
		m_pPMXActor->PlayAnimation();

		// Playerの見た目(PMXMesh). 同じレンダラー(パイプライン)を共有する.
		// 動作確認しやすいよう原点から少しずらして配置(重ならないように).
		m_upPlayer = std::make_unique<Player>();
		m_upPlayer->SetPosition({ 30.0f, 0.0f, 0.0f });
		auto p_player_mesh = std::make_shared<PMXMesh>("Data\\Model\\PMX\\Hatune\\REM式プロセカ風初音ミクN25.pmx", *m_pPMXRenderer);
		p_player_mesh->Play();
		m_upPlayer->AttachMesh(p_player_mesh);

		// Enemy(動作確認用). Combat/Dodgeを実際に試せる相手として、Playerの近くに
		// AggroRangeより少し離して配置する(Idle→Chaseへの遷移も確認できるように).
		m_upEnemy = std::make_unique<Enemy>();
		m_upEnemy->SetPosition({ 30.0f, 0.0f, 12.0f });
		auto p_enemy_mesh = std::make_shared<PMXMesh>("Data\\Model\\PMX\\Hatune\\REM式プロセカ風初音ミクN25.pmx", *m_pPMXRenderer);
		p_enemy_mesh->Play();
		m_upEnemy->AttachMesh(p_enemy_mesh);

		// XParser経由の.x表示確認用. 元モデルは全体で約1.3x1.5x2.0(単位)しか無く、
		// Hatuneモデル等(MMDスケール)と比べて非常に小さいため15倍に拡大して表示する.
		// 動作確認用に他のモデルと重ならない位置(Playerの反対側)へ配置.
		m_upXActor = std::make_unique<XActor>("Data\\Model\\X\\player.x", *m_pPMXRenderer);
		m_upXActor->SetWorldMatrix(DirectX::XMMatrixScaling(15.0f, 15.0f, 15.0f) * DirectX::XMMatrixTranslation(-30.0f, 0.0f, 0.0f));
		m_upXActor->PlayAnimation("player_run"); // キーフレーム再生の動作確認用.
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

	if (m_upXActor) {
		m_upXActor->Update();
	}

	if (m_upEnemy && m_upPlayer) {
		// ロックオン等は無く、Playerの位置をそのままEnemyのターゲットとして毎フレーム渡す.
		m_upEnemy->SetTargetPos(m_upPlayer->GetPosition());
	}

	if (m_upPlayer) {
		m_upPlayer->Update();
	}

	if (m_upEnemy) {
		m_upEnemy->Update();
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

	if (m_upXActor) {
		m_upXActor->Draw();

		ImGui::Begin("XActor Animation", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		const auto& clips = m_upXActor->GetClips();
		for (size_t i = 0; i < clips.size(); ++i) {
			ImGui::PushID(static_cast<int>(i));
			const bool is_current = (static_cast<int>(i) == m_upXActor->GetCurrentClipIndex());
			if (is_current) { ImGui::Text("> "); ImGui::SameLine(); }
			if (ImGui::Button(clips[i].Name.c_str())) {
				m_upXActor->PlayAnimation(clips[i].Name);
			}
			ImGui::PopID();
		}
		ImGui::End();
	}

	if (m_upPlayer) {
		m_upPlayer->Draw();

		ImGui::Begin("Player", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		const DirectX::XMFLOAT3& position = m_upPlayer->GetPosition();
		ImGui::Text("Position: (%.2f, %.2f, %.2f)", position.x, position.y, position.z);
		ImGui::Text("State: %s", ToDebugString(m_upPlayer->GetCurrentStateID()));
		ImGui::End();
	}

	if (m_upEnemy) {
		m_upEnemy->Draw();

		ImGui::Begin("Enemy", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		const DirectX::XMFLOAT3& position = m_upEnemy->GetPosition();
		ImGui::Text("Position: (%.2f, %.2f, %.2f)", position.x, position.y, position.z);
		ImGui::Text("State: %s", ToDebugString(m_upEnemy->GetCurrentStateID()));
		ImGui::Text("HP: %.0f / %.0f", m_upEnemy->GetHealth().GetHP(), m_upEnemy->GetHealth().GetMaxHP());
		ImGui::End();
	}
}
