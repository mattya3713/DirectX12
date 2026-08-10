#pragma once

#include "00_Game/40_Collision/00_Core/CollisionInfo.h"

class BoxCollider;
class CapsuleCollider;
class SphereCollider;

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/11.
* @brief     : 形状の組み合わせごとの衝突判定の実体(狭域判定). ColliderBase::DispatchCollision
*            : は必ずここへ「決まった引数順」で委譲すること. どちらの形状のCheckCollisionから
*            : 呼ばれても同じ関数・同じ引数順で計算されるため、結果が呼び出し方向に依存しない
*            : (Senzanは形状ペアの片方向にしか判定の実装が無く、コライダーの登録順次第で
*            : 判定が抜け落ちるバグがあったため、その対策として関数を一本化した).
*            :
*            : 法線は全て第1引数→第2引数の方向を正とする.
**********************************************************************************/

namespace CollisionMath {

	CollisionInfo TestCapsuleVsCapsule(const CapsuleCollider& A, const CapsuleCollider& B);
	CollisionInfo TestCapsuleVsSphere(const CapsuleCollider& Capsule, const SphereCollider& Sphere);
	CollisionInfo TestSphereVsSphere(const SphereCollider& A, const SphereCollider& B);
	CollisionInfo TestBoxVsBox(const BoxCollider& A, const BoxCollider& B);
	CollisionInfo TestBoxVsSphere(const BoxCollider& Box, const SphereCollider& Sphere);
	CollisionInfo TestBoxVsCapsule(const BoxCollider& Box, const CapsuleCollider& Capsule);

} // namespace CollisionMath
