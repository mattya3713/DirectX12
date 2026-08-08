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

protected:

	// ビュー(カメラ)変換の更新.
	void ViewUpdate();
	// プロジェクション(射影)変換の更新.
	void ProjectionUpdate();

protected:
	std::unique_ptr<Transform> m_upTransform;	// カメラの位置・回転(RotationはX:Pitch, Y:Yawとして使用).
	DirectX::XMFLOAT3          m_LookPos;		// 注視点.

	DirectX::XMMATRIX m_View;	// ビュー行列.
	DirectX::XMMATRIX m_Proj;	// プロジェクション行列.

	float m_FovY;		// 垂直画角(ラジアン).
	float m_Aspect;		// アスペクト比.
	float m_NearClip;	// ニアクリップ.
	float m_FarClip;	// ファークリップ.
};
