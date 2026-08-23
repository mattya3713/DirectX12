#include "BTTreeEditor.h"

#include <algorithm>
#include <cstring>
#include <string>

#include "ImGuiManager.h"
#include "99_Utility/BehaviorTree/ActionNode.h"
#include "99_Utility/BehaviorTree/CompositeNodeBase.h"
#include "99_Utility/BehaviorTree/DecoratorNode.h"
#include "99_Utility/BehaviorTree/NodeStatus.h"
#include "99_Utility/BehaviorTree/SelectorNode.h"
#include "99_Utility/BehaviorTree/SequenceNode.h"
#include "99_Utility/FileManager/FileManager.h"

namespace {

	// 編集前後のモデルスナップショットを記録するコマンド(モデルは小さいので全状態複製).
	class BTModelCommand final : public IEditorCommand
	{
	public:
		BTModelCommand(BTTreeEditor* pOwner, BTTreeModel OldModel, BTTreeModel NewModel, const char* pLabel)
			: m_pOwner(pOwner), m_OldModel(std::move(OldModel)), m_NewModel(std::move(NewModel)), m_Label(pLabel)
		{
		}

		void Execute() override { m_pOwner->ApplyModel(m_NewModel); }
		void Undo() override { m_pOwner->ApplyModel(m_OldModel); }
		const char* GetLabel() const override { return m_Label; }

	private:
		BTTreeEditor* const m_pOwner;
		const BTTreeModel   m_OldModel;
		const BTTreeModel   m_NewModel;
		const char* const   m_Label;
	};

	// 子追加UI用の種別一覧.
	constexpr const char* kNodeTypes[] = { "Selector", "Sequence", "Decorator", "Action" };

} // namespace

BTTreeEditor::BTTreeEditor()
{
	// 空の状態から始める(読込ボタンまたは子追加で構築する).
}

// 編集データを一括差し替える.
void BTTreeEditor::ApplyModel(const BTTreeModel& Model)
{
	m_Model = Model;

	// 選択中ノードが消えていたら選択解除.
	if (!m_SelectedId.empty() && !m_Model.Find(m_SelectedId))
	{
		m_SelectedId.clear();
	}
}

// 編集操作をスナップショット経由でUndoスタックへ積みながら実行する.
void BTTreeEditor::Mutate(const char* pLabel, const std::function<void(BTTreeModel&)>& Operation)
{
	BTTreeModel before = m_Model;
	Operation(m_Model);

	// 変化がなければ履歴に積まない.
	const bool changed = !(before.Nodes == m_Model.Nodes && before.RootId == m_Model.RootId);
	if (changed)
	{
		auto p_command = std::make_unique<BTModelCommand>(this, std::move(before), m_Model, pLabel);
		m_Commands.Execute(std::move(p_command));
	}
}

// ツリー階層の再帰描画.
void BTTreeEditor::DrawNodeTree(const std::string& Id, int Depth)
{
	const BTNodeDesc* p_node = m_Model.Find(Id);
	if (!p_node) { return; }

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (p_node->Children.empty()) { flags |= ImGuiTreeNodeFlags_Leaf; }
	if (Id == m_SelectedId) { flags |= ImGuiTreeNodeFlags_Selected; }

	ImGui::SetNextItemOpen(Depth == 0, ImGuiCond_FirstUseEver);
	const bool opened = ImGui::TreeNodeEx(Id.c_str(), flags, "%s[%s] %s",
		p_node->Type.c_str(), p_node->Id.c_str(), p_node->Name.c_str());

	if (ImGui::IsItemClicked())
	{
		m_SelectedId = Id;
	}

	if (opened)
	{
		for (const std::string& child : p_node->Children)
		{
			DrawNodeTree(child, Depth + 1);
		}
		ImGui::TreePop();
	}
}

