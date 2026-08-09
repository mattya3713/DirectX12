#pragma once

#include <cstdint>
#include <string_view>

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/09.
* @brief     : 文字列をコンパイル時にハッシュ化し、名前引きのキーとして使うための型.
*            : デバッグビルドでは元の文字列も保持し、リリースビルドではハッシュ値のみを持つ.
**********************************************************************************/

class HashedString final
{
public:
	using HashType = uint32_t;

	// 文字列(string_view)からコンパイル時にハッシュを計算する.
	constexpr HashedString(std::string_view Str) noexcept
		: m_Hash        (Hash(Str))
#if _DEBUG
		, m_DebugString (Str)
#endif // _DEBUG
	{
	}

	// ハッシュ値の取得.
	constexpr HashType GetHash() const noexcept { return m_Hash; }

	constexpr bool operator==(const HashedString& Other) const noexcept { return m_Hash == Other.m_Hash; }
	constexpr bool operator!=(const HashedString& Other) const noexcept { return !(*this == Other); }

#if _DEBUG
	// 元の文字列を取得(デバッグビルドのみ、ログ・デバッガ表示用).
	constexpr std::string_view GetDebugString() const noexcept { return m_DebugString; }
#endif // _DEBUG

private:
	// FNV-1aハッシュ(コンパイル時計算可能。charの符号拡張を避けるためunsigned charを経由する).
	static constexpr HashType Hash(std::string_view Str) noexcept
	{
		HashType hash = 2166136261u;
		for (char c : Str) {
			hash ^= static_cast<HashType>(static_cast<unsigned char>(c));
			hash *= 16777619u;
		}
		return hash;
	}

private:
	HashType m_Hash;
#if _DEBUG
	std::string_view m_DebugString; // リリースビルドには存在しないメンバ.
#endif // _DEBUG
};

// std::unordered_map等のキーとして使うためのハッシュ特殊化.
namespace std {
	template<>
	struct hash<HashedString>
	{
		size_t operator()(const HashedString& Value) const noexcept
		{
			return static_cast<size_t>(Value.GetHash());
		}
	};
}
