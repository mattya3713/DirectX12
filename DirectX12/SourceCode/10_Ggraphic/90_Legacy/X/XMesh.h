#pragma once

#include <memory>
#include <string>

struct Transform;
class XActor;
class PMXRenderer;

#include "10_Ggraphic/Model/IMesh.h"

class XMesh final : public IMesh
{
public:
	explicit XMesh(const std::string& FilePath, PMXRenderer& Renderer);
	~XMesh() override;

	XMesh(const XMesh&)            = delete;
	XMesh& operator=(const XMesh&) = delete;
	XMesh(XMesh&&)                 = delete;
	XMesh& operator=(XMesh&&)      = delete;

	void Update() override;
	void Draw() override;
	void SetWorldTransform(const Transform& InTransform) override;
	void PlayNamedClip(const std::string& ClipName) override;
	void SetCurrentFrame(float ActionFrame) override;

#if _DEBUG
	float GetLocalHeight() const override;
#endif

private:
	std::unique_ptr<XActor> m_upActor;
};
