#pragma once

#include <memory>
#include <string>

#include "00_Game/50_Enemy/Definition/EnemyDefinitionCatalog.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/10_Enemy/Enemy.h"
#include "99_Utility/Transform/Transform.h"

/**********************************************************************************
* @author    : 朱雀 (Suzaku / 閃斬 Production Loop Coder).
* @date      : 2026/08/23.
* @brief     : EnemyDefinitionカタログから安全にEnemyを生成するファクトリ.
*            : 未知ID・不正定義では生成せずnullptrを返す(部分生成も登録残りも無し.
*            : 唯一の副作用であるコライダー登録はEnemyのctor内で完結しており、
*            : unique_ptr破棄時にCharacter dtor経由で解除される).
*            : 初期化規約: 構築(コライダー登録)→定義適用(ApplyTuning)→
*            : Transform設定→AI接続(Idleステートはctor内で初期化済み).
*            : モデルの割り当て(AttachMesh)はGPUを要するため呼び出し側の責務.
**********************************************************************************/

// 生成要求(解決済み定義+初期Transform). Factoryが組み立てる.
struct EnemySpawnRequest
{
	const EnemyDefinition* Definition = nullptr; // カタログ内の有効な定義への非所有参照.
	Transform              InitialTransform{};   // スポーン時のTransform.

	// 要求として成立しているか.
	bool IsValid() const noexcept { return Definition != nullptr && Definition->IsValid(); }
};

class EnemyFactory final
{
public:
	explicit EnemyFactory(const EnemyDefinitionCatalog& Catalog) noexcept
		: m_Catalog(Catalog)
	{
	}

	EnemyFactory(const EnemyFactory&)            = delete;
	EnemyFactory& operator=(const EnemyFactory&) = delete;

	// IDから定義を解決し、生成要求を組み立てる(未知ID・不正定義はfalseで何も書き込まない).
	bool TryBuildRequest(const std::string& DefinitionId, EnemySpawnRequest& OutRequest) const noexcept
	{
		const EnemyDefinition* p_definition = m_Catalog.Find(DefinitionId);
		if (!p_definition || !p_definition->IsValid()) { return false; }

		OutRequest.Definition = p_definition;
		return true;
	}

	// 要求からEnemyを生成する(未知ID・不正要求はnullptr. 定義適用→Transformの順で初期化).
	std::unique_ptr<Enemy> Create(const EnemySpawnRequest& Request) const noexcept
	{
		if (!Request.IsValid()) { return nullptr; }

		// 構築(この時点でコライダー登録とIdleステート初期化が完了する).
		// 以降の処理はnoexceptなセッターのみのため、部分生成は発生しない.
		auto enemy = std::make_unique<Enemy>();

		// 定義適用(移動速度・最大HP).
		enemy->ApplyTuning(Request.Definition->MoveSpeed, Request.Definition->MaxHP);

		// Transform設定.
		enemy->SetTransform(Request.InitialTransform);

		return enemy;
	}

private:
	const EnemyDefinitionCatalog& m_Catalog;
};
