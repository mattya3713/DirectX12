// Combat基礎SE接続の単体テスト(スタンドアロン. ゲーム本体には含まれない).
// 実Character+実CollisionDetector+実EventBusを使い、成立確定地点からのイベント発火を検証する.
//
// ビルド方法: tests\CombatSEEventTest_build.cmd(依存エンジンソースを直接リンクする. /utf-8必須).
//
// 確認内容:
// 1. 攻撃ヒット成立でCombatHitEventが1回だけ発火する(同一スイング中の重複発火なし)
// 2. スイングを切り直す(有効化ID更新)と再発火する
// 3. イベントに被弾キャラクターと接触点が正しく入る
// 4. Player被弾ではPlayerDamagedEventのみ発火し、攻撃ヒットと混線しない
// 5. パリィ成功(CombatCoordinator::OnParrySuccess)でParrySuccessEventが発火する
// 6. ShouldIgnoreHitで無視されたヒット(パリィ済み攻撃相当)はSEイベントを発火しない
// 7. EventBus未登録でもヒット処理がクラッシュしない

#include <iostream>

#include <DirectXMath.h>

#include "../SourceCode/00_Game/10_Object/10_MeshObject/00_Character/Character.h"
#include "../SourceCode/00_Game/00_GameLoop/Time/Time.h"
#include "../SourceCode/00_Game/40_Collision/CollisionDetector.h"
#include "../SourceCode/00_Game/60_Combat/CombatCoordinator.h"
#include "../SourceCode/00_Game/60_Combat/CombatEvents.h"
#include "../SourceCode/99_Utility/Event/EventBus.h"
#include "../SourceCode/99_Utility/ServiceLocator/ServiceLocator.h"

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
		std::cout << "PASS: " << Label << std::endl; // クラッシュ特定のため毎行フラッシュする.
	}

	// マスクを差し替えられるテスト用キャラクター(実体押し出しは無効化して位置を固定する).
	class SeTestChar : public Character
	{
	public:
		void ConfigureAsAttacker(eCollisionGroup AttackMine, eCollisionGroup AttackTarget)
		{
			m_DamageCollider.SetMyMask(eCollisionGroup::None);
			m_DamageCollider.SetTargetMask(eCollisionGroup::None);
			m_BodyCollider.SetMyMask(eCollisionGroup::None);
			m_BodyCollider.SetTargetMask(eCollisionGroup::None);
			m_AttackCollider.SetMyMask(AttackMine);
			m_AttackCollider.SetTargetMask(AttackTarget);
			m_AttackCollider.SetAttackAmount(10.0f);
			SetAttackColliderActive(true);
		}

		void ConfigureAsVictim(eCollisionGroup DamageMine, eCollisionGroup DamageTarget)
		{
			m_DamageCollider.SetMyMask(DamageMine);
			m_DamageCollider.SetTargetMask(DamageTarget);
			m_BodyCollider.SetMyMask(eCollisionGroup::None);
			m_BodyCollider.SetTargetMask(eCollisionGroup::None);
			m_AttackCollider.SetMyMask(eCollisionGroup::None);
			m_AttackCollider.SetTargetMask(eCollisionGroup::None);
		}
	};

	// 全てのヒットを無視するテスト用被弾者(パリィ済み攻撃の二重処理防止経路の再現).
	class IgnoringVictim : public SeTestChar
	{
	protected:
		bool ShouldIgnoreHit(const CollisionInfo&) const noexcept override { return true; }
	};

	void StepOneFrame(CollisionDetector& Detector, Character& Attacker, Character& Victim)
	{
		Detector.ExecuteCollisionDetection();
		Attacker.Update();
		Victim.Update();
	}

	void ResetPosition(Character& Char, float Z)
	{
		Transform t{};
		t.Position = { 0.0f, 0.0f, Z };
		Char.SetTransform(t);
	}

}

