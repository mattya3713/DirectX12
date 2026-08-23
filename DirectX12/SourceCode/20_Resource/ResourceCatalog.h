#pragma once

/**********************************************************************************
* @author    : Coder 青龍(せいりゅう).
* @date      : 2026/08/23.
* @brief     : リソースの論理カテゴリと正規化済みキーのカタログ(パス規約の単一窓口).
*            : ロード処理は持たない(AssetManager<T>と責務分離。呼び出し側が
*            : このカタログでキーを作り、AssetManagerへ渡す形を想定).
*            : 不正カテゴリ・空ID・絶対パス・パス脱出(..)は拒否する.
**********************************************************************************/

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_set>

enum class eResourceCategory : std::uint32_t
{
	ImageWorldSprite = 0, // ワールド空間に置くスプライト(Sprite3D接続予定).
	ImageUISprite    = 1, // UI用スプライト(Sprite2D接続予定).
	Effect           = 2, // パーティクル/エフェクトプリセット(ParticleSystem接続予定).
	Mesh             = 3, // メッシュ(mmdl/mskn等).
	Sound            = 4, // サウンド(SE/BGM. SoundManager接続予定).

	_Invalid = 0xFFFFFFFF, // 範囲外判定用.
};

class ResourceCatalog final
{
public:
	ResourceCatalog() = default;
	~ResourceCatalog() = default;

	ResourceCatalog(const ResourceCatalog&)            = delete;
	ResourceCatalog& operator=(const ResourceCatalog&) = delete;

	// カテゴリの論理ルート名を取得する("image/world"等).
	static const char* GetCategoryRoot(eResourceCategory Category) noexcept
	{
		switch (Category)
		{
		case eResourceCategory::ImageWorldSprite: return "image/world";
		case eResourceCategory::ImageUISprite:    return "image/ui";
		case eResourceCategory::Effect:           return "effect";
		case eResourceCategory::Mesh:             return "mesh";
		case eResourceCategory::Sound:            return "sound";
		default:                                  return nullptr;
		}
	}

	// カテゴリとIDから正規化済みキーを生成する.
	// 失敗する条件: 不正カテゴリ/空ID/絶対パス/ドライブ文字/".."によるパス脱出.
	// 成功する条件: 表記揺れ(./、重複スラッシュ、中間のa/../)は同一キーへ収束する.
	static bool TryMakeKey(eResourceCategory Category, const std::string& ResourceId, std::string& OutKey)
	{
		const char* root = GetCategoryRoot(Category);
		if (root == nullptr) { return false; }       // 不正カテゴリ.
		if (ResourceId.empty()) { return false; }    // 空ID.

		// バックスラッシュはスラッシュへ統一(Windows表記ゆれの吸収).
		std::string id = ResourceId;
		for (char& c : id)
		{
			if (c == '\\') { c = '/'; }
		}

		const std::filesystem::path normalized = std::filesystem::path(id).lexically_normal();

		if (normalized.empty()) { return false; }
		if (normalized.is_absolute()) { return false; } // 絶対パス(C:/、/、\\サーバー等).

		// lexically_normal後も".."が残る=カテゴリルート外への脱出なので拒否する.
		{
			static const std::string parent = "..";
			std::string current = normalized.generic_string();
			size_t start = 0;
			while (start <= current.size())
			{
				const size_t end = current.find('/', start);
				const std::string component = current.substr(start, (end == std::string::npos) ? std::string::npos : end - start);
				if (component == parent || component.empty()) { return false; } // 脱出または空要素(連続スラッシュの末尾処理漏れ).
				if (end == std::string::npos) { break; }
				start = end + 1;
			}
		}

		OutKey = std::string(root) + "/" + normalized.generic_string();
		return true;
	}

	// キーをカタログへ登録する(二重登録はfalse).
	bool Register(const std::string& Key) { return m_Keys.insert(Key).second; }

	// キーがカタログに登録されているか.
	bool Contains(const std::string& Key) const { return m_Keys.count(Key) > 0; }

	// キーをカタログから解除する(存在しなければfalse).
	bool Unregister(const std::string& Key) { return m_Keys.erase(Key) > 0; }

	// 登録数.
	size_t Count() const noexcept { return m_Keys.size(); }

private:
	std::unordered_set<std::string> m_Keys; // 正規化済みキーの集合.
};
