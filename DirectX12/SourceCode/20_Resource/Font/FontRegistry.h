#pragma once

#include <filesystem>
#include <string>
#include <vector>

/**********************************************************************************
* @date      : 2026-08-23.
* @brief     : 外部フォント資産の登録・選択レジストリ.
*            : Data/Font/fonts.json(ID/file/family/size)を読み込んでID付きで
*            : 登録し、Resolve()でFontLoaderのフォントIDへ解決する。
*            * 同一設定の二重ロードはせず、欠損ファイルはMeiryoへフォールバック.
**********************************************************************************/

class FontRegistry final
{
public:
	// 登録エントリ(DEBUG表示・単体テスト用).
	struct Entry
	{
		std::string Id;
		std::string File;
		std::wstring Family;
		int         PixelHeight = 32;
		int         LoaderFontId = -1; // FontLoaderが返したID(-1=未解決).
		bool        Fallback     = false; // ファイル欠損等で既定フォントへ落ちたか.
	};

	// レジストリJSONを読み込んで登録する(重複IDは既存優先). 成功した登録数を返す.
	static int LoadRegistry(const std::filesystem::path& JsonPath);

	// コードから直接登録する(テスト・プログラム側定義用). 重複IDは既存を返す.
	static int Register(const std::string& Id, const std::string& File,
		const std::wstring& Family, int PixelHeight);

	// 指定IDをFontLoaderのフォントIDへ解決する(未登録なら-1. 二度目以降はキャッシュ).
	static int Resolve(const std::string& Id);

	// 登録エントリ一覧(DEBUG表示用).
	static const std::vector<Entry>& Entries();

	// 登録を全消去する(単体テスト用).
	static void Clear();

	// 単体テスト用: 実ロード処理の差し替え(File/Family/Height→FontLoader ID).
	using LoaderFn = int (*)(const std::string& File, const std::wstring& Family, int PixelHeight);
	static void SetLoaderForTest(LoaderFn Fn) noexcept { s_Loader = Fn; }

private:
	static int DefaultLoader(const std::string& File, const std::wstring& Family, int PixelHeight);
	static LoaderFn s_Loader;
};
