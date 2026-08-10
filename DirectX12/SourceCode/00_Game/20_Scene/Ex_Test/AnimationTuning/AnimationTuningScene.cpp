#include "AnimationTuningScene.h"

#include <cassert>

#include "10_Ggraphic/DirectX/DirectX12.h"
#include "10_Ggraphic/PMX/PMXActor.h"
#include "10_Ggraphic/PMX/PMXRenderer.h"
#include "00_Game/30_Camera/99_Manager/CameraManager.h"
#include "00_Game/30_Camera/30_Debug/DebugCamera.h"
#include "00_Game/30_Camera/00_Base/CameraBase.h"
#include "00_Game/50_Input/Input.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"
#include "99_Utility/String/String.h"
#include "99_Utility/Debug/Imgui/AnimationEditor.h"
#include "99_System/Scene/SceneManager.h"

AnimationTuningScene::AnimationTuningScene()
{
}

AnimationTuningScene::~AnimationTuningScene()
{
}

void AnimationTuningScene::Initialize()
{
}

void AnimationTuningScene::Create()
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
	}
	catch (const std::runtime_error& Msg) {
		// エラーメッセージを表示(未捕捉のまま伝播させてabortするのを防ぐ).
		std::wstring w_str = MyString::StringToWString(Msg.what());
		_ASSERT_EXPR(false, w_str.c_str());
	}

	m_upAnimationEditor = std::make_unique<AnimationEditor>();
	m_upAnimationEditor->Toggle(); // このシーンでは常時表示する.

	// Editorは既定で一時停止状態(Stepボタンでのみ進む)だが、それだと初期姿勢が未計算のまま
	// (ボーン変換が一度も更新されず)モデルが表示されないため、最初の姿勢だけ計算しておく.
	if (m_pPMXActor) {
		m_pPMXActor->StepFrame();
	}
}

void AnimationTuningScene::Update()
{
#if _DEBUG
	// F1でメインシーンへ戻る.
	if (Input::IsKeyDown(VK_F1)) {
		if (SceneManager* p_scene_manager = ServiceLocator::Get<SceneManager>()) {
			p_scene_manager->LoadScene(SceneManager::eList::MainScene);
			return;
		}
	}
#endif // _DEBUG.

	DirectX12* p_dx12 = ServiceLocator::Get<DirectX12>();

	if (CameraManager* p_camera_manager = ServiceLocator::Get<CameraManager>()) {
		p_camera_manager->Update();

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
		const bool step_requested = m_upAnimationEditor->Draw(*m_pPMXActor);

		if (step_requested) {
			m_pPMXActor->StepFrame();
		}
	}
}

void AnimationTuningScene::LateUpdate()
{
}

void AnimationTuningScene::Draw()
{
	DirectX12* p_dx12 = ServiceLocator::Get<DirectX12>();
	if (!p_dx12) { return; }

	p_dx12->GetCommandList()->SetPipelineState(m_pPMXRenderer->GetPipelineState());
	p_dx12->GetCommandList()->SetGraphicsRootSignature(m_pPMXRenderer->GetRootSignature());
	p_dx12->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	if (m_pPMXActor) {
		m_pPMXActor->Draw();
	}
}
