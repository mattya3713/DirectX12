// コライダー登録/解除ライフサイクル単体テスト(スタンドアロン. ゲーム本体には含まれない).
// ビルド例: scripts配下ではなく tests\run_collider_lifecycle_test.cmd を参照.
//
// 確認内容(Character/Player/Bossの実デストラクタと同じ登録/解除パターンを模倣):
// 1. Character相当(3コライダー)の登録
// 2. Player相当(Parry追加)の登録
// 3. 派生デストラクタ→基底デストラクタの順で全コライダーが解除されること
// 4. 未登録ポインタ・二重解除が無害であること
// 5. ServiceLocator未登録状態での生成/破棄がクラッシュしないこと
// 6. シーン切替相当: 旧インスタンスのポインタが残らないこと
// 7. 勝敗確定後相当(破棄せず更新停止): 登録維持+検出実行がクラッシュしないこと

#include <algorithm>
#include <iostream>
#include <vector>

#include "../SourceCode/00_Game/40_Collision/CollisionDetector.h"
#include "../SourceCode/00_Game/40_Collision/00_Capsule/CapsuleCollider.h"
#include "../SourceCode/99_Utility/ServiceLocator/ServiceLocator.h"

namespace {

	// Characterと同じ「コンストラクタで3コライダー登録/デストラクタで解除」を行う最小模倣体
	// (実クラスはモデル・描画依存が重くスタンドアロン不可のため).
	struct CharacterLike
	{
		CapsuleCollider m_DamageCollider{ nullptr };
		CapsuleCollider m_AttackCollider{ nullptr };
		CapsuleCollider m_BodyCollider  { nullptr };

		CharacterLike()
		{
			// 全グループNone(テストでは衝突計算・Transform参照を発生させない).
			m_DamageCollider.SetMyMask(eCollisionGroup::None);
			m_AttackCollider.SetMyMask(eCollisionGroup::None);
			m_BodyCollider.SetMyMask(eCollisionGroup::None);

			if (CollisionDetector* p_detector = ServiceLocator::Get<CollisionDetector>())
			{
				p_detector->RegisterCollider(m_DamageCollider);
				p_detector->RegisterCollider(m_AttackCollider);
				p_detector->RegisterCollider(m_BodyCollider);
			}
		}

		virtual ~CharacterLike()
		{
			if (CollisionDetector* p_detector = ServiceLocator::Get<CollisionDetector>())
			{
				p_detector->UnregisterCollider(&m_DamageCollider);
				p_detector->UnregisterCollider(&m_AttackCollider);
				p_detector->UnregisterCollider(&m_BodyCollider);
			}
		}
	};

	// Playerと同じ「派生デストラクタでParryを先に解除」する模倣体.
	struct PlayerLike : CharacterLike
	{
		CapsuleCollider m_ParryCollider{ nullptr };

		PlayerLike()
		{
			m_ParryCollider.SetMyMask(eCollisionGroup::None);

			if (CollisionDetector* p_detector = ServiceLocator::Get<CollisionDetector>())
			{
				p_detector->RegisterCollider(m_ParryCollider);
			}
		}

		~PlayerLike() override
		{
			if (CollisionDetector* p_detector = ServiceLocator::Get<CollisionDetector>())
			{
				p_detector->UnregisterCollider(&m_ParryCollider);
			}
		}
	};

	// Bossと同じ「追加コライダー無し(基底の解除のみ)」の模倣体.
	struct BossLike : CharacterLike {};

	bool Contains(const std::vector<ColliderBase*>& Colliders, const void* pTarget)
	{
		return std::find(Colliders.begin(), Colliders.end(), pTarget) != Colliders.end();
	}

}

