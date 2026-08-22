#include "MeshObject.h"

#include "10_Ggraphic/30_Asset/Parser/IMesh.h"

void MeshObject::Update()
{
	if (!m_spMesh) { return; }

	m_spMesh->SetWorldTransform(GetTransform());
	m_spMesh->Update();
}

void MeshObject::Draw()
{
	if (!m_spMesh) { return; }

	m_spMesh->Draw();
}

void MeshObject::PlayNamedClip(const std::string& ClipName)
{
	if (!m_spMesh) { return; }

	m_spMesh->PlayNamedClip(ClipName);
}

void MeshObject::SetCurrentFrame(float ActionFrame) noexcept
{
	if (m_spMesh) { m_spMesh->SetCurrentFrame(ActionFrame); }
}
