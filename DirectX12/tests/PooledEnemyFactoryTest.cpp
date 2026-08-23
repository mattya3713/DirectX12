// PooledEnemyFactory単体テスト(スタンドアロン. ゲーム本体には含まれない).
// Enemy実体の構築はServiceLocator未登録でもnullガードで動くため、
// 実Enemyを生成して返却→再取得の完全リセットまで検証する.
// コライダー有効状態のGetterは_DEBUG限定のため本テストでは未検証(コード目視と実機DEBUG統計で確認).
//
// ビルド方法: tests\PooledEnemyFactoryTest_build.cmd(依存エンジンソースを直接リンクする.
// /utf-8必須. 実行コードページへ変換された埋め込みJSONの日本語がnlohmannのUTF-8検証に落ちるため).
//
// 確認内容:
// 1. 返却→再取得でHP/Transform/Stateが完全リセットされる(Created不変=新規alloc無し)
// 2. 二重返却・所有権外ポインタ・nullptr返却はfalseで拒否される
// 3. 容量上限超過のSpawnはスキップ(nullptr)され、返却後は再取得できる
// 4. 再ロード相当(全返却→全再取得)を2回繰り返しても新規allocが発生しない

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#include <d3d12.h>
#include <DirectXMath.h>

#include "../SourceCode/00_Game/50_Enemy/Definition/EnemyDefinitionCatalog.h"
#include "../SourceCode/00_Game/50_Enemy/Factory/PooledEnemyFactory.h"

namespace {

	int g_CheckCount = 0;

	void Check(bool Condition, const char* Label)
	{
		++g_CheckCount;
		if (!Condition)
		{
			std::cerr << "FAILED: " << Label << '\n';
			std::exit(1);
		}
		std::cout << "PASS: " << Label << '\n';
	}

}

