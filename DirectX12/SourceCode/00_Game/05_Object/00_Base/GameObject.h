#pragma once

#include <DirectXMath.h>

#include "00_Game/05_Object/00_Base/IUpdatable.h"
#include "00_Game/05_Object/00_Base/IDrawable.h"
#include "99_Utility/Transform/Transform.h"

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/10.
* @brief     : ゲーム内オブジェクトの具象基底クラス. 純粋インターフェースにはせず、
*            : Transformの保持とUpdate/Drawフックの実装を持つ(継承前提のため
*            : コピー・ムーブは禁止しスライシングを防ぐ).
*            : 横断的関心事(HP等)は本クラスに詰め込まず、小さいインターフェースを
*            : 個別に多重継承させて乗せる方針(例: class Character : public GameObject, public IHealthSystem).
**********************************************************************************/

class GameObject : public IUpdatable, public IDrawable
{
public:
	GameObject();
	virtual ~GameObject();

	GameObject(const GameObject&)            = delete;
	GameObject& operator=(const GameObject&) = delete;
	GameObject(GameObject&&)                 = delete;
	GameObject& operator=(GameObject&&)      = delete;

	// 毎フレーム更新(既定では何もしない. 派生クラスでオーバーライドする).
	void Update() override;

	// 描画(既定では何もしない. 派生クラスでオーバーライドする).
	void Draw() override;

public: // Getter・Setter.

	// Transformの取得・設定.
	const Transform& GetTransform() const noexcept { return m_Transform; }
	void SetTransform(const Transform& InTransform) noexcept { m_Transform = InTransform; }

	// 位置の取得・設定.
	const DirectX::XMFLOAT3& GetPosition() const noexcept { return m_Transform.Position; }
	void SetPosition(const DirectX::XMFLOAT3& Position) noexcept { m_Transform.Position = Position; }

protected:
	Transform m_Transform; // 位置・回転・スケール.
};
