#include "MainScene.h"

#include <cassert>
#include <cmath>
#include <filesystem>

#include "10_Ggraphic/10_Device/DirectX/DirectX12.h"
#include "10_Ggraphic/30_Asset/RuntimeModel/MMdl/MmdlRenderer.h"
#include "10_Ggraphic/30_Asset/RuntimeModel/MMdl/MMdlMesh.h"
#include "10_Ggraphic/20_Render/Light/DirectionLight.h"
#if _DEBUG
#include "10_Ggraphic/20_Render/Debug/DebugColliderRenderer.h"
#endif
#include "00_Game/30_Camera/99_Manager/CameraManager.h"
#include "00_Game/30_Camera/30_Debug/DebugCamera.h"
#include "00_Game/30_Camera/40_Third/ThirdPersonCamera.h"
#include "00_Game/30_Camera/60_LockOn/LockOnCamera.h"
#include "00_Game/30_Camera/00_Base/CameraBase.h"
#include "00_Game/50_Input/Input.h"
#include "00_Game/50_Input/VirtualPad.h"
#include "00_Game/00_GameLoop/Time/Time.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/Player.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/Boss.h"
#include "00_Game/60_Combat/CombatCoordinator.h"
#include "00_Game/80_CutScene/CutScenePlayer.h"
#include "99_Utility/Debug/Imgui/ImGuiManager.h"
#include "99_Utility/Debug/Imgui/ModelPreviewPanel.h"
#include "99_Utility/Debug/Imgui/SceneView.h"
#include "99_Utility/Debug/Log/DebugLog.h"
#include "99_Utility/Profiling/Profiler.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"
#include "99_Utility/String/String.h"
#include "00_Game/00_Scene/SceneManager.h"

#if _DEBUG
#include "99_Utility/Debug/Imgui/CutSceneEditor.h"
#endif

MainScene::MainScene() = default;

