#pragma once

#include <memory>
#include <string>

#include "10_Ggraphic/Model/IMesh.h"

struct Transform;
class PMXActor;
class PMXRenderer;

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/10.
* @brief     : PMXActorをラップし、MeshObjectから必要な最低限の操作だけを見せるクラス.
**********************************************************************************/

class PMXMesh final : public IMesh
{
public:
	PMXMesh(const std::string& FilePath, PMXRenderer& Renderer);
	~PMXMesh() override;

	PMXMesh(const PMXMesh&)            = delete;
	PMXMesh& operator=(const PMXMesh&) = delete;
	PMXMesh(PMXMesh&&)                 = delete;
	PMXMesh& operator=(PMXMesh&&)      = delete;
	
	void Update() override;
	void Draw() override;

	// ワールド変換を反映する(GameObject::GetTransform()等から渡す).
	void SetWorldTransform(const Transform& InTransform) override;

	void PlayNamedClip(const std::string& ClipName) override;

	// モーション(VMD)を読み込み再生を開始する.
	void LoadMotion(const std::string& VmdFilePath);

	// 再生を開始する(コンストラクタ時点で読み込まれているモーションを使う).
	void Play();

	// アニメーションの再生範囲・速度をまとめて適用する.
	void ApplyAnimationClip(float StartFrame, float EndFrame, float Speed);

private:
	std::shared_ptr<PMXActor> m_pActor;
};
