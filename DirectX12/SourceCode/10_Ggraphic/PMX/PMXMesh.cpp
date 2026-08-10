#include "PMXMesh.h"

#include "10_Ggraphic/PMX/PMXActor.h"
#include "99_Utility/Transform/Transform.h"

PMXMesh::PMXMesh(const std::string& FilePath, PMXRenderer& Renderer)
	: m_pActor { std::make_shared<PMXActor>(FilePath.c_str(), Renderer) }
{
}

PMXMesh::~PMXMesh() = default;

void PMXMesh::Update()
{
	m_pActor->Update();
}

void PMXMesh::Draw()
{
	m_pActor->Draw();
}

void PMXMesh::SetWorldTransform(const Transform& InTransform)
{
	m_pActor->SetWorldMatrix(InTransform.GetMatrix());
}

void PMXMesh::LoadMotion(const std::string& VmdFilePath)
{
	m_pActor->LoadVMDFile(VmdFilePath);
	m_pActor->PlayAnimation();
}

void PMXMesh::Play()
{
	m_pActor->PlayAnimation();
}

void PMXMesh::ApplyAnimationClip(float StartFrame, float EndFrame, float Speed)
{
	m_pActor->SetPlaybackRange(StartFrame, EndFrame);
	m_pActor->SetAnimationSpeed(Speed);
}
