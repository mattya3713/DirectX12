#include "RagdollComponent.h"

#include <cmath>

// 定義を設定する.
void RagdollComponent::SetDefinition(const RagdollDefinition* pDefinition)
{
	// Active中の差し替えは状態破綻のもとになるため、Inactive時のみ許可する.
	if (m_State == eState::Active) { return; }

	m_pDefinition = pDefinition;
}

// アニメーション姿勢から物理姿勢へ引き継いで活性化する.
bool RagdollComponent::Activate(const std::vector<DirectX::XMFLOAT3>& BoneWorldPositions)
{
	if (m_State == eState::Active) { return false; }         // 二重生成しない.
	if (!m_pDefinition || m_pDefinition->IsEmpty()) { return false; }

	m_Bodies.clear();
	for (const RagdollBoneDesc& bone : m_pDefinition->Bones)
	{
		BodyState state;
		state.Position = (m_Bodies.size() < BoneWorldPositions.size())
			? BoneWorldPositions[m_Bodies.size()]
			: DirectX::XMFLOAT3{ 0.0f, 0.0f, 0.0f };
		state.Velocity = { 0.0f, 0.0f, 0.0f };
		m_Bodies.push_back(state);
	}

	m_State = eState::Active;
	return true;
}

// 非活性化する.
bool RagdollComponent::Deactivate()
{
	if (m_State != eState::Active) { return false; }

	m_Bodies.clear();
	m_State = eState::Inactive;
	return true;
}

// 状態を初期化する(冪等).
void RagdollComponent::Reset()
{
	m_Bodies.clear();
	m_State = eState::Inactive;
}

// 物理ステップ(重力+減衰の簡易積分).
void RagdollComponent::Update(float DeltaTime, float Gravity, float DampingRate)
{
	if (m_State != eState::Active || DeltaTime <= 0.0f) { return; }

	for (BodyState& body : m_Bodies)
	{
		body.Velocity.y -= Gravity * DeltaTime;

		const float damping = std::exp(-DampingRate * DeltaTime);
		body.Velocity.x *= damping;
		body.Velocity.y *= damping;
		body.Velocity.z *= damping;

		body.Position.x += body.Velocity.x * DeltaTime;
		body.Position.y += body.Velocity.y * DeltaTime;
		body.Position.z += body.Velocity.z * DeltaTime;

		// 仮地面(簡易衝突. 本実装はCollider/Constraint拡張で置き換える).
		if (body.Position.y < 0.0f)
		{
			body.Position.y = 0.0f;
			body.Velocity.y = 0.0f;
		}
	}
}
