#pragma once

#include <cstdint>

/**********************************************************************************
* @author    : Coder 青龍(せいりゅう).
* @date      : 2026/08/23.
* @brief     : ECSのEntity識別子. Index(下位32bit)+Generation(上位32bit).
*            : 破棄済みIndexが再利用されてもGenerationが異なるため、
*            : 古い参照は自動的に無効として扱える.
**********************************************************************************/

namespace ECS {

	constexpr std::uint32_t kInvalidIndex      = 0xFFFFFFFF;
	constexpr std::uint32_t kFirstGeneration   = 1;

	struct Entity
	{
		std::uint32_t Index      = kInvalidIndex;
		std::uint32_t Generation = 0;

		bool IsValid() const noexcept { return Index != kInvalidIndex && Generation != 0; }
	};

	inline bool operator==(const Entity& A, const Entity& B) noexcept { return A.Index == B.Index && A.Generation == B.Generation; }
	inline bool operator!=(const Entity& A, const Entity& B) noexcept { return !(A == B); }

} // namespace ECS
