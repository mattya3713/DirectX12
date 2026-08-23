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

	// 親子間の初期距離を制約の静止長として記録する.
	m_RestLengths.assign(m_Bodies.size(), 0.0f);
	for (size_t i = 0; i < m_pDefinition->Bones.size(); ++i)
	{
		const int parent = m_pDefinition->Bones[i].ParentIndex;
		if (parent < 0 || parent >= static_cast<int>(m_Bodies.size())) { continue; }

		const DirectX::XMFLOAT3& child  = m_Bodies[i].Position;
		const DirectX::XMFLOAT3& parent_pos = m_Bodies[static_cast<size_t>(parent)].Position;
		const float dx = child.x - parent_pos.x;
		const float dy = child.y - parent_pos.y;
		const float dz = child.z - parent_pos.z;
		m_RestLengths[i] = std::sqrt(dx * dx + dy * dy + dz * dz);
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
			body.Velocity.y = -body.Velocity.y * 0.3f;
		}
	}

	SolveConstraints();
}

// 親子間の距離を初期長へ保つ位置ベース制約を解決する(2反復).
void RagdollComponent::SolveConstraints()
{
	if (!m_pDefinition) { return; }

	const auto distance = [](const DirectX::XMFLOAT3& A, const DirectX::XMFLOAT3& B) {
		const float dx = B.x - A.x, dy = B.y - A.y, dz = B.z - A.z;
		return std::sqrt(dx * dx + dy * dy + dz * dz);
	};

	for (int iteration = 0; iteration < 2; ++iteration)
	{
		for (size_t i = 0; i < m_pDefinition->Bones.size(); ++i)
		{
			const int parent = m_pDefinition->Bones[i].ParentIndex;
			if (parent < 0 || parent >= static_cast<int>(m_Bodies.size())) { continue; }

			BodyState& child  = m_Bodies[i];
			BodyState& parent_body = m_Bodies[static_cast<size_t>(parent)];

			const float current = distance(child.Position, parent_body.Position);
			const float target  = m_RestLengths[i];
			if (current < 1e-5f || target <= 0.0f) { continue; }

			// 子を引き寄せる(親は質量比で僅かに引かれる).
			const float diff = (current - target) / current;
			child.Position.x -= (child.Position.x - parent_body.Position.x) * diff * 0.8f;
			child.Position.y -= (child.Position.y - parent_body.Position.y) * diff * 0.8f;
			child.Position.z -= (child.Position.z - parent_body.Position.z) * diff * 0.8f;
		}
	}
}

// 各ボディの位置行列を返す(DEBUG可視化・骨行列反映用).
std::vector<DirectX::XMFLOAT4X4> RagdollComponent::GetBodyMatrices() const
{
	std::vector<DirectX::XMFLOAT4X4> matrices(m_Bodies.size());
	for (size_t i = 0; i < m_Bodies.size(); ++i)
	{
		const DirectX::XMMATRIX m = DirectX::XMMatrixTranslation(
			m_Bodies[i].Position.x, m_Bodies[i].Position.y, m_Bodies[i].Position.z);
		DirectX::XMStoreFloat4x4(&matrices[i], m);
	}
	return matrices;
}