#pragma once

#include <memory>
#include <string>
#include <filesystem>

struct Transform;
class MmdlActor;
class MmdlResource;
class MmdlRenderer;

#include "10_Ggraphic/30_Asset/Parser/IMesh.h"

class MMdlMesh final : public IMesh
{
public:
	explicit MMdlMesh(const std::string& FilePath, MmdlRenderer& Renderer);
	explicit MMdlMesh(const std::filesystem::path& FilePath, MmdlRenderer& Renderer);
	MMdlMesh(std::shared_ptr<MmdlResource> Resource, MmdlRenderer& Renderer);
	~MMdlMesh() override;

	MMdlMesh(const MMdlMesh&)            = delete;
	MMdlMesh& operator=(const MMdlMesh&) = delete;
	MMdlMesh(MMdlMesh&&)                 = delete;
	MMdlMesh& operator=(MMdlMesh&&)      = delete;

	void Update() override;
	void Draw() override;
	void SetWorldTransform(const Transform& InTransform) override;
	void PlayNamedClip(const std::string& ClipName) override;
	void SetCurrentFrame(float ActionFrame) override;

#if _DEBUG
	float GetLocalHeight() const override;
#endif

private:
	std::unique_ptr<MmdlActor> m_upActor;
};
