#pragma once

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>

#include "json/json.hpp"

/**********************************************************************************
* @author    : 朱雀 (Suzaku / 閃斬 Production Loop Coder).
* @date      : 2026/08/23.
* @brief     : ユーザー設定(音量・キーバインド・カメラ感度等)の永続化管理.
*            : キー→JSON値の単純な保存域で、Load時に欠損キーは既定値扱いになる.
*            : 実行時生成データのため出力先はData\Json\Settings.json(imgui.rulと
*            : 同様の位置づけ). 値の型はnlohmannが扱えるもの(int/float/bool/string等).
*            : スレッドセーフではない.
**********************************************************************************/

class SettingsManager final
{
public:
	static constexpr const char* kDefaultPath = "Data\\Json\\Settings.json";

	SettingsManager() = default;

	SettingsManager(const SettingsManager&)            = delete;
	SettingsManager& operator=(const SettingsManager&) = delete;

	static SettingsManager& Instance()
	{
		static SettingsManager instance;
		return instance;
	}

	// 設定を読み込む(ファイルが無くても失敗敗扱いにしない=初回起動は既定値で開始).
	bool Load(const std::filesystem::path& Path = kDefaultPath)
	{
		std::ifstream file(Path);
		if (!file) { return false; }

		try {
			nlohmann::json root{};
			file >> root;
			if (!root.is_object()) { return false; }

			m_Values.clear();
			m_Path = Path;
			for (auto it = root.begin(); it != root.end(); ++it)
			{
				m_Values[it.key()] = it.value();
			}
			return true;
		}
		catch (const nlohmann::json::exception&) {
			return false; // 破損ファイル時は現行値を壊さない.
		}
	}

	// 全設定を書き戻す.
	bool Save(const std::filesystem::path& Path = "") const
	{
		const std::filesystem::path path = Path.empty() ? m_Path : Path;
		if (path.empty()) { return false; }

		nlohmann::json root = nlohmann::json::object();
		for (const auto& [key, value] : m_Values)
		{
			root[key] = value;
		}

		std::ofstream file(path);
		if (!file) { return false; }
		file << root.dump(2);
		return true;
	}

	// 値を設定する(呼び出し後にSave()を忘れずに).
	template<typename T>
	void Set(const std::string& Key, const T& Value)
	{
		m_Values[Key] = Value;
	}

	// 値を取得する(未設定なら既定値を返す. アサートではなく既定値方針).
	template<typename T>
	T Get(const std::string& Key, const T& DefaultValue) const
	{
		const auto it = m_Values.find(Key);
		if (it == m_Values.end()) { return DefaultValue; }
		try {
			return it->second.get<T>();
		}
		catch (const nlohmann::json::exception&) {
			return DefaultValue;
		}
	}

	bool Contains(const std::string& Key) const noexcept
	{
		return m_Values.find(Key) != m_Values.end();
	}

private:
	std::unordered_map<std::string, nlohmann::json> m_Values;
	std::filesystem::path m_Path = kDefaultPath;
};
