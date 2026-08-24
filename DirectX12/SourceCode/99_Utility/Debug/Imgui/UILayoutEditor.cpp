#include "UILayoutEditor.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <functional>
#include <string>

#include "ImGuiManager.h"
#include "99_Utility/FileManager/FileManager.h"

namespace {

	// 編集前後のモデルスナップショットを記録するコマンド(モデルは小さいので全状態複製).
	class UIModelCommand final : public IEditorCommand
	{
	public:
		UIModelCommand(UILayoutEditor* pOwner, UILayoutModel OldModel, UILayoutModel NewModel, const char* pLabel)
			: m_pOwner(pOwner), m_OldModel(std::move(OldModel)), m_NewModel(std::move(NewModel)), m_Label(pLabel)
		{
		}

		void Execute() override { m_pOwner->ApplyModel(m_NewModel); }
		void Undo() override { m_pOwner->ApplyModel(m_OldModel); }
		const char* GetLabel() const override { return m_Label; }

	private:
		UILayoutEditor* const m_pOwner;
		const UILayoutModel   m_OldModel;
		const UILayoutModel   m_NewModel;
		const char* const     m_Label;
	};

	// プレビュー用の解像度プリセット(幅, 高さ).
	constexpr float kResolutions[][2] =
	{
		{ 1280.0f, 720.0f },
		{ 1920.0f, 1080.0f },
		{ 2560.0f, 1440.0f },
	};
	constexpr int kResolutionCount = static_cast<int>(std::size(kResolutions));

	constexpr const char* kElementTypes[] = { "Sprite", "Text" };

} // namespace

UILayoutEditor::UILayoutEditor()
{
}

// 編集データを一括差し替える.
void UILayoutEditor::ApplyModel(const UILayoutModel& Model)
{
	m_Model = Model;

	// 選択中要素が消えていたら選択解除.
	if (!m_SelectedId.empty() && !m_Model.Find(m_SelectedId))
	{
		m_SelectedId.clear();
	}
}

// 編集操作をスナップショット経由でUndoスタックへ積みながら実行する.
void UILayoutEditor::Mutate(const char* pLabel, const std::function<void(UILayoutModel&)>& Operation)
{
	UILayoutModel before = m_Model;
	Operation(m_Model);

	// 変化がなければ履歴に積まない.
	if (!UILayoutModel::Equal(before, m_Model))
	{
		auto p_command = std::make_unique<UIModelCommand>(this, std::move(before), m_Model, pLabel);
		m_Commands.Execute(std::move(p_command));
	}
}

// ドラッグ開始時のスナップショットを記録する.
void UILayoutEditor::BeginDrag()
{
	m_IsDragging = true;
	m_DragOldModel = m_Model;
}

// ドラッグ終了時に変化があれば履歴へ積む.
void UILayoutEditor::EndDrag(const char* pLabel)
{
	if (!m_IsDragging) { return; }

	m_IsDragging = false;

	if (UILayoutModel::Equal(m_DragOldModel, m_Model)) { return; }

	auto p_command = std::make_unique<UIModelCommand>(this, m_DragOldModel, m_Model, pLabel);
	m_Commands.Execute(std::move(p_command));
}

// 選択解像度を取得する.
void UILayoutEditor::GetResolution(float& OutWidth, float& OutHeight) const noexcept
{
	const int index = std::clamp(m_ResolutionIndex, 0, kResolutionCount - 1);
	OutWidth = kResolutions[index][0];
	OutHeight = kResolutions[index][1];
}

