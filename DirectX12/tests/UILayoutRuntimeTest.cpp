// Standalone verification for UILayoutRuntime (task_ui_runtime_integration acceptance criteria).
#include <cassert>
#include <cstdio>

#include "00_Game/20_UI/UILayoutRuntime.h"

#include "10_Ggraphic/10_Device/DirectX/DirectX12.h"
#include "10_Ggraphic/20_Render/Sprite/SpriteRenderer.h"

// Draw経路は実機確認で検証するため、デバイス依存シンボルのみダミー実装へ差し替える.
MyComPtr<ID3D12Resource> DirectX12::GetTextureByPath(const char*) { return {}; }
void SpriteRenderer::DrawSprite2D(ID3D12Resource*, float, float, float, float, const DirectX::XMFLOAT4&) {}
namespace MyString {
	std::wstring StringToWString(const std::string&) { return {}; }
}

namespace {

	UIElementDesc MakeElement(const char* pId, const char* pName, const char* pType,
		float AnchorX, float AnchorY, float OffsetX, float OffsetY, float Width, float Height, int Layer, bool IsVisible)
	{
		UIElementDesc desc;
		desc.Id = pId;
		desc.Name = pName;
		desc.Type = pType;
		desc.ImageId = "Data\\Image\\toon01.bmp";
		desc.AnchorX = AnchorX;
		desc.AnchorY = AnchorY;
		desc.OffsetX = OffsetX;
		desc.OffsetY = OffsetY;
		desc.Width = Width;
		desc.Height = Height;
		desc.Layer = Layer;
		desc.IsVisible = IsVisible;

		return desc;
	}

} // namespace

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

	// --- BuildDrawItems: layer order / visibility / type filter / anchor math ---
	std::vector<UIElementDesc> elements = {
		MakeElement("a", "BossHPBack", "Sprite", 0.5f, 0.0f, -100.0f, 10.0f, 200.0f, 20.0f, 1, true),
		MakeElement("b", "BossHP",     "Sprite", 0.5f, 0.0f, -98.0f, 12.0f, 196.0f, 16.0f, 2, true),
		MakeElement("c", "Hidden",     "Sprite", 0.5f, 0.5f, 0.0f, 0.0f, 50.0f, 50.0f, 3, false),
		MakeElement("d", "TimerLabel", "Text",   0.9f, 0.9f, 0.0f, 0.0f, 80.0f, 24.0f, 4, true),
		MakeElement("e", "NoImage",    "Sprite", 0.1f, 0.1f, 0.0f, 0.0f, 30.0f, 30.0f, 5, true),
	};
	elements[4].ImageId.clear(); // 欠損画像の代わり(空ID).

	const std::map<std::string, float> no_ratios;
	const std::vector<UISpriteDrawItem> items =
		UILayoutRuntime::BuildDrawItems(elements, no_ratios, 1920.0f, 1080.0f);

	expect(items.size() == 3, "only visible Sprite items with an image id are built");
	expect(items[0].PosX == 860.0f && items[0].PosY == 10.0f, "anchor+offset position is resolved (1920x1080)");
	expect(items[1].Width == 196.0f && items[1].Height == 16.0f, "second item keeps its size");

	bool text_included = false;
	for (const UISpriteDrawItem& item : items)
	{
		if (item.ImageId.empty()) { text_included = true; }
	}
	expect(!text_included, "Text elements and image-less entries are skipped");

	// --- ratio applies to width only ---
	std::map<std::string, float> ratios;
	ratios["BossHP"] = 0.25f;

	const std::vector<UISpriteDrawItem> filled =
		UILayoutRuntime::BuildDrawItems(elements, ratios, 1920.0f, 1080.0f);
	expect(filled.size() == 3, "ratios do not change the item count");
	expect(filled[1].Width == 49.0f, "HP fill ratio scales the bar width (196 * 0.25)");
	expect(filled[0].Width == 200.0f, "back bar without a ratio entry keeps full width");

	// --- resolution change follows anchors ---
	const std::vector<UISpriteDrawItem> at_720p =
		UILayoutRuntime::BuildDrawItems(elements, ratios, 1280.0f, 720.0f);
	expect(at_720p[0].PosX == 540.0f, "anchor position recalculated for 1280x720 without breaking layout");

	// --- LoadFromFile: missing file -> false ---
	UILayoutRuntime runtime;
	expect(!runtime.LoadFromFile(std::filesystem::path{"Data/Json/UI/no_such_layout.json"}),
	       "missing layout file reports failure");

	// --- fallback default HUD ---
	runtime.LoadDefault();
	expect(runtime.GetElements().size() == 4, "default HUD has back+fill bars for Player and Boss");

	bool has_player = false;
	bool has_boss = false;
	for (const UIElementDesc& e : runtime.GetElements())
	{
		has_player |= (e.Name == "PlayerHP");
		has_boss |= (e.Name == "BossHP");
	}
	expect(has_player && has_boss, "default HUD contains PlayerHP/BossHP fill bars");

	const std::vector<UISpriteDrawItem> default_items =
		UILayoutRuntime::BuildDrawItems(runtime.GetElements(), { }, 1920.0f, 1080.0f);
	expect(default_items.size() == 4 && default_items[3].Width == 400.0f,
	       "default HUD resolves to 4 draw items at full HP");

	std::printf("--- %d passed, %d failed ---\n", passed, failed);

	return failed == 0 ? 0 : 1;
}
