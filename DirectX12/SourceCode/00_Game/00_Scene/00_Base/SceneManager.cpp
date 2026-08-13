#include "SceneManager.h"

#include "00_Game/00_Scene/00_Base/SceneBase.h"
#include "00_Game/00_Scene/10_Main/MainScene.h"
#include "10_Ggraphic/DirectX/DirectX12.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"

#if _DEBUG
#include "00_Game/00_Scene/Ex_Test/AnimationTuning/AnimationTuningScene.h"
#include "99_Utility/Debug/Imgui/ImGuiManager.h"
#endif // _DEBUG.

SceneManager::SceneManager() = default;
SceneManager::~SceneManager() = default;

void SceneManager::LoadData(eList InitialScene)
{
	MakeScene(InitialScene);
	if (m_upScene) {
		m_upScene->Initialize();
		m_upScene->Create();
	}
}

void SceneManager::LoadScene(eList Scene)
{
	m_NextSceneID = Scene;
}

void SceneManager::Update()
{
	// 予約中のシーン切り替えを、Update()実行中に自分自身を破棄しないようここで安全に行う.
	if (m_NextSceneID != eList::MAX) {
		eList next = m_NextSceneID;
		m_NextSceneID = eList::MAX;

		// GPUがまだ参照中かもしれない旧シーンのリソースを破棄する前に完了を待つ
		// (フレームインフライト方式のため、待たずに破棄するとアプリが不正終了する).
		if (DirectX12* p_dx12 = ServiceLocator::Get<DirectX12>()) {
			p_dx12->WaitForGPU();
		}

		m_upScene.reset();
		MakeScene(next);
		if (m_upScene) {
			m_upScene->Initialize();
			m_upScene->Create();
		}
	}

	if (m_upScene) {
		m_upScene->Update();
		m_upScene->LateUpdate();
	}

#if _DEBUG
	const char* scene_name = "Unknown";
	switch (m_CurrentSceneID) {
		case eList::MainScene:       scene_name = "MainScene"; break;
		case eList::AnimationTuning: scene_name = "AnimationTuning"; break;
		default: break;
	}

	// 常に同じ初期位置に置き、他のデバッグウィンドウと重ならないようにする(初回起動時のみ).
	ImGui::SetNextWindowPos(ImVec2(20.0f, 290.0f), ImGuiCond_FirstUseEver);
	ImGui::Begin("Scene");
	ImGui::Text("Current: %s", scene_name);
	ImGui::Separator();
	if (ImGui::Button("MainScene")) { LoadScene(eList::MainScene); }
	if (ImGui::Button("AnimationTuning")) { LoadScene(eList::AnimationTuning); }
	ImGui::End();
#endif // _DEBUG.
}

void SceneManager::Draw()
{
	if (m_upScene) {
		m_upScene->Draw();
	}
}

void SceneManager::MakeScene(eList Scene)
{
#if _DEBUG
	m_CurrentSceneID = Scene;
#endif // _DEBUG.

	switch (Scene) {
		case eList::MainScene:
			m_upScene = std::make_unique<MainScene>();
			break;

#if _DEBUG
		case eList::AnimationTuning:
			m_upScene = std::make_unique<AnimationTuningScene>();
			break;
#endif // _DEBUG.

		case eList::MAX:
		default:
			break;
	}
}
