// Standalone verification for UILayoutModel (task_ui_layout_editor acceptance criteria).
#include <cassert>
#include <cstdio>

#include "00_Game/10_Object/30_UIObject/UILayoutModel.h"

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

	// --- add / find ---
	UILayoutModel model;
	const std::string hp_bar = model.AddElement("Sprite", "HPBar");
	const std::string boss_gauge = model.AddElement("Sprite", "BossGauge");
	const std::string timer = model.AddElement("Text", "Timer");

	expect(model.Elements.size() == 3, "three elements added");
	expect(model.Find(hp_bar) != nullptr && model.Find(hp_bar)->Name == "HPBar", "added element is found");
	expect(model.Find("ui1")->Type == "Sprite" && model.Find(timer)->Type == "Text", "types are stored");
	expect(model.AddElement("Sprite", "") .size() > 0, "auto naming keeps id when name empty");

	// --- anchor based position ---
	UIElementDesc* p_hp = model.Find(hp_bar);
	p_hp->AnchorX = 0.0f;
	p_hp->AnchorY = 1.0f;
	p_hp->OffsetX = 16.0f;
	p_hp->OffsetY = -32.0f;

	float pos_x = 0.0f;
	float pos_y = 0.0f;
	UILayoutModel::GetPosition(*p_hp, 1920.0f, 1080.0f, pos_x, pos_y);
	expect(pos_x == 16.0f && pos_y == 1048.0f, "bottom-left anchor position at 1920x1080");

	UILayoutModel::GetPosition(*p_hp, 1280.0f, 720.0f, pos_x, pos_y);
	expect(pos_x == 16.0f && pos_y == 688.0f, "same layout recalculated at 1280x720 follows the anchor");

	// --- layers ---
	model.Find(boss_gauge)->Layer = 5;
	model.Find(timer)->Layer = -2;

	const std::vector<std::string> sorted = model.GetSortedIdsByLayer();
	expect(sorted.size() == 4 && sorted[0] == timer && sorted[1] == hp_bar && sorted[3] == boss_gauge,
	       "layer sort is ascending and stable");

	// --- duplicate ---
	const std::string copied = model.DuplicateElement(boss_gauge);
	expect(!copied.empty() && copied != boss_gauge, "duplicate returns a new id");

	const UIElementDesc* p_copy = model.Find(copied);
	const UIElementDesc* p_source = model.Find(boss_gauge);
	expect(p_copy && p_copy->OffsetX == p_source->OffsetX + 16.0f, "duplicate is offset to avoid overlap");
	expect(p_copy->Name.find("(copy)") != std::string::npos, "duplicate name is marked");

	// --- remove ---
	expect(model.RemoveElement(copied), "remove succeeds for existing id");
	expect(!model.RemoveElement("no_such_id"), "remove fails for unknown id");
	expect(model.Find(copied) == nullptr, "removed element is gone");

	// --- JSON round trip ---
	const nlohmann::json data = model.ToJson();
	UILayoutModel loaded;
	loaded.FromJson(data);
	expect(UILayoutModel::Equal(model, loaded), "JSON round trip preserves the whole layout");

	// --- malformed input ---
	UILayoutModel broken;
	broken.FromJson(nlohmann::json::object());
	expect(broken.Elements.empty(), "JSON without elements array yields empty model");

	nlohmann::json bad_entry = nlohmann::json::array();
	bad_entry.push_back({ { "type", "Sprite" } }); // no id.
	UILayoutModel dropped;
	dropped.FromJson({ { "elements", bad_entry } });
	expect(dropped.Elements.empty(), "entry without id is dropped on load");

	std::printf("--- %d passed, %d failed ---\n", passed, failed);

	return failed == 0 ? 0 : 1;
}
