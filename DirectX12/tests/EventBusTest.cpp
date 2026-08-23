// EventBus単体テスト(スタンドアロン. ゲーム本体には含まれない).
// ビルド例: cl /nologo /EHsc /std:c++20 /W4 EventBusTest.cpp /Fe:EventBusTest.exe
//
// 確認内容:
// 1. 同一イベント型を複数購読し、Publishで全員に届くこと
// 2. 購読者のいないイベント型のPublishで何も起きないこと(クラッシュしない)
// 3. 異なるイベント型が互いに混線しないこと
// 4. UnsubscribeAll()で解除できること
// 5. Publish中に新規Subscribeしても安全であること

#include <cassert>
#include <iostream>
#include <string>

#include "../SourceCode/99_Utility/Event/EventBus.h"

namespace {

	struct EnemyDefeatedEvent
	{
		int EnemyId = 0;
	};

	struct PlayerDamagedEvent
	{
		float Amount = 0.0f;
	};

}

int main()
{
	int failures = 0;
	const auto check = [&failures](bool Condition, const char* pLabel) {
		std::cout << (Condition ? "[PASS] " : "[FAIL] ") << pLabel << std::endl;
		if (!Condition) { ++failures; }
	};

	EventBus bus;

	// --- 1: 複数購読者への一斉通知 ---
	int enemy_defeated_count = 0;
	int last_enemy_id = -1;

	bus.Subscribe<EnemyDefeatedEvent>([&](const EnemyDefeatedEvent&) { ++enemy_defeated_count; });
	bus.Subscribe<EnemyDefeatedEvent>([&](const EnemyDefeatedEvent& e) {
		++enemy_defeated_count;
		last_enemy_id = e.EnemyId;
	});

	bus.Publish(EnemyDefeatedEvent{ 7 });

	check(enemy_defeated_count == 2, "1a: 2購読者両方に通知が届く");
	check(last_enemy_id == 7, "1b: イベントの内容が正しく渡る");

	// --- 2: 購読者ゼロでのPublish ---
	bus.Publish(PlayerDamagedEvent{ 10.0f });
	check(true, "2: 購読者ゼロのPublishでもクラッシュしない");

	// --- 3: 異なるイベント型の混線なし ---
	int player_damaged_count = 0;
	bus.Subscribe<PlayerDamagedEvent>([&](const PlayerDamagedEvent&) { ++player_damaged_count; });

	bus.Publish(EnemyDefeatedEvent{ 3 });
	check(player_damaged_count == 0, "3a: 別型イベントでPlayerDamaged購読者は呼ばれない");
	check(enemy_defeated_count == 4, "3b: EnemyDefeated購読者は従来どおり呼ばれる");

	bus.Publish(PlayerDamagedEvent{ 5.0f });
	check(player_damaged_count == 1, "3c: 自型イベントでは呼ばれる");

	// --- 5: Publish中の新規購読 ---
	int nested_seen = 0;
	bus.Subscribe<EnemyDefeatedEvent>([&](const EnemyDefeatedEvent&) {
		if (nested_seen == 0)
		{
			nested_seen = 1;
			// Publish中に新しい購読を足す(コピーしたリストで回っているため安全).
			bus.Subscribe<EnemyDefeatedEvent>([](const EnemyDefeatedEvent&) {});
		}
	});
	bus.Publish(EnemyDefeatedEvent{ 1 });
	check(nested_seen == 1, "5: Publish中のSubscribeでもクラッシュしない");

	// --- 4: UnsubscribeAll ---
	bus.UnsubscribeAll<EnemyDefeatedEvent>();
	const int count_before = enemy_defeated_count;
	bus.Publish(EnemyDefeatedEvent{ 9 });
	check(enemy_defeated_count == count_before, "4a: UnsubscribeAll後に通知されない");

	std::cout << (failures == 0 ? "\nALL TESTS PASSED" : "\nTESTS FAILED") << " (" << failures << " failures)" << std::endl;
	return failures == 0 ? 0 : 1;
}
