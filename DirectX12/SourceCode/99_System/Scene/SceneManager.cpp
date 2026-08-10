#include "SceneManager.h"

#include "99_System/Scene/SceneBase.h"
#include "00_Game/20_Scene/10_Main/MainScene.h"

#if _DEBUG
#include "00_Game/20_Scene/Ex_Test/AnimationTuning/AnimationTuningScene.h"
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
	// 予約中のシーン切り替えがあれば、ここで安全に切り替える
	// (シーン自身のUpdate()実行中に呼ばれても、自分自身を即座に破棄しないようにするため).
	if (m_NextSceneID != eList::MAX) {
		eList next = m_NextSceneID;
		m_NextSceneID = eList::MAX;

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
