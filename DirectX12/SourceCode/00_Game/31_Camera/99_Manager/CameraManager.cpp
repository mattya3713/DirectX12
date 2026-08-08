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

void CameraManager::Register(const std::string& Name, std::unique_ptr<CameraBase> Camera)
{
	m_Cameras[Name] = std::move(Camera);
}

void CameraManager::SetActive(const std::string& Name)
{
	auto it = m_Cameras.find(Name);
	if (it == m_Cameras.end()) {
		throw std::runtime_error(Name + "という名前のカメラは登録されていません。");
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
