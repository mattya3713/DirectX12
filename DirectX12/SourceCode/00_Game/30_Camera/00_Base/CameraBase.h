#pragma once

#include <memory>
#include <DirectXMath.h>

struct Transform;

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/06.
* @brief     : カメラの基底クラス.
**********************************************************************************/

class CameraBase
{
public:
	CameraBase();
	virtual ~CameraBase();

	CameraBase(const CameraBase&)            = delete;
	CameraBase& operator=(const CameraBase&) = delete;
	CameraBase(CameraBase&&)                 = delete;
	CameraBase& operator=(CameraBase&&)      = delete;

	// 毎フレーム更新(派生クラスで実装).
	virtual void Update() = 0;

	// ビュー・プロジェクション行列の更新.
	void UpdateViewProjection();

public: // Getter・Setter.

	// 視点(カメラ位置)の取得・設定.
	const DirectX::XMFLOAT3& GetPosition() const noexcept;
	void SetPosition(const DirectX::XMFLOAT3& Position);

	// 注視点の取得・設定.
	const DirectX::XMFLOAT3& GetLook() const noexcept;
	void SetLook(const DirectX::XMFLOAT3& Look);

	// ビュー行列の取得.
	const DirectX::XMMATRIX& GetViewMatrix() const noexcept;
	// プロジェクション行列の取得.
	const DirectX::XMMATRIX& GetProjMatrix() const noexcept;
	// ビュー・プロジェクション合成行列の取得.
	DirectX::XMMATRIX GetViewProjMatrix() const noexcept;

	// 前方向ベクトルの取得.
	DirectX::XMFLOAT3 GetForward() const noexcept;
	// 右方向ベクトルの取得.
	DirectX::XMFLOAT3 GetRight() const noexcept;

	// Yawの取得・設定(ラジアン).
	float GetYaw() const noexcept;
	void SetYaw(float Yaw) noexcept;

	// Pitchの取得・設定(ラジアン).
	float GetPitch() const noexcept;
	void SetPitch(float Pitch) noexcept;

	// 垂直画角の取得・設定(ラジアン).
	float GetFovY() const noexcept;
	void SetFovY(float FovY) noexcept;

	// アスペクト比の設定.
	void SetAspect(float Aspect) noexcept;

	// カメラを揺らす(Intensity: 揺れ幅, Duration: 持続時間(秒)). 呼び出す度に上書きされる
	// (時間経過で揺れ幅が0に減衰する. ViewUpdate()で毎フレーム自動的に適用される).
	void Shake(float Intensity, float Duration) noexcept;

protected:

	// ビュー(カメラ)変換の更新.
	void ViewUpdate();
	// プロジェクション(射影)変換の更新.
	void ProjectionUpdate();

private:
	// シェイクの経過を進め、現在フレームのオフセット量を計算する.
	DirectX::XMFLOAT3 UpdateShake() noexcept;

protected:
	std::unique_ptr<Transform> m_upTransform;	// カメラの位置・回転(RotationはX:Pitch, Y:Yawとして使用).
	DirectX::XMFLOAT3          m_LookPos;		// 注視点.

	DirectX::XMMATRIX m_View;	// ビュー行列.
	DirectX::XMMATRIX m_Proj;	// プロジェクション行列.

	float m_FovY;		// 垂直画角(ラジアン).
	float m_Aspect;		// アスペクト比.
	float m_NearClip;	// ニアクリップ.
	float m_FarClip;	// ファークリップ.

private:
	float m_ShakeIntensity = 0.0f;	// 揺れ幅(開始時の最大値).
	float m_ShakeDuration  = 0.0f;	// 持続時間(秒).
	float m_ShakeElapsed   = 0.0f;	// 経過時間(秒).
};
