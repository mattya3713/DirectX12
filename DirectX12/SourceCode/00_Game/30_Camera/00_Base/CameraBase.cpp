#include "stdafx.h"
#include "CameraBase.h"

#include <algorithm>
#include <cstdlib>

#include "00_Game/00_GameLoop/Time/Time.h"
#include "99_Utility/Transform/Transform.h"

namespace {
	constexpr float DEFAULT_FOVY = DirectX::XMConvertToRadians(50.0f);
	constexpr float DEFAULT_NEAR = 1.0f;
	constexpr float DEFAULT_FAR  = 1000.0f;
}

CameraBase::CameraBase()
	: m_upTransform	{ std::make_unique<Transform>() }
	, m_LookPos		{ 0.0f, 0.0f, 0.0f }
	, m_View		{}
	, m_Proj		{}
	, m_FovY		{ DEFAULT_FOVY }
	, m_Aspect		{ WND_WF / WND_HF }
	, m_NearClip	{ DEFAULT_NEAR }
	, m_FarClip		{ DEFAULT_FAR }
{
}

CameraBase::~CameraBase()
{
}

void CameraBase::UpdateViewProjection()
{
	ViewUpdate();
	ProjectionUpdate();
}

const DirectX::XMFLOAT3& CameraBase::GetPosition() const noexcept
{
	return m_upTransform->Position;
}

void CameraBase::SetPosition(const DirectX::XMFLOAT3& Position)
{
	m_upTransform->Position = Position;
}

const DirectX::XMFLOAT3& CameraBase::GetLook() const noexcept
{
	return m_LookPos;
}

void CameraBase::SetLook(const DirectX::XMFLOAT3& Look)
{
	m_LookPos = Look;
}

const DirectX::XMMATRIX& CameraBase::GetViewMatrix() const noexcept
{
	return m_View;
}

const DirectX::XMMATRIX& CameraBase::GetProjMatrix() const noexcept
{
	return m_Proj;
}

DirectX::XMMATRIX CameraBase::GetViewProjMatrix() const noexcept
{
	return m_View * m_Proj;
}

// Forward/Rightを1回の行列生成で両方出す(呼び出し側が2回Getすると行列が2回作っていた).
void CameraBase::GetBasis(DirectX::XMFLOAT3& Forward, DirectX::XMFLOAT3& Right) const noexcept
{
	using namespace DirectX;

	const XMVECTOR v_rotation = XMLoadFloat3(&m_upTransform->Rotation);
	const XMMATRIX rotation_matrix = XMMatrixRotationRollPitchYawFromVector(v_rotation);

	XMStoreFloat3(&Forward, XMVector3Normalize(rotation_matrix.r[2]));
	XMStoreFloat3(&Right,   XMVector3Normalize(rotation_matrix.r[0]));
}

DirectX::XMFLOAT3 CameraBase::GetForward() const noexcept
{
	DirectX::XMFLOAT3 forward, right;
	GetBasis(forward, right);
	return forward;
}

DirectX::XMFLOAT3 CameraBase::GetRight() const noexcept
{
	DirectX::XMFLOAT3 forward, right;
	GetBasis(forward, right);
	return right;
}

float CameraBase::GetYaw() const noexcept
{
	return m_upTransform->Rotation.y;
}

void CameraBase::SetYaw(float Yaw) noexcept
{
	m_upTransform->Rotation.y = Yaw;
}

float CameraBase::GetPitch() const noexcept
{
	return m_upTransform->Rotation.x;
}

void CameraBase::SetPitch(float Pitch) noexcept
{
	m_upTransform->Rotation.x = Pitch;
}

float CameraBase::GetFovY() const noexcept
{
	return m_FovY;
}

void CameraBase::SetFovY(float FovY) noexcept
{
	m_FovY = FovY;
}

void CameraBase::SetAspect(float Aspect) noexcept
{
	m_Aspect = Aspect;
}

void CameraBase::ViewUpdate()
{
	const DirectX::XMFLOAT3 shake_offset = UpdateShake();

	DirectX::XMFLOAT3 shaken_position = m_upTransform->Position;
	shaken_position.x += shake_offset.x;
	shaken_position.y += shake_offset.y;
	shaken_position.z += shake_offset.z;

	DirectX::XMVECTOR v_position = DirectX::XMLoadFloat3(&shaken_position);
	DirectX::XMVECTOR v_look     = DirectX::XMLoadFloat3(&m_LookPos);
	DirectX::XMVECTOR v_up       = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	m_View = DirectX::XMMatrixLookAtLH(v_position, v_look, v_up);
}

void CameraBase::ProjectionUpdate()
{
	m_Proj = DirectX::XMMatrixPerspectiveFovLH(m_FovY, m_Aspect, m_NearClip, m_FarClip);
}

void CameraBase::Shake(float Intensity, float Duration) noexcept
{
	m_ShakeIntensity = Intensity;
	m_ShakeDuration  = Duration;
	m_ShakeElapsed   = 0.0f;
}

DirectX::XMFLOAT3 CameraBase::UpdateShake() noexcept
{
	if (m_ShakeDuration <= 0.0f || m_ShakeElapsed >= m_ShakeDuration)
	{
		return { 0.0f, 0.0f, 0.0f };
	}

	m_ShakeElapsed += GameTime::GetDeltaTime();

	// 時間経過で揺れ幅を1.0→0.0へ線形に減衰させる.
	const float ratio = 1.0f - std::min(m_ShakeElapsed / m_ShakeDuration, 1.0f);
	const float current_intensity = m_ShakeIntensity * ratio;

	const auto random_offset = [current_intensity]() noexcept {
		const float t = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX); // [0,1].
		return (t * 2.0f - 1.0f) * current_intensity; // [-current_intensity, current_intensity].
	};

	return { random_offset(), random_offset(), random_offset() };
}
