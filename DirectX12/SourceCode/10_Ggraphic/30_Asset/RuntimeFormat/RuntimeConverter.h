#pragma once

#include "RuntimeFormat.h"

#include <filesystem>
#include <string>
#include <vector>

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/15.
* @brief     : PMX/X/VMDをランタイム形式へ変換するオフラインAPI.
**********************************************************************************/

namespace RuntimeConverter {

	// 変換結果と継続可能な警告を保持する.
	struct ConversionResult {
		bool Success = false;
		std::string Error;
		std::vector<std::string> Warnings;
	};

	// PMXと任意のVMDからMSKN/MMAT/MCLPを生成する.
	ConversionResult ConvertPmx(const std::filesystem::path& PmxPath, const std::filesystem::path& VmdPath, const std::filesystem::path& OutputDirectory);
	// XからMSKN/MMATとAnimationSetごとのMCLPを生成する.
	ConversionResult ConvertX(const std::filesystem::path& XPath, const std::filesystem::path& OutputDirectory);

} // namespace RuntimeConverter
