// Standalone verification for ActionTimelineEditor numeric direct input validation
// (ClampSettings: same range constraints as drag editing).
#include <cstdio>

#include "99_Utility/Debug/Imgui/ActionTimelineEditor.h"

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

	using Settings = ActionTimelineEditor::SettingsData;

	// --- already valid data: no clamping happens ---
	Settings valid;
	valid.ComboStartTime    = 0.2f;
	valid.MinComboTransTime = 0.5f;
	valid.ComboEndTime      = 1.0f;
	valid.Windows.push_back({ 0.3f, 0.4f });

	expect(ActionTimelineEditor::ClampSettings(valid) == false, "valid input is not clamped");
	expect(valid.ComboStartTime == 0.2f && valid.MinComboTransTime == 0.5f && valid.ComboEndTime == 1.0f,
	       "valid combo times are untouched");
	expect(valid.Windows[0].Start == 0.3f && valid.Windows[0].Duration == 0.4f, "valid window is untouched");

	// --- ComboEndTime range (min 0.1, max 3600) ---
	Settings end_range;
	end_range.ComboEndTime = -5.0f;
	ActionTimelineEditor::ClampSettings(end_range);
	expect(end_range.ComboEndTime == 0.1f, "negative combo end clamps to 0.1");

	end_range.ComboEndTime = 99999.0f;
	ActionTimelineEditor::ClampSettings(end_range);
	expect(end_range.ComboEndTime == 3600.0f, "huge combo end clamps to 3600");

	// --- collider window constraints ---
	Settings windows;
	windows.ComboEndTime = 1.0f;
	windows.Windows.push_back({ -0.5f,  0.2f });  // start < 0 -> 0.
	windows.Windows.push_back({  0.5f, -1.0f });  // duration <= 0 -> 0.01.
	windows.Windows.push_back({  0.9f,  0.5f });  // start+duration > end -> duration = end-start.

	ActionTimelineEditor::ClampSettings(windows);
	expect(windows.Windows[0].Start == 0.0f, "negative window start clamps to 0");
	expect(windows.Windows[1].Duration == 0.01f, "non-positive duration clamps to 0.01");
	expect(windows.Windows[2].Duration > 0.099f && windows.Windows[2].Duration < 0.101f,
	       "window overflowing combo end clamps to fit (1.0-0.9)");

	// --- combo time ordering: start <= min_trans <= end ---
	Settings order;
	order.ComboEndTime      = 1.0f;
	order.ComboStartTime    = 2.0f; // start > end -> clamped to end.
	order.MinComboTransTime = 0.1f; // then min_trans < start -> clamped to start.

	ActionTimelineEditor::ClampSettings(order);
	expect(order.ComboStartTime == 1.0f, "combo start beyond end clamps to end");
	expect(order.MinComboTransTime == 1.0f, "min trans below start clamps up to start");

	order.ComboStartTime    = 0.2f;
	order.MinComboTransTime = 5.0f; // min_trans > end -> clamped to end.
	ActionTimelineEditor::ClampSettings(order);
	expect(order.MinComboTransTime == 1.0f, "min trans beyond end clamps to end");
	expect(order.ComboStartTime <= order.MinComboTransTime && order.MinComboTransTime <= order.ComboEndTime,
	       "ordering constraint start <= min_trans <= end always holds");

	// --- was_clamped flag semantics ---
	Settings untouched = valid;
	expect(ActionTimelineEditor::ClampSettings(untouched) == false, "no-change input keeps was_clamped=false");

	std::printf("--- %d passed, %d failed ---\n", passed, failed);

	return failed == 0 ? 0 : 1;
}
