#pragma once

#include <string>
#include <vector>

#include "00_Game/50_Enemy/Definition/EnemyDefinition.h"
#include "99_Utility/Data/DataTable.h"

/**********************************************************************************
* @author    : 朱雀 (Suzaku / 閃斬 Production Loop Coder).
* @date      : 2026/08/23.
* @brief     : 敵種別定義のカタログ. DataTable<EnemyDefinition>でJSON
*            : (Data\Json\Enemy\Definitions.json想定)から1度だけロードし、
*            : ID検索・存在確認・フォールバック取得を提供する.
*            : 将来のEnemyFactoryやLevel配置側はこのヘッダーAPIだけを参照する.
*            : 読み込み時に不正データ(Id空/MaxHP<=0/MoveSpeed<0)は除外するため、
*            : 壊れたエントリがカタログへ混入しない.
**********************************************************************************/

class EnemyDefinitionCatalog final
{
public:
	// 既定のフォールバック定義(未登録IDを問い合わせた際に返る無害な値).
	static const EnemyDefinition& Fallback() noexcept
	{
		static const EnemyDefinition fallback{
			std::string("fallback"), std::string("Unknown"),
			std::string(), 100.0f, 3.0f, std::string(), std::string() };
		return fallback;
	}

	EnemyDefinitionCatalog() = default;

	EnemyDefinitionCatalog(const EnemyDefinitionCatalog&)            = delete;
	EnemyDefinitionCatalog& operator=(const EnemyDefinitionCatalog&) = delete;

	// JSONから敵定義を読み込む(既存エントリはクリア. 不正データは読み飛ばす).
	bool Load(const std::filesystem::path& Path)
	{
		if (!m_Table.Load(Path)) { return false; }

		// 不正データを除去(from_json段階では欠損補完のみのため、ここで成立性を見る).
		std::vector<std::string> invalid_keys;
		for (const auto& [key, definition] : m_Table)
		{
			if (!definition.IsValid()) { invalid_keys.push_back(key); }
		}
		for (const std::string& key : invalid_keys)
		{
			m_Table.Remove(key);
		}

		return true;
	}

	// ID検索(見つからなければnullptr).
	const EnemyDefinition* Find(const std::string& Id) const noexcept
	{
		return m_Table.Find(Id);
	}

	// ID検索(見つからなければFallback()を返す. 呼び出し側でnullチェックが不要).
	const EnemyDefinition& GetOrFallback(const std::string& Id) const noexcept
	{
		const EnemyDefinition* p_definition = Find(Id);
		return p_definition ? *p_definition : Fallback();
	}

	// 存在確認.
	bool Contains(const std::string& Id) const noexcept { return m_Table.Contains(Id); }

	// 登録済みIDの一覧を取得する(エディタのコンボ候補等).
	std::vector<std::string> GetIds() const
	{
		std::vector<std::string> ids;
		for (const auto& [key, value] : m_Table) { ids.push_back(key); }
		return ids;
	}

	size_t Size() const noexcept { return m_Table.Size(); }

private:
	DataTable<EnemyDefinition> m_Table{ "Id" }; // キーフィールドはId.
};
