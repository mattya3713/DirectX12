#pragma once

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "json/json.hpp"

/**********************************************************************************
* @author    : 朱雀 (Suzaku / 閃斬 Production Loop Coder).
* @date      : 2026/08/23.
* @brief     : JSON駆動の汎用データテーブル(名前→データ).
*            : 「AttackCombo_*.json(Combat調整値)」「AnimationClipTable(名前→
*            : クリップ範囲)」等の個別実装パターンを一般化したもの.
*            : Tはデフォルト構築可能であり、nlohmann::jsonとの相互変換として
*            : from_json/to_json(ADL)を実装していること(既存のnlohmann規約に準拠).
*            : 対応JSONレイアウト: 配列形式([{<KeyField>:"名前",...},...])と
*            : オブジェクト形式({"名前":{...},...})の両方. Saveは配列形式で書き出す.
*            : スキーマバリデーションは対象外.
**********************************************************************************/

template<typename T>
class DataTable final
{
public:
	explicit DataTable(std::string KeyField = "Name") noexcept
		: m_KeyField(std::move(KeyField))
	{
	}

	DataTable(const DataTable&)            = delete;
	DataTable& operator=(const DataTable&) = delete;
	DataTable(DataTable&&)                 = default;
	DataTable& operator=(DataTable&&)      = default;

	// JSONファイルを読み込み、テーブルを再構築する(既存エントリはクリアされる).
	bool Load(const std::filesystem::path& Path)
	{
		std::ifstream file(Path);
		if (!file) { return false; }

		nlohmann::json root{};
		try {
			file >> root;
		}
		catch (const nlohmann::json::exception&) {
			return false; // 不正なJSONは丸ごと失敗扱い(部分的な読込で壊れないようにする).
		}

		m_Entries.clear();
		return Parse(root);
	}

	// 現在のテーブル内容をJSON(配列形式)へ書き戻す(ActionTimeline系ツールの編集後保存を想定).
	bool Save(const std::filesystem::path& Path) const
	{
		nlohmann::json array = nlohmann::json::array();
		for (const auto& [key, value] : m_Entries)
		{
			nlohmann::json entry = value; // to_json(ADL).
			entry[m_KeyField]        = key; // キー列を必ず同期させる(to_json側で省略されても良い).
			array.push_back(std::move(entry));
		}

		std::ofstream file(Path);
		if (!file) { return false; }
		file << array.dump(2);
		return true;
	}

	// 指定キーが存在するか.
	bool Contains(const std::string& Key) const noexcept
	{
		return m_Entries.find(Key) != m_Entries.end();
	}

	// 指定キーのデータを取得する(無ければnullptr).
	const T* Find(const std::string& Key) const noexcept
	{
		const auto it = m_Entries.find(Key);
		return (it != m_Entries.end()) ? &it->second : nullptr;
	}

	// 指定キーのデータを取得する(無ければassert. 既存コードの方針に合わせて呼び出し側の取り違えを見つける).
	const T& Get(const std::string& Key) const
	{
		static const T default_value{};
		const auto it = m_Entries.find(Key);
		if (it == m_Entries.end())
		{
			assert(false && "DataTable: 指定キーが存在しません");
			return default_value;
		}
		return it->second;
	}

	// エントリを追加/上書きする(ツール側からの編集用).
	void Set(const std::string& Key, const T& Value)
	{
		m_Entries[Key] = Value;
	}

	// 指定キーのエントリを削除する(存在しなければ何もしない).
	void Remove(const std::string& Key)
	{
		m_Entries.erase(Key);
	}

	size_t Size() const noexcept { return m_Entries.size(); }
	void Clear() noexcept { m_Entries.clear(); }

	auto begin() noexcept { return m_Entries.begin(); }
	auto end() noexcept { return m_Entries.end(); }
	auto begin() const noexcept { return m_Entries.cbegin(); }
	auto end() const noexcept { return m_Entries.cend(); }

private:
	// 配列形式・オブジェクト形式の両方に対応して解析する.
	bool Parse(const nlohmann::json& Root)
	{
		try {
			if (Root.is_array())
			{
				for (const nlohmann::json& entry : Root)
				{
					if (!entry.is_object() || !entry.contains(m_KeyField)) { continue; }
					const std::string key = entry.at(m_KeyField).get<std::string>();
					if (key.empty()) { continue; }
					m_Entries[key] = entry.get<T>(); // from_json(ADL).
				}
				return true;
			}

			if (Root.is_object())
			{
				for (auto it = Root.begin(); it != Root.end(); ++it)
				{
					m_Entries[it.key()] = it.value().get<T>();
				}
				return true;
			}
		}
		catch (const nlohmann::json::exception&) {
			m_Entries.clear(); // 変換失敗時は部分構築の状態を残さない.
			return false;
		}

		return false;
	}

private:
	std::string                      m_KeyField; // 配列形式のとき、このフィールドをキーとして使う.
	std::unordered_map<std::string, T> m_Entries; // 名前→データ.
};
