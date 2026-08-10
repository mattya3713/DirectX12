#include "FileManager.h"

#include <fstream>
#include <iomanip>

#include "99_Utility/String/String.h"

nlohmann::json FileManager::JsonLoad(const std::filesystem::path& FilePath)
{
	nlohmann::json out;

	if (!std::filesystem::exists(FilePath) || std::filesystem::file_size(FilePath) == 0)
	{
		return out;
	}

	std::ifstream file(FilePath);
	if (!file.is_open())
	{
		const std::wstring w_message = MyString::StringToWString(FilePath.string() + "が開けませんでした。");
		_ASSERT_EXPR(false, w_message.c_str());
		return out;
	}

	file >> out;
	return out;
}

bool FileManager::JsonSave(const std::filesystem::path& FilePath, const nlohmann::json& JsonData)
{
	std::ofstream file(FilePath);
	if (!file.is_open())
	{
		return false;
	}

	file << std::setw(2) << JsonData << std::endl;
	return true;
}
