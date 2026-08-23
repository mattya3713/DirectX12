// Standalone verification for SpriteObject / UIObject base classes
// (task_sprite_ui_object_bases acceptance criteria).
#include <cassert>
#include <cstdio>
#include <limits>
#include <concepts>
#include <memory>

#include "00_Game/10_Object/00_Base/GameObject.h"
#include "00_Game/10_Object/20_SpriteObject/SpriteObject.h"
#include "00_Game/10_Object/30_UIObject/UIObject.h"
#include "00_Game/10_Object/30_UIObject/UISpriteObject.h"

namespace {

	// Counts Update calls through the UIObject interface.
	class TestUIWidget final : public UIObject
	{
	public:
		~TestUIWidget() override = default;

		void Update() override { ++m_UpdateCount; }

		int m_UpdateCount = 0;
	};

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

	// --- SpriteObject: defaults ---
	SpriteObject sprite;
	expect(sprite.GetImageId().empty(), "SpriteObject default image id is empty");
	expect(sprite.GetSize().x == 1.0f && sprite.GetSize().y == 1.0f, "SpriteObject default size is 1x1");
	expect(sprite.GetColor().x == 1.0f && sprite.GetColor().w == 1.0f, "SpriteObject default color is white");
	expect(sprite.GetUVRect().x == 0.0f && sprite.GetUVRect().z == 1.0f, "SpriteObject default UV rect is full");
	expect(sprite.IsVisible(), "SpriteObject is visible by default");
	expect(sprite.GetBillboard() == SpriteObject::eBillboard::None, "SpriteObject default billboard is None");

	// --- SpriteObject: setters/getters ---
	sprite.SetImageId("damage_effect_01");
	sprite.SetSize({ 2.0f, 3.0f });
	sprite.SetColor({ 1.0f, 0.5f, 0.25f, 0.75f });
	sprite.SetUVRect({ 0.1f, 0.2f, 0.6f, 0.9f });
	sprite.SetVisible(false);
	sprite.SetBillboard(SpriteObject::eBillboard::YAxisLocked);
	sprite.SetPosition({ 4.0f, 5.0f, -6.0f });

	expect(sprite.GetImageId() == "damage_effect_01", "image id round-trips");
	expect(sprite.GetSize().x == 2.0f && sprite.GetSize().y == 3.0f, "size round-trips");
	expect(sprite.GetColor().y == 0.5f && sprite.GetColor().w == 0.75f, "color round-trips");
	expect(sprite.GetUVRect().y == 0.2f && sprite.GetUVRect().w == 0.9f, "UV rect round-trips");
	expect(!sprite.IsVisible(), "visible flag round-trips");
	expect(sprite.GetBillboard() == SpriteObject::eBillboard::YAxisLocked, "billboard mode round-trips");

	// --- SpriteObject: world transform via GameObject base ---
	GameObject* as_base = &sprite;
	expect(as_base->GetPosition().x == 4.0f && as_base->GetPosition().z == -6.0f,
	       "world position reachable through GameObject interface");

	sprite.Update();
	sprite.Draw();
	as_base->Update();

	// --- UIObject: defaults and setters ---
	UIObject ui;
	expect(ui.GetPosition().x == 0.0f && ui.GetPosition().y == 0.0f, "UIObject default position is origin");
	expect(ui.GetSize().x == 32.0f && ui.GetSize().y == 32.0f, "UIObject default size is 32x32");
	expect(ui.GetRotationDeg() == 0.0f, "UIObject default rotation is 0");
	expect(ui.GetLayer() == 0, "UIObject default layer is 0");
	expect(ui.IsVisible(), "UIObject is visible by default");

	ui.SetPosition({ 128.0f, 256.0f });
	ui.SetSize({ 64.0f, 48.0f });
	ui.SetRotationDeg(45.0f);
	ui.SetLayer(10);
	ui.SetColor({ 0.0f, 0.0f, 1.0f, 1.0f });
	ui.SetVisible(false);

	expect(ui.GetPosition().y == 256.0f && ui.GetSize().x == 64.0f, "screen rect round-trips");
	expect(ui.GetRotationDeg() == 45.0f && ui.GetLayer() == 10, "rotation/layer round-trip");
	expect(!ui.IsVisible(), "UI visible flag round-trips");

	// --- UI hierarchy: virtual dispatch through the base ---
	TestUIWidget widget;
	UIObject* p_ui = &widget;

	p_ui->Update();
	p_ui->Update();
	p_ui->Draw();

	expect(widget.m_UpdateCount == 2, "derived Update is dispatched virtually");

	// --- UISpriteObject ---
	UISpriteObject ui_sprite;
	expect(ui_sprite.IsVisible() && ui_sprite.GetImageId().empty(), "UISpriteObject has sane defaults");

	ui_sprite.SetImageId("ui/hp_gauge");
	ui_sprite.SetUVRect({ 0.0f, 0.5f, 0.5f, 1.0f });
	ui_sprite.SetPosition({ 100.0f, 50.0f });

	expect(ui_sprite.GetImageId() == "ui/hp_gauge", "UI sprite image id round-trips");
	expect(ui_sprite.GetUVRect().z == 0.5f, "UI sprite UV rect round-trips");

	const UIObject* p_ui_base = &ui_sprite;
	expect(p_ui_base->GetPosition().x == 100.0f, "UISpriteObject behaves as UIObject in hierarchy");

	std::printf("--- %d passed, %d failed ---\n", passed, failed);

	return failed == 0 ? 0 : 1;
}
