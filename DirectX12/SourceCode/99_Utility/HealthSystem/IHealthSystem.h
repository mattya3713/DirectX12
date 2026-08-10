#pragma once

#include <functional>

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/10.
* @brief     : HPを持つオブジェクトのインターフェース. HP等の横断的関心事はこのような
*            : 小さいインターフェースを多重継承させて乗せる方針(DESIGN.md参照).
*            : ダメージ時・死亡時の処理はコールバック(std::function)で外部から設定できる.
**********************************************************************************/

class IHealthSystem
{
public:
	using DamageCallback = std::function<void(float)>;	// 引数: 実際に与えられたダメージ量.
	using DeathCallback  = std::function<void()>;

	virtual ~IHealthSystem() = default;

	// 最大HPの取得.
	virtual float GetMaxHP() const noexcept = 0;
	// 現在HPの取得.
	virtual float GetHP() const noexcept = 0;
	// 生存しているか.
	virtual bool IsAlive() const noexcept = 0;
	// ダメージを与える(内部でOnDamage/OnDeathコールバックを呼ぶ).
	virtual void ApplyDamage(float DamageAmount) = 0;

	// ダメージを受けた時のコールバックを設定する.
	virtual void SetOnDamage(DamageCallback Callback) = 0;
	// 死亡した瞬間(生存→死亡に変化した時)のコールバックを設定する.
	virtual void SetOnDeath(DeathCallback Callback) = 0;
};
