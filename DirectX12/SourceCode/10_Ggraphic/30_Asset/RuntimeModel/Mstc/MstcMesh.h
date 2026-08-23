#pragma once

#include <memory>

#include "10_Ggraphic/30_Asset/Parser/IMesh.h"

// 前方宣言.
class MstcActor;
class MstcRenderer;
struct Transform;

/**********************************************************************************
* @author    : Coder(閃斬 Production Loop).
* @date      : 2026/08/23.
* @brief     : 静的メッシュ(.mstc)のMstcActorをラップするファサード.
*            : MMdlMeshと同じくIMeshを実装し、MeshObject経由で扱えるようにする
*            : (静的メッシュのためアニメーション操作は全て無操作).
**********************************************************************************/

class MstcMesh final : public IMesh
{
public:
	MstcMesh(const std::filesystem::path& FilePath, MstcRenderer& Renderer);
	~MstcMesh() override;

	MstcMesh(const MstcMesh&)            = delete;
	MstcMesh& operator=(const MstcMesh&) = delete;
	MstcMesh(MstcMesh&&)                 = delete;
	MstcMesh& operator=(MstcMesh&&)      = delete;

	void Update() override;
	void Draw() override;
	void SetWorldTransform(const Transform& InTransform) override;
	// 静的メッシュにアニメーションは無いため何もしない.
	void PlayNamedClip(const std::string& ClipName) override {}
	void SetCurrentFrame(float ActionFrame) override {}
	float GetCurrentAnimationSeconds() const override { return 0.0f; }

#if _DEBUG
	// バインドポーズでのY軸方向の高さは静的メッシュでは未実装(0を返す).
	float GetLocalHeight() const override { return 0.0f; }
#endif

private:
	std::unique_ptr<MstcActor> m_upActor; // 実体(描画・ワールド変換を持つ).
};
