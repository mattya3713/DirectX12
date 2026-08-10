#pragma once

#include <memory>

#include "99_System/Scene/SceneBase.h"

class PMXRenderer;
class PMXActor;
class Player;

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
	std::unique_ptr<Player>      m_upPlayer;
};
