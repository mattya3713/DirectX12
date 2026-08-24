#pragma once

#include <DirectXMath.h>

#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/State/BossStateBase.h"

namespace BossState {

	/**********************************************************************************
	* @author    : mattya3713.
	* @date      : 2026/08/13.
	* @brief     : パリィされた直後のBossのリアクション. CombatCoordinatorから渡された
	*            : 目標位置・向きへ遷移し(位置はEasing、向きは既存のRotateToTargetで
	*            : 最短経路ラープ)、終わったらMove(見失っていればIdle)へ戻る.
	*            : 攻撃判定は硬直として無効化する(DurationはPlayer反応より延長されており、
	*            : この間がPlayerの確定反撃時間. 硬直中に攻撃を命中させるとBossは吹き飛ぶ).
	*            : 汎用のBoss::ChangeState(BossState::eID)では目標データを渡せないため、
	*            : このステートはBoss::EnterParryReaction()経由でのみ入る(専用エントリ).
	**********************************************************************************/

	class ParryReaction final : public BossStateBase
	{
	public:
		ParryReaction(Boss* pOwner, const DirectX::XMFLOAT3& TargetPosition, float TargetYawDeg, float Duration) noexcept;
		~ParryReaction() override = default;

		BossState::eID GetStateID() const override { return BossState::eID::ParryReaction; }

		void Enter() override;
		void Update() override;

	private:
		DirectX::XMFLOAT3 m_StartPosition {};    // Enter時点のBoss位置(補間の始点).
		DirectX::XMFLOAT3 m_TargetPosition;      // 補間の終点(CombatCoordinator計算値).
		float m_TargetYawDeg;                    // 目標Yaw角(度).
		float m_Duration;                        // 遷移にかける時間(秒).
		float m_ElapsedTime = 0.0f;              // Enterからの経過時間(秒).
		DirectX::XMFLOAT3 m_KnockBackVelocity { 0.0f, 0.0f, 0.0f }; // 硬直中のヒットで受けた吹き飛び残速(減衰しながら位置へ加算. 水平のみ).
	};

} // namespace BossState
