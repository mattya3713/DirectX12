#include "MainScene.h"

#include <cassert>
#include <cmath>
#include <filesystem>

#include "10_Ggraphic/10_Device/DirectX/DirectX12.h"
#include "10_Ggraphic/30_Asset/RuntimeModel/MMdl/MmdlRenderer.h"
#include "10_Ggraphic/30_Asset/RuntimeModel/MMdl/MMdlMesh.h"
#include "10_Ggraphic/30_Asset/RuntimeModel/Mstc/MstcActor.h"
#include "10_Ggraphic/30_Asset/RuntimeModel/Mstc/MstcRenderer.h"
#include "10_Ggraphic/20_Render/Light/DirectionLight.h"
#include "10_Ggraphic/20_Render/Sprite/SpriteRenderer.h"
#include "10_Ggraphic/20_Render/Sprite/TextRenderer.h"
#include "20_Resource/Font/FontLoader.h"
#include "00_Game/20_UI/UILayoutRuntime.h"
#include "10_Ggraphic/20_Render/Particle/ParticleSystem.h"
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
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/PlayerAccessKeys.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/Boss.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/10_Enemy/Enemy.h"
#include "00_Game/50_Enemy/Definition/EnemyDefinitionCatalog.h"
#include "00_Game/50_Enemy/Factory/EnemyFactory.h"
#include "00_Game/50_Enemy/Planner/EnemySpawnPlanner.h"
#include "00_Game/60_Combat/CombatCoordinator.h"
#include "00_Game/80_CutScene/CutScenePlayer.h"
#include "99_Utility/Debug/Imgui/ImGuiManager.h"
#include "99_Utility/Debug/Imgui/CombatDebugHud.h"
#include "99_Utility/Debug/Imgui/LevelEditor.h"
#include "99_Utility/Debug/Imgui/CombatTuningEditor.h"
#include "99_Utility/Debug/Imgui/DebugConsole.h"
#include "99_Utility/Localization/LocalizationTable.h"
#include "99_Utility/Settings/Settings.h"
#include "00_Game/60_Combat/CombatTuning.h"
#include "99_Utility/Debug/Imgui/ParticleSystemEditor.h"
#include "99_Utility/Debug/Imgui/SoundEventEditor.h"
#include "99_Utility/Async/AsyncModelLoader.h"
#include "99_Utility/ECS/World.h"
#include "99_Utility/ECS/SampleComponents.h"
#include "99_Utility/Debug/PlaytestRecorder.h"
#include "99_Utility/Debug/Imgui/ModelPreviewPanel.h"
#include "99_Utility/Debug/Imgui/SceneView.h"
#include "99_Utility/Debug/Log/DebugLog.h"
#include "99_Utility/Event/EventBus.h"
#include "99_Utility/Profiling/Profiler.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"
#include "99_Utility/String/String.h"
#include "00_Game/00_Scene/SceneManager.h"

#if _DEBUG
#include "99_Utility/Debug/Imgui/CutSceneEditor.h"
#endif

namespace {

	// レベルオブジェクトの位置・回転(オイラー角・度)・スケールからワールド行列を作る.
	DirectX::XMMATRIX ComposeLevelObjectWorld(const DirectX::XMFLOAT3& Position,
		const DirectX::XMFLOAT3& RotationDeg, const DirectX::XMFLOAT3& Scale)
	{
		constexpr float deg_to_rad = 3.14159265358979f / 180.0f;
		const DirectX::XMVECTOR rotation = DirectX::XMQuaternionRotationRollPitchYaw(
			RotationDeg.x * deg_to_rad, RotationDeg.y * deg_to_rad, RotationDeg.z * deg_to_rad);
		return DirectX::XMMatrixTransformation(
			DirectX::g_XMZero,                          // 拡縮基準点(原点).
			DirectX::XMQuaternionIdentity(),            // 拡縮軸の向き(ワールド軸).
			DirectX::XMLoadFloat3(&Scale),
			DirectX::g_XMZero,                          // 回転基準点(原点).
			rotation,
			DirectX::XMLoadFloat3(&Position));
	}

}

MainScene::MainScene() = default;

