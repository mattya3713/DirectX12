#pragma once

#include <filesystem>

#include "json/json.hpp"

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/11.
* @brief     : JSON形式のファイルの読み込み・書き込み.
**********************************************************************************/

namespace FileManager {

	// JSON形式のファイルを開いてJSONデータを返す(存在しない・空・開けない場合は空のJSONを返す).
	nlohmann::json JsonLoad(const std::filesystem::path& FilePath);

	// JSONデータをJSON形式でファイルに書き込む.
	bool JsonSave(const std::filesystem::path& FilePath, const nlohmann::json& JsonData);

} // namespace FileManager
