#include "DirectionLight.h"

#include <cmath>

#if _DEBUG
#include "99_Utility/Debug/Imgui/ImGuiManager.h"
#endif

namespace {

	// ライトが注視するシーン中心(Player/Bossの中間付近の仮固定値).
	constexpr DirectX::XMFLOAT3 SCENE_CENTER{ 0.0f, 0.0f, 4.0f };

}

DirectionLight::DirectionLight() noexcept
	: m_Direction { 0.57735f, -0.57735f, 0.57735f } // 旧Pixel.hlslのfloat3(1,-1,1)を正規化したもの.
	, m_Color     { 1.0f, 1.0f, 1.0f }
	, m_ShadowBias{ 0.003f }
{
}

void DirectionLight::SetDirection(const DirectX::XMFLOAT3& Direction) noexcept
{
	const float length = std::sqrt(
		Direction.x * Direction.x + Direction.y * Direction.y + Direction.z * Direction.z);
	if (length > 1e-6f)
	{
		m_Direction = { Direction.x / length, Direction.y / length, Direction.z / length };
	}
}

DirectX::XMMATRIX DirectionLight::GetLightViewMatrix() const noexcept
{
	// シーン中心へ向かって光が降り注ぐ位置にライトカメラを置く(既存カメラと同じLookAtLH規約).
	const DirectX::XMVECTOR center = DirectX::XMLoadFloat3(&SCENE_CENTER);
	const DirectX::XMVECTOR dir    = DirectX::XMLoadFloat3(&m_Direction);
	const DirectX::XMVECTOR eye    = DirectX::XMVectorSubtract(center, DirectX::XMVectorScale(dir, LIGHT_DISTANCE));

	return DirectX::XMMatrixLookAtLH(eye, center, DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
}

DirectX::XMMATRIX DirectionLight::GetLightProjMatrix() const noexcept
{
	// 固定範囲の正射影(動的フィッティングは非対応の仮実装).
	return DirectX::XMMatrixOrthographicLH(SHADOW_AREA_SIZE, SHADOW_AREA_SIZE, SHADOW_NEAR_Z, SHADOW_FAR_Z);
}

#if _DEBUG
void DirectionLight::DrawDebugPanel()
{
	if (!ImGui::Begin("Direction Light", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::End();
		return;
	}

	DirectX::XMFLOAT3 direction = m_Direction;
	if (ImGui::DragFloat3("Light Dir", &direction.x, 0.02f, -1.0f, 1.0f, "%.2f"))
	{
		SetDirection(direction);
	}
	ImGui::Text("Dir (normalized) : %.2f, %.2f, %.2f", m_Direction.x, m_Direction.y, m_Direction.z);

	float color[3] = { m_Color.x, m_Color.y, m_Color.z };
	if (ImGui::ColorEdit3("Light Color", color))
	{
		SetColor({ color[0], color[1], color[2] });
	}

	float bias = m_ShadowBias;
	if (ImGui::SliderFloat("Shadow Bias", &bias, 0.0f, 0.02f, "%.4f"))
	{
		m_ShadowBias = bias;
	}

	ImGui::End();
}
#endif
