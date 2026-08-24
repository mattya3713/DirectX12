#pragma once

#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "ComponentStorage.h"
#include "EntityRegistry.h"
#include "SystemScheduler.h"

/**********************************************************************************
* @author    : Coder 青龍(せいりゅう).
* @date      : 2026/08/23.
* @brief     : EntityRegistry + ComponentStorage群 + SystemSchedulerを統括するWorld.
*            : 更新中のEntity破棄はDestroy()でキューへ積み、FlushDestroyed()で
*            : 遅延適用する(System実行中の反復を壊さないための規約).
*            : Shutdown()でComponent/Systemを安全に解放する.
**********************************************************************************/

namespace ECS {

	class World final
	{
	public:
		World() = default;
		~World() { Shutdown(); }

		World(const World&)            = delete;
		World& operator=(const World&) = delete;

		// ----- Entity -----

		Entity CreateEntity() { return m_Registry.Create(); }

		// Entityを遅延破棄する(FlushDestroyed()まで有効なまま. 反復中も安全に呼べる).
		bool DestroyEntity(const Entity& Entity)
		{
			if (!m_Registry.IsAlive(Entity) || IsMarkedForDestruction(Entity)) { return false; }
			m_DestroyQueue.push_back(Entity);
			return true;
		}

		bool IsAlive(const Entity& Entity) const noexcept
		{
			return m_Registry.IsAlive(Entity) && !IsMarkedForDestruction(Entity);
		}

		// 遅延破棄を適用する(各Systemの終了後・フレーム末に呼ぶ).
		void FlushDestroyed()
		{
			for (const Entity& entity : m_DestroyQueue)
			{
				if (!m_Registry.IsAlive(entity)) { continue; }
				for (auto& pair : m_Storages) { pair.second->RemoveIfPresent(entity); }
				m_Registry.Destroy(entity);
			}
			m_DestroyQueue.clear();
		}

		// ----- Component -----

	template<typename T>
	ComponentStorage<T>& GetOrCreateStorage()
	{
		const std::type_index key(typeid(T));

		const auto it = m_Storages.find(key);
		if (it != m_Storages.end()) { return *static_cast<ComponentStorage<T>*>(it->second.get()); }

		auto storage = std::make_unique<ComponentStorage<T>>();
		ComponentStorage<T>* raw = storage.get();
		m_Storages[key] = std::move(storage);
		return *raw;
	}
		template<typename T, typename... Args>
		T& AddComponent(const Entity& Entity, Args&&... Args_)
		{
			return GetOrCreateStorage<T>().Add(Entity, T{ std::forward<Args>(Args_)... });
		}

		template<typename T>
		T* GetComponent(const Entity& Entity)
		{
			return GetOrCreateStorage<T>().TryGet(Entity);
		}

		template<typename T>
		bool HasComponent(const Entity& Entity) { return GetOrCreateStorage<T>().Has(Entity); }

		template<typename T>
		bool RemoveComponent(const Entity& Entity) { return GetOrCreateStorage<T>().Remove(Entity); }

		// 1種のComponentを持つ全Entityへ処理する.
		template<typename T, typename Func>
		void ForEach(Func&& Func_)
		{
			auto& storage = GetOrCreateStorage<T>();
			for (size_t i = 0; i < storage.Size(); ++i)
			{
				const Entity entity = storage.EntityAt(i);
				if (!IsAlive(entity)) { continue; }
				if (T* component = storage.TryGet(entity)) { Func_(entity, *component); }
			}
		}

		// 2種のComponentを持つ全Entityへ処理する(片方を基準にもう片方を検索).
		template<typename TFirst, typename TSecond, typename Func>
		void ForEach(Func&& Func_)
		{
			auto& first = GetOrCreateStorage<TFirst>();
			auto& second = GetOrCreateStorage<TSecond>();

			for (size_t i = 0; i < first.Size(); ++i)
			{
				const Entity entity = first.EntityAt(i);
				if (!IsAlive(entity)) { continue; }

				TSecond* p_second = second.TryGet(entity);
				if (!p_second) { continue; }

				Func_(entity, *first.TryGet(entity), *p_second);
			}
		}

		// ----- System -----

		size_t AddSystem(const std::string& Name, SystemScheduler::SystemFn System)
		{
			return m_Scheduler.Add(Name, std::move(System));
		}

		void RunSystems(float DeltaTime) { m_Scheduler.Run(*this, DeltaTime); }

		// WorldのShutdown: 全SystemとComponentを安全に解放する.
		void Shutdown()
		{
			m_Scheduler.Shutdown();
			m_Storages.clear();
			m_DestroyQueue.clear();
		}

		// デバッグ用統計.
		size_t GetAliveEntityCount() const noexcept { return m_Registry.AliveCount(); }
		size_t GetComponentTypeCount() const noexcept { return m_Storages.size(); }
		size_t GetSystemCount() const noexcept { return m_Scheduler.Count(); }
		float GetSystemExecutionSeconds(size_t Index) const noexcept { return m_Scheduler.GetLastExecutionSeconds(Index); }
		const std::string& GetSystemName(size_t Index) const noexcept { return m_Scheduler.GetSystemName(Index); }

	private:
		bool IsMarkedForDestruction(const Entity& TargetEntity) const noexcept
		{
			using QueuedEntity = ECS::Entity;
			for (const QueuedEntity& queued : m_DestroyQueue)
			{
				if (queued == TargetEntity) { return true; }
			}
			return false;
		}

		EntityRegistry                                                   m_Registry;
		std::unordered_map<std::type_index, std::unique_ptr<IComponentStorage>> m_Storages; // 型消去されたComponentStorage群.
		SystemScheduler                                                  m_Scheduler;
		std::vector<Entity>                                              m_DestroyQueue;
	};

} // namespace ECS
