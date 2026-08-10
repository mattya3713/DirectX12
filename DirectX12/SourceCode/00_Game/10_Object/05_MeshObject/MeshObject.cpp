#include "MeshObject.h"

#include "10_Ggraphic/PMX/PMXMesh.h"

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

void MeshObject::ApplyAnimationClip(float StartFrame, float EndFrame, float Speed)
{
	if (!m_pMesh) { return; }

	m_pMesh->ApplyAnimationClip(StartFrame, EndFrame, Speed);
}
