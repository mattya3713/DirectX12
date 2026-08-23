#pragma once

#include <DirectXMath.h>

/**********************************************************************************
* @author    : Coder.
* @date      : 2026/08/23.
* @brief     : 重力・減衰・速度積分をまとめた最小限の物理コンポーネント.
*            : GameObjectの継承ではなくメンバとして持たせる(HealthSystemと同じ
*            : コンポジション方式).
**********************************************************************************/

class PhysicsBody
{
public:
	PhysicsBody() noexcept;
	~PhysicsBody() noexcept = default;

	PhysicsBody(const PhysicsBody&)            = delete;
	PhysicsBody& operator=(const PhysicsBody&) = delete;

public: // Getter・Setter.

	// 現在の速度の取得.
	const DirectX::XMFLOAT3& GetVelocity() const noexcept { return m_Velocity; }
	// 速度の設定(ノックバック初速等の代入用).
	void SetVelocity(const DirectX::XMFLOAT3& Velocity) noexcept { m_Velocity = Velocity; }

public: // 更新処理(dtは秒. 各メソッドは呼び出し順に作用する).

	// 重力を速度Y成分へ加算する(下方向が-Y).
	void ApplyGravity(float DeltaTime, float Gravity) noexcept;

	// 水平速度(X/Z)を指数減衰させる(フレームレート非依存).
	void ApplyDamping(float DeltaTime, float DampingRate) noexcept;

	// 現在の速度からフレーム内の移動量を算出して返す(位置への反映は呼び出し側で行う).
	DirectX::XMFLOAT3 Integrate(float DeltaTime) const noexcept;

private:
	DirectX::XMFLOAT3 m_Velocity{ 0.0f, 0.0f, 0.0f }; // 現在の速度(単位/秒).
};