MainScene::~MainScene()
{
	// カットシーンシステム・キャラクターの非所有参照を破棄前に解除する.
	ServiceLocator::Provide<CutScenePlayer>(nullptr);
	ServiceLocator::Provide<ParticleSystem>(nullptr);
	ServiceLocator::Provide<SoundEventEditor>(nullptr);
	ServiceLocator::Provide<Player>(nullptr);
	ServiceLocator::Provide<Boss>(nullptr);

	// Create()で購読したデモイベントを解除する(シーン再入のたびにハンドラが蓄積するのを防ぐ).
	if (EventBus* p_event_bus = ServiceLocator::Get<EventBus>()) {
		p_event_bus->UnsubscribeAll<BossDefeatedEvent>();
	}

	if (CombatCoordinator* p_combat_coordinator = ServiceLocator::Get<CombatCoordinator>()) {
		p_combat_coordinator->Clear();
	}

	// 非同期ローダーを停止(残ジョブ処理後にワーカー終了).
	if (m_upAsyncModels) { m_upAsyncModels->Shutdown(); }
}

void MainScene::Initialize()
{
}

void MainScene::Create()
{
	DirectX12* p_dx12 = ServiceLocator::Get<DirectX12>();

	// ユーザー設定の復元(ThirdPersonCameraのctorが参照するため登録前に読む).
	SettingsManager::Instance().Load();

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
		m_upSpriteRenderer = std::make_unique<SpriteRenderer>(*p_dx12);
		m_upTextRenderer = std::make_unique<TextRenderer>(*m_upSpriteRenderer);
		m_pMstcRenderer = std::make_shared<MstcRenderer>(*p_dx12);
	}
	catch (const std::runtime_error& Msg) {
		if (DebugLog* p_debug_log = ServiceLocator::Get<DebugLog>()) {
			p_debug_log->LogError(Msg.what());
		}
		std::wstring w_str = MyString::StringToWString(Msg.what());
		_ASSERT_EXPR(false, w_str.c_str());
	}

	// UI Layoutランタイム(layout.jsonを読み込み、欠損/破損時は既定表示へフォールバック).
	m_upUILayoutRuntime = std::make_unique<UILayoutRuntime>();
	if (!m_upUILayoutRuntime->LoadFromJson("Data/Json/UI/layout.json")) {
		m_upUILayoutRuntime->LoadDefaultLayout();
	}

	try {
		m_upPlayer = std::make_unique<Player>();
		Transform player_transform;
		player_transform.Position = { 0.0f, 0.0f, 0.0f };
		player_transform.Scale = { 1.36f, 1.36f, 1.36f }; // モデルサイズ検知パネルで実測し、当たり判定の高さ(2.0)に合わせて調整済み.
		m_upPlayer->SetTransform(player_transform);

		m_upBoss = std::make_unique<Boss>();
		Transform boss_transform;
		boss_transform.Position = { 0.0f, 0.0f, 8.0f };
		boss_transform.Scale = { 1.05f, 1.05f, 1.05f }; // モデルサイズ検知パネルで実測し、当たり判定の高さ(2.0)に合わせて調整済み.
		m_upBoss->SetTransform(boss_transform);

		// モデルは非同期ロード(Updateは継続. CPUパース完了後にメインスレッドでGPU接続).
		m_upAsyncModels = std::make_unique<AsyncModelLoader>();
		m_upAsyncModels->Initialize();
		m_PlayerModelRequest = m_upAsyncModels->Request("Data/Model/mmdl/mskin/player.mskn");
		m_BossModelRequest   = m_upAsyncModels->Request("Data/Model/mmdl/mskin/boss.mskn");

		if (CombatCoordinator* p_combat_coordinator = ServiceLocator::Get<CombatCoordinator>()) {
			p_combat_coordinator->Initialize(PlayerCombatView{ *m_upPlayer }, BossCombatView{ *m_upBoss });
		}

		// カットシーンシステム用に実インスタンスを非所有参照として登録する
		// (CutScenePlayerのExistingInstanceトラックがServiceLocator経由で解決する).
		ServiceLocator::Provide<Player>(m_upPlayer.get());
		ServiceLocator::Provide<Boss>(m_upBoss.get());

		// EventBusデモ: Boss死亡イベントを購読し、ログへ出力する(実機確認用. 本格導入は別タスク).
		if (EventBus* p_event_bus = ServiceLocator::Get<EventBus>()) {
			p_event_bus->Subscribe<BossDefeatedEvent>([](const BossDefeatedEvent& Event) {
				if (DebugLog* p_debug_log = ServiceLocator::Get<DebugLog>()) {
					p_debug_log->LogInfo("EventBus demo: BossDefeated (Who!=null: " + std::string(Event.Who ? "true" : "false") + ")");
				}
			});
		}

		m_upCutScenePlayer = std::make_unique<CutScenePlayer>();
		ServiceLocator::Provide<CutScenePlayer>(m_upCutScenePlayer.get());

		// パーティクルシステム(パイプライン構築+ServiceLocator登録).
		m_upParticleSystem = std::make_unique<ParticleSystem>();
		if (m_upParticleSystem->Initialize(p_dx12->GetDevice().Get())) {
			ServiceLocator::Provide<ParticleSystem>(m_upParticleSystem.get());
		}

		// レベルデータ(Data\Json\Level配下)から静的オブジェクトを構築し、
		// スポーン地点指定(キーが有る場合のみ)でPlayer/Boss初期位置を上書きする.
		const std::filesystem::path default_level_path = "Data/Json/Level/main.json";
		if (std::filesystem::exists(default_level_path)) {
			LoadLevelFromJson(default_level_path);
		}

#if _DEBUG
		m_upCutSceneEditor = std::make_unique<CutSceneEditor>();

		m_upLevelEditor = std::make_unique<LevelEditor>();
		m_upLevelEditor->SetOnLevelChanged([this](const std::filesystem::path& Path) {
			LoadLevelFromJson(Path);
		});

		m_upParticleEditor = std::make_unique<ParticleSystemEditor>();

		// Sound Event EditorをServiceLocatorへ登録(Combat等からPlayCombatEventで呼べるように).
		m_upSoundEventEditor = std::make_unique<SoundEventEditor>();
		ServiceLocator::Provide<SoundEventEditor>(m_upSoundEventEditor.get());

		// Combat調整値のプリセット(Data\Json\Combat\tuning.json)があれば自動読込.
		m_upCombatTuningEditor = std::make_unique<CombatTuningEditor>();
		CombatTuning::Load("Data/Json/Combat/tuning.json");
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

	// F7で撃破シーケンスを強制発動する(演出確認用デバッグキー).
	if (Input::IsKeyDown(VK_F7)) {
		DebugStartFinisherSequence();
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

			// 敗北確定時は巻き戻り逆再生を開始する(保存フレームが無ければ即LOSE表示へ).
			DirectX12* p_dx12_for_rewind = ServiceLocator::Get<DirectX12>();
			if (p_dx12_for_rewind && !m_WinnerIsPlayer) {
				p_dx12_for_rewind->StartRewindPlayback();
			}
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

	// 雑魚敵AI更新(Playerをターゲットとして毎フレーム渡す).
	if (!is_paused && !m_IsGameOver && !m_upEnemies.empty()) {
		const DirectX::XMFLOAT3 player_pos = m_upPlayer ? m_upPlayer->GetPosition() : DirectX::XMFLOAT3{ 0.0f, 0.0f, 0.0f };
		for (std::unique_ptr<Enemy>& enemy : m_upEnemies) {
			enemy->SetTargetPos(player_pos);
			enemy->Update();
		}
	}

	// 撃破シーケンス基盤(ゲージ加速/成立判定/演出フック). Player/Boss更新後に呼ぶ.
	TickFinisherSequence();

	// UI Layoutランタイム: HUD要素へゲーム値(HP)を反映する.
	if (m_upUILayoutRuntime && m_upPlayer && m_upBoss) {
		UIHudSnapshot snapshot{};
		snapshot.PlayerHpRatio = m_upPlayer->GetHealth().IsAlive()
			? (m_upPlayer->GetHealth().GetHP() / std::max(m_upPlayer->GetHealth().GetMaxHP(), 1.0f)) : 0.0f;
		snapshot.BossHpRatio = m_upBoss->GetHealth().IsAlive()
			? (m_upBoss->GetHealth().GetHP() / std::max(m_upBoss->GetHealth().GetMaxHP(), 1.0f)) : 0.0f;
		m_upUILayoutRuntime->BindGameValues(snapshot);
	}

	// カットシーン再生(Player/Boss更新後に呼び、カットシーン側のTransformを優先させる).
	// 撃破シーケンス中(Playing)は戦闘停止後も演出側の更新を継続させる.
	if (m_upCutScenePlayer && !is_paused && (!m_IsGameOver || m_FinisherPhase == FinisherPhase::Playing)) {
		m_upCutScenePlayer->Update(GameTime::GetDeltaTime());
	}

	// パーティクル更新(一時停止・勝敗確定後は止める).
	if (m_upParticleSystem && !is_paused && !m_IsGameOver) {
		m_upParticleSystem->Update(GameTime::GetDeltaTime());
	}

	// 非同期モデルロードの完了取り込み(CPUパース完了→メインスレッドでGPU接続).
	if (m_upAsyncModels)
	{
		auto attach_when_ready = [&](std::shared_ptr<AsyncModelRequest>& request, Character& owner, const char* label) {
			if (!request || request->GetState() != eModelLoadState::CpuParsed) { return; }

			owner.AttachMesh(std::make_shared<MMdlMesh>(request->GetResource(), *m_pMmdlRenderer));
			request->MarkGpuAttached();

			if (DebugLog* p_debug_log = ServiceLocator::Get<DebugLog>()) {
				p_debug_log->LogInfo(std::string("Async model ready: ") + label);
			}
		};

		attach_when_ready(m_PlayerModelRequest, *m_upPlayer, "player");
		attach_when_ready(m_BossModelRequest,   *m_upBoss,   "boss");

		// 失敗した要求は1回だけログへ出す(DEBUG表示. ゲーム自体は継続).
		for (std::shared_ptr<AsyncModelRequest>* request : { &m_PlayerModelRequest, &m_BossModelRequest })
		{
			if (*request && (*request)->GetState() == eModelLoadState::Failed && !(*request)->IsFailureLogged())
			{
				(*request)->SetFailureLogged();
				if (DebugLog* p_debug_log = ServiceLocator::Get<DebugLog>()) {
					p_debug_log->LogError("Async model load failed: " + (*request)->GetPath().generic_string()
						+ " / " + (*request)->GetError());
				}
			}
		}

		// ロード状態のDEBUG表示+欠損モデル要求テスト(Failed経路の動作確認用).
		static std::shared_ptr<AsyncModelRequest> s_FailureTestRequest;
		ImGui::Begin("Async Model Load", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		auto state_text = [](eModelLoadState s) -> const char* {
			switch (s) {
			case eModelLoadState::Loading:   return "Loading";
			case eModelLoadState::CpuParsed: return "CpuParsed";
			case eModelLoadState::Ready:     return "Ready";
			case eModelLoadState::Failed:    return "FAILED";
			default:                         return "?";
			}
		};
		ImGui::Text("Player: %s", state_text(m_PlayerModelRequest ? m_PlayerModelRequest->GetState() : eModelLoadState::Failed));
		ImGui::Text("Boss  : %s", state_text(m_BossModelRequest ? m_BossModelRequest->GetState() : eModelLoadState::Failed));
		if (ImGui::Button(IMGUI_JP("欠損モデル要求テスト"))) {
			s_FailureTestRequest = m_upAsyncModels->Request("debug_missing/missing.mskn");
		}
		if (s_FailureTestRequest) {
			ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "FailureTest: %s %s",
				state_text(s_FailureTestRequest->GetState()), s_FailureTestRequest->GetError().c_str());
		}
		ImGui::End();
	}

#if _DEBUG
	// ECS動作確認サンプル(雑魚敵/Ragdoll移行前の最小構成. 本格移行は別タスク).
	static ECS::World s_EcsSampleWorld;
	static bool s_EcsInitialized = false;
	if (!s_EcsInitialized)
	{
		for (int i = 0; i < 3; ++i)
		{
			const ECS::Entity entity = s_EcsSampleWorld.CreateEntity();
			auto& transform = s_EcsSampleWorld.AddComponent<ECS::TransformComponent>(entity);
			transform.Position = { static_cast<float>(i), 0.0f, 0.0f };
			s_EcsSampleWorld.AddComponent<ECS::HealthComponent>(entity);
		}

		// 既存GameObjectからEntityを参照するブリッジサンプル.
		if (m_upPlayer) {
			m_upPlayer->SetEntityHandle(s_EcsSampleWorld.CreateEntity());
		}

		s_EcsSampleWorld.AddSystem("sample_move", [](ECS::World& world, float delta_time) {
			world.ForEach<ECS::TransformComponent>([delta_time](const ECS::Entity&, ECS::TransformComponent& transform) {
				transform.Position.x += 0.5f * delta_time;
			});
		});

		s_EcsInitialized = true;

		if (DebugLog* p_debug_log = ServiceLocator::Get<DebugLog>()) {
			p_debug_log->LogInfo("ECS sample: entities=" + std::to_string(s_EcsSampleWorld.GetAliveEntityCount())
				+ " componentTypes=" + std::to_string(s_EcsSampleWorld.GetComponentTypeCount())
				+ " systems=" + std::to_string(s_EcsSampleWorld.GetSystemCount()));
		}
	}

	if (!is_paused && !m_IsGameOver) {
		s_EcsSampleWorld.RunSystems(GameTime::GetDeltaTime());
		s_EcsSampleWorld.FlushDestroyed();
	}
#endif

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
	// レベルシーン編集ツール(デバッグ用ImGui).
	if (m_upLevelEditor) {
		m_upLevelEditor->Draw();
	}

	// Combat調整ツール(デバッグ用ImGui).
	if (m_upCombatTuningEditor) {
		m_upCombatTuningEditor->Draw();
	}

	// デバッグコンソールへのカスタムコマンド登録(コンソール実体は最初のDrawで生成されるため1度だけ).
	static bool s_CustomCommandsRegistered = false;
	if (!s_CustomCommandsRegistered) {
		if (DebugConsole* p_console = ServiceLocator::Get<DebugConsole>()) {
			s_CustomCommandsRegistered = true;

			// カメラ感度を変更してSettings.jsonへ永続化する(再起動後も復元される).
			p_console->RegisterCommand("cam_speed", [this](const DebugConsole::CommandArgs& Args) {
				if (Args.size() < 2 || !m_pThirdPersonCamera) { return; }
				const float speed = static_cast<float>(std::atof(Args[1].c_str()));
				if (speed <= 0.0f) { return; }
				m_pThirdPersonCamera->SetMouseRotationSpeed(speed);
				SettingsManager::Instance().Set("camera.mouse_rotation_speed", speed);
				SettingsManager::Instance().Save();
			});

			// 言語切替(lang ja / lang en. ダミー翻訳デモ用).
			p_console->RegisterCommand("lang", [](const DebugConsole::CommandArgs& Args) {
				if (Args.size() < 2) { return; }
				LocalizationTable::Instance().SetLanguage(Args[1]);
			});
		}
	}
#endif

#if _DEBUG
	// パーティクル編集ツール(デバッグ用ImGui).
	if (m_upParticleEditor) {
		m_upParticleEditor->Draw();
	}

	// Sound Event編集ツール(デバッグ用ImGui).
	if (m_upSoundEventEditor) {
		m_upSoundEventEditor->Draw();
	}
#endif

#if _DEBUG
	// Playtest Recorder(F9で記録開始/停止. 入力/State/HP/カメラをCSV保存するデバッグ用).
	if (Input::IsKeyDown(VK_F9)) {
		PlaytestRecorder::Instance().Toggle();
	}
	PlaytestRecorder::Instance().Tick();
	PlaytestRecorder::Instance().DrawImGui();
#endif

#if _DEBUG
	// 勝敗確定後だと分かる表示(デバッグ用ImGui. 巻き戻り再生中は演出を見せるため隠す).
	DirectX12* p_dx12_for_result = ServiceLocator::Get<DirectX12>();
	const bool is_rewind_playing = p_dx12_for_result && p_dx12_for_result->IsRewindActive();
	if (m_IsGameOver && !is_rewind_playing) {
		ImGui::Begin("Game Result", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::TextUnformatted(m_WinnerIsPlayer ? "WIN" : "LOSE");
		ImGui::End();
	}

	// 戦闘状態一括表示(HP/State/コンボ/必殺ゲージ/TimeScale/コライダー有効状態).
	CombatDebugHud::Draw(m_upPlayer.get(), m_upBoss.get());

	// 敵スポーン結果(生成数/除外数/失敗理由).
	ImGui::Begin("Enemy Spawns", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Text("Alive   : %d", static_cast<int>(m_upEnemies.size()));
	ImGui::Text("Planned : %d", static_cast<int>(m_EnemySpawnPlanned));
	ImGui::Text("Spawned : %d", static_cast<int>(m_EnemySpawnedCount));
	ImGui::Text("Excluded: %d", static_cast<int>(m_EnemySpawnIssues.size()));
	for (const auto& [id, reason] : m_EnemySpawnIssues) {
		ImGui::Text("  - %s (%s)", id.c_str(), reason.c_str());
	}
	ImGui::End();
#endif
}

// ===== 撃破シーケンス基盤(演出内容はユーザー実装. ここでは状態遷移とフックのみ扱う) =====

// ゲージ加速・必殺ヒット成立判定・演出開始フックの毎フレーム処理.
void MainScene::TickFinisherSequence()
{
	if (!m_upPlayer || !m_upBoss) { return; }

	const float boss_hp  = m_upBoss->GetHealth().GetHP();
	const float boss_max = m_upBoss->GetHealth().GetMaxHP();

	if (m_FinisherPhase == FinisherPhase::None && !m_IsGameOver)
	{
		// ゲージ加速: Boss HPが20%以下になったら必殺ゲージを自動チャージする(仮値. 2秒で満タン).
		constexpr float GAUGE_BOOST_HP_RATIO = 0.2f;
		constexpr float GAUGE_BOOST_RATE     = 0.5f;
		if (boss_max > 0.0f && boss_hp / boss_max <= GAUGE_BOOST_HP_RATIO) {
			m_upPlayer->ChargeUltByRatio(GAUGE_BOOST_RATE * GameTime::GetDeltaTime());
		}

		// 成立判定: 必殺技(仮)がBossへヒットしたフレーム(HPが減少した帧)にゲージ満タンなら撃破成立.
		if (m_PrevBossHpForFinisher > boss_hp &&
			m_upPlayer->GetCurrentStateID() == PlayerState::eID::SpecialMove &&
			m_upPlayer->GetCurrentUltValue() >= m_upPlayer->GetMaxUltValue()) {
			BeginFinisherSequence();
		}
	}

	m_PrevBossHpForFinisher = boss_hp;
}

// 撃破成立: 戦闘/AI停止+撃破イベント発行+演出開始フック.
void MainScene::BeginFinisherSequence()
{
	if (m_FinisherPhase != FinisherPhase::None) { return; }

	m_FinisherPhase  = FinisherPhase::Playing;
	m_WinnerIsPlayer = true;
	m_IsGameOver     = true; // 既存ガードでPlayer/Boss/パーティクルの通常更新を停止させる.

	// 撃破成立: Boss HPを下限無視で0へ.
	if (m_upBoss) {
		m_upBoss->ForceKill();
	}

	// 撃破成立イベント(EventBus購読者へ通知).
	if (EventBus* p_event_bus = ServiceLocator::Get<EventBus>()) {
		if (m_upBoss) { p_event_bus->Publish(BossDefeatedEvent{ m_upBoss.get() }); }
	}

	// 演出開始フック: カットシーン"Finisher"を再生し、完了時にNotifyFinisherCutsceneFinished()
	// を呼んでもらう。カットシーン未登録の場合は即座に完了扱いとする(基盤単体でも動作させるため).
	if (m_upCutScenePlayer && m_upCutScenePlayer->Play("Finisher", [this]() { NotifyFinisherCutsceneFinished(); })) {
		return; // 再生開始. 完了通知を待つ.
	}

	NotifyFinisherCutsceneFinished();
}

// 【演出完了通知API】カットシーン側から呼ばれる(完了→状態区切りとして次状態へ).
void MainScene::NotifyFinisherCutsceneFinished()
{
	if (m_FinisherPhase != FinisherPhase::Playing) { return; }

	// 完了: 以降は既存のWIN表示(Game Result)等の後続処理へ流れる.
	m_FinisherPhase = FinisherPhase::Completed;
}

#if _DEBUG
// デバッグ用強制発動(演出確認用. 正式な発動経路はゲージMAX+ヒット).
void MainScene::DebugStartFinisherSequence()
{
	if (m_FinisherPhase != FinisherPhase::None || !m_upPlayer || !m_upBoss) { return; }

	m_upPlayer->ChargeUltByRatio(1.0f);
	BeginFinisherSequence();
}
#endif

void MainScene::LateUpdate()
{
}

void MainScene::LoadLevelFromJson(const std::filesystem::path& Path)
{
	m_LevelActors.clear();
	m_LevelPath.clear();

	if (!m_pMstcRenderer) { return; }

	const LevelDesc level = LevelEditor::LoadLevelJson(Path);

	constexpr const char* kMstcDirectory = "Data/Model/mmdl/mstc/";
	for (const LevelObjectDesc& object : level.Objects)
	{
		try {
			auto actor = std::make_unique<MstcActor>(
				std::filesystem::path(kMstcDirectory) / object.MstcFile, *m_pMstcRenderer);
			actor->SetWorldMatrix(ComposeLevelObjectWorld(object.Position, object.RotationDeg, object.Scale));
			m_LevelActors.push_back(std::move(actor));
		}
		catch (const std::runtime_error& Msg) {
			if (DebugLog* p_debug_log = ServiceLocator::Get<DebugLog>()) {
				p_debug_log->LogError(Msg.what());
			}
		}
	}

	// スポーン地点(JSONにキーが有る場合のみ、MainScene::Createの既定値を上書きする).
	if (level.PlayerSpawn.HasValue && m_upPlayer) {
		Transform transform = m_upPlayer->GetTransform();
		transform.Position  = level.PlayerSpawn.Position;
		transform.Rotation.y = DirectX::XMConvertToRadians(level.PlayerSpawn.YawDeg);
		m_upPlayer->SetTransform(transform);
	}

	if (level.BossSpawn.HasValue && m_upBoss) {
		Transform transform = m_upBoss->GetTransform();
		transform.Position  = level.BossSpawn.Position;
		transform.Rotation.y = DirectX::XMConvertToRadians(level.BossSpawn.YawDeg);
		m_upBoss->SetTransform(transform);
	}

	// 敵実体の再構築(シーン再入・レベル再ロードでコライダーが二重登録されないよう既存を先に破棄).
	m_upEnemies.clear();
	if (!level.EnemySpawns.empty() && m_pMmdlRenderer) {
		SpawnEnemiesFromLevel(level);
	}

	m_LevelPath = Path;
}

void MainScene::SpawnEnemiesFromLevel(const LevelDesc& Level)
{
	// カタログは初回スポーン時に1度だけJSONから読む(再ロードでは流用する).
	if (!m_upEnemyCatalog) {
		auto catalog = std::make_unique<EnemyDefinitionCatalog>();
		if (!catalog->Load("Data/Json/Enemy/Definitions.json")) {
			if (DebugLog* p_debug_log = ServiceLocator::Get<DebugLog>()) {
				p_debug_log->LogError("EnemyDefinitionCatalog: failed to load Data/Json/Enemy/Definitions.json");
			}
			return;
		}
		m_upEnemyCatalog = std::move(catalog);
	}

#if _DEBUG
	m_EnemySpawnIssues.clear();
#endif

	const EnemySpawnPlanner planner(*m_upEnemyCatalog);
	const EnemySpawnPlan plan = planner.Plan(Level.EnemySpawns);
	const EnemyFactory factory(*m_upEnemyCatalog);

#if _DEBUG
	m_EnemySpawnPlanned = plan.Count();
#endif

	for (const EnemySpawnPlanEntry& entry : plan.Entries) {
		EnemySpawnRequest request{};
		request.Definition        = entry.Definition;
		request.InitialTransform  = entry.Transform;

		std::unique_ptr<Enemy> enemy = factory.Create(request);
		if (!enemy) {
			if (DebugLog* p_debug_log = ServiceLocator::Get<DebugLog>()) {
				p_debug_log->LogError("EnemyFactory: create failed (Id=" + entry.InstanceName + ")");
			}
#if _DEBUG
			m_EnemySpawnIssues.emplace_back(entry.Definition->Id, "create_failed");
#endif
			continue;
		}

		// モデル割当は呼び出し側の責務. ModelIdは未整備(ResourceCatalog将来導入)のため
		// 暫定で全敵にCubeを割当てる(生成失敗時もゲーム全体は止めない).
		try {
			enemy->AttachMesh(std::make_shared<MMdlMesh>(
				std::filesystem::path{"Data/Model/mmdl/mskin/Cube.mskn"}, *m_pMmdlRenderer));
		}
		catch (const std::runtime_error& Msg) {
			if (DebugLog* p_debug_log = ServiceLocator::Get<DebugLog>()) {
				p_debug_log->LogError(Msg.what());
			}
#if _DEBUG
			m_EnemySpawnIssues.emplace_back(entry.Definition->Id, "attach_mesh_failed");
#endif
			continue;
		}

		m_upEnemies.push_back(std::move(enemy));
	}

#if _DEBUG
	m_EnemySpawnedCount = m_upEnemies.size();
	for (const EnemySpawnIssue& issue : plan.Issues) {
		m_EnemySpawnIssues.emplace_back(issue.DefinitionId, issue.Reason);
	}

	if (DebugLog* p_debug_log = ServiceLocator::Get<DebugLog>()) {
		p_debug_log->LogInfo("EnemySpawns: planned=" + std::to_string(m_EnemySpawnPlanned)
			+ " spawned=" + std::to_string(m_EnemySpawnedCount)
			+ " excluded=" + std::to_string(m_EnemySpawnIssues.size()));
	}
#endif
}

void MainScene::Draw()
{
	DirectX12* p_dx12 = ServiceLocator::Get<DirectX12>();
	if (!p_dx12) { return; }

	// 巻き戻り逆再生中: 通常描画を止め、リングバッファの内容をフルスクリーン表示する.
	if (p_dx12->IsRewindActive()) {
		p_dx12->DrawRewindFrame();
		return;
	}

	// シャドウ深度パス(光源視点でPlayer/Bossをシャドウマップへ描く. メインパスの前に実施する).
	m_pMmdlRenderer->BeginShadowPass();

	if (m_upPlayer) {
		m_upPlayer->Draw();
	}

	if (m_upBoss) {
		m_upBoss->Draw();
	}

	for (std::unique_ptr<Enemy>& enemy : m_upEnemies) {
		enemy->Draw();
	}

	// シャドウマップをSRV状態へ遷移させ、メインパスのレンダーターゲットを復帰させる.
	m_pMmdlRenderer->EndShadowPass();

	m_pMmdlRenderer->BeforDraw();
	p_dx12->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	Profiler::Instance().GpuBegin("GPU:Characters");

	if (m_upPlayer) {
		m_upPlayer->Draw();
	}

	if (m_upBoss) {
		m_upBoss->Draw();
	}

	for (std::unique_ptr<Enemy>& enemy : m_upEnemies) {
		enemy->Draw();
	}

	Profiler::Instance().GpuEnd("GPU:Characters");

	// Sprite2D/Sprite3D描画基盤の動作確認表示(画面端にUIスプライト+Player頭上にビルボード).
	if (m_upSpriteRenderer) {
		ID3D12Resource* p_sprite_tex = p_dx12->GetTextureByPath("Data\\Image\\toon\\toon01.bmp").Get();
		if (p_sprite_tex) {
			m_upSpriteRenderer->DrawSprite2D(p_sprite_tex, 40.0f, 40.0f, 128.0f, 128.0f);
			if (m_upPlayer) {
				DirectX::XMFLOAT3 head_pos = m_upPlayer->GetPosition();
				head_pos.y += 2.6f;
				m_upSpriteRenderer->DrawSprite3D(p_sprite_tex, head_pos, 0.8f, 0.8f);
			}
		}
	}

	// テキスト描画の動作確認(既定フォント+ローカライズ).
	if (m_upTextRenderer) {
		const int font_id = FontLoader::GetDefaultFont();
		m_upTextRenderer->DrawText2D(font_id, "FPS Demo / 日本語テスト", 40.0f, 180.0f, 0.75f);
		m_upTextRenderer->DrawTextLocalized(font_id, "test_hello", 40.0f, 220.0f, 0.75f);
	}

	// UI Layoutランタイム(layout.json/既定表示のHUDを描画).
	if (m_upUILayoutRuntime && m_upSpriteRenderer) {
		m_upUILayoutRuntime->Draw(*m_upSpriteRenderer);
	}

	// 巻き戻り用にこのフレームの描画結果をリングバッファへ保存する
	// (ImGuiオーバーレイ前・デバッグコライダー描画前のゲーム描画だけを保存する).
	p_dx12->CaptureForRewind();

	// レベル静的オブジェクト(専用パイプラインへ切替て描画).
	if (!m_LevelActors.empty() && m_pMstcRenderer) {
		Profiler::Instance().GpuBegin("GPU:StaticLevel");
		m_pMstcRenderer->BeforDraw();
		p_dx12->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		for (const std::unique_ptr<MstcActor>& actor : m_LevelActors) {
			actor->Draw();
		}
		Profiler::Instance().GpuEnd("GPU:StaticLevel");
	}

	// パーティクル(半透明のためキャラ・静的オブジェクトの後. 深度書き込みなし).
	if (m_upParticleSystem) {
		m_upParticleSystem->Draw();
	}

#if _DEBUG
	// コライダー描画は「各キャラが登録→DebugColliderRendererがまとめて描画」の分離方式.
	// Root Signature/PSOの切替はDebugColliderRenderer::Draw()の1箇所に集約される.
	if (m_upPlayer) {
		m_upPlayer->DrawDebugColliders();
	}

	if (m_upBoss) {
		m_upBoss->DrawDebugColliders();
	}

	for (std::unique_ptr<Enemy>& enemy : m_upEnemies) {
		enemy->DrawDebugColliders();
	}

	if (DebugColliderRenderer* p_collider_renderer = ServiceLocator::Get<DebugColliderRenderer>()) {
		Profiler::Instance().GpuBegin("GPU:Colliders");
		p_collider_renderer->Draw();
		Profiler::Instance().GpuEnd("GPU:Colliders");
	}
#endif
}
