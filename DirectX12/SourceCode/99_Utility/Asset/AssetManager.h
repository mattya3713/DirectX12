#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

/**********************************************************************************
* @author    : Coder(青龍/せいりゅう).
* @date      : 2026/08/23.
* @brief     : パスをキーにした汎用アセットキャッシュ(weak_ptr参照カウント式).
*            : 同じパスのロードは1回だけ実行され、参照が全て無くなれば
*            : 自然に解放される(明示的なUnloadも可能).
*            : 非同期ロード・ホットリロードは対象外.
**********************************************************************************/

template<typename T>
class AssetManager final
{
public:
	AssetManager() = default;
	~AssetManager() = default;

	AssetManager(const AssetManager&)            = delete;
	AssetManager& operator=(const AssetManager&) = delete;

	// パスをキーにアセットを取得する. 未ロード(または解放済み)ならLoaderを呼んでキャッシュする.
	std::shared_ptr<T> Load(const std::filesystem::path& Path,
		std::function<std::shared_ptr<T>(const std::filesystem::path&)> Loader)
	{
		const std::string key = MakeKey(Path);

		if (std::shared_ptr<T> cached = FindLocked(key)) { return cached; }

		std::shared_ptr<T> loaded = Loader ? Loader(Path) : nullptr;
		if (loaded) { m_Cache[key] = loaded; }
		return loaded;
	}

	// 生きているキャッシュがあれば取得する(無ければnullptr).
	std::shared_ptr<T> Find(const std::filesystem::path& Path) const
	{
		return FindLocked(MakeKey(Path));
	}

	// 外部で構築済みのインスタンスをキャッシュへ登録する.
	void Register(const std::filesystem::path& Path, std::shared_ptr<T> Instance)
	{
		if (Instance) { m_Cache[MakeKey(Path)] = std::move(Instance); }
	}

	// 指定パスのキャッシュを明示的に破棄する(生きている参照がある場合は実体は存続する).
	void Unload(const std::filesystem::path& Path)
	{
		m_Cache.erase(MakeKey(Path));
	}

	// 参照が切れた(解放済み)エントリを掃除する.
	void ClearExpired()
	{
		for (auto it = m_Cache.begin(); it != m_Cache.end();)
		{
			if (it->second.expired()) { it = m_Cache.erase(it); }
			else { ++it; }
		}
	}

	// 現在のエントリ数(解放済みを含む).
	size_t Size() const noexcept { return m_Cache.size(); }

private:
	// パスを正規化してキャッシュキーへ変換する(表記ゆれを同一視).
	static std::string MakeKey(const std::filesystem::path& Path)
	{
		return Path.lexically_normal().generic_string();
	}

	// キーから生きているキャッシュを取得する(期限切れエントリは掃除する).
	std::shared_ptr<T> FindLocked(const std::string& Key) const
	{
		const auto it = m_Cache.find(Key);
		if (it == m_Cache.end()) { return nullptr; }

		if (std::shared_ptr<T> cached = it->second.lock()) { return cached; }
		m_Cache.erase(it); // 全参照が切れたエントリはここで破棄.
		return nullptr;
	}

	mutable std::unordered_map<std::string, std::weak_ptr<T>> m_Cache; // パス→弱参照(参照切れで自然解放).
};