MainScene::~MainScene()
{
	// カットシーンシステム・キャラクターの非所有参照を破棄前に解除する.
	ServiceLocator::Provide<CutScenePlayer>(nullptr);
	ServiceLocator::Provide<Player>(nullptr);
	ServiceLocator::Provide<Boss>(nullptr);

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

	// カメラを登録・有効化(既定はThirdPerson. DebugはF2で切替可能なデバッグ用として維持).
	if (CameraManager* p_camera_manager = ServiceLocator::Get<CameraManager>()) {
		p_camera_manager->Register("Debug", std::make_unique<DebugCamera>());

		auto up_third_person = std::make_unique<ThirdPersonCamera>();
		m_pThirdPersonCamera = up_third_person.get();
		p_camera_manager->Register("Third", std::move(up_third_person));

		auto up_lock_on = std::make_unique<LockOnCamera>();
		m_pLockOnCamera = up_lock_on.get();
		p_camera_manager->Register("LockOn", std::move(up_lock_on));

		p_camera_manager->SetActive("Third");
	}

	try {
		m_pMmdlRenderer = std::make_shared<MmdlRenderer>(*p_dx12);
		m_upDirectionLight = std::make_unique<DirectionLight>();
	}
	catch (const std::runtime_error& Msg) {
		if (DebugLog* p_debug_log = ServiceLocator::Get<DebugLog>()) {
			p_debug_log->LogError(Msg.what());
		}
		std::wstring w_str = MyString::StringToWString(Msg.what());
		_ASSERT_EXPR(false, w_str.c_str());
	}

	try {
		m_upPlayer = std::make_unique<Player>();
		m_upPlayer->AttachMesh(std::make_shared<MMdlMesh>(std::filesystem::path{"Data/Model/mmdl/mskin/player.mskn"}, *m_pMmdlRenderer));
		Transform player_transform;
		player_transform.Position = { 0.0f, 0.0f, 0.0f };
		player_transform.Scale = { 1.36f, 1.36f, 1.36f }; // モデルサイズ検知パネルで実測し、当たり判定の高さ(2.0)に合わせて調整済み.
		m_upPlayer->SetTransform(player_transform);

		m_upBoss = std::make_unique<Boss>();
		m_upBoss->AttachMesh(std::make_shared<MMdlMesh>(std::filesystem::path{"Data/Model/mmdl/mskin/boss.mskn"}, *m_pMmdlRenderer));
		Transform boss_transform;
		boss_transform.Position = { 0.0f, 0.0f, 8.0f };
		boss_transform.Scale = { 1.05f, 1.05f, 1.05f }; // モデルサイズ検知パネルで実測し、当たり判定の高さ(2.0)に合わせて調整済み.
		m_upBoss->SetTransform(boss_transform);

		if (CombatCoordinator* p_combat_coordinator = ServiceLocator::Get<CombatCoordinator>()) {
			p_combat_coordinator->Initialize(PlayerCombatView{ *m_upPlayer }, BossCombatView{ *m_upBoss });
		}

		// カットシーンシステム用に実インスタンスを非所有参照として登録する
		// (CutScenePlayerのExistingInstanceトラックがServiceLocator経由で解決する).
		ServiceLocator::Provide<Player>(m_upPlayer.get());
		ServiceLocator::Provide<Boss>(m_upBoss.get());

		m_upCutScenePlayer = std::make_unique<CutScenePlayer>();
		ServiceLocator::Provide<CutScenePlayer>(m_upCutScenePlayer.get());

#if _DEBUG
		m_upCutSceneEditor = std::make_unique<CutSceneEditor>();
#endif
	}
	catch (const std::runtime_error& Msg) {
		if (DebugLog* p_debug_log = ServiceLocator::Get<DebugLog>()) {
			p_debug_log->LogError(Msg.what());
		}
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

		// 向きデバッグ: 実際のYawと移動方向の角度を表示する(一致していれば正面を向いている).
		const DirectX::XMFLOAT3& move_vec = m_upPlayer->GetMoveVec();
		ImGui::Text("Player Yaw   : %.1f deg", transform.Rotation.y * (180.0f / DirectX::XM_PI));
		if (std::fabs(move_vec.x) > 1e-4f || std::fabs(move_vec.z) > 1e-4f) {
			ImGui::Text("Move Dir     : %.1f deg", std::atan2f(move_vec.x, move_vec.z) * (180.0f / DirectX::XM_PI));
		}
		else {
			ImGui::Text("Move Dir     : --");
		}

		// モデル正面軸のズレ補正角を実行中に調整できるスライダー(確定後、既定値へ焼き込む).
		float front_offset = m_upPlayer->GetModelFrontOffsetDeg();
		ImGui::SliderFloat("Model Front Offset", &front_offset, -180.0f, 180.0f, "%.0f deg");
		m_upPlayer->SetModelFrontOffsetDeg(front_offset);
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

#if _DEBUG
	// 実行中にPlayer/BossのHPを確認できるデバッグパネル(戦闘フィードバック用).
	ImGui::Begin("HP", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

	if (m_upPlayer) {
		const HealthSystem& health = m_upPlayer->GetHealth();
		const float max_hp = health.GetMaxHP();
		const float hp = health.GetHP();
		const float ratio = (max_hp > 0.0f) ? (hp / max_hp) : 0.0f;
		ImGui::Text("Player HP : %.0f / %.0f", hp, max_hp);
		ImGui::ProgressBar(ratio, ImVec2(-1.0f, 0.0f));
	}

	if (m_upBoss) {
		const HealthSystem& health = m_upBoss->GetHealth();
		const float max_hp = health.GetMaxHP();
		const float hp = health.GetHP();
		const float ratio = (max_hp > 0.0f) ? (hp / max_hp) : 0.0f;
		ImGui::Text("Boss HP   : %.0f / %.0f", hp, max_hp);
		ImGui::ProgressBar(ratio, ImVec2(-1.0f, 0.0f));
	}

	ImGui::End();
#endif

	DirectX12* p_dx12 = ServiceLocator::Get<DirectX12>();

	if (CameraManager* p_camera_manager = ServiceLocator::Get<CameraManager>()) {
#if _DEBUG
		// F2でDebug⇔ThirdPersonカメラをトグルする(実機確認用).
		if (Input::IsKeyDown(VK_F2)) {
			const bool is_third_active = (p_camera_manager->GetActive() == static_cast<CameraBase*>(m_pThirdPersonCamera));
			p_camera_manager->SetActive(is_third_active ? "Debug" : "Third");
		}

		// F3でThird⇔LockOnカメラをトグルする(Bossを常に画面内に収める).
		if (Input::IsKeyDown(VK_F3)) {
			const bool is_lock_on_active = (p_camera_manager->GetActive() == static_cast<CameraBase*>(m_pLockOnCamera));
			p_camera_manager->SetActive(is_lock_on_active ? "Third" : "LockOn");
		}
#endif

		// ThirdPersonカメラへPlayerの位置を渡して追従させる(Sceneがカメラとオブジェクトを仲介する設計).
		if (m_pThirdPersonCamera && m_upPlayer && p_camera_manager->GetActive() == static_cast<CameraBase*>(m_pThirdPersonCamera)) {
			m_pThirdPersonCamera->SetTargetPosition(m_upPlayer->GetPosition());
		}

		// LockOnカメラへPlayer/Bossの位置を渡す(注視点をBossに固定するため).
		if (m_pLockOnCamera && m_upPlayer && m_upBoss && p_camera_manager->GetActive() == static_cast<CameraBase*>(m_pLockOnCamera)) {
			m_pLockOnCamera->SetPlayerPosition(m_upPlayer->GetPosition());
			m_pLockOnCamera->SetBossPosition(m_upBoss->GetPosition());
		}

		p_camera_manager->Update();

		// アクティブカメラの行列をDirectX12側へ反映.
		if (CameraBase* active_camera = p_camera_manager->GetActive()) {
			const ImVec2 scene_view_size = SceneView::GetContentSize();
			if (scene_view_size.x > 0.0f && scene_view_size.y > 0.0f)
			{
				active_camera->SetAspect(scene_view_size.x / scene_view_size.y);
			}
			p_dx12->SetCamera(
				active_camera->GetViewMatrix(),
				active_camera->GetProjMatrix(),
				active_camera->GetPosition());
		}
	}

	// 平行光源をDirectX12側へ反映(UpdateSceneBuffer()がシーン定数バッファへ書き込む).
	if (m_upDirectionLight) {
#if _DEBUG
		m_upDirectionLight->DrawDebugPanel();
#endif
		const DirectX::XMFLOAT4 light_direction{
			m_upDirectionLight->GetDirection().x,
			m_upDirectionLight->GetDirection().y,
			m_upDirectionLight->GetDirection().z,
			m_upDirectionLight->GetShadowBias() };
		// a=1で影サンプリング有効(シャドウパス未実装の間はシェーダー側フラグで影はまだ出ない).
		const DirectX::XMFLOAT4 light_color{
			m_upDirectionLight->GetColor().x,
			m_upDirectionLight->GetColor().y,
			m_upDirectionLight->GetColor().z,
			0.0f };
		p_dx12->SetLight(
			m_upDirectionLight->GetLightViewMatrix(),
			m_upDirectionLight->GetLightProjMatrix(),
			light_direction,
			light_color);
	}

	if (p_dx12) {
		p_dx12->Update();
	}

	// Pauseアクション(ESC/コントローラーStart)でGameTimeの一時停止をトグルする.
	if (VirtualPad* p_pad = ServiceLocator::Get<VirtualPad>()) {
		if (p_pad->IsActionDown(VirtualPad::eGameAction::Pause)) {
			GameTime::SetPaused(!GameTime::IsPaused());
		}
	}

#if _DEBUG
	// 一時停止中だと分かる表示(デバッグ用ImGui).
	if (GameTime::IsPaused()) {
		ImGui::Begin("Pause", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Text("PAUSED (Press ESC/Start to resume)");
		ImGui::End();
	}
#endif

	// 勝敗判定: Player/Bossのどちらかが死亡した時点で確定させる(Character.h等は変更せずMainScene側でポーリング).
	if (!m_IsGameOver && m_upPlayer && m_upBoss) {
		const bool is_player_alive = m_upPlayer->GetHealth().IsAlive();
		const bool is_boss_alive = m_upBoss->GetHealth().IsAlive();
		if (!is_player_alive || !is_boss_alive) {
			m_IsGameOver = true;
			m_WinnerIsPlayer = is_boss_alive; // Bossが生存していればPlayerの勝ち.
		}
	}

	if (m_upBoss && m_upPlayer) {
		// ロックオン等は無く、Playerの位置をそのままEnemyのターゲットとして毎フレーム渡す.
		m_upBoss->SetTargetPos(m_upPlayer->GetPosition());
	}

	// 一時停止中・勝敗確定後はPlayer/Bossの更新をスキップする(カメラ・ImGui・描画は止めない).
	const bool is_paused = GameTime::IsPaused();

	if (m_upPlayer && !is_paused && !m_IsGameOver) {
		m_upPlayer->Update();
	}

	if (m_upBoss && !is_paused && !m_IsGameOver) {
		m_upBoss->Update();
	}

	// カットシーン再生(Player/Boss更新後に呼び、カットシーン側のTransformを優先させる).
	if (m_upCutScenePlayer && !is_paused && !m_IsGameOver) {
		m_upCutScenePlayer->Update(GameTime::GetDeltaTime());
	}

#if _DEBUG
	// カットシーン編集ツールとテスト再生(デバッグ用ImGui).
	if (m_upCutSceneEditor) {
		m_upCutSceneEditor->Draw();

		if (m_upCutScenePlayer && m_upCutScenePlayer->IsPlaying()) {
			ImGui::Begin("Cut Scene Player", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
			ImGui::Text(IMGUI_JP("再生中: %.2f秒"), m_upCutScenePlayer->GetElapsedTime());
			if (ImGui::Button(IMGUI_JP("強制停止"))) {
				m_upCutScenePlayer->Stop();
			}
			ImGui::End();
		}
	}
#endif

#if _DEBUG
	// 勝敗確定後だと分かる表示(デバッグ用ImGui).
	if (m_IsGameOver) {
		ImGui::Begin("Game Result", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::TextUnformatted(m_WinnerIsPlayer ? "WIN" : "LOSE");
		ImGui::End();
	}
#endif
}

void MainScene::LateUpdate()
{
}

void MainScene::Draw()
{
	DirectX12* p_dx12 = ServiceLocator::Get<DirectX12>();
	if (!p_dx12) { return; }

	m_pMmdlRenderer->BeforDraw();
	p_dx12->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	Profiler::Instance().GpuBegin("GPU:Characters");

	if (m_upPlayer) {
		m_upPlayer->Draw();
	}

	if (m_upBoss) {
		m_upBoss->Draw();
	}

	Profiler::Instance().GpuEnd("GPU:Characters");

#if _DEBUG
	// コライダー描画は「各キャラが登録→DebugColliderRendererがまとめて描画」の分離方式.
	// Root Signature/PSOの切替はDebugColliderRenderer::Draw()の1箇所に集約される.
	if (m_upPlayer) {
		m_upPlayer->DrawDebugColliders();
	}

	if (m_upBoss) {
		m_upBoss->DrawDebugColliders();
	}

	if (DebugColliderRenderer* p_collider_renderer = ServiceLocator::Get<DebugColliderRenderer>()) {
		Profiler::Instance().GpuBegin("GPU:Colliders");
		p_collider_renderer->Draw();
		Profiler::Instance().GpuEnd("GPU:Colliders");
	}
#endif
}
