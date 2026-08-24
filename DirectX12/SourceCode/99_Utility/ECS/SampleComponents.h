#pragma once

#include <DirectXMath.h>

/**********************************************************************************
* @author    : Coder 青龍(せいりゅう).
* @date      : 2026/08/23.
* @brief     : ECS動作確認用の最小サンプルComponent群.
*            : (Phase5の単体テストと、雑魚敵/Ragdoll移行前の動作確認で使用する)
**********************************************************************************/

namespace ECS {

	// 位置・Yaw回転・スケールの最小Transform.
	struct TransformComponent
	{
		DirectX::XMFLOAT3 Position{ 0.0f, 0.0f, 0.0f };
		float             RotationYDeg = 0.0f;
		float             Scale        = 1.0f;
	};

	// HPの最小構成(0以下で死亡扱い. サンプルSystemから破棄要求を出す).
	struct HealthComponent
	{
		float HP    = 1.0f;
		float MaxHP = 1.0f;
	};

} // namespace ECS