int main()
{
	const std::filesystem::path definitions_json = "PooledEnemyFactoryTest.json";
	{
		std::ofstream file(definitions_json);
		file << R"([
			{"Id": "goblin",      "DisplayName": "ゴブリン",     "MaxHP": 40, "MoveSpeed": 3.0},
			{"Id": "goblin_fast", "DisplayName": "速いゴブリン", "MaxHP": 20, "MoveSpeed": 6.5}
		])";
	}

	EnemyDefinitionCatalog catalog;
	Check(catalog.Load(definitions_json) && catalog.Size() == 2, "catalog Load");

	// ===== 1. Spawn: 既知IDから実体を生成し定義を適用する =====
	{
		PooledEnemyFactory factory(catalog, 2);

		Transform spawn_transform{};
		spawn_transform.Position = { 1.0f, 0.0f, 2.0f };

		Enemy* p_enemy = factory.Spawn("goblin", spawn_transform);
		Check(p_enemy != nullptr, "Spawn(goblin)");
		Check(p_enemy->GetHealth().GetMaxHP() == 40.0f, "definition MaxHP applied");
		Check(p_enemy->GetHealth().GetHP() == 40.0f, "spawned at full HP");
		Check(p_enemy->GetCurrentStateID() == EnemyState::eID::Idle, "initial state Idle");

		const PooledEnemyFactory::Stats stats = factory.GetStats();
		Check(stats.Created == 1 && stats.Active == 1 && stats.Reused == 0, "stats after first Spawn");
	}

	// ===== 1b. 未知ID・不正定義はnullptr =====
	{
		PooledEnemyFactory factory(catalog, 2);
		Transform t{};
		Check(factory.Spawn("dragon", t) == nullptr, "unknown id returns nullptr");
	}

	// ===== 2. 死亡→返却→再取得で完全リセット(新規alloc無し) =====
	{
		PooledEnemyFactory factory(catalog, 2);

		Transform first{};
		first.Position = { 0.0f, 0.0f, 0.0f };
		first.Scale    = { 1.0f, 1.0f, 1.0f };
		Enemy* p_first = factory.Spawn("goblin", first);

		// 使用中に壊す(死亡フロー+移動+スケール変更).
		p_first->ApplyDebugKill(); // HP=0でOnDeath発火→Dead遷移.
		Check(p_first->GetHealth().GetHP() == 0.0f, "killed to HP=0");
		Check(p_first->GetCurrentStateID() == EnemyState::eID::Dead, "state Dead after kill");

		Transform moved{};
		moved.Position   = { 50.0f, 3.0f, -7.0f };
		moved.Rotation.y = 1.5f;
		moved.Scale      = { 2.0f, 2.0f, 2.0f };
		p_first->SetTransform(moved);

		Check(factory.Return(p_first), "Return after death");
		Check(p_first == nullptr, "Return nulls caller reference");

		Transform second{};
		second.Position = { 9.0f, 0.0f, 9.0f };
		second.Scale    = { 1.0f, 1.0f, 1.0f };
		Enemy* p_second = factory.Spawn("goblin", second);
		Check(p_second != nullptr, "re-Spawn after return");
		Check(p_second->GetHealth().GetHP() == 40.0f, "HP reset on reuse");
		Check(p_second->GetCurrentStateID() == EnemyState::eID::Idle, "state reset to Idle on reuse");
		Check(p_second->GetHealth().IsAlive(), "alive again on reuse");

		const Transform restored = p_second->GetTransform();
		Check(restored.Position.x == 9.0f && restored.Position.z == 9.0f, "position reset on reuse");
		Check(restored.Scale.x == 1.0f, "scale reset on reuse");

		const PooledEnemyFactory::Stats stats = factory.GetStats();
		Check(stats.Created == 1, "Created still 1 (reused, no new alloc)");
		Check(stats.Reused == 1 && stats.Returned == 1, "reuse/return counted");
	}

	// ===== 3. 二重返却・所有権外ポインタ・nullptrの拒否 =====
	{
		PooledEnemyFactory factory(catalog, 2);
		Transform t{};
		t.Scale = { 1.0f, 1.0f, 1.0f };

		Enemy* p_enemy = factory.Spawn("goblin", t);
		Enemy* p_copy  = p_enemy;
		Check(factory.Return(p_enemy), "first Return accepted");
		Check(!factory.Return(p_copy), "double Return rejected");
		Enemy* p_null = nullptr;
		Check(!factory.Return(p_null), "nullptr Return rejected");

		Enemy foreign; // プール外で直接構築した所有権外インスタンス.
		Enemy* p_foreign = &foreign;
		Check(!factory.Return(p_foreign), "foreign pointer rejected");

		const PooledEnemyFactory::Stats stats = factory.GetStats();
		Check(stats.Returned == 1 && stats.Active == 0, "rejects leave pool state untouched");
	}

	// ===== 4. 容量上限超過のSpawnはスキップ、返却後は再取得できる =====
	{
		PooledEnemyFactory factory(catalog, 2);
		Transform t{};
		t.Scale = { 1.0f, 1.0f, 1.0f };

		Enemy* p_a = factory.Spawn("goblin", t);
		Enemy* p_b = factory.Spawn("goblin_fast", t);
		Check(p_a != nullptr && p_b != nullptr, "two spawns up to capacity");

		Enemy* p_c = factory.Spawn("goblin", t);
		Check(p_c == nullptr, "capacity overflow skipped (nullptr)");

		Check(factory.Return(p_a), "return A");
		p_c = factory.Spawn("goblin_fast", t);
		Check(p_c != nullptr, "spawn succeeds after return");
		Check(p_c->GetHealth().GetMaxHP() == 20.0f, "reused instance re-tuned to new definition");

		const PooledEnemyFactory::Stats stats = factory.GetStats();
		Check(stats.Created == 2, "no new alloc in overflow cycle");
	}

	// ===== 5. 再ロード相当(全返却→全再取得)を2回繰り返しても新規allocゼロ =====
	{
		PooledEnemyFactory factory(catalog, 8);

		std::vector<Enemy*> active;
		Transform t{};
		t.Scale = { 1.0f, 1.0f, 1.0f };

		// レベル再ロードの相当処理(使用中を全返却→計画数だけ再取得).
		const auto reload = [&factory, &active, &t]() {
			for (Enemy* p_enemy : active) {
				(void)factory.Return(p_enemy);
			}
			active.clear();
			for (size_t i = 0; i < 5; ++i) {
				Transform spawn_transform = t;
				spawn_transform.Position  = { static_cast<float>(i), 0.0f, static_cast<float>(i) };
				Enemy* p_enemy = factory.Spawn((i % 2 == 0) ? "goblin" : "goblin_fast", spawn_transform);
				if (p_enemy) { active.push_back(p_enemy); }
			}
		};

		reload();
		const size_t created_after_first_load = factory.GetStats().Created;
		Check(created_after_first_load == 5, "first load creates exactly plan count");

		reload(); // 再ロード1回目.
		reload(); // 再ロード2回目.

		const PooledEnemyFactory::Stats stats = factory.GetStats();
		Check(stats.Created == created_after_first_load, "reload x2 keeps Created unchanged (zero new alloc)");
		Check(stats.Active == active.size(), "active count matches alive enemies");
		Check(factory.GetStats().Returned == 10, "all returns recorded");
	}

	std::filesystem::remove(definitions_json);

	std::cout << "All " << g_CheckCount << " checks passed.\n";
	return 0;
}