#include "MmdlResource.h"

MmdlResource::MmdlResource(const std::filesystem::path& FilePath)
	: m_FilePath { FilePath }
{
}

void MmdlResource::StoreCpuData(Model::ModelData ModelData, XSkeleton::SkeletalData Skeleton,
	float LocalHeight, std::vector<RuntimeFormat::SkinSubmeshRole> SubmeshRoles,
	float FrontCompositeOpacity, float FrontCompositeMaxDistance)
{
	if (HasCpuData()) { return; }
	m_ModelData = std::move(ModelData);
	m_Skeleton = std::move(Skeleton);
	m_LocalHeight = LocalHeight;
	m_SkinSubmeshRoles = std::move(SubmeshRoles);
	m_FrontCompositeOpacity = FrontCompositeOpacity;
	m_FrontCompositeMaxDistance = FrontCompositeMaxDistance;
}

void MmdlResource::StoreGpuResources(MyComPtr<ID3D12Resource> VertexBuffer, MyComPtr<ID3D12Resource> IndexBuffer,
	MyComPtr<ID3D12Resource> MaterialBuffer, std::vector<MyComPtr<ID3D12Resource>> BaseTextureResources,
	std::vector<MyComPtr<ID3D12Resource>> ToonTextureResources,
	std::vector<MyComPtr<ID3D12Resource>> SphereTextureResources)
{
	if (m_pVertexBuffer || m_pIndexBuffer || m_pMaterialBuffer) { return; }
	m_pVertexBuffer = std::move(VertexBuffer);
	m_pIndexBuffer = std::move(IndexBuffer);
	m_pMaterialBuffer = std::move(MaterialBuffer);
	m_pBaseTextureResources = std::move(BaseTextureResources);
	m_pToonTextureResources = std::move(ToonTextureResources);
	m_pSphereTextureResources = std::move(SphereTextureResources);
}
