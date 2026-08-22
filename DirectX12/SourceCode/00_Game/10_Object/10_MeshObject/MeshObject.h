#pragma once

#include <memory>
#include <string>

#include "00_Game/10_Object/00_Base/GameObject.h"
#include "10_Ggraphic/30_Asset/Parser/IMesh.h"

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
	void AttachMesh(std::shared_ptr<IMesh> pMesh) noexcept { m_spMesh = std::move(pMesh); }

	// アニメーションの再生範囲・速度をまとめて適用する(メッシュ未アタッチなら何もしない).
	void PlayNamedClip(const std::string& ClipName);

	// アタッチ中のメッシュへActionFrameを直接指定する.
	void SetCurrentFrame(float ActionFrame) noexcept;

	// アタッチ中のメッシュの現在のアニメーション内再生位置を秒で取得する.
	float GetCurrentAnimationSeconds() const noexcept { return m_spMesh ? m_spMesh->GetCurrentAnimationSeconds() : 0.0f; }

#if _DEBUG
	// アタッチ中のメッシュのローカル高さを取得する(Debugビルドのみ).
	float GetLocalHeight() const noexcept { return m_spMesh ? m_spMesh->GetLocalHeight() : 0.0f; }
#endif

protected:
	std::shared_ptr<IMesh> m_spMesh; // アタッチ中のメッシュ.
};
