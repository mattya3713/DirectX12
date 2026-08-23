#include "MmdlMesh.h"

#include "10_Ggraphic/30_Asset/RuntimeModel/MMdl/MMdlActor.h"
#include "10_Ggraphic/30_Asset/RuntimeModel/MMdl/MmdlRenderer.h"
#include "99_Utility/Debug/Log/DebugLog.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"
#include "99_Utility/Transform/Transform.h"

MMdlMesh::MMdlMesh(const std::string& FilePath, MmdlRenderer& Renderer)
	: m_upActor { std::make_unique<MmdlActor>(FilePath.c_str(), Renderer) }
{
}

MMdlMesh::MMdlMesh(const std::filesystem::path& FilePath, MmdlRenderer& Renderer)
	: m_upActor { std::make_unique<MmdlActor>(FilePath, Renderer) }
{
}

MMdlMesh::MMdlMesh(std::shared_ptr<MmdlResource> Resource, MmdlRenderer& Renderer)
	: m_upActor { std::make_unique<MmdlActor>(std::move(Resource), Renderer) }
{
}

MMdlMesh::~MMdlMesh() = default;

void MMdlMesh::Update()
{
	m_upActor->Update();
}

void MMdlMesh::Draw()
{
	m_upActor->Draw();
}

void MMdlMesh::SetWorldTransform(const Transform& InTransform)
{
	m_upActor->SetWorldMatrix(InTransform.GetMatrix());
}

void MMdlMesh::PlayNamedClip(const std::string& ClipName)
{
	for (const XSkeleton::AnimationClip& clip : m_upActor->GetClips())
	{
		if (clip.Name == ClipName)
		{
			m_upActor->PlayAnimation(ClipName);
			return;
		}
	}

	// クリップが見つからない場合は現在のアニメーション再生を継続する
	// (StopAnimationするとバインドポーズ固定になり以後一切動かなくなるため).
	if (DebugLog* p_debug_log = ServiceLocator::Get<DebugLog>()) {
		p_debug_log->LogWarning("PlayNamedClip: clip not found = " + ClipName);
	}
}

void MMdlMesh::SetCurrentFrame(float ActionFrame)
{
	m_upActor->SetCurrentFrame(ActionFrame);
}

float MMdlMesh::GetCurrentAnimationSeconds() const
{
	return m_upActor->GetCurrentAnimationSeconds();
}

void MMdlMesh::SetPlaybackSpeed(float Speed)
{
	m_upActor->SetPlaybackSpeed(Speed);
}

#if _DEBUG
float MMdlMesh::GetLocalHeight() const
{
	return m_upActor->GetLocalHeight();
}
#endif
