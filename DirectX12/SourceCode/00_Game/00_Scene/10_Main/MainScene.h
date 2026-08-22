#pragma once

#include <memory>

#include "00_Game/00_Scene/00_Base/SceneBase.h"

class MmdlRenderer;
class Player;
class Boss;
class ModelPreviewPanel;
class ThirdPersonCamera;

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
	std::shared_ptr<MmdlRenderer> m_pMmdlRenderer;

	std::unique_ptr<Player>      m_upPlayer;
	std::unique_ptr<Boss>        m_upBoss;

	ThirdPersonCamera* m_pThirdPersonCamera = nullptr; // 追従対象を渡すための非所有ポインタ(所有はCameraManager).

	bool m_IsGameOver = false;     // 勝敗確定フラグ(確定後はPlayer/Bossの更新を止める).
	bool m_WinnerIsPlayer = false; // true=Player勝利(WIN). false=Boss勝利(LOSE).
};
