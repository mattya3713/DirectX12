#include "AnimationTuningScene.h"

#include <cassert>

#include "10_Ggraphic/DirectX/DirectX12.h"
#include "10_Ggraphic/PMX/PMXRenderer.h"
#include "00_Game/30_Camera/99_Manager/CameraManager.h"
#include "00_Game/30_Camera/30_Debug/DebugCamera.h"
#include "00_Game/30_Camera/00_Base/CameraBase.h"
#include "00_Game/50_Input/Input.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"
#include "99_Utility/Debug/Imgui/ModelPreviewPanel.h"
#include "00_Game/00_Scene/SceneManager.h"

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

	m_pPMXRenderer = std::make_shared<PMXRenderer>(*p_dx12);
	m_upModelPreviewPanel = std::make_unique<ModelPreviewPanel>(*m_pPMXRenderer);
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

	if (m_upModelPreviewPanel) {
		m_upModelPreviewPanel->Update();
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

	if (m_upModelPreviewPanel) {
		m_upModelPreviewPanel->Draw();
	}
}
