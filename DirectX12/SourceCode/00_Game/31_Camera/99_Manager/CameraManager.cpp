#include "stdafx.h"
#include "CameraManager.h"

CameraManager::CameraManager()
	: m_Cameras			{}
	, m_pActiveCamera	{ nullptr }
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
	m_pActiveCamera = it->second.get();
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
