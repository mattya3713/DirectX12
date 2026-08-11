#pragma once

#include <DirectXMath.h>

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/11.
* @brief     : ダメージ処理へ渡す最小限の被弾情報. CollisionInfoから組み立てる.
*            : Characterが自分の被弾コライダーのCollisionInfoを毎フレーム監視し、
*            : ヒットを見つけたらHitEventに変換してApplyDamage()へ渡す(ポーリング方式.
*            : Senzanには「HitEvent」に相当する概念は存在せず、Player側で同様のポーリングを
*            : 直接書いていた. 本プロジェクトではCharacter側の共通処理にして
*            : Player/Enemy/Boss全てで使い回せるようにした).
**********************************************************************************/

struct HitEvent
{
	float AttackAmount = 0.0f;			// 攻撃力.
	DirectX::XMFLOAT3 ContactPoint = {};	// 接触点(ワールド座標).
	DirectX::XMFLOAT3 Normal = {};			// 衝突法線(自分→相手方向).
};
