#pragma once

#include <DirectXMath.h>

class Character;

/**********************************************************************************
* @author    : Coder 朱雀(閃斬 Production Loop).
* @date      : 2026/08/24.
* @brief     : Combat基礎SE用のEventBusイベント定義(攻撃ヒット/パリィ成功/Player被弾の3種).
*            : 発火は成立が確定した地点(Character::ProcessHits/CombatCoordinator::OnParrySuccess)
*            : のみに限定し、再生側(起動時にMainで購読)と疎結合に繋ぐ. フレームずれ防止のため
*            : アニメーションUpdate内のタイマー判定からは発火させないこと.
**********************************************************************************/

// 攻撃ヒット成立時に発行(Player攻撃→Enemy/Boss着弾など、Player被弾以外の全ヒット. SE/UI等の購読想定).
struct CombatHitEvent
{
	class Character* Victim = nullptr; // 被弾したキャラクター(非所有).
	DirectX::XMFLOAT3 ContactPoint{};  // ヒット接触点(ワールド座標. 将来のエフェクト/距離減衰用).
};

// Player被弾確定時に発行(Enemy/Boss攻撃がPlayerに通った瞬間. パリィ済み攻撃はShouldIgnoreHitで除外されるため発行されない).
struct PlayerDamagedEvent
{
	class Character* Victim = nullptr; // 被弾したPlayer(非所有).
	DirectX::XMFLOAT3 ContactPoint{};  // ヒット接触点(ワールド座標).
};

// パリィ成功時に発行(PlayerState::Parryの検出をCombatCoordinator::OnParrySuccess()が受けた瞬間.
// 購読者は音の種別だけを必要とするため付随データは無し. 必要になった時点で拡張する).
struct ParrySuccessEvent
{
};
