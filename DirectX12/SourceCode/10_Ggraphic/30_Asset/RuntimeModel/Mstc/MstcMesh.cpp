#include "MstcMesh.h"

#include <filesystem>

#include "MstcActor.h"
#include "99_Utility/Transform/Transform.h"

MstcMesh::MstcMesh(const std::filesystem::path& FilePath, MstcRenderer& Renderer)
	: m_upActor { std::make_unique<MstcActor>(FilePath, Renderer) }
{
}

MstcMesh::~MstcMesh() = default;

void MstcMesh::Update()
{
	// 静的メッシュのため毎フレームの更新処理は無し(ワールド行列はSetWorldTransformで即時反映).
}

void MstcMesh::Draw()
{
	m_upActor->Draw();
}

void MstcMesh::SetWorldTransform(const Transform& InTransform)
{
	m_upActor->SetWorldMatrix(InTransform.GetMatrix());
}
