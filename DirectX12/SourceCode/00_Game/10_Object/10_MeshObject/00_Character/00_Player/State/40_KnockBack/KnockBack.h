#pragma once

#include <DirectXMath.h>

#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/PlayerStateBase.h"

namespace PlayerState {

	/**********************************************************************************
	* @author    : mattya3713.
	* @date      : 2026/08/23.
	* @brief     : 被弾ノックバック. 吹き飛び方向へ水平初速+垂直初速を与え、
	*            : 重力と減衰で放物線移動させ、地面(Y=0)に着地したらIdleへ戻る.
	*            : player.msknに被弾用クリップが存在しないため、アニメーション変更は
	*            : 行わず物理移動のみで表現する(空クリップ指定時のバインドポーズ
	*            : フォールバック仕様の回避).
	**********************************************************************************/

	class KnockBack final : public PlayerStateBase
	{
	public:
		explicit KnockBack(Player* pOwner) noexcept;
		~KnockBack() override = default;

		PlayerState::eID GetStateID() const override { return PlayerState::eID::KnockBack; }

		void Enter() override;
		// ノックバック中は操作割り込み(Attack/Dodge/Parry)を受け付けない.
		void Update() override {}
		void LateUpdate() override;

	private:
		DirectX::XMFLOAT3 m_Velocity{ 0.0f, 0.0f, 0.0f }; // 現在の速度(Playerが保持する初速をEnterで受け取る).
	};

} // namespace PlayerState