int main()
{
	int hit_count     = 0;
	int damaged_count = 0;
	int parry_count   = 0;

	CombatHitEvent     last_hit{};
	PlayerDamagedEvent last_damaged{};

	EventBus bus;
	bus.Subscribe<CombatHitEvent>([&](const CombatHitEvent& e) { ++hit_count; last_hit = e; });
	bus.Subscribe<PlayerDamagedEvent>([&](const PlayerDamagedEvent& e) { ++damaged_count; last_damaged = e; });
	bus.Subscribe<ParrySuccessEvent>([&](const ParrySuccessEvent&) { ++parry_count; });

	CollisionDetector detector;
	GameTime game_time; // ヒットストップ発火(SetTimeScale)がServiceLocator経由のため実体が必要.
	ServiceLocator::Provide<EventBus>(&bus);
	ServiceLocator::Provide<CollisionDetector>(&detector);
	ServiceLocator::Provide<GameTime>(&game_time);

	// ===== 1. 攻撃ヒット成立 → CombatHitEvent(同一スイング中は1回だけ) =====
	{
		SeTestChar attacker;
		SeTestChar target;
		attacker.ConfigureAsAttacker(eCollisionGroup::PlayerAttack, eCollisionGroup::EnemyDamage);
		target.ConfigureAsVictim(eCollisionGroup::EnemyDamage, eCollisionGroup::PlayerAttack);
		ResetPosition(attacker, -2.5f);
		ResetPosition(target, 0.0f);

		hit_count = damaged_count = 0;
		StepOneFrame(detector, attacker, target);

		Check(hit_count == 1, "1a: 攻撃ヒットでCombatHitEventが1回発火");
		Check(damaged_count == 0, "1b: 被弾イベントは混線しない");

		// 同一スイングのまま再検出しても増えない(重複SE防止).
		StepOneFrame(detector, attacker, target);
		Check(hit_count == 1, "1c: 同一スイング中は再発火しない");
	}

	// ===== 2. スイング切替(有効化ID更新)で再発火 =====
	{
		SeTestChar attacker;
		SeTestChar target;
		attacker.ConfigureAsAttacker(eCollisionGroup::PlayerAttack, eCollisionGroup::EnemyDamage);
		target.ConfigureAsVictim(eCollisionGroup::EnemyDamage, eCollisionGroup::PlayerAttack);
		ResetPosition(attacker, -2.5f);
		ResetPosition(target, 0.0f);

		hit_count = 0;
		StepOneFrame(detector, attacker, target);
		Check(hit_count == 1, "2a: 1スイング目で発火");

		attacker.SetAttackColliderActive(false);
		attacker.SetAttackColliderActive(true); // 有効化IDが進み新しいスイングになる.
		StepOneFrame(detector, attacker, target);
		Check(hit_count == 2, "2b: 新しいスイングで再発火する");
	}

	// ===== 3. イベントペイロード(Victim/ContactPoint) =====
	{
		SeTestChar attacker;
		SeTestChar target;
		attacker.ConfigureAsAttacker(eCollisionGroup::PlayerAttack, eCollisionGroup::EnemyDamage);
		target.ConfigureAsVictim(eCollisionGroup::EnemyDamage, eCollisionGroup::PlayerAttack);
		ResetPosition(attacker, -2.5f);
		ResetPosition(target, 0.0f);

		hit_count = 0;
		last_hit  = CombatHitEvent{};
		StepOneFrame(detector, attacker, target);

		Check(hit_count == 1, "3a: 発火済み");
		Check(last_hit.Victim == &target, "3b: Victimに被弾キャラが入る");
		const bool contact_is_between = last_hit.ContactPoint.z > -2.5f && last_hit.ContactPoint.z < 1.0f;
		Check(contact_is_between && last_hit.ContactPoint.y > 0.5f, "3c: 接触点が2体の間の高さ付近にある");
	}

	// ===== 4. Player被弾 → PlayerDamagedEventのみ =====
	{
		SeTestChar attacker;
		SeTestChar player_victim;
		attacker.ConfigureAsAttacker(eCollisionGroup::EnemyAttack, eCollisionGroup::PlayerDamage);
		player_victim.ConfigureAsVictim(eCollisionGroup::PlayerDamage, eCollisionGroup::EnemyAttack);
		ResetPosition(attacker, -2.5f);
		ResetPosition(player_victim, 0.0f);

		hit_count = damaged_count = 0;
		last_damaged = PlayerDamagedEvent{};
		StepOneFrame(detector, attacker, player_victim);

		Check(damaged_count == 1, "4a: Player被弾でPlayerDamagedEventが1回発火");
		Check(hit_count == 0, "4b: 攻撃ヒットイベントは発火しない");
		Check(last_damaged.Victim == &player_victim, "4c: VictimにPlayerが入る");
	}

	// ===== 5. パリィ成功 → ParrySuccessEvent(View未接続でも成立通知は発火する) =====
	{
		CombatCoordinator coordinator; // 意図的にInitializeしない(発火がView依存でないことの検証).

		parry_count = 0;
		coordinator.OnParrySuccess();
		Check(parry_count == 1, "5a: OnParrySuccessでParrySuccessEventが発火");

		coordinator.OnParrySuccess();
		Check(parry_count == 2, "5b: 成立のたびに発火する");
	}

	// ===== 6. 無視されたヒット(パリィ済み攻撃相当)はSEイベントを発火しない =====
	{
		SeTestChar attacker;
		IgnoringVictim victim;
		attacker.ConfigureAsAttacker(eCollisionGroup::PlayerAttack, eCollisionGroup::EnemyDamage);
		victim.ConfigureAsVictim(eCollisionGroup::EnemyDamage, eCollisionGroup::PlayerAttack);
		ResetPosition(attacker, -2.5f);
		ResetPosition(victim, 0.0f);

		hit_count = damaged_count = parry_count = 0;
		StepOneFrame(detector, attacker, victim);

		Check(hit_count == 0 && damaged_count == 0 && parry_count == 0,
			"6: ShouldIgnoreHitのヒットからはSEイベントが発火しない");
	}

	// ===== 7. EventBus未登録でもクラッシュしない =====
	{
		ServiceLocator::Provide<EventBus>(nullptr);

		SeTestChar attacker;
		SeTestChar target;
		attacker.ConfigureAsAttacker(eCollisionGroup::PlayerAttack, eCollisionGroup::EnemyDamage);
		target.ConfigureAsVictim(eCollisionGroup::EnemyDamage, eCollisionGroup::PlayerAttack);
		ResetPosition(attacker, -2.5f);
		ResetPosition(target, 0.0f);

		bool reached_end = false;
		// クラッシュ時はプロセスが落ちるため「ここまで戻ってきたこと」自体を合格とする.
		StepOneFrame(detector, attacker, target);
		reached_end = true;
		Check(reached_end, "7: EventBus未登録でもヒット処理が完走する");

		ServiceLocator::Provide<EventBus>(&bus);
	}

	ServiceLocator::Provide<CollisionDetector>(nullptr);
	ServiceLocator::Provide<GameTime>(nullptr);
	ServiceLocator::Provide<EventBus>(nullptr);

	std::cout << "\nALL TESTS PASSED (" << g_CheckCount << " checks)" << std::endl;
	return 0;
}
