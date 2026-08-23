#pragma once

#include <string>

#include "json/json.hpp"

/**********************************************************************************
* @author    : 朱雀 (Suzaku / 閃斬 Production Loop Coder).
* @date      : 2026/08/23.
* @brief     : 敵種別1体分の静的定義(ID・表示名・モデル・最大HP・移動速度・
*            : 攻撃参照・Ragdoll定義ID). JSONとの相互変換はfrom_json/to_json
*            : (ADL規約)で提供し、DataTable<EnemyDefinition>へそのまま格納できる.
*            : AIやStateMachine等の振る舞いは含まない(純粋な定義データ).
**********************************************************************************/

struct EnemyDefinition
{
	std::string Id;           // 種別ID(カタログ検索キー. 例: "goblin", "goblin_fast").
	std::string DisplayName;  // デバッグ表示用の名前.
	std::string ModelId;      // 使用モデルの識別子(将来ResourceCatalogから解決).
	float       MaxHP        = 100.0f;
	float       MoveSpeed    = 3.0f;
	std::string AttackRef;    // 攻撃定義への参照キー(将来の攻撃カタログ用. 空なら攻撃なし).
	std::string RagdollDefID; // Ragdoll定義への参照キー(空ならRagdoll無効).

	// 値として成立しているか(不正データのカタログ混入を防ぐための最小検証).
	bool IsValid() const noexcept
	{
		return !Id.empty() && MaxHP > 0.0f && MoveSpeed >= 0.0f;
	}
};

// nlohmann相互変換(ADL. 欠損キーは既定値で補完する).
inline void from_json(const nlohmann::json& Data, EnemyDefinition& Def)
{
	Def.Id          = Data.value("Id", std::string());
	Def.DisplayName = Data.value("DisplayName", Def.Id);
	Def.ModelId     = Data.value("ModelId", std::string());
	Def.MaxHP       = Data.value("MaxHP", 100.0f);
	Def.MoveSpeed   = Data.value("MoveSpeed", 3.0f);
	Def.AttackRef   = Data.value("AttackRef", std::string());
	Def.RagdollDefID = Data.value("RagdollDefId", std::string());
}

inline void to_json(nlohmann::json& Data, const EnemyDefinition& Def)
{
	Data["Id"]          = Def.Id;
	Data["DisplayName"] = Def.DisplayName;
	Data["ModelId"]     = Def.ModelId;
	Data["MaxHP"]       = Def.MaxHP;
	Data["MoveSpeed"]   = Def.MoveSpeed;
	if (!Def.AttackRef.empty()) { Data["AttackRef"] = Def.AttackRef; }
	if (!Def.RagdollDefID.empty()) { Data["RagdollDefId"] = Def.RagdollDefID; }
}