int main()
{
	int failures = 0;
	const auto check = [&failures](bool Condition, const char* pLabel) {
		std::cout << (Condition ? "[PASS] " : "[FAIL] ") << pLabel << std::endl;
		if (!Condition) { ++failures; }
	};

	CollisionDetector detector;
	ServiceLocator::Provide<CollisionDetector>(&detector);

	// --- 1: 初期状態 ---
	check(detector.GetColliders().empty(), "1. 初期状態で登録数0");

	// --- 2/3: Player相当の登録(Character3件+Parry1件=4) ---
	ColliderBase* p_player_parry = nullptr;
	{
		PlayerLike player;
		p_player_parry = &player.m_ParryCollider;
		check(detector.GetColliders().size() == 4, "2. Player相当構築で4件登録");
		check(Contains(detector.GetColliders(), &player.m_ParryCollider), "3a. Parryコライダーが登録済み");
		check(Contains(detector.GetColliders(), &player.m_DamageCollider), "3b. Damageコライダーが登録済み");

		// スコープ抜け: ~PlayerLike(Parry解除)→~CharacterLike(3件解除)の順で走る.
	}
	check(detector.GetColliders().empty(), "4. 派生→基底のデストラクタ順で全解除される");
	check(!Contains(detector.GetColliders(), p_player_parry), "5. 解除後の旧ポインタは一覧に無い");

	// --- 6: Boss相当(BossはBossLikeとして追加コライダー無し) ---
	{
		BossLike boss;
		check(detector.GetColliders().size() == 3, "6. Boss相当構築で3件登録");
	}
	check(detector.GetColliders().empty(), "7. Boss相当破棄で全解除");

	// --- 8/9: 未登録ポインタ・二重解除の無害性 ---
	CapsuleCollider orphan{ nullptr };
	detector.UnregisterCollider(&orphan);
	check(detector.GetColliders().empty(), "8. 未登録ポインタの解除は無害");

	{
		PlayerLike player;
		detector.UnregisterCollider(&player.m_ParryCollider); // 先に一度だけ解除.
		const size_t after_first = detector.GetColliders().size();
		detector.UnregisterCollider(&player.m_ParryCollider); // 二重解除.
		check(detector.GetColliders().size() == after_first, "9. 二重解除は無害(数不変)");
	}
	check(detector.GetColliders().empty(), "10. 二重解除後も残りの解除は正常に完了");

	// --- 11: ServiceLocator未登録でも生成/破棄がクラッシュしない ---
	ServiceLocator::Provide<CollisionDetector>(nullptr);
	{
		PlayerLike player; // Get()がnullptrなので何も登録されない.
		BossLike boss;
		check(true, "11a. ServiceLocator未登録での生成はクラッシュしない");
	}
	check(true, "11b. ServiceLocator未登録での破棄はクラッシュしない");

	// --- 12/13: シーン切替相当(旧ポインタが残らず新規のみ) ---
	ServiceLocator::Provide<CollisionDetector>(&detector);
	std::vector<const void*> old_addresses;
	{
		PlayerLike player;
		BossLike boss;
		for (ColliderBase* p_collider : detector.GetColliders()) { old_addresses.push_back(p_collider); }
		check(detector.GetColliders().size() == 7, "12. Player+Boss同時存在で7件");
	}
	check(detector.GetColliders().empty(), "13. 同時破棄でも全解除(取りこぼしなし)");

	{
		PlayerLike new_player;
		bool stale_found = false;
		for (const void* p_old : old_addresses)
		{
			stale_found = stale_found || Contains(detector.GetColliders(), p_old);
		}
		check(!stale_found, "14. 再入場後の一覧に旧シーンのポインタは無い");
		check(detector.GetColliders().size() == 4, "15. 再入場後は新規分のみ登録");
	}

	// --- 16: 勝敗確定後相当(オブジェクト生存のまま検出のみ継続) ---
	{
		PlayerLike player;
		BossLike boss;
		detector.ExecuteCollisionDetection(); // Main::Updateは勝敗確定後も毎フレーム呼ぶ.
		check(detector.GetColliders().size() == 7, "16a. 更新停止中も登録は維持される");
		bool any_event = false;
		for (ColliderBase* p_collider : detector.GetColliders())
		{
			any_event = any_event || !p_collider->GetCollisionEvents().empty();
		}
		check(!any_event, "16b. マスク無効時は検出実行でもイベントが発生しない(Transform未参照)");
	}
	check(detector.GetColliders().empty(), "17. 検出実行後の破棄でも全解除");

	ServiceLocator::Provide<CollisionDetector>(nullptr);

	std::cout << (failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED") << std::endl;
	return failures == 0 ? 0 : 1;
}
