#pragma once

#include <DirectXMath.h>
#include <vector>

#include "RagdollDefinition.h"

// ラグドールのライフサイクル管理コンポーネント.
// Definitionを指定してActivateすると物理姿勢へ切り替わり、
// Deactivate/Resetで元の状態へ戻る。二重生成・残留状態を作らない.
class RagdollComponent final
{
public:
	// 1リジッドボディ分の動的状態.
	struct BodyState
	{
		DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };
		DirectX::XMFLOAT3 Velocity = { 0.0f, 0.0f, 0.0f };
	};

	// 定義を設定する(所有は呼び出し側. 生存期間はComponentより長くすること).
	void SetDefinition(const RagdollDefinition* pDefinition);

	const RagdollDefinition* GetDefinition() const noexcept { return m_pDefinition; }

	// アニメーション姿勢(ボーンワールド位置)を受け取り、物理姿勢へ引き継いで活性化する.
	// 既にActiveの場合はfalse(二重生成しない). Definition未設定/空もfalse.
	bool Activate(const std::vector<DirectX::XMFLOAT3>& BoneWorldPositions);

	// 非活性化して内部状態を破棄する(非Active時はfalse).
	bool Deactivate();

	// 状態を初期化する(Deactivateと同義だが冪等).
	void Reset();

	bool IsActive() const noexcept { return m_State == eState::Active; }

	// 物理ステップを進める(重力+減衰の簡易積分. Active時のみ).
	void Update(float DeltaTime, float Gravity = 20.0f, float DampingRate = 0.5f);

	const std::vector<BodyState>& GetBodyStates() const noexcept { return m_Bodies; }

private:
	enum class eState
	{
		Inactive,
		Active,
	};

	const RagdollDefinition* m_pDefinition = nullptr;
	eState                   m_State       = eState::Inactive;
	std::vector<BodyState>   m_Bodies;      // Definition.Bonesと同じ順序.
};
