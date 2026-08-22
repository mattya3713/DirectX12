#include "Character.h"

#include "00_Game/40_Collision/CollisionDetector.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"

#if _DEBUG
#include "99_Utility/Debug/Imgui/ImGuiManager.h"
#include "10_Ggraphic/20_Render/Debug/DebugColliderRenderer.h"

#include <typeinfo>

namespace
{
	constexpr float SIZE_WARNING_RATIO_MIN = 0.5f;
	constexpr float SIZE_WARNING_RATIO_MAX = 2.0f;

	constexpr DirectX::XMFLOAT3 DAMAGE_COLLIDER_COLOR{ 0.0f, 0.4f, 1.0f }; // 被弾判定=青.
	constexpr DirectX::XMFLOAT3 ATTACK_COLLIDER_COLOR{ 1.0f, 0.15f, 0.15f }; // 攻撃判定=赤.
}
#endif

Character::Character()
	: m_Health         { 100.0f }
	, m_DamageCollider { &GetTransform() }
	, m_AttackCollider { &GetTransform() }
{
	m_DamageCollider.SetRadius(0.5f);
	m_DamageCollider.SetHeight(2.0f);
	m_DamageCollider.SetPositionOffset({ 0.0f, 1.0f, 0.0f });

	m_AttackCollider.SetRadius(1.0f);
	m_AttackCollider.SetHeight(2.0f);
	m_AttackCollider.SetPositionOffset({ 0.0f, 1.0f, 1.5f }); // 正面側に配置.
	m_AttackCollider.SetActive(false); // 攻撃系Stateが有効化するまで無効.

	if (CollisionDetector* p_detector = ServiceLocator::Get<CollisionDetector>())
	{
		p_detector->RegisterCollider(m_DamageCollider);
		p_detector->RegisterCollider(m_AttackCollider);
	}
}

Character::~Character()
{
	if (CollisionDetector* p_detector = ServiceLocator::Get<CollisionDetector>())
	{
		p_detector->UnregisterCollider(&m_DamageCollider);
		p_detector->UnregisterCollider(&m_AttackCollider);
	}
}

void Character::Update()
{
	MeshObject::Update();
	ProcessHits();
}

#if _DEBUG
void Character::Draw()
{
	MeshObject::Draw();

	const float local_height = GetLocalHeight();
	if (local_height <= 0.0f) { return; }

	const float expected_height = m_DamageCollider.GetHeight();
	if (expected_height <= 0.0f) { return; }

	const float world_height = local_height * GetTransform().Scale.y;
	const float ratio = world_height / expected_height;

	if (ratio < SIZE_WARNING_RATIO_MIN || ratio > SIZE_WARNING_RATIO_MAX)
	{
		const std::string window_title = std::string("Model Size Warning: ") + typeid(*this).name();
		ImGui::Begin(window_title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::TextColored(
			ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
			ratio > 1.0f ? IMGUI_JP("モデルが大きすぎます。") : IMGUI_JP("モデルが小さすぎます。"));
		ImGui::Text("Height: %.2f / Expected: %.2f (x%.2f)", world_height, expected_height, ratio);
		ImGui::End();
	}
}

void Character::DrawDebugColliders() const
{
	DrawColliderDebug(m_DamageCollider, DAMAGE_COLLIDER_COLOR);
	DrawColliderDebug(m_AttackCollider, ATTACK_COLLIDER_COLOR);
}

void Character::DrawColliderDebug(const CapsuleCollider& Collider, const DirectX::XMFLOAT3& Color) const
{
	if (!Collider.GetActive()) { return; }

	DebugColliderRenderer* p_collider_renderer = ServiceLocator::Get<DebugColliderRenderer>();
	if (p_collider_renderer == nullptr) { return; }

	DirectX::XMFLOAT3 start{};
	DirectX::XMFLOAT3 end{};
	DirectX::XMStoreFloat3(&start, Collider.GetSegmentStart());
	DirectX::XMStoreFloat3(&end, Collider.GetSegmentEnd());
	p_collider_renderer->DrawCapsule(start, end, Collider.GetRadius(), Color);
}
#endif

void Character::ProcessHits()
{
	for (const CollisionInfo& info : m_DamageCollider.GetCollisionEvents())
	{
		if (!info.IsHit) { continue; }

		HitEvent hit_event{};
		hit_event.AttackAmount = info.AttackAmount;
		hit_event.ContactPoint = info.ContactPoint;
		hit_event.Normal       = info.Normal;

		ApplyDamage(hit_event);
	}
}
