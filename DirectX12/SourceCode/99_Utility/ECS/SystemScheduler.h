#pragma once

#include <chrono>
#include <functional>
#include <string>
#include <vector>

/**********************************************************************************
* @author    : Coder 青龍(せいりゅう).
* @date      : 2026/08/23.
* @brief     : Systemの登録・決定的な順序実行・実行時間計測を行うスケジューラ.
*            : 実行順は登録順(依存がある場合はその順で登録する規約).
*            : 更新中のEntity/Component削除は各System内で即時適用せず、
*            : World側の遅延破棄キューへ積むことを推奨(World::FlushDestroyed参照).
**********************************************************************************/

namespace ECS {

	class World;

	class SystemScheduler final
	{
	public:
		using SystemFn = std::function<void(World&, float)>;

		// Systemを登録する(戻り値は登録順インデックス).
		size_t Add(const std::string& Name, SystemFn System)
		{
			m_Systems.push_back({ Name, std::move(System), std::chrono::duration<float>::zero() });
			return m_Systems.size() - 1;
		}

		// 登録順に全Systemを実行し、各Systemの最終実行時間を記録する.
		void Run(World& World, float DeltaTime)
		{
			for (Entry& entry : m_Systems)
			{
				if (!entry.System) { continue; }

				const auto start = std::chrono::steady_clock::now();
				entry.System(World, DeltaTime);
				const auto end = std::chrono::steady_clock::now();

				entry.LastExecutionSeconds = std::chrono::duration<float>(end - start);
			}
		}

		// 全Systemを解放する(WorldのShutdown時に呼ぶ).
		void Shutdown() { m_Systems.clear(); }

		size_t Count() const noexcept { return m_Systems.size(); }

		// デバッグ表示用: 指定Systemの最終実行時間(秒).
		float GetLastExecutionSeconds(size_t Index) const noexcept
		{
			return (Index < m_Systems.size()) ? m_Systems[Index].LastExecutionSeconds.count() : -1.0f;
		}

		const std::string& GetSystemName(size_t Index) const noexcept { return m_Systems[Index].Name; }

	private:
		struct Entry
		{
			std::string                     Name;
			SystemFn                        System;
			std::chrono::duration<float>    LastExecutionSeconds{};
		};

		std::vector<Entry> m_Systems;
	};

} // namespace ECS
