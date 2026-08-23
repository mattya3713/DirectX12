#pragma once

#include <DirectXMath.h>

#include "99_Utility/ECS/EntityTypes.h"
#include "99_Utility/Transform/Transform.h"

/**********************************************************************************
* @author    : mattya3713 / Coder 青龍(せいりゅう).
* @date      : 2026/08/10 / 2026-08-23 ECSブリッジ追加.
* @brief     : ゲーム内オブジェクトの具象基底クラス.
*            : ECS WorldのEntityへの非所有参照(Handle)を持てる.
*            : Entityの実体・寿命はWorldが所有するため二重管理しないこと.
**********************************************************************************/

class GameObject
{
public:
	GameObject();
	virtual ~GameObject();

	GameObject(const GameObject&)            = delete;
	GameObject& operator=(const GameObject&) = delete;
	GameObject(GameObject&&)                 = delete;
	GameObject& operator=(GameObject&&)      = delete;

	virtual void Update();
	virtual void Draw();

public: // Getter・Setter.

	// Transformの取得・設定.
	const Transform& GetTransform() const noexcept { return m_Transform; }
	void SetTransform(const Transform& InTransform) noexcept { m_Transform = InTransform; }

	// 位置の取得・設定.
	const DirectX::XMFLOAT3& GetPosition() const noexcept { return m_Transform.Position; }
	void SetPosition(const DirectX::XMFLOAT3& Position) noexcept { m_Transform.Position = Position; }

	// 位置を加算(移動量の適用など、差分を積みたい場合に使う).
	void AddPosition(const DirectX::XMFLOAT3& Delta) noexcept
	{
		m_Transform.Position.x += Delta.x;
		m_Transform.Position.y += Delta.y;
		m_Transform.Position.z += Delta.z;
	}

	// 目標Yaw角(度)へ最短経路でラープ回転する(Player/Enemy共通で使うため本クラスに置く).
	void RotateToTarget(float TargetAngleDeg, float SpeedDegPerSec) noexcept;

	// ECS Entityハンドル(非所有参照. 実体はWorldが所有する).
	void SetEntityHandle(const ECS::Entity& Entity) noexcept { m_Entity = Entity; }
	const ECS::Entity& GetEntityHandle() const noexcept { return m_Entity; }

protected:
	Transform    m_Transform;              // 位置・回転・スケール.
	ECS::Entity  m_Entity{};               // ECS Entityハンドル(EntityTypes.hの既定値=無効).
};
