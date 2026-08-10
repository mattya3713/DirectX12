#pragma once

#include <memory>

#include "99_System/Scene/SceneBase.h"

class PMXRenderer;
class PMXActor;
class AnimationEditor;

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/10.
* @brief     : アニメーション調整用シーン(デバッグ専用. Senzanの`AnimationTuningScene`を参考).
*            : AnimationEditorを常時表示し、再生範囲・速度・1フレームステップを調整できる.
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
	std::shared_ptr<PMXRenderer>     m_pPMXRenderer;
	std::shared_ptr<PMXActor>        m_pPMXActor;
	std::unique_ptr<AnimationEditor> m_upAnimationEditor;
};
