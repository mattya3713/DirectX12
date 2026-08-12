#pragma once

#include <memory>

#include "99_System/Scene/SceneBase.h"

class PMXRenderer;
class PMXActor;
class XActor;
class Player;
class Enemy;
class ModelPreviewPanel;

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/10.
* @brief     : 通常のメインシーン(元々Main.cppが直接持っていたPMXモデル表示部分を抽出).
**********************************************************************************/

class MainScene final : public SceneBase
{
public:
	MainScene();
	~MainScene() override;

	void Initialize() override;
	void Create() override;
	void Update() override;
	void LateUpdate() override;
	void Draw() override;

private:
	std::shared_ptr<PMXRenderer> m_pPMXRenderer;
	std::shared_ptr<PMXActor>    m_pPMXActor;
	std::unique_ptr<XActor>      m_upXActor;
	std::unique_ptr<Player>      m_upPlayer;
	std::unique_ptr<Enemy>       m_upEnemy;

#if _DEBUG
	// モデル確認用パネル(デバッグ専用). シーンを切り替えずにいつでも任意のモデルを
	// プレビュー・アニメーション確認できる(Unityのシーンビュー/ゲームビューのイメージ).
	std::unique_ptr<ModelPreviewPanel> m_upModelPreviewPanel;
#endif // _DEBUG.
};
