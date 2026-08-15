#pragma once

#include "RuntimeFormat.h"

#include <filesystem>

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/15.
* @brief     : ランタイムモデルフォーマットのバイナリ入出力.
**********************************************************************************/

namespace RuntimeFormatIO {

	// 静的メッシュデータをMSTC形式で書き出す。パスが開けない場合やサイズが表現範囲を超える場合はfalseを返す.
	bool WriteMstc(const std::filesystem::path& FilePath, const RuntimeFormat::MstcData& Data);
	// MSTC形式を読み込み、マジック・バージョン・ペイロード長が不正な場合はfalseを返す.
	bool ReadMstc(const std::filesystem::path& FilePath, RuntimeFormat::MstcData& OutData);
	// スキニングメッシュデータをMSKN形式で書き出す。固定長ボーン名が終端されない場合はfalseを返す.
	bool WriteMskn(const std::filesystem::path& FilePath, const RuntimeFormat::MsknData& Data);
	// MSKN形式を読み込み、ヘッダーの要素数から算出した長さと実データが一致しない場合はfalseを返す.
	bool ReadMskn(const std::filesystem::path& FilePath, RuntimeFormat::MsknData& OutData);
	// アニメーションクリップデータをMCLP形式で書き出す。要素数がuint32_tで表せない場合はfalseを返す.
	bool WriteMclp(const std::filesystem::path& FilePath, const RuntimeFormat::MclpData& Data);
	// MCLP形式を読み込み、トラックまたはクリップ名の範囲が不正な場合はfalseを返す.
	bool ReadMclp(const std::filesystem::path& FilePath, RuntimeFormat::MclpData& OutData);

} // namespace RuntimeFormatIO
