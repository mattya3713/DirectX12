#pragma once

#include <DirectXMath.h>
#include <vector>

#include "00_Game/40_Collision/00_Core/CollisionInfo.h"

struct Transform;
class BoxCollider;
class CapsuleCollider;
class SphereCollider;

// 当たり判定のグループ(ビットフラグ). Combat系のStateを実装するタイミングで
// 攻撃判定/被弾判定を追加した. 必要になったら随時追加していく.
enum class eCollisionGroup : uint32_t
{
	None    = 0,
	Default = 1 << 0,

	PlayerAttack = 1 << 1, // Playerの攻撃判定.
	PlayerDamage = 1 << 2, // Playerの被弾判定.
	EnemyAttack  = 1 << 3, // Enemy/Bossの攻撃判定.
	EnemyDamage  = 1 << 4, // Enemy/Bossの被弾判定.

	_Max = 0xFFFFFFFF,
};
DEFINE_ENUM_FLAG_OPERATORS(eCollisionGroup)

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/11.
* @brief     : 当たり判定の基底クラス. 形状ごとの判定は二重ディスパッチ(DispatchCollision)
*            : でCapsuleCollider/SphereColliderへ委譲する.
* @pattern   : Visitor(二重ディスパッチ).
**********************************************************************************/

class ColliderBase
{
public:
	// 形状派生クラスからDispatchCollisionを呼べるようにフレンド登録.
	friend class BoxCollider;
	friend class CapsuleCollider;
	friend class SphereCollider;

public:
	// 当たり判定の形状.
	enum class eShapeType : uint32_t
	{
		Box = 0,
		Capsule,
		Sphere,
		_Max
	};

public:
	explicit ColliderBase(const Transform* pOwnerTransform) noexcept;
	virtual ~ColliderBase() = default;

	ColliderBase(const ColliderBase&)            = delete;
	ColliderBase& operator=(const ColliderBase&) = delete;
	ColliderBase(ColliderBase&&)                 = delete;
	ColliderBase& operator=(ColliderBase&&)      = delete;

	// 毎フレーム更新(形状によっては不要. 既定では何もしない).
	virtual void Update() {}

	// 自身の形状種別.
	virtual eShapeType GetShapeType() const noexcept = 0;

	// ワールド座標(オフセットを持ち主の回転(Yaw)で回転させてから加算したもの).
	DirectX::XMFLOAT3 GetPosition() const noexcept;

public: // Getter・Setter.

	// オフセット座標の取得・設定.
	const DirectX::XMFLOAT3& GetPositionOffset() const noexcept { return m_PositionOffset; }
	void SetPositionOffset(const DirectX::XMFLOAT3& PositionOffset) noexcept { m_PositionOffset = PositionOffset; }

	// 有効か否かの取得・設定.
	bool GetActive() const noexcept { return m_IsActive; }
	void SetActive(bool IsActive) noexcept { m_IsActive = IsActive; }

	// 攻撃力の取得・設定.
	float GetAttackAmount() const noexcept { return m_AttackAmount; }
	void SetAttackAmount(float AttackAmount) noexcept { m_AttackAmount = AttackAmount; }

	// 衝突情報の追加・取得・クリア(CollisionDetectorが毎フレーム更新する).
	void AddCollisionInfo(const CollisionInfo& Info) noexcept { m_CollisionEvents.push_back(Info); }
	const std::vector<CollisionInfo>& GetCollisionEvents() const noexcept { return m_CollisionEvents; }
	void ClearCollisionEvents() noexcept { m_CollisionEvents.clear(); }

	// グループマスクの取得・設定.
	void SetMyMask(eCollisionGroup MyMask) noexcept { m_MyMask = MyMask; }
	void SetTargetMask(eCollisionGroup TargetMask) noexcept { m_TargetMask = TargetMask; }
	eCollisionGroup GetMyMask() const noexcept { return m_MyMask; }
	eCollisionGroup GetTargetMask() const noexcept { return m_TargetMask; }

	// 相手と衝突すべきか(お互いのマスクが噛み合っているかを両方向でチェック).
	bool ShouldCollide(const ColliderBase& Other) const noexcept
	{
		const bool a_targets_b = (m_TargetMask & Other.m_MyMask) != eCollisionGroup::None;
		const bool b_targets_a = (Other.m_TargetMask & m_MyMask) != eCollisionGroup::None;
		return a_targets_b && b_targets_a;
	}

	// 他のColliderとの衝突判定.
	virtual CollisionInfo CheckCollision(const ColliderBase& Other) const = 0;

protected:
	// 形状ごとの衝突処理(二重ディスパッチ).
	// NOTE: 実装はCollisionMath.hの共通関数へ委譲し、必ず同じ引数順で呼ぶこと.
	//       (どちらの形状からDispatchされても同じ計算結果になるようにするため.
	//       参考にしたSenzanの実装は片方向にしか実装が無く、コライダーの登録順次第で
	//       同じ組み合わせでも判定が抜け落ちるバグがあった).
	virtual CollisionInfo DispatchCollision(const BoxCollider& Other) const = 0;
	virtual CollisionInfo DispatchCollision(const CapsuleCollider& Other) const = 0;
	virtual CollisionInfo DispatchCollision(const SphereCollider& Other) const = 0;

protected:
	const Transform* m_pOwnerTransform; // 持ち主のTransform(非所有. 持ち主と同じ寿命であることが前提).

	DirectX::XMFLOAT3 m_PositionOffset { 0.0f, 0.0f, 0.0f };
	bool  m_IsActive     = true;
	float m_AttackAmount = 0.0f;

	eCollisionGroup m_MyMask     = eCollisionGroup::_Max; // 自身が所属するグループ.
	eCollisionGroup m_TargetMask = eCollisionGroup::_Max; // 衝突対象とするグループ.

	std::vector<CollisionInfo> m_CollisionEvents; // 検出された衝突情報のリスト(1フレーム分).
};
