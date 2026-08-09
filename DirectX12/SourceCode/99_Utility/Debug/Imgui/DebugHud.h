#pragma once

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/10.
* @brief     : ImGuiで表示するデバッグHUD(FPS・デルタタイム・アクティブカメラ情報).
*            : GameTime/CameraManagerからServiceLocator経由で情報を取得し、
*            : ImGuiManager経由で毎フレーム描画する.
**********************************************************************************/

class DebugHud final
{
public:
	DebugHud() = default;
	~DebugHud() = default;

	// 毎フレーム描画(ImGuiManager::NewFrame()〜Render()の間で呼ぶ).
	static void Draw();
};