// 選択中ノードに対する操作UI.
void BTTreeEditor::DrawSelectedPanel()
{
	const BTNodeDesc* p_selected = m_Model.Find(m_SelectedId);

	ImGui::Separator();

	if (!p_selected)
	{
		ImGui::TextUnformatted(IMGUI_JP("ノードを選択すると編集できます"));
		return;
	}

	char name_buf[64] = {};
	strncpy_s(name_buf, p_selected->Name.c_str(), sizeof(name_buf) - 1);
	if (ImGui::InputText(IMGUI_JP("名前"), name_buf, sizeof(name_buf)))
	{
		const std::string new_name = name_buf;
		Mutate("名前変更", [&SelectedId = m_SelectedId, new_name](BTTreeModel& model) {
			if (BTNodeDesc* p_node = model.Find(SelectedId)) { p_node->Name = new_name; }
		});
	}

	// Decorator専用プロパティ.
	if (p_selected->Type == "Decorator")
	{
		const char* mode_labels[] = { IMGUI_JP("反転"), IMGUI_JP("必ず成功"), IMGUI_JP("リトライ") };
		int mode = p_selected->DecoratorMode;
		if (ImGui::Combo(IMGUI_JP("変換ルール"), &mode, mode_labels, 3) && mode != p_selected->DecoratorMode)
		{
			Mutate("変換ルール変更", [&SelectedId = m_SelectedId, mode](BTTreeModel& model) {
				if (BTNodeDesc* p_node = model.Find(SelectedId)) { p_node->DecoratorMode = mode; }
			});
		}

		int retry = static_cast<int>(p_selected->MaxRetry);
		if (ImGui::InputInt(IMGUI_JP("最大リトライ"), &retry))
		{
			retry = std::clamp(retry, 1, 99);
			if (retry != static_cast<int>(p_selected->MaxRetry))
			{
				Mutate("リトライ回数変更", [&SelectedId = m_SelectedId, retry](BTTreeModel& model) {
					if (BTNodeDesc* p_node = model.Find(SelectedId)) { p_node->MaxRetry = static_cast<std::uint32_t>(retry); }
				});
			}
		}
	}

	ImGui::Spacing();

	// 子追加(Composite/Decorator/Rootのみ. Actionは子を持てない).
	const int max_children = BTMaxChildren(p_selected->Type);
	if (static_cast<int>(p_selected->Children.size()) < max_children)
	{
		ImGui::SetNextItemWidth(120.0f);
		ImGui::Combo("##newtype", &m_NewNodeType, kNodeTypes, static_cast<int>(std::size(kNodeTypes)));
		ImGui::SameLine();
		if (ImGui::Button(IMGUI_JP("子ノード追加")))
		{
			const std::string type = kNodeTypes[m_NewNodeType];
			const std::string parent = m_SelectedId;
			Mutate("子ノード追加", [parent, type](BTTreeModel& model) {
				const std::string child_id = model.AddNode(type, "");
				model.Attach(parent, child_id);
			});

			// 追加した末尾ノードを選択しておく(連続作成しやすい).
			if (!m_Model.Nodes.empty()) { m_SelectedId = m_Model.Nodes.back().Id; }
		}
	}

	if (ImGui::Button(IMGUI_JP("複製")))
	{
		const std::string target = m_SelectedId;
		Mutate("部分木複製", [target](BTTreeModel& model) {
			model.DuplicateSubtree(target);
		});
	}

	ImGui::SameLine();
	if (m_SelectedId != m_Model.RootId && ImGui::Button(IMGUI_JP("削除")))
	{
		const std::string target = m_SelectedId;
		Mutate("部分木削除", [target](BTTreeModel& model) {
			model.DeleteSubtree(target);
		});
	}
}

