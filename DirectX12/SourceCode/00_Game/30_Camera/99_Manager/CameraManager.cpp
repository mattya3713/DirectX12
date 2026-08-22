#include "stdafx.h"
#include "CameraManager.h"

CameraManager::CameraManager()
	: m_Cameras			{}
	, m_pActiveCamera	{ nullptr }
	, m_ActiveName		{}
{
}

CameraManager::~CameraManager()
{
}

void CameraManager::Register(std::string_view Name, std::unique_ptr<CameraBase> upCamera)
{
	m_Cameras[std::string(Name)] = std::move(upCamera);
}

void CameraManager::SetActive(std::string_view Name)
{
	auto it = m_Cameras.find(std::string(Name));
	if (it == m_Cameras.end()) {
		throw std::runtime_error(std::string(Name) + "という名前のカメラは登録されていません。");
	}
	if (m_pActiveCamera && m_pActiveCamera != it->second.get()) {
		m_pActiveCamera->OnDeactivated();
	}
	m_pActiveCamera = it->second.get();
	m_ActiveName = Name;
	m_pActiveCamera->OnActivated();
}

CameraBase* CameraManager::GetActive() const noexcept
{
	return m_pActiveCamera;
}

void CameraManager::Update()
{
	if (m_pActiveCamera) {
		m_pActiveCamera->Update();
	}
}

void CameraManager::PlayOneShot(std::string_view Name, std::vector<CameraKeyframe> Keyframes, bool IsRelativeToFirst)
{
	// 戻り先はこの時点でアクティブだったカメラの名前(KeyframeCamera再生後に自動で戻す).
	std::string return_to = m_ActiveName;

	auto up_camera = std::make_unique<KeyframeCamera>(std::move(Keyframes), IsRelativeToFirst, [this, return_to] { SetActive(return_to); });
	Register(Name, std::move(up_camera));
	SetActive(Name);
}
