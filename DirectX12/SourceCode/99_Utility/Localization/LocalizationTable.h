#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>

#include "json/json.hpp"

/**********************************************************************************
* @author    : 朱雀 (Suzaku / 閃斬 Production Loop Coder).
* @date      : 2026/08/23.
* @brief     : 文字列ローカライゼーション基盤. Data\Json\Localization配下の
*            : <言語名>.json(キー→翻訳文字列)を読み、GetText(Key)で現在言語の
*            : 文字列を返す. 未登録キーはキー文字列そのものを返す(欠落が画面に
*            : 分かる仕様. ダミー翻訳で構わないため自動フォールバックはしない).
*            : 言語切替はSetLanguage(Lang)のみ. スレッドセーフではない.
**********************************************************************************/

class LocalizationTable final
{
public:
	static constexpr const char* kLocalizationDir = "Data\\Json\\Localization";
	static constexpr const char* kDefaultLanguage = "ja";

	LocalizationTable() = default;

	LocalizationTable(const LocalizationTable&)            = delete;
	LocalizationTable& operator=(const LocalizationTable&) = delete;

	static LocalizationTable& Instance()
	{
		static LocalizationTable instance;
		return instance;
	}

	// 現在言語を設定し、対応するJSONを読み込む(ファイルが無くても現状維持).
	void SetLanguage(const std::string& Language)
	{
		if (Language.empty()) { return; }

		std::unordered_map<std::string, std::string> loaded;
		if (!ReadLanguageFile(Language, loaded)) { return; } // 失敗時は現言語のまま.

		m_Language = Language;
		m_Strings  = std::move(loaded);
	}

	const std::string& GetLanguage() const noexcept { return m_Language; }

	// 現在言語の文字列を取得する(未登録キーはキーそのものを返す).
	const std::string& GetText(const std::string& Key) const
	{
		const auto it = m_Strings.find(Key);
		if (it != m_Strings.end()) { return it->second; }

		// 未登録キーはキャッシュして同じ文字列への参照を返し続ける(ImGuiへ渡す際の寿命対策).
		return m_MissCache.try_emplace(Key, Key).first->second;
	}

private:
	// <lang>.jsonを読み込む(オブジェクト形式: {"キー": "文字列", ...}).
	bool ReadLanguageFile(const std::string& Language, std::unordered_map<std::string, std::string>& Out) const
	{
		const std::filesystem::path path =
			std::filesystem::path(kLocalizationDir) / (Language + ".json");

		std::ifstream file(path);
		if (!file) { return false; }

		try {
			nlohmann::json root{};
			file >> root;
			if (!root.is_object()) { return false; }

			Out.clear();
			for (auto it = root.begin(); it != root.end(); ++it)
			{
				if (!it.value().is_string()) { continue; }
				Out[it.key()] = it.value().get<std::string>();
			}
			return true;
		}
		catch (const nlohmann::json::exception&) {
			return false;
		}
	}

private:
	std::string                                   m_Language = kDefaultLanguage;
	std::unordered_map<std::string, std::string>  m_Strings;   // キー→現在言語の文字列.
	mutable std::unordered_map<std::string, std::string> m_MissCache; // 未登録キーの参照安定化用.
};
