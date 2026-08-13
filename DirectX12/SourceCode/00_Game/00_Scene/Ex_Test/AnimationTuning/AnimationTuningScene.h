#pragma once

#include <memory>

#include "00_Game/00_Scene/00_Base/SceneBase.h"

class PMXRenderer;
class ModelPreviewPanel;

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/10.
* @brief     : アニメーション調整用シーン(デバッグ専用). 中身はModelPreviewPanel(モデル
*            : 選択+AnimationEditor)そのもので、専用カメラを持つ以外の役割はほぼ無い.
*            : MainSceneにも同じModelPreviewPanelを常駐させているため、通常はF1等で
*            : わざわざこのシーンへ切り替えなくてもモデル確認ができる(Unityのシーン
*            : ビュー/ゲームビューのように、ゲーム本体を止めずに確認できるのが狙い).
**********************************************************************************/

class AnimationTuningScene final : public SceneBase
{
public:
	AnimationTuningScene();
	~AnimationTuningScene() override;

	void Initialize() override;
	void Create() override;
	void Update() override;
	void LateUpdate() override;
	void Draw() override;

private:
	std::shared_ptr<PMXRenderer>       m_pPMXRenderer;
	std::unique_ptr<ModelPreviewPanel> m_upModelPreviewPanel;
};
