#include "FontRegistry.h"

#include <windows.h>

#include "FontLoader.h"
#include "99_Utility/FileManager/FileManager.h"
#include "99_Utility/String/String.h"

FontRegistry::LoaderFn FontRegistry::s_Loader = nullptr;

namespace {
	std::vector<FontRegistry::Entry>& Table()
	{
		static std::vector<FontRegistry::Entry> table;
		return table;
	}

	// フォントファイルをプロセスへプライベート登録する(成功: 1, 失敗: 0).
	bool InstallPrivateFont(const std::filesystem::path& FilePath)
	{
		return AddFontResourceExW(FilePath.c_str(), FR_PRIVATE, nullptr) != 0;
	}
} // namespace

// コードから直接登録する.
int FontRegistry::Register(const std::string& Id, const std::string& File,
	const std::wstring& Family, int PixelHeight)
{
	for (const FontRegistry::Entry& entry : Table())
	{
		if (entry.Id == Id) { return entry.LoaderFontId; } // 二重登録は既存を返す.
	}

	Entry entry{};
	entry.Id          = Id;
	entry.File        = File;
	entry.Family      = Family;
	entry.PixelHeight = PixelHeight;

	Table().push_back(std::move(entry));
	return -1; // 未解決(Resolve時に初回ロード).
}

// レジストリJSONを読み込んで登録する.
int FontRegistry::LoadRegistry(const std::filesystem::path& JsonPath)
{
	const nlohmann::json data = FileManager::JsonLoad(JsonPath);
	if (!data.contains("fonts") || !data["fonts"].is_array()) { return 0; }

	int registered = 0;
	for (const nlohmann::json& font : data["fonts"])
	{
		if (!font.is_object()) { continue; }

		const std::string id   = font.value("id", "");
		const std::string file = font.value("file", "");
		const int size         = font.value("size", 32);

		if (id.empty()) { continue; }
		Register(id, file, MyString::StringToWString(font.value("family", "")), size);
		++registered;
	}
	return registered;
}

// 指定IDをFontLoaderのフォントIDへ解決する.
int FontRegistry::Resolve(const std::string& Id)
{
	for (FontRegistry::Entry& entry : Table())
	{
		if (entry.Id != Id) { continue; }
		if (entry.LoaderFontId >= 0) { return entry.LoaderFontId; } // キャッシュ済み.

		std::filesystem::path file_path(entry.File);
		const bool file_exists = !entry.File.empty() && std::filesystem::exists(file_path);

#ifdef FONT_REGISTRY_NO_DEFAULT_LOADER
		if (!s_Loader) { return -1; }
#endif
		entry.LoaderFontId = s_Loader
			? s_Loader(entry.File, entry.Family, entry.PixelHeight)
			: DefaultLoader(entry.File, entry.Family, entry.PixelHeight);
		entry.Fallback = !file_exists || entry.LoaderFontId < 0;
		return entry.LoaderFontId;
	}

	return -1; // 未登録ID.
}

// 登録エントリ一覧.
const std::vector<FontRegistry::Entry>& FontRegistry::Entries()
{
	return Table();
}

// 登録を全消去する.
void FontRegistry::Clear()
{
	Table().clear();
}


// 既定のロード処理(プライベートフォント登録→GDIラスタライズ).
#ifndef FONT_REGISTRY_NO_DEFAULT_LOADER
int FontRegistry::DefaultLoader(const std::string& File, const std::wstring& Family, int PixelHeight)
{
	if (!File.empty())
	{
		const std::filesystem::path path(File);
		if (std::filesystem::exists(path))
		{
			InstallPrivateFont(path);
		}
	}
	return FontLoader::LoadFont(Family, PixelHeight);
}
#endif