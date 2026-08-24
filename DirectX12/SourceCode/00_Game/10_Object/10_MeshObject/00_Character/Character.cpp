#include "Character.h"

#include "00_Game/00_GameLoop/Time/Time.h"
#include "00_Game/40_Collision/CollisionDetector.h"
#include "00_Game/60_Combat/CombatEvents.h"
#include "00_Game/60_Combat/CombatTuning.h"
#include "10_Ggraphic/20_Render/Particle/ParticleSystem.h"
#include "99_Utility/Event/EventBus.h"
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
	, m_BodyCollider   { &GetTransform() }
{
	m_DamageCollider.SetRadius(0.5f);
	m_DamageCollider.SetHeight(2.0f);
	m_DamageCollider.SetPositionOffset({ 0.0f, 1.0f, 0.0f });

	m_AttackCollider.SetRadius(1.0f);
	m_AttackCollider.SetHeight(2.0f);
	m_AttackCollider.SetPositionOffset({ 0.0f, 1.0f, 1.5f }); // 正面側に配置.
	m_AttackCollider.SetActive(false); // 攻撃系Stateが有効化するまで無効.

	// 実体判定(押し出し専用). グループは派生クラスがSetBodyCollisionMasks()で設定する.
	m_BodyCollider.SetRadius(0.5f);
	m_BodyCollider.SetHeight(2.0f);
	m_BodyCollider.SetPositionOffset({ 0.0f, 1.0f, 0.0f });
	m_BodyCollider.SetActive(true); // 常時アクティブ(すり抜け防止用).

	if (CollisionDetector* p_detector = ServiceLocator::Get<CollisionDetector>())
	{
		p_detector->RegisterCollider(m_DamageCollider);
		p_detector->RegisterCollider(m_AttackCollider);
		p_detector->RegisterCollider(m_BodyCollider);
	}
}

Character::~Character()
{
	if (CollisionDetector* p_detector = ServiceLocator::Get<CollisionDetector>())
	{
		p_detector->UnregisterCollider(&m_DamageCollider);
		p_detector->UnregisterCollider(&m_AttackCollider);
		p_detector->UnregisterCollider(&m_BodyCollider);
	}
}

void Character::Update()
{
	MeshObject::Update();
	ProcessHits();
	ProcessBodyCollisions();
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

	// 実際の描画は行わず、DebugColliderRendererへ情報を登録するだけ
	// (描画はMainSceneのDraw末尾でDebugColliderRenderer::Draw()がまとめて行う).
	DirectX::XMFLOAT3 start{};
	DirectX::XMFLOAT3 end{};
	DirectX::XMStoreFloat3(&start, Collider.GetSegmentStart());
	DirectX::XMStoreFloat3(&end, Collider.GetSegmentEnd());
	p_collider_renderer->RegisterCapsule(start, end, Collider.GetRadius(), Color);
}
#endif

void Character::ProcessHits()
{
	for (const CollisionInfo& info : m_DamageCollider.GetCollisionEvents())
	{
		if (!info.IsHit) { continue; }

		// 同一スイング(相手攻撃コライダーの同一有効化)中の再ヒットは無視する
		// (重なっているフレームの間ダメージが毎フレーム入ることを防ぐ).
		const auto it = m_ProcessedAttackIds.find(info.OtherCollider);
		if (it != m_ProcessedAttackIds.end() && it->second >= info.AttackActivationId) { continue; }
		m_ProcessedAttackIds[info.OtherCollider] = info.AttackActivationId;

		// 派生クラスが無視を指定したヒットはダメージ適用しない
		// (パリィ成立済み攻撃の二重処理防止. 記録は上で済ませるため同一スイング中は二度と入らない).
		if (ShouldIgnoreHit(info)) { continue; }

		HitEvent hit_event{};
		hit_event.AttackAmount = info.AttackAmount;
		hit_event.ContactPoint = info.ContactPoint;
		hit_event.Normal       = info.Normal;

		ApplyDamage(hit_event);

		// ヒットストップ(命中演出. ほぼ停止に近いスケールを一瞬だけ. Player/Boss対称).
		GameTime::SetTimeScale(CombatTuning::Get().HitStopScale, CombatTuning::Get().HitStopDuration);

		// Combat基礎SEイベント(EventBus経由の疎結合配信. パリィ済み攻撃は上で除外済みのため発火しない).
		// Player被弾だけは専用SEを当てる(読み合いの勝ち/負けを音で判別できるようにするため).
		if (EventBus* p_event_bus = ServiceLocator::Get<EventBus>())
		{
			const bool victim_is_player = (m_DamageCollider.GetMyMask() & eCollisionGroup::PlayerDamage) != eCollisionGroup::None;
			if (victim_is_player)
			{
				p_event_bus->Publish(PlayerDamagedEvent{ this, hit_event.ContactPoint });
			}
			else
			{
				p_event_bus->Publish(CombatHitEvent{ this, hit_event.ContactPoint });
			}
		}
	}
}

void Character::ProcessBodyCollisions()
{
	// 押し出し専用の実体判定. 重なった相手とめり込み深さを半分ずつ分担して離れる.
	// (NormalはSelfCollider→OtherCollider方向のため、自分は法線の逆方向へ移動する).
	for (const CollisionInfo& info : m_BodyCollider.GetCollisionEvents())
	{
		if (!info.IsHit) { continue; }

		const float push = info.PenetrationDepth * 0.5f;

		// 水平成分のみ押し出す(Y押し出しは地面コライダーが無い現状では沈み込み・浮きの原因になるため).
		AddPosition({ -info.Normal.x * push, 0.0f, -info.Normal.z * push });
	}
}

// ワールド座標指定のエフェクト再生(パーティクルシステムへ接続. 現在はヒットエフェクトのみ).
void Character::PlayEffectAtWorldPos(const std::string& Name, const DirectX::XMFLOAT3& WorldPos, float Scale, bool IsUI)
{
	(void)Name; (void)Scale; (void)IsUI; // v1は名前・スケール未対応の固定ヒットエフェクト.

	if (ParticleSystem* p_particle_system = ServiceLocator::Get<ParticleSystem>()) {
		p_particle_system->SpawnHitEffect(WorldPos);
	}
}