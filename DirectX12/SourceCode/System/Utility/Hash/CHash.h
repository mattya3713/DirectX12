#pragma once
#include "Utility/Hash/Crc32Table/HashTable.h"  // Hashテーブル.

#define ENABLE_HASH_DEBUG (0) // 1: Hashのデバッグ機能を有効.
                              // 0: 無効.

/****************************
*	ハッシュ値クラス.
*	担当：淵脇 未来.
*   参考URL:https://norizn.hatenablog.com/entry/2020/10/18/145628#2
****/

class CHash
{
public:
    CHash(const char* str) 
        : m_digest(0)
#if ENABLE_HASH_DEBUG
        , m_debug_str("")
#endif
    {
        m_digest = GetDigest(str, std::string(str).length());

#if ENABLE_HASH_DEBUG
        m_debug_str = str;
#endif
    };

    // ハッシュ値を取得.
    uint32_t GetDigest() const { return m_digest; }

    // 文字列からハッシュ値を作成して取得.
    static constexpr uint32_t GetDigest(const char* str, const size_t length);

private:
    uint32_t    m_digest;          // ハッシュ値.
  

#if ENABLE_HASH_DEBUG
public:
    // (デバッグ用) 文字列を取得.
    const char* GetDebugStr() const { return m_debug_str.c_str(); }

private:
    std::string m_debug_str;   // (デバッグ用) 文字列
#endif
};

// 文字列をハッシュ値に変換する。
// 使用例: HASH_DIGEST("STAGE_FOREST")
// sizeofはnull終端を含み文字数+1されるので-1して文字数のみにしておく。 
#define HASH_DIGEST(str) CHash::GetDigest(str, (sizeof(str)-1))

// MEMO : 処理速度+警告が出るため.hで実装.
// 文字列をhashにする.
constexpr uint32_t CHash::GetDigest(const char* str, const size_t length)
{
    uint32_t digest = 0xffffffff;
    for (size_t i = 0; i < length; i++)
    {
        digest = (digest << 8) ^ s_crc32_table[((digest >> 24) ^ str[i]) & 0xff];
    }

    return digest;
}