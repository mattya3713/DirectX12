#pragma once

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/14.
* @brief     : 3Dシーンを描画したオフスクリーンテクスチャ(DirectX12::CreateSceneColorTarget())を
*            : ImGuiパネル内に表示する(Unity/UnrealのSceneビューに相当).
*            : Stage A: パネルサイズに合わせた動的リサイズ・ドッキング統合は未対応(固定サイズのテクスチャを
*            : そのまま表示するのみ). ドッキングレイアウトへの統合は別タスクで行う.
**********************************************************************************/

class SceneView final
{
public:
	SceneView() = default;
	~SceneView() = default;

	// 毎フレーム描画(ImGuiManager::NewFrame()〜Render()の間、DirectX12::PrepareUIRenderTarget()の後で呼ぶ.
	// オフスクリーンバッファがPIXEL_SHADER_RESOURCE状態になっている必要があるため).
	static void Draw();
};
