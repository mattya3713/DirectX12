#include "UILayoutModel.h"

#include <algorithm>

namespace {

	// 既存IDと衝突しない新しいIDを採番する("ui1","ui2",...).
	std::string GenerateId(const UILayoutModel& Model)
	{
		int index = static_cast<int>(Model.Elements.size()) + 1;
		std::string id = "ui" + std::to_string(index);
		while (Model.Find(id) != nullptr)
		{
			++index;
			id = "ui" + std::to_string(index);
		}
		return id;
	}

	const char* kTypeSprite = "Sprite";
	const char* kTypeText = "Text";

} // namespace

const UIElementDesc* UILayoutModel::Find(const std::string& Id) const
{
	for (const UIElementDesc& element : Elements)
	{
		if (element.Id == Id) { return &element; }
	}

	return nullptr;
}

UIElementDesc* UILayoutModel::Find(const std::string& Id)
{
	for (UIElementDesc& element : Elements)
	{
		if (element.Id == Id) { return &element; }
	}

	return nullptr;
}

std::string UILayoutModel::AddElement(const std::string& Type, const std::string& Name)
{
	UIElementDesc desc;
	desc.Id = GenerateId(*this);
	desc.Type = Type;
	desc.Name = Name.empty() ? desc.Id : Name;

	if (Type == kTypeSprite)
	{
		desc.ImageId = "placeholder";
	}

	if (Type == kTypeText)
	{
		desc.Text = "Text";
	}

	Elements.push_back(std::move(desc));

	return Elements.back().Id;
}

bool UILayoutModel::RemoveElement(const std::string& Id)
{
	for (std::size_t i = 0; i < Elements.size(); ++i)
	{
		if (Elements[i].Id == Id)
		{
			Elements.erase(Elements.begin() + static_cast<std::ptrdiff_t>(i));

			return true;
		}
	}

	return false;
}

std::string UILayoutModel::DuplicateElement(const std::string& Id)
{
	UIElementDesc* p_source = Find(Id);
	if (!p_source) { return ""; }

	UIElementDesc copy = *p_source;
	copy.Id = GenerateId(*this);
	copy.Name += "(copy)";
	copy.OffsetX += 16.0f; // 複製元と重ならないよう少しずらす.
	copy.OffsetY += 16.0f;

	Elements.push_back(std::move(copy));

	return Elements.back().Id;
}

void UILayoutModel::GetPosition(const UIElementDesc& Element, float ScreenWidth, float ScreenHeight, float& OutX, float& OutY)
{
	OutX = Element.AnchorX * ScreenWidth + Element.OffsetX;
	OutY = Element.AnchorY * ScreenHeight + Element.OffsetY;
}

std::vector<std::string> UILayoutModel::GetSortedIdsByLayer() const
{
	std::vector<const UIElementDesc*> sorted;
	sorted.reserve(Elements.size());
	for (const UIElementDesc& element : Elements) { sorted.push_back(&element); }

	// 安定ソートで同レイヤーは登録順を維持する.
	std::stable_sort(sorted.begin(), sorted.end(),
		[](const UIElementDesc* p_a, const UIElementDesc* p_b) { return p_a->Layer < p_b->Layer; });

	std::vector<std::string> ids;
	ids.reserve(sorted.size());
	for (const UIElementDesc* p_element : sorted) { ids.push_back(p_element->Id); }

	return ids;
}

nlohmann::json UILayoutModel::ToJson() const
{
	nlohmann::json array = nlohmann::json::array();

	for (const UIElementDesc& e : Elements)
	{
		array.push_back({
			{ "id",         e.Id },
			{ "type",       e.Type },
			{ "name",       e.Name },
			{ "image_id",   e.ImageId },
			{ "text",       e.Text },
			{ "anchor_x",   e.AnchorX },
			{ "anchor_y",   e.AnchorY },
			{ "offset_x",   e.OffsetX },
			{ "offset_y",   e.OffsetY },
			{ "width",      e.Width },
			{ "height",     e.Height },
			{ "rotation",   e.RotationDeg },
			{ "layer",      e.Layer },
			{ "color_r",    e.ColorR },
			{ "color_g",    e.ColorG },
			{ "color_b",    e.ColorB },
			{ "color_a",    e.ColorA },
			{ "visible",    e.IsVisible },
		});
	}

	return { { "elements", array } };
}

void UILayoutModel::FromJson(const nlohmann::json& Data)
{
	Elements.clear();
	if (!Data.contains("elements") || !Data["elements"].is_array()) { return; }

	for (const nlohmann::json& entry : Data["elements"])
	{
		UIElementDesc e;
		e.Id         = entry.value("id", "");
		e.Type       = entry.value("type", kTypeSprite);
		e.Name       = entry.value("name", "");
		e.ImageId    = entry.value("image_id", "");
		e.Text       = entry.value("text", "");
		e.AnchorX    = entry.value("anchor_x", 0.5f);
		e.AnchorY    = entry.value("anchor_y", 0.5f);
		e.OffsetX    = entry.value("offset_x", 0.0f);
		e.OffsetY    = entry.value("offset_y", 0.0f);
		e.Width      = entry.value("width", 100.0f);
		e.Height     = entry.value("height", 32.0f);
		e.RotationDeg = entry.value("rotation", 0.0f);
		e.Layer      = entry.value("layer", 0);
		e.ColorR     = entry.value("color_r", 1.0f);
		e.ColorG     = entry.value("color_g", 1.0f);
		e.ColorB     = entry.value("color_b", 1.0f);
		e.ColorA     = entry.value("color_a", 1.0f);
		e.IsVisible  = entry.value("visible", true);

		if (e.Id.empty()) { continue; } // IDの無い要素は壊れデータとして捨てる.

		Elements.push_back(std::move(e));
	}
}

bool UILayoutModel::Equal(const UILayoutModel& A, const UILayoutModel& B)
{
	if (A.Elements.size() != B.Elements.size()) { return false; }

	for (std::size_t i = 0; i < A.Elements.size(); ++i)
	{
		if (A.Elements[i] != B.Elements[i]) { return false; }
	}

	return true;
}
