#pragma once

#include <memory>

#include "00_Game/10_Object/00_Base/GameObject.h"

class PMXMesh;

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/10.
* @brief     : メッシュ(PMXMesh)を保持するゲームオブジェクト基底クラス.
*            : Update()でGameObjectのTransformをメッシュへ反映してから更新し、
*            : Draw()で描画を委譲する. メッシュ未アタッチなら何もしない.
**********************************************************************************/

class MeshObject : public GameObject
{
public:
	MeshObject() = default;
	~MeshObject() override = default;

	void Update() override;
	void Draw() override;

public:
	// メッシュをアタッチする.
	void AttachMesh(std::shared_ptr<PMXMesh> pMesh) noexcept { m_pMesh = std::move(pMesh); }

	// アニメーションの再生範囲・速度をまとめて適用する(メッシュ未アタッチなら何もしない).
	void ApplyAnimationClip(float StartFrame, float EndFrame, float Speed);

protected:
	std::shared_ptr<PMXMesh> m_pMesh; // アタッチ中のメッシュ.
};
