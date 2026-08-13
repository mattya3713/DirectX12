#include "XMesh.h"

#include "10_Ggraphic/X/XActor.h"
#include "10_Ggraphic/PMX/PMXRenderer.h"
#include "99_Utility/Transform/Transform.h"

XMesh::XMesh(const std::string& FilePath, PMXRenderer& Renderer)
	: m_upActor { std::make_unique<XActor>(FilePath.c_str(), Renderer) }
{
}

XMesh::~XMesh() = default;

void XMesh::Update()
{
	m_upActor->Update();
}

void XMesh::Draw()
{
	m_upActor->Draw();
}

void XMesh::SetWorldTransform(const Transform& InTransform)
{
	m_upActor->SetWorldMatrix(InTransform.GetMatrix());
}

void XMesh::PlayNamedClip(const std::string& ClipName)
{
	for (const XSkeleton::AnimationClip& clip : m_upActor->GetClips())
	{
		if (clip.Name == ClipName)
		{
			m_upActor->PlayAnimation(ClipName);
			return;
		}
	}

	m_upActor->StopAnimation();
}
