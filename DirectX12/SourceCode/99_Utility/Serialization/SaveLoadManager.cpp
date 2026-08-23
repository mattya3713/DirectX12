#include "SaveLoadManager.h"

#include <fstream>
#include <iomanip>

namespace {

	// ルートオブジェクトを読み込む(ファイル無し・空・壊れ・非オブジェクトは空オブジェクト扱い).
	nlohmann::json LoadRoot(const std::filesystem::path& FilePath)
	{
		std::ifstream file(FilePath);
		if (!file) { return nlohmann::json::object(); }

		try
		{
			nlohmann::json parsed;
			file >> parsed;
			return parsed.is_object() ? parsed : nlohmann::json::object();
		}
		catch (const nlohmann::json::parse_error&)
		{
			return nlohmann::json::object();
		}
	}

	// ルートオブジェクトを書き込む.
	bool SaveRoot(const std::filesystem::path& FilePath, const nlohmann::json& Root)
	{
		std::ofstream file(FilePath);
		if (!file) { return false; }

		file << std::setw(2) << Root << std::endl;
		return true;
	}

} // namespace

// 指定キーのオブジェクトJSONを、既存ファイルの他キーを保持したまま書き込む.
bool SaveLoadManager::Save(const std::filesystem::path& FilePath, const std::string& Key, const ISerializable& Object)
{
	if (Key.empty()) { return false; }

	nlohmann::json root = LoadRoot(FilePath);
	root[Key] = Object.Serialize();
	return SaveRoot(FilePath, root);
}

// 指定キーのデータを読み込んでDeserializeする.
bool SaveLoadManager::Load(const std::filesystem::path& FilePath, const std::string& Key, ISerializable& Object)
{
	if (Key.empty()) { return false; }

	const nlohmann::json root = LoadRoot(FilePath);
	if (!root.contains(Key) || !root[Key].is_object()) { return false; }

	Object.Deserialize(root[Key]);
	return true;
}

// 複数オブジェクトを1ファイルへまとめて保存する.
bool SaveLoadManager::SaveAll(const std::filesystem::path& FilePath, const std::map<std::string, const ISerializable*>& Objects)
{
	nlohmann::json root = LoadRoot(FilePath);

	for (const auto& [key, object] : Objects)
	{
		if (key.empty() || object == nullptr) { continue; }
		root[key] = object->Serialize();
	}

	return SaveRoot(FilePath, root);
}

// 複数オブジェクトを1ファイルからまとめて読み込む.
bool SaveLoadManager::LoadAll(const std::filesystem::path& FilePath, const std::map<std::string, ISerializable*>& Objects)
{
	const nlohmann::json root = LoadRoot(FilePath);
	if (root.empty()) { return false; }

	bool all_found = true;
	for (const auto& [key, object] : Objects)
	{
		if (object == nullptr) { continue; }
		if (!root.contains(key) || !root[key].is_object()) { all_found = false; continue; }
		object->Deserialize(root[key]);
	}

	return all_found;
}
