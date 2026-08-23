// Standalone verification for BTDemoEnemy (task_bt_demo_ai acceptance criteria).
// Reproduces the existing Enemy FSM decision-making (Idle/Chase/Attack) with a
// pure Behavior Tree and asserts equivalent transitions on scripted scenarios.
#include <cassert>
#include <cstdio>
#include <cmath>

#include "99_Utility/BehaviorTree/BTDemoEnemy.h"

namespace {

	constexpr float DT = 1.0f / 60.0f;

	// Places the target at the given XZ distance from the demo enemy (+Z direction).
	void PlaceTargetAtDistance(BTDemoEnemy& Demo, float Distance)
	{
		const float x = Demo.GetPosition().x;
		const float z = Demo.GetPosition().z;

		Demo.SetTargetPos({ x, 0.0f, z + Distance });
	}

	// Runs one update and prints any phase transition (FSM comparison log).
	void Step(BTDemoEnemy& Demo)
	{
		const BTDemoEnemy::ePhase before = Demo.GetPhase();

		Demo.Update(DT);

		const BTDemoEnemy::ePhase after = Demo.GetPhase();

		if (before != after)
		{
			const char* pName = [](BTDemoEnemy::ePhase Phase)
			{
				switch (Phase)
				{
				case BTDemoEnemy::ePhase::Chase:  return "Chase";
				case BTDemoEnemy::ePhase::Attack: return "Attack";
				default: break;
				}

				return "Idle";
			}(after);

			std::printf("transition -> %s\n", pName);
		}
	}

	void StepTimes(BTDemoEnemy& Demo, int Count)
	{
		for (int i = 0; i < Count; ++i) { Step(Demo); }
	}

}

int main()
{
	int passed = 0;
	int failed = 0;

	const auto expect = [&](bool Condition, const char* pWhat)
	{
		if (Condition) {
			++passed;
			std::printf("PASS: %s\n", pWhat);
		}
		else {
			++failed;
			std::printf("FAIL: %s\n", pWhat);
		}
	};

	BTDemoEnemy Demo;

	// --- 1: Far outside both ranges -> stays Idle ---
	PlaceTargetAtDistance(Demo, 30.0f);
	StepTimes(Demo, 10);
	expect(Demo.GetPhase() == BTDemoEnemy::ePhase::Idle, "dist=30 (out of lose/aggro) stays Idle");

	// --- 2: Within aggro range -> Idle turns Chase ---
	PlaceTargetAtDistance(Demo, 8.0f);
	Step(Demo);
	expect(Demo.GetPhase() == BTDemoEnemy::ePhase::Chase, "dist=8 (<= aggro 10) starts Chase");

	// --- 3: Hysteresis - chasing keeps chasing between aggro(10) and lose(20) ---
	PlaceTargetAtDistance(Demo, 15.0f);
	StepTimes(Demo, 30);
	expect(Demo.GetPhase() == BTDemoEnemy::ePhase::Chase, "dist=15 while chasing keeps Chase");

	// --- 4: Beyond lose range -> gives up back to Idle ---
	PlaceTargetAtDistance(Demo, 25.0f);
	Step(Demo);
	expect(Demo.GetPhase() == BTDemoEnemy::ePhase::Idle, "dist=25 (> lose 20) returns to Idle");

	// --- 5: Idle does NOT re-aggro beyond aggro range even under lose range ---
	PlaceTargetAtDistance(Demo, 15.0f);
	StepTimes(Demo, 30);
	expect(Demo.GetPhase() == BTDemoEnemy::ePhase::Idle, "dist=15 while idle stays Idle (aggro hysteresis)");

	// --- 6: Within attack range -> Attack starts and is NOT interrupted even if target flees ---
	PlaceTargetAtDistance(Demo, 2.0f);
	Step(Demo);
	expect(Demo.GetPhase() == BTDemoEnemy::ePhase::Attack, "dist=2 (<= attack 2.5) starts Attack");

	PlaceTargetAtDistance(Demo, 50.0f); // flee far during windup.

	bool interrupted = false;
	bool active_window_seen = false;
	int ticks = 0;
	while (Demo.GetPhase() == BTDemoEnemy::ePhase::Attack && ticks < 200)
	{
		if (Demo.IsAttackActiveWindow()) { active_window_seen = true; }

		Step(Demo);
		++ticks;
	}

	interrupted = (Demo.GetPhase() == BTDemoEnemy::ePhase::Attack);

	expect(!interrupted, "attack finishes on its own timer");
	expect(ticks >= 65 && ticks <= 68, "attack duration matches windup+active+recovery (approx 1.1s)");
	expect(active_window_seen, "attack active window was observed");
	expect(Demo.GetPhase() == BTDemoEnemy::ePhase::Idle, "target fled beyond lose range -> Idle after attack");

	// --- 7: Attack finishing with target within lose range -> resumes Chase ---
	PlaceTargetAtDistance(Demo, 2.0f);
	Step(Demo);
	expect(Demo.GetPhase() == BTDemoEnemy::ePhase::Attack, "second encounter starts Attack");

	while (Demo.GetPhase() == BTDemoEnemy::ePhase::Attack) { Step(Demo); }

	expect(Demo.GetPhase() == BTDemoEnemy::ePhase::Chase, "target within lose range after attack -> Chase");

	std::printf("--- %d passed, %d failed ---\n", passed, failed);

	return failed == 0 ? 0 : 1;
}
