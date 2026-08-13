#include "MeshObject.h"

#include "10_Ggraphic/Model/IMesh.h"

void MeshObject::Update()
{
	if (!m_pMesh) { return; }

	m_pMesh->SetWorldTransform(GetTransform());
	m_pMesh->Update();
}

void MeshObject::Draw()
{
	if (!m_pMesh) { return; }

	m_pMesh->Draw();
}

void MeshObject::PlayNamedClip(const std::string& ClipName)
{
	if (!m_pMesh) { return; }

	m_pMesh->PlayNamedClip(ClipName);
}
