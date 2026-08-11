#pragma once

#include <DirectXMath.h>

#include "99_Utility/Transform/Transform.h"

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/10.
* @brief     : ゲーム内オブジェクトの具象基底クラス. 純粋インターフェースにはせず、
*            : Transformの保持とUpdate/Drawフックの実装を持つ(継承前提のため
*            : コピー・ムーブは禁止しスライシングを防ぐ).
*            : Update/DrawはIUpdatable/IDrawableのような別インターフェースに分けず、
*            : 本クラスの仮想関数として直接持たせる(GameObjectの外側でUpdateだけ
*            : 欲しいクラスが出てきたら改めて検討する).
*            : 横断的関心事(HP等)は本クラスに詰め込まず、小さいインターフェースを
*            : 個別に多重継承させて乗せる方針(例: class Character : public GameObject, public IHealthSystem).
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

	// 毎フレーム更新(既定では何もしない. 派生クラスでオーバーライドする).
	virtual void Update();

	// 描画(既定では何もしない. 派生クラスでオーバーライドする).
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

protected:
	Transform m_Transform; // 位置・回転・スケール.
};
