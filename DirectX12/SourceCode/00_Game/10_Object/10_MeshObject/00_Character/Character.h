#pragma once

#include <DirectXMath.h>
#include <string>

#include "00_Game/10_Object/10_MeshObject/MeshObject.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/CharacterAccessKeys.h"
#include "99_Utility/HealthSystem/HealthSystem.h"

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/10.
* @brief     : ゲーム内キャラクターの基底クラス. MeshObjectを継承する具象クラス
*            : (見た目(PMXMesh)を持てる). HPはHealthSystemをメンバとして持つ
*            : (コンポジション. IHealthSystemを直接継承しない理由はHealthSystem.h参照).
*            : 静的な小道具やトリガー等、生命・行動を持たないGameObjectと区別するための層.
**********************************************************************************/

class Character : public MeshObject
{
public:
	Character();
	virtual ~Character();

	Character(const Character&)            = delete;
	Character& operator=(const Character&) = delete;
	Character(Character&&)                 = delete;
	Character& operator=(Character&&)      = delete;

public:
	// HP関連の情報取得.
	const HealthSystem& GetHealth() const noexcept { return m_Health; }

public: // エフェクト再生(フックのみ. 中身は未実装 — Effekseer/自作パーティクル等、方式決定後に実装する).

	// 自分の位置からの相対オフセットで再生する.
	void PlayEffect(const std::string& Name, const DirectX::XMFLOAT3& Offset = { 0.0f, 0.0f, 0.0f }, float Scale = 1.0f, bool IsUI = false) {}

	// ワールド座標を指定して再生する.
	void PlayEffectAtWorldPos(const std::string& Name, const DirectX::XMFLOAT3& WorldPos, float Scale = 1.0f, bool IsUI = false) {}

	// ワールド座標+回転を指定して再生する.
	void PlayEffectAtWorldPos(const std::string& Name, const DirectX::XMFLOAT3& WorldPos, const DirectX::XMFLOAT3& EulerRotation, float Scale = 1.0f, bool IsUI = false) {}

	// スクリーン座標を指定してUIエフェクトを再生する.
	void PlayEffectUIAtScreenPos(const std::string& Name, const DirectX::XMFLOAT2& ScreenPos, float Scale = 1.0f) {}

protected:

	// ダメージ/死亡コールバックの登録
	void SetOnDamage(HealthSystem::DamageCallback Callback) { m_Health.SetOnDamage(std::move(Callback)); }
	void SetOnDeath(HealthSystem::DeathCallback Callback) { m_Health.SetOnDeath(std::move(Callback)); }

protected:
	HealthSystem m_Health; // HP・ダメージ処理・コールバック.
};
