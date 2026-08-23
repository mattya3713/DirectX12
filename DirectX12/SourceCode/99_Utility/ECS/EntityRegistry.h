#pragma once

#include <cstdint>
#include <vector>

#include "EntityTypes.h"

/**********************************************************************************
* @author    : Coder 青龍(せいりゅう).
* @date      : 2026/08/23.
* @brief     : Entityの生成・破棄・生存判定を管理するレジストリ.
*            : 破棄済みIndexはフリーリストへ戻り、再利用時にGenerationが加算される.
**********************************************************************************/

namespace ECS {

	class EntityRegistry final
	{
	public:
		EntityRegistry() = default;

		// Entityを生成する.
		Entity Create()
		{
			std::uint32_t index = 0;
			std::uint32_t generation = kFirstGeneration;

			if (m_FreeIndices.empty())
			{
				index = static_cast<std::uint32_t>(m_Generations.size());
				m_Generations.push_back(generation);
			}
			else
			{
				index = m_FreeIndices.back();
				m_FreeIndices.pop_back();
				generation = ++m_Generations[index]; // 再利用時は世代を進める.
			}

			return { index, generation };
		}

		// Entityを破棄する(存在しない場合はfalse).
		bool Destroy(const Entity& Entity)
		{
			if (!IsAlive(Entity)) { return false; }

			++m_Generations[Entity.Index];
			m_FreeIndices.push_back(Entity.Index);
			return true;
		}

		// Entityが生存しているか(Generation不一致は無効扱い).
		bool IsAlive(const Entity& Entity) const noexcept
		{
			return Entity.IsValid()
				&& Entity.Index < m_Generations.size()
				&& m_Generations[Entity.Index] == Entity.Generation;
		}

		// 生存しているEntity数.
		size_t AliveCount() const noexcept
		{
			return m_Generations.size() - m_FreeIndices.size();
		}

	private:
		std::vector<std::uint32_t> m_Generations;  // Indexごとの現在世代(1開始. 0は未使用マーク相当にしない).
		std::vector<std::uint32_t> m_FreeIndices;  // 破棄済みIndexの再利用待ち.
	};

} // namespace ECS
