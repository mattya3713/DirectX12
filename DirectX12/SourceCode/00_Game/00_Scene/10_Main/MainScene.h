#pragma once

#include <memory>

#include "00_Game/00_Scene/00_Base/SceneBase.h"

class PMXRenderer;
class PMXActor;
class XActor;
class Player;
class Boss;
class ModelPreviewPanel;

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/10.
* @brief     : メインシーン.
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

	std::unique_ptr<Player>      m_upPlayer;
	std::unique_ptr<Boss>        m_upBoss;
};
