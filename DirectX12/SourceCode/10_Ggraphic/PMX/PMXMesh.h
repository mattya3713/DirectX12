#pragma once

#include <memory>
#include <string>

struct Transform;
class PMXActor;
class PMXRenderer;

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/10.
* @brief     : PMXActorをラップし、MeshObjectから必要な最低限の操作だけを見せるクラス.
*            : ボーン・GPUバッファ等の低レベルな詳細はPMXActor内に隠蔽する.
*            : PMXActor自体はshared_ptrで保持する(現状は1インスタンス=1所有だが、
*            : 将来モデルデータを複数GameObjectで使い回す拡張の余地を残すため).
**********************************************************************************/

class PMXMesh final
{
public:
	PMXMesh(const std::string& FilePath, PMXRenderer& Renderer);
	~PMXMesh();

	PMXMesh(const PMXMesh&)            = delete;
	PMXMesh& operator=(const PMXMesh&) = delete;
	PMXMesh(PMXMesh&&)                 = delete;
	PMXMesh& operator=(PMXMesh&&)      = delete;

	// 毎フレーム更新.
	void Update();
	// 描画(パイプライン・ルートシグネチャの設定は呼び出し側で行っておくこと).
	void Draw();

	// ワールド変換を反映する(GameObject::GetTransform()等から渡す).
	void SetWorldTransform(const Transform& InTransform);

	// モーション(VMD)を読み込み再生を開始する.
	void LoadMotion(const std::string& VmdFilePath);

	// 再生を開始する(コンストラクタ時点で読み込まれているモーションを使う).
	void Play();

	// アニメーションの再生範囲・速度をまとめて適用する.
	void ApplyAnimationClip(float StartFrame, float EndFrame, float Speed);

private:
	std::shared_ptr<PMXActor> m_pActor;
};
