#include "UILayoutRuntime.h"

#include <algorithm>

#include "10_Ggraphic/10_Device/DirectX/DirectX12.h"
#include "10_Ggraphic/20_Render/Sprite/SpriteRenderer.h"
#include "99_Utility/FileManager/FileManager.h"

namespace {

	// 既定HUDで使う画像(実機での読込実績がある既存テクスチャ).
	constexpr const char* kDefaultImage = "Data\\Image\\toon01.bmp";

	// 既定レイアウトのHPバー要素を作る(背景+塗りの2枚組).
	UIElementDesc MakeHpBar(const char* pName, float AnchorX, float AnchorY, float OffsetX, float OffsetY, float Width, float Height, bool IsBack, float R, float G, float B)
	{
		UIElementDesc desc;
		desc.Type = "Sprite";
		desc.Name = IsBack ? std::string(pName) + "Back" : pName;
		desc.ImageId = kDefaultImage;
		desc.AnchorX = AnchorX;
		desc.AnchorY = AnchorY;
		desc.OffsetX = OffsetX;
		desc.OffsetY = OffsetY;
		desc.Width = IsBack ? Width + 4.0f : Width;
		desc.Height = IsBack ? Height + 4.0f : Height;
		desc.Layer = IsBack ? 0 : 1;
		desc.ColorR = IsBack ? 0.05f : R;
		desc.ColorG = IsBack ? 0.05f : G;
		desc.ColorB = IsBack ? 0.05f : B;
		desc.ColorA = 0.85f;

		return desc;
	}

} // namespace

// レイアウトJSONを読み込む(ファイル無し/壊れ/Sprite要素0件はfalse).
bool UILayoutRuntime::LoadFromFile(const std::filesystem::path& Path)
{
	const nlohmann::json data = FileManager::JsonLoad(Path);
	if (data.empty()) { return false; }

	UILayoutModel model;
	model.FromJson(data);

	std::vector<UIElementDesc> sprites;
	for (const UIElementDesc& element : model.Elements)
	{
		if (element.Type == "Sprite") { sprites.push_back(element); }
	}

	if (sprites.empty()) { return false; }

	m_Elements = std::move(sprites);
	m_Ratios.clear();

	return true;
}

// コード内蔵の最小HUDレイアウトへ切り替える.
void UILayoutRuntime::LoadDefault()
{
	m_Elements.clear();
	m_Ratios.clear();

	// Boss: 上中央 / Player: 左下(ゲーム画面の慣習的な配置).
	m_Elements.push_back(MakeHpBar("BossHP",   0.5f, 0.04f, -300.0f, 16.0f, 600.0f, 22.0f, true,  1.0f, 0.25f, 0.25f));
	m_Elements.push_back(MakeHpBar("BossHP",   0.5f, 0.04f, -300.0f, 16.0f, 600.0f, 22.0f, false, 1.0f, 0.25f, 0.25f));
	m_Elements.push_back(MakeHpBar("PlayerHP", 0.02f, 0.94f,    0.0f,  0.0f, 400.0f, 18.0f, true,  0.2f, 1.0f, 0.35f));
	m_Elements.push_back(MakeHpBar("PlayerHP", 0.02f, 0.94f,    0.0f,  0.0f, 400.0f, 18.0f, false, 0.2f, 1.0f, 0.35f));
}

// 読み込みを試み、失敗したら既定レイアウトへフォールバックする.
void UILayoutRuntime::LoadOrDefault(const std::filesystem::path& Path)
{
	if (!LoadFromFile(Path))
	{
		LoadDefault();
	}
}

// 名前指定でバーの塗り比を設定する.
void UILayoutRuntime::SetRatio(const std::string& Name, float Ratio)
{
	m_Ratios[Name] = std::clamp(Ratio, 0.0f, 1.0f);
}

// 描画内容を確定する(レイヤー順・可視のみ・塗り比適用).
std::vector<UISpriteDrawItem> UILayoutRuntime::BuildDrawItems(
	const std::vector<UIElementDesc>& Elements,
	const std::map<std::string, float>& Ratios,
	float ScreenWidth, float ScreenHeight)
{
	// レイヤー昇順(同値は登録順)で描く.
	std::vector<const UIElementDesc*> sorted;
	sorted.reserve(Elements.size());
	for (const UIElementDesc& element : Elements) { sorted.push_back(&element); }

	std::stable_sort(sorted.begin(), sorted.end(),
		[](const UIElementDesc* p_a, const UIElementDesc* p_b) { return p_a->Layer < p_b->Layer; });

	std::vector<UISpriteDrawItem> items;
	items.reserve(sorted.size());

	for (const UIElementDesc* p_element : sorted)
	{
		if (!p_element->IsVisible || p_element->ImageId.empty()) { continue; }

		float ratio = 1.0f;
		if (const auto it = Ratios.find(p_element->Name); it != Ratios.end()) { ratio = it->second; }

		UISpriteDrawItem item;
		item.ImageId = p_element->ImageId;
		UILayoutModel::GetPosition(*p_element, ScreenWidth, ScreenHeight, item.PosX, item.PosY);
		item.Width  = p_element->Width * ratio;
		item.Height = p_element->Height;
		item.Color  = { p_element->ColorR, p_element->ColorG, p_element->ColorB, p_element->ColorA };

		items.push_back(std::move(item));
	}

	return items;
}

// 保持しているSprite要素をSprite2Dで描画する.
void UILayoutRuntime::Draw(SpriteRenderer& Renderer, DirectX12& Dx12)
{
	const float screen_w = static_cast<float>(Dx12.GetBackBufferWidth());
	const float screen_h = static_cast<float>(Dx12.GetBackBufferHeight());

	for (const UISpriteDrawItem& item : BuildDrawItems(m_Elements, m_Ratios, screen_w, screen_h))
	{
		ID3D12Resource* p_texture = Dx12.GetTextureByPath(item.ImageId.c_str()).Get();
		if (!p_texture) { continue; } // 欠損画像は描画を飛ばして継続(クラッシュさせない).

		Renderer.DrawSprite2D(p_texture, item.PosX, item.PosY, item.Width, item.Height, item.Color);
	}
}
