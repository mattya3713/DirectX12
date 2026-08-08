#include "stdafx.h"
#include "CameraBase.h"

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

DirectX::XMFLOAT3 CameraBase::GetForward() const noexcept
{
	DirectX::XMVECTOR v_rotation = DirectX::XMLoadFloat3(&m_upTransform->Rotation);
	DirectX::XMMATRIX rotation_matrix = DirectX::XMMatrixRotationRollPitchYawFromVector(v_rotation);
	DirectX::XMVECTOR v_forward = DirectX::XMVector3Normalize(rotation_matrix.r[2]);

	DirectX::XMFLOAT3 forward = {};
	DirectX::XMStoreFloat3(&forward, v_forward);
	return forward;
}

DirectX::XMFLOAT3 CameraBase::GetRight() const noexcept
{
	DirectX::XMVECTOR v_rotation = DirectX::XMLoadFloat3(&m_upTransform->Rotation);
	DirectX::XMMATRIX rotation_matrix = DirectX::XMMatrixRotationRollPitchYawFromVector(v_rotation);
	DirectX::XMVECTOR v_right = DirectX::XMVector3Normalize(rotation_matrix.r[0]);

	DirectX::XMFLOAT3 right = {};
	DirectX::XMStoreFloat3(&right, v_right);
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
	DirectX::XMVECTOR v_position = DirectX::XMLoadFloat3(&m_upTransform->Position);
	DirectX::XMVECTOR v_look     = DirectX::XMLoadFloat3(&m_LookPos);
	DirectX::XMVECTOR v_up       = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	m_View = DirectX::XMMatrixLookAtLH(v_position, v_look, v_up);
}

void CameraBase::ProjectionUpdate()
{
	m_Proj = DirectX::XMMatrixPerspectiveFovLH(m_FovY, m_Aspect, m_NearClip, m_FarClip);
}
