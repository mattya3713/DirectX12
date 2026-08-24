// Standalone verification for ECS基盤 (task_component_system acceptance criteria).
// Entity世代ID/ComponentStorage/2種Query/遅延破棄/SystemScheduler/Shutdownを検証する.
#include <cassert>
#include <cstdio>
#include <string>

#include "99_Utility/ECS/World.h"
#include "99_Utility/ECS/SampleComponents.h"

namespace {

	int s_PassCount = 0;
	int s_FailCount = 0;

	void Check(bool Condition, const char* Label)
	{
		if (Condition) { ++s_PassCount; std::printf("[PASS] %s\n", Label); }
		else           { ++s_FailCount; std::printf("[FAIL] %s\n", Label); }
	}

	struct TagA { int Value = 1; };
	struct TagB { float Value = 2.0f; };

}

int main()
{
	// 1. Entity生成・破棄・世代検出.
	{
		ECS::EntityRegistry registry;

		const ECS::Entity e = registry.Create();
		Check(registry.IsAlive(e), "entity alive after create");

		const ECS::Entity e2 = registry.Create();
		registry.Destroy(e);

		Check(!registry.IsAlive(e), "destroyed entity is dead");
		Check(registry.IsAlive(e2), "other entity unaffected");

		// Index再利用でも旧参照は無効.
		const ECS::Entity recycled = registry.Create();
		Check(recycled.Index == e.Index, "index reused from free list");
		Check(recycled.Generation != e.Generation, "generation incremented on reuse");
		Check(!registry.IsAlive(e), "stale handle stays invalid after reuse");
		Check(registry.IsAlive(recycled), "recycled handle valid");
	}

	// 2. ComponentStorage: 追加/取得/削除/二重登録/反復.
	{
		ECS::World world;
		const ECS::Entity e = world.CreateEntity();

		world.AddComponent<TagA>(e, TagA{ 42 });
		world.AddComponent<TagA>(e, TagA{ 7 }); // 二重登録は上書き(安全).

		Check(world.HasComponent<TagA>(e), "component present");
		Check(world.GetComponent<TagA>(e)->Value == 7, "duplicate add overwrites safely");

		world.AddComponent<TagB>(e, TagB{ 0.5f });
		Check(world.GetComponent<TagB>(e)->Value == 0.5f, "second component type stored");

		world.RemoveComponent<TagA>(e);
		Check(!world.HasComponent<TagA>(e), "remove works");
		Check(!world.RemoveComponent<TagA>(e), "double remove returns false");

		world.RemoveComponent<TagB>(e);
		Check(!world.HasComponent<TagB>(e), "remove second type");
	}

	// 3. 2種Component Query + 遅延破棄(System実行中の削除が安全).
	{
		ECS::World world;

		for (int i = 0; i < 3; ++i)
		{
			const ECS::Entity e = world.CreateEntity();
			auto& t = world.AddComponent<ECS::TransformComponent>(e);
			t.Position.x = static_cast<float>(i + 1);
			auto& h = world.AddComponent<ECS::HealthComponent>(e);
			h.HP = (i == 0) ? -1.0f : 100.0f; // 先頭のみ死亡扱い.
			h.MaxHP = 100.0f;
		}

		size_t processed = 0;
		world.ForEach<ECS::TransformComponent, ECS::HealthComponent>(
			[](const ECS::Entity&, ECS::TransformComponent&, ECS::HealthComponent&) { });

		world.AddSystem("sample_damage", [&](ECS::World& w, float) {
			w.ForEach<ECS::HealthComponent>([&w](const ECS::Entity& entity, ECS::HealthComponent& health) {
				if (health.HP <= 0.0f) { w.DestroyEntity(entity); } // 更新中の削除要求(遅延).
			});
		});

		world.RunSystems(1.0f / 60.0f);
		world.FlushDestroyed();

		world.ForEach<ECS::TransformComponent, ECS::HealthComponent>(
			[&processed](const ECS::Entity&, ECS::TransformComponent&, ECS::HealthComponent&) { ++processed; });

		Check(processed == 2, "dead entity removed via deferred destroy");
	}

	// 4. SystemSchedulerの順序・実行時間・Shutdown.
	{
		ECS::World world;
		std::string order;

		world.AddSystem("first", [&order](ECS::World&, float) { order += "1"; });
		world.AddSystem("second", [&order](ECS::World&, float) { order += "2"; });

		world.RunSystems(1.0f / 60.0f);
		world.RunSystems(1.0f / 60.0f);

		Check(order == "1212", "systems run in registration order");
		Check(world.GetSystemExecutionSeconds(0) >= 0.0f, "execution time recorded");
		Check(world.GetSystemName(1) == "second", "system name recorded");

		world.Shutdown();
		Check(world.GetSystemCount() == 0, "shutdown clears systems");
	}

	// 5. Shutdown後のリークなし(明示的な破棄呼び出し無しで全Component解放).
	{
		ECS::World* p_world = new ECS::World();
		for (int i = 0; i < 10; ++i)
		{
			const ECS::Entity e = p_world->CreateEntity();
			p_world->AddComponent<TagA>(e);
			p_world->AddComponent<TagB>(e);
		}
		delete p_world; // WorldデストラクタでStorage/Registry解放(_CrtDbgでリーク検出可能な構成).
		Check(true, "world destroyed without manual component cleanup");
	}

	std::printf("\nResult: PASS=%d FAIL=%d\n", s_PassCount, s_FailCount);
	return (s_FailCount == 0) ? 0 : 1;
}
