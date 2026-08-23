#pragma once

#include <filesystem>
#include <map>
#include <string>

#include "ISerializable.h"

/**********************************************************************************
* @date      : 2026-08-23.
* @brief     : ISerializable実装クラスをJSONファイルへまとめて保存/読込するユーティリティ.
*            : ファイル形式は { "キー名": { オブジェクトのSerialize()結果 } } の1階層.
*            : 単体Save/Loadは「1キーだけのファイル」として動作し、SaveAll/LoadAllで
*            : 複数オブジェクトを1ファイルへまとめられる。既存ファイルへの単体Saveは
*            : 他のキーを壊さず該当キーのみ差し替える.
**********************************************************************************/

class SaveLoadManager
{
public:
	// 1オブジェクトを指定キーで保存する(既存ファイルの他キーは保持).
	static bool Save(const std::filesystem::path& FilePath, const std::string& Key, const ISerializable& Object);

	// 指定キーのデータを読み込んでDeserializeする(ファイル・キーが無い場合はfalse).
	static bool Load(const std::filesystem::path& FilePath, const std::string& Key, ISerializable& Object);

	// 複数オブジェクトを1ファイルへまとめて保存する(既存の他キーは保持).
	static bool SaveAll(const std::filesystem::path& FilePath, const std::map<std::string, const ISerializable*>& Objects);

	// 複数オブジェクトを1ファイルからまとめて読み込む(無いキーはfalseを返すが、あるキーは読む).
	static bool LoadAll(const std::filesystem::path& FilePath, const std::map<std::string, ISerializable*>& Objects);
};
