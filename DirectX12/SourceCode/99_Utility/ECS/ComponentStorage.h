#pragma once

#include <cstdint>
#include <vector>

#include "EntityTypes.h"

/**********************************************************************************
* @author    : Coder 青龍(せいりゅう).
* @date      : 2026/08/23.
* @brief     : 単一Component型のためのSparseSetストレージ.
*            : 追加/取得/削除O(1)、密な配列への反復はキャッシュフレンドリー.
*            : 同一Entityへの二重Addは値を上書きして1インスタンスのみ保持する.
**********************************************************************************/

namespace ECS {

	// 型消去用の基底インターフェース(Worldが全ストレージへ一括削除指示を出すために使用).
	class IComponentStorage
	{
	public:
		virtual ~IComponentStorage() = default;
		virtual bool RemoveIfPresent(const Entity& Entity) = 0;
	};

	template<typename T>
	class ComponentStorage final : public IComponentStorage
	{
	public:
		using iterator = typename std::vector<T>::iterator;
		using const_iterator = typename std::vector<T>::const_iterator;

		// Componentを追加する(既に存在する場合は上書き. 戻り値はComponentへの参照).
		T& Add(const Entity& Entity, const T& Value)
		{
			if (T* existing = TryGet(Entity))
			{
				*existing = Value;
				return *existing;
			}

			GrowFor(Entity.Index);

			m_DenseComponents.push_back(Value);
			m_DenseEntities.push_back(Entity);
			m_SparseToDense[Entity.Index] = static_cast<std::uint32_t>(m_DenseEntities.size() - 1);

			return m_DenseComponents.back();
		}

		// Componentを取得する(無ければnullptr).
		T* TryGet(const Entity& Entity) noexcept
		{
			const std::int32_t dense_index = DenseIndexOf(Entity);
			return (dense_index >= 0) ? &m_DenseComponents[static_cast<size_t>(dense_index)] : nullptr;
		}

		const T* TryGet(const Entity& Entity) const noexcept
		{
			return const_cast<ComponentStorage*>(this)->TryGet(Entity);
		}

		// Componentを削除する(存在しなければfalse). 削除はswap-and-pop(O(1)).
		bool Remove(const Entity& Entity)
		{
			const std::int32_t dense_index = DenseIndexOf(Entity);
			if (dense_index < 0) { return false; }

			const size_t index = static_cast<size_t>(dense_index);
			const size_t last = m_DenseComponents.size() - 1;

			if (index != last)
			{
				// 末尾要素を詰め替えて、そのEntityの逆引きも更新する.
				m_DenseComponents[index] = std::move(m_DenseComponents[last]);
				m_DenseEntities[index]   = m_DenseEntities[last];
				m_SparseToDense[m_DenseEntities[index].Index] = static_cast<std::uint32_t>(index);
			}

			m_DenseComponents.pop_back();
			m_DenseEntities.pop_back();
			m_SparseToDense[Entity.Index] = kInvalidDense;

			return true;
		}

		// Entityの全Component削除時に呼ぶ(Entity破棄時).
		bool RemoveIfPresent(const Entity& Entity) override { return Remove(Entity); }

		bool Has(const Entity& Entity) const noexcept { return DenseIndexOf(Entity) >= 0; }

		size_t Size() const noexcept { return m_DenseComponents.size(); }

		iterator begin() noexcept { return m_DenseComponents.begin(); }
		iterator end() noexcept   { return m_DenseComponents.end(); }
		const_iterator begin() const noexcept { return m_DenseComponents.begin(); }
		const_iterator end() const noexcept   { return m_DenseComponents.end(); }

		// 密配列に紐づくEntity(反復中の対応付け用).
		const Entity& EntityAt(size_t Index) const noexcept { return m_DenseEntities[Index]; }

	private:
		static constexpr std::uint32_t kInvalidDense = 0xFFFFFFFF;

		// Entity.Index → 密配列位置(存在しなければ-1).
		std::int32_t DenseIndexOf(const Entity& Entity) const noexcept
		{
			if (!Entity.IsValid() || Entity.Index >= m_SparseToDense.size()) { return -1; }
			const std::uint32_t dense = m_SparseToDense[Entity.Index];
			return (dense == kInvalidDense) ? -1 : static_cast<std::int32_t>(dense);
		}

		void GrowFor(std::uint32_t Index)
		{
			if (Index >= m_SparseToDense.size()) { m_SparseToDense.resize(static_cast<size_t>(Index) + 1, kInvalidDense); }
		}

		std::vector<T>                m_DenseComponents; // 密な実体配列.
		std::vector<Entity>           m_DenseEntities;   // 密配列各要素の所有Entity.
		std::vector<std::uint32_t>    m_SparseToDense;   // Entity.Index → 密配列位置.
	};

} // namespace ECS