// 毎フレーム呼ぶ.
void BTTreeEditor::Draw()
{
	ImGui::SetNextWindowPos(ImVec2(860.0f, 430.0f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(420.0f, 380.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin(IMGUI_JP("Behavior Tree Editor"))) { ImGui::End(); return; }

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
			Mutate("JSON読込", [&data](BTTreeModel& model) { model.FromJson(data); });
			m_SaveStatus = std::string(IMGUI_JP("読み込みました: ")) + m_FilePath;
		}
		else
		{
			m_SaveStatus = std::string(IMGUI_JP("ファイルが無いか空です: ")) + m_FilePath;
		}
	}
	ImGui::SameLine();

	// 履歴状況の表示(Ctrl+Z / Ctrl+Yの案内を兼ねる).
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

	ImGui::Separator();

	// ---- ツリー階層 ----
	if (m_Model.RootId.empty())
	{
		if (ImGui::Button(IMGUI_JP("根ノードを作成(Selector)")))
		{
			Mutate("根ノード作成", [](BTTreeModel& model) {
				model.RootId = model.AddNode("Selector", "root");
			});
		}
	}
	else
	{
		DrawNodeTree(m_Model.RootId, 0);
	}

	DrawSelectedPanel();

#if _DEBUG
	if (!m_LastActiveAction.empty())
	{
		ImGui::Separator();
		ImGui::Text(IMGUI_JP("直近実行Action: %s"), m_LastActiveAction.c_str());
	}
#endif

	ImGui::End();
}

// 現在のモデルからランタイムツリーを構築する.
std::unique_ptr<RootNode> BTTreeEditor::BuildRuntime(const ActionRegistry& Actions)
{
	// 再帰構築ヘルパ(モデルID→ランタイムノード).
	std::function<NodeBase* (const std::string&)> build = [&](const std::string& id) -> NodeBase*
	{
		const BTNodeDesc* p_desc = m_Model.Find(id);
		if (!p_desc) { return nullptr; }

		if (p_desc->Type == "Selector")
		{
			SelectorNode* p_selector = new SelectorNode();
			for (const std::string& child : p_desc->Children)
			{
				if (NodeBase* p_child = build(child)) { p_selector->AdoptChild(std::unique_ptr<NodeBase>(p_child)); }
			}
			return p_selector;
		}

		if (p_desc->Type == "Sequence")
		{
			SequenceNode* p_sequence = new SequenceNode();
			for (const std::string& child : p_desc->Children)
			{
				if (NodeBase* p_child = build(child)) { p_sequence->AdoptChild(std::unique_ptr<NodeBase>(p_child)); }
			}
			return p_sequence;
		}

		if (p_desc->Type == "Decorator")
		{
			DecoratorNode* p_decorator = new DecoratorNode(
				static_cast<DecoratorNode::Mode>(p_desc->DecoratorMode), p_desc->MaxRetry);
			for (const std::string& child : p_desc->Children)
			{
				if (NodeBase* p_child = build(child))
				{
					p_decorator->SetChild(std::unique_ptr<NodeBase>(p_child));
					break; // 子は1件まで.
				}
			}
			return p_decorator;
		}

		if (p_desc->Type == "Action")
		{
#if _DEBUG
			// 実行中ノード表示のため、Action実行時に名前を記録してからレジストリ処理へ委譲する.
			const std::string action_name = p_desc->Name;
			std::function<NodeStatus()> wrapped = [this, action_name, &Actions]() -> NodeStatus {
				m_LastActiveAction = action_name;
				const auto it = Actions.find(action_name);
				return (it != Actions.end()) ? it->second() : NodeStatus::Failure;
			};
			return new ActionNode(std::move(wrapped));
#else
			const auto it = Actions.find(p_desc->Name);
			std::function<NodeStatus()> func = (it != Actions.end())
				? it->second
				: []() { return NodeStatus::Failure; };
			return new ActionNode(std::move(func));
#endif
		}

		return nullptr;
	};

	std::unique_ptr<RootNode> p_root = std::make_unique<RootNode>();
	if (NodeBase* p_child = build(m_Model.RootId))
	{
		p_root->SetChild(std::unique_ptr<NodeBase>(p_child));
	}

	return p_root;
}
