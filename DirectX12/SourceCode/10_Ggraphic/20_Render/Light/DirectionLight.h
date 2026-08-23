#pragma once

#include <DirectXMath.h>

/**************************************************
*	平行光源(Directional Light).
*	ライト方向・色を持ち、シャドウマッピング用の
*	光源視点View/Projection行列も供給する.
**/
class DirectionLight
{
public:
	DirectionLight() noexcept;

	// ライト方向(正規化して保持する)の取得・設定.
	void SetDirection(const DirectX::XMFLOAT3& Direction) noexcept;
	const DirectX::XMFLOAT3& GetDirection() const noexcept { return m_Direction; }

	// ライト色の取得・設定.
	void SetColor(const DirectX::XMFLOAT3& Color) noexcept { m_Color = Color; }
	const DirectX::XMFLOAT3& GetColor() const noexcept { return m_Color; }

	// シャドウバイアス(深度比較時に引くオフセット)の取得・設定.
	void SetShadowBias(float Bias) noexcept { m_ShadowBias = Bias; }
	float GetShadowBias() const noexcept { return m_ShadowBias; }

	// 光源視点のView行列(シーン中心を注視する).
	DirectX::XMMATRIX GetLightViewMatrix() const noexcept;
	// 光源視点の正射影Projection行列(固定範囲. Player/Boss周辺をカバー).
	DirectX::XMMATRIX GetLightProjMatrix() const noexcept;

#if _DEBUG
	// ImGuiで方向・色・バイアスを調整するデバッグパネル.
	void DrawDebugPanel();
#endif

private:
	// シャドウマップが覆盖する範囲(ワールド単位. Player/Boss戦闘域をカバーする仮の固定値).
	static constexpr float SHADOW_AREA_SIZE = 30.0f;
	// ライトカメラの距離(シーン中心からの後退距離)とクリップ範囲.
	static constexpr float LIGHT_DISTANCE = 25.0f;
	static constexpr float SHADOW_NEAR_Z  = 1.0f;
	static constexpr float SHADOW_FAR_Z   = 60.0f;

	DirectX::XMFLOAT3 m_Direction;  // 光の進行方向(正規化済み).
	DirectX::XMFLOAT3 m_Color;      // ライト色(RGB).
	float             m_ShadowBias; // シャドウ深度比較用バイアス.
};
