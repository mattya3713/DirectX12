#pragma once

#include <DirectXMath.h>

#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/PlayerStateBase.h"

namespace PlayerState {

	/**********************************************************************************
	* @author    : mattya3713.
	* @date      : 2026/08/11.
	* @brief     : 回避系ステートの基底. 回避方向の決定と、回避中の無敵化(被弾判定OFF)を
	*            : 担当する. SenzanはCollisionDetectorへの登録/解除で無敵を実現していたが、
	*            : Parryは別の方式(SetActive(false))を使っており2種類の無敵化手段が混在
	*            : していたため、本プロジェクトではSetDamageColliderActive()に統一した.
	**********************************************************************************/

	class Dodge : public PlayerStateBase
	{
	public:
		explicit Dodge(Player* pOwner) noexcept;
		~Dodge() override = default;

		void Enter() override;
		void Exit() override;

	protected:
		DirectX::XMFLOAT2 m_InputVec    { 0.0f, 0.0f }; // 回避方向(XZ平面、正規化済み).
		float              m_Distance    = 25.0f; // 移動距離(派生クラスで上書きする).
		float              m_MaxTime     = 1.7f;  // 所要時間(秒、派生クラスで上書きする).
		float              m_CurrentTime = 0.0f;  // Enterからの経過時間(秒).
	};

} // namespace PlayerState