// 毎フレーム呼ぶ.
void UILayoutEditor::Draw()
{
	ImGui::SetNextWindowPos(ImVec2(60.0f, 60.0f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(780.0f, 520.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin(IMGUI_JP("UI Layout Editor"))) { ImGui::End(); return; }

	// Ctrl+Z / Ctrl+Y(Ctrl+Shift+Z).
	m_Commands.HandleShortcuts();

	// ---- ファイル操作 ----
	if (ImGui::Button(IMGUI_JP("保存")))
	{
		m_SaveStatus = FileManager::JsonSave(m_FilePath, m_Model.ToJson())
			? std::string(IMGUI_JP("保存しました: ")) + m_FilePath
			: std::string(IMGUI_JP("保存に失敗しました"));
	}
	ImGui::SameLine();
	if (ImGui::Button(IMGUI_JP("読込")))
	{
		const nlohmann::json data = FileManager::JsonLoad(m_FilePath);
		if (!data.empty())
		{
			Mutate(IMGUI_JP("JSON読込"), [&data](UILayoutModel& model) { model.FromJson(data); });
			m_SaveStatus = std::string(IMGUI_JP("読み込みました: ")) + m_FilePath;
		}
		else
		{
			m_SaveStatus = std::string(IMGUI_JP("ファイルが無いか空です: ")) + m_FilePath;
		}
	}
	ImGui::SameLine();
	{
		char undo_text[48] = {};
		snprintf(undo_text, sizeof(undo_text), "Undo:%d Redo:%d",
			static_cast<int>(m_Commands.GetUndoDepth()), static_cast<int>(m_Commands.GetRedoDepth()));
		ImGuiManager::Text(undo_text);
	}
	if (!m_SaveStatus.empty())
	{
		ImGuiManager::Text(m_SaveStatus.c_str());
	}

	// ---- 要素追加 ----
	ImGui::SetNextItemWidth(100.0f);
	ImGui::Combo("##newtype", &m_NewElementType, kElementTypes, static_cast<int>(std::size(kElementTypes)));
	ImGui::SameLine();
	if (ImGui::Button(IMGUI_JP("要素追加")))
	{
		const std::string type = kElementTypes[m_NewElementType];
		std::string selected;
		Mutate(IMGUI_JP("要素追加"), [&type, &selected](UILayoutModel& model) {
			selected = model.AddElement(type, "");
		});
		if (!selected.empty()) { m_SelectedId = selected; }
	}

	ImGui::Separator();

	// ---- 左: 要素一覧 / 右: プレビュー ----
	DrawElementList();
	ImGui::SameLine();
	DrawPreview();
	ImGui::Separator();
	DrawSelectedPanel();

	ImGui::End();
}

// 要素一覧(レイヤー順)の描画.
void UILayoutEditor::DrawElementList()
{
	ImGui::BeginGroup();
	ImGui::TextUnformatted(IMGUI_JP("要素(レイヤー順)"));

	ImGui::BeginChild("element_list", ImVec2(280.0f, -ImGui::GetFrameHeightWithSpacing()), true);
	for (const std::string& id : m_Model.GetSortedIdsByLayer())
	{
		const UIElementDesc* p_element = m_Model.Find(id);
		if (!p_element) { continue; }

		char label[96] = {};
		snprintf(label, sizeof(label), "%s[%s] %s%s",
			p_element->Type.c_str(), p_element->Id.c_str(), p_element->Name.c_str(),
			p_element->IsVisible ? "" : IMGUI_JP("(非表示)"));

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth;
		if (id == m_SelectedId) { flags |= ImGuiTreeNodeFlags_Selected; }

		const bool opened = ImGui::TreeNodeEx(id.c_str(), flags, "%s", label);
		if (opened) { ImGui::TreePop(); }

		if (ImGui::IsItemClicked()) { m_SelectedId = id; }
	}
	ImGui::EndChild();
	ImGui::EndGroup();
}

// アンカー配置のプレビュー描画.
void UILayoutEditor::DrawPreview()
{
	ImGui::BeginGroup();
	ImGui::TextUnformatted(IMGUI_JP("プレビュー"));
	const ImVec2 origin = ImGui::GetCursorScreenPos();

	ImGui::BeginChild("preview_area", ImVec2(kPreviewWidth, kPreviewHeight), true);
	ImDrawList* p_draw = ImGui::GetWindowDrawList();
	p_draw->AddRectFilled(origin, ImVec2(origin.x + kPreviewWidth, origin.y + kPreviewHeight), IM_COL32(24, 24, 32, 255));

	float res_w = 0.0f;
	float res_h = 0.0f;
	GetResolution(res_w, res_h);
	const float scale_x = kPreviewWidth / res_w;
	const float scale_y = kPreviewHeight / res_h;

	// 見た目上の手前→奥の順で当たり判定を見るため、描画はレイヤー昇順のまま行う.
	for (const std::string& id : m_Model.GetSortedIdsByLayer())
	{
		const UIElementDesc* p_element = m_Model.Find(id);
		if (!p_element || !p_element->IsVisible) { continue; }

		float pos_x = 0.0f;
		float pos_y = 0.0f;
		UILayoutModel::GetPosition(*p_element, res_w, res_h, pos_x, pos_y);

		const ImVec2 min(origin.x + pos_x * scale_x, origin.y + pos_y * scale_y);
		const ImVec2 max(min.x + p_element->Width * scale_x, min.y + p_element->Height * scale_y);
		const ImU32 fill = ImGui::ColorConvertFloat4ToU32(ImVec4(
			p_element->ColorR * 0.6f, p_element->ColorG * 0.6f, p_element->ColorB * 0.6f, p_element->ColorA * 0.5f));
		p_draw->AddRectFilled(min, max, fill);
		p_draw->AddRect(min, max, IM_COL32(120, 200, 255, 255));
		p_draw->AddText(min, IM_COL32(230, 230, 230, 255),
			p_element->Type == "Text" && !p_element->Text.empty() ? p_element->Text.c_str() : p_element->Name.c_str());
	}

	// ---- ドラッグ移動(選択中要素を掴んで動かす) ----
	const ImGuiIO& io = ImGui::GetIO();
	const bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
	const ImVec2 mouse(io.MousePos.x - origin.x, io.MousePos.y - origin.y);
	const float mouse_res_x = mouse.x / scale_x;
	const float mouse_res_y = mouse.y / scale_y;

	if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !m_IsDragging)
	{
		// 手前(レイヤー大)から探して最初にヒットした要素を選択し掴む.
		const std::vector<std::string> sorted = m_Model.GetSortedIdsByLayer();
		for (auto it = sorted.rbegin(); it != sorted.rend(); ++it)
		{
			UIElementDesc* p_element = m_Model.Find(*it);
			if (!p_element || !p_element->IsVisible) { continue; }

			float pos_x = 0.0f;
			float pos_y = 0.0f;
			UILayoutModel::GetPosition(*p_element, res_w, res_h, pos_x, pos_y);

			if (mouse_res_x < pos_x || mouse_res_y < pos_y ||
			    mouse_res_x > pos_x + p_element->Width || mouse_res_y > pos_y + p_element->Height) { continue; }

			m_SelectedId = *it;
			m_GrabOffsetX = mouse_res_x - pos_x;
			m_GrabOffsetY = mouse_res_y - pos_y;
			BeginDrag();

			break;
		}
	}

	if (m_IsDragging)
	{
		UIElementDesc* p_selected = m_Model.Find(m_SelectedId);
		if (!p_selected)
		{
			EndDrag(IMGUI_JP("移動"));
		}
		else
		{
			// アンカー位置は固定し、オフセットだけ動かす(解像度を変えても配置意図が保存される).
			p_selected->OffsetX = mouse_res_x - p_selected->AnchorX * res_w - m_GrabOffsetX;
			p_selected->OffsetY = mouse_res_y - p_selected->AnchorY * res_h - m_GrabOffsetY;

			if (!io.MouseDown[ImGuiMouseButton_Left]) { EndDrag(IMGUI_JP("移動")); }
		}
	}

	ImGui::EndChild();
	ImGui::EndGroup();
}

// 選択中要素に対する操作UI.
void UILayoutEditor::DrawSelectedPanel()
{
	const UIElementDesc* p_selected = m_Model.Find(m_SelectedId);

	ImGui::Separator();

	if (!p_selected)
	{
		ImGui::TextUnformatted(IMGUI_JP("要素を選択すると編集できます"));
		return;
	}

	const std::string selected_id = m_SelectedId;

	char name_buf[64] = {};
	strncpy_s(name_buf, p_selected->Name.c_str(), sizeof(name_buf) - 1);
	if (ImGui::InputText(IMGUI_JP("名前"), name_buf, sizeof(name_buf)))
	{
		Mutate(IMGUI_JP("名前変更"), [selected_id, text = std::string(name_buf)](UILayoutModel& model) {
			if (UIElementDesc* p_element = model.Find(selected_id)) { p_element->Name = text; }
		});
	}

	// Sprite/Textそれぞれの固有プロパティ.
	if (p_selected->Type == "Sprite")
	{
		char image_buf[128] = {};
		strncpy_s(image_buf, p_selected->ImageId.c_str(), sizeof(image_buf) - 1);
		if (ImGui::InputText(IMGUI_JP("画像ID"), image_buf, sizeof(image_buf)))
		{
			Mutate(IMGUI_JP("画像ID変更"), [selected_id, text = std::string(image_buf)](UILayoutModel& model) {
				if (UIElementDesc* p_element = model.Find(selected_id)) { p_element->ImageId = text; }
			});
		}
	}
	if (p_selected->Type == "Text")
	{
		char text_buf[128] = {};
		strncpy_s(text_buf, p_selected->Text.c_str(), sizeof(text_buf) - 1);
		if (ImGui::InputText(IMGUI_JP("文字列"), text_buf, sizeof(text_buf)))
		{
			Mutate(IMGUI_JP("文字列変更"), [selected_id, text = std::string(text_buf)](UILayoutModel& model) {
				if (UIElementDesc* p_element = model.Find(selected_id)) { p_element->Text = text; }
			});
		}
	}

	// アンカー(画面比0〜1).
	float anchor[2] = { p_selected->AnchorX, p_selected->AnchorY };
	if (ImGui::SliderFloat2(IMGUI_JP("アンカー"), anchor, 0.0f, 1.0f, "%.3f"))
	{
		Mutate(IMGUI_JP("アンカー変更"), [selected_id, ax = anchor[0], ay = anchor[1]](UILayoutModel& model) {
			if (UIElementDesc* p_element = model.Find(selected_id))
			{
				p_element->AnchorX = ax;
				p_element->AnchorY = ay;
			}
		});
	}

	// オフセット(px).
	float offset[2] = { p_selected->OffsetX, p_selected->OffsetY };
	if (ImGui::DragFloat2(IMGUI_JP("オフセット"), offset, 1.0f))
	{
		Mutate(IMGUI_JP("オフセット変更"), [selected_id, ox = offset[0], oy = offset[1]](UILayoutModel& model) {
			if (UIElementDesc* p_element = model.Find(selected_id))
			{
				p_element->OffsetX = ox;
				p_element->OffsetY = oy;
			}
		});
	}

	// サイズ(px).
	float size[2] = { p_selected->Width, p_selected->Height };
	if (ImGui::DragFloat2(IMGUI_JP("サイズ"), size, 1.0f, 1.0f, 4096.0f, "%.1f"))
	{
		Mutate(IMGUI_JP("サイズ変更"), [selected_id, w = size[0], h = size[1]](UILayoutModel& model) {
			if (UIElementDesc* p_element = model.Find(selected_id))
			{
				p_element->Width = w;
				p_element->Height = h;
			}
		});
	}

	float rotation = p_selected->RotationDeg;
	if (ImGui::DragFloat(IMGUI_JP("回転(度)"), &rotation, 0.5f, -360.0f, 360.0f, "%.1f"))
	{
		Mutate(IMGUI_JP("回転変更"), [selected_id, rotation](UILayoutModel& model) {
			if (UIElementDesc* p_element = model.Find(selected_id)) { p_element->RotationDeg = rotation; }
		});
	}

	int layer = p_selected->Layer;
	if (ImGui::InputInt(IMGUI_JP("レイヤー"), &layer))
	{
		Mutate(IMGUI_JP("レイヤー変更"), [selected_id, layer](UILayoutModel& model) {
			if (UIElementDesc* p_element = model.Find(selected_id)) { p_element->Layer = layer; }
		});
	}

	float color[4] = { p_selected->ColorR, p_selected->ColorG, p_selected->ColorB, p_selected->ColorA };
	if (ImGui::ColorEdit4(IMGUI_JP("色"), color))
	{
		Mutate(IMGUI_JP("色変更"), [selected_id, r = color[0], g = color[1], b = color[2], a = color[3]](UILayoutModel& model) {
			if (UIElementDesc* p_element = model.Find(selected_id))
			{
				p_element->ColorR = r;
				p_element->ColorG = g;
				p_element->ColorB = b;
				p_element->ColorA = a;
			}
		});
	}

	bool visible = p_selected->IsVisible;
	if (ImGui::Checkbox(IMGUI_JP("表示"), &visible))
	{
		Mutate(IMGUI_JP("表示切替"), [selected_id, visible](UILayoutModel& model) {
			if (UIElementDesc* p_element = model.Find(selected_id)) { p_element->IsVisible = visible; }
		});
	}

	ImGui::Spacing();
	if (ImGui::Button(IMGUI_JP("複製")))
	{
		Mutate(IMGUI_JP("要素複製"), [selected_id](UILayoutModel& model) {
			model.DuplicateElement(selected_id);
		});
	}
	ImGui::SameLine();
	if (ImGui::Button(IMGUI_JP("削除")))
	{
		Mutate(IMGUI_JP("要素削除"), [selected_id](UILayoutModel& model) {
			model.RemoveElement(selected_id);
		});
	}
}
