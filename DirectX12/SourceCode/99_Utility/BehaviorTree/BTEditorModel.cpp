#include "BTEditorModel.h"

#include <algorithm>
#include <set>

namespace {

	// 既存IDと衝突しない新しいIDを採番する("n1","n2",...).
	std::string GenerateId(const BTTreeModel& Model)
	{
		int index = static_cast<int>(Model.Nodes.size()) + 1;
		std::string id = "n" + std::to_string(index);
		while (Model.Find(id) != nullptr)
		{
			++index;
			id = "n" + std::to_string(index);
		}
		return id;
	}

	// 部分木のID一覧を集める(自分を含む).
	void CollectSubtreeIds(BTTreeModel& Model, const std::string& Id, std::vector<std::string>& Out)
	{
		BTNodeDesc* p_node = Model.Find(Id);
		if (!p_node) { return; }

		Out.push_back(Id);
		for (const std::string& child : p_node->Children)
		{
			CollectSubtreeIds(Model, child, Out);
		}
	}

} // namespace

int BTMaxChildren(const std::string& Type)
{
	if (Type == "Decorator") { return 1; }
	if (Type == "Action") { return 0; }
	return 8; // Selector/Sequence(実用上の上限. 循環は接続側で禁止).
}

std::string BTValidateModel(const BTTreeModel& Model)
{
	if (Model.RootId.empty()) { return "根ノードがありません"; }
	if (!Model.Find(Model.RootId)) { return "根ノードのIDが不正です"; }

	std::set<std::string> visited;
	std::vector<std::string> stack{ Model.RootId };

	while (!stack.empty())
	{
		const std::string id = stack.back();
		stack.pop_back();

		if (visited.contains(id)) { return "循環参照があります: " + id; }
		visited.insert(id);

		const BTNodeDesc* p_node = Model.Find(id);
		if (!p_node) { return "存在しない子IDが参照されています: " + id; }

		const int max_children = BTMaxChildren(p_node->Type);
		if (static_cast<int>(p_node->Children.size()) > max_children) { return "子数が上限超過です: " + id; }

		for (const std::string& child : p_node->Children) { stack.push_back(child); }
	}

	if (visited.size() != Model.Nodes.size())
	{
		return "根から到達できないノードがあります";
	}

	return "";
}

const BTNodeDesc* BTTreeModel::Find(const std::string& Id) const
{
	for (const BTNodeDesc& node : Nodes)
	{
		if (node.Id == Id) { return &node; }
	}
	return nullptr;
}

BTNodeDesc* BTTreeModel::Find(const std::string& Id)
{
	for (BTNodeDesc& node : Nodes)
	{
		if (node.Id == Id) { return &node; }
	}
	return nullptr;
}

std::string BTTreeModel::AddNode(const std::string& Type, const std::string& Name)
{
	BTNodeDesc node{};
	node.Id   = GenerateId(*this);
	node.Type = Type;
	node.Name = Name.empty() ? Type : Name;

	if (Type == "Decorator") { node.DecoratorMode = 0; node.MaxRetry = 1; }

	Nodes.push_back(std::move(node));
	return Nodes.back().Id;
}

bool BTTreeModel::Attach(const std::string& ParentId, const std::string& ChildId)
{
	BTNodeDesc* p_parent = Find(ParentId);
	BTNodeDesc* p_child = Find(ChildId);
	if (!p_parent || !p_child) { return false; }

	// 循環防止(子の部分木に親がいるなら接続不可).
	if (ParentId == ChildId) { return false; }
	std::vector<std::string> subtree;
	CollectSubtreeIds(*this, ChildId, subtree);
	if (std::find(subtree.begin(), subtree.end(), ParentId) != subtree.end()) { return false; }

	const int max_children = BTMaxChildren(p_parent->Type);
	if (static_cast<int>(p_parent->Children.size()) >= max_children) { return false; }

	// 既に別の親に繋がっている場合は一旦外す.
	Detach(ChildId);

	p_parent->Children.push_back(ChildId);
	return true;
}

bool BTTreeModel::Detach(const std::string& ChildId)
{
	for (BTNodeDesc& node : Nodes)
	{
		const auto it = std::find(node.Children.begin(), node.Children.end(), ChildId);
		if (it != node.Children.end())
		{
			node.Children.erase(it);
			return true;
		}
	}
	return false;
}

std::string BTTreeModel::FindParentId(const std::string& ChildId) const
{
	for (const BTNodeDesc& node : Nodes)
	{
		if (std::find(node.Children.begin(), node.Children.end(), ChildId) != node.Children.end())
		{
			return node.Id;
		}
	}
	return "";
}

bool BTTreeModel::DeleteSubtree(const std::string& Id)
{
	if (Id == RootId || Id.empty()) { return false; } // 根と未定義は削除しない.

	BTNodeDesc* p_node = Find(Id);
	if (!p_node) { return false; }

	std::vector<std::string> subtree;
	CollectSubtreeIds(*this, Id, subtree);

	Detach(Id);

	for (const std::string& remove_id : subtree)
	{
		Nodes.erase(std::remove_if(Nodes.begin(), Nodes.end(),
			[&remove_id](const BTNodeDesc& Node) { return Node.Id == remove_id; }),
			Nodes.end());
	}

	return true;
}

std::string BTTreeModel::DuplicateSubtree(const std::string& Id)
{
	BTNodeDesc* p_source = Find(Id);
	if (!p_source) { return ""; }

	std::vector<std::string> subtree;
	CollectSubtreeIds(*this, Id, subtree);

	// 複製元→新IDの対応を作ってから全ノード複製(子参照も張り替える).
	std::map<std::string, std::string> id_map;
	for (const std::string& source_id : subtree)
	{
		id_map[source_id] = AddNode(Find(source_id)->Type, Find(source_id)->Name);
	}

	for (const std::string& source_id : subtree)
	{
		const BTNodeDesc* p_src = Find(source_id);
		BTNodeDesc* p_copy = Find(id_map[source_id]);
		p_copy->DecoratorMode = p_src->DecoratorMode;
		p_copy->MaxRetry      = p_src->MaxRetry;

		for (const std::string& child : p_src->Children)
		{
			p_copy->Children.push_back(id_map[child]);
		}
	}

	// 複製した根を元の根と同じ場所へ接続する.
	const std::string new_root_id = id_map[Id];
	const std::string parent_id = FindParentId(Id);
	if (!parent_id.empty())
	{
		Attach(parent_id, new_root_id);
	}

	return new_root_id;
}

bool BTTreeModel::IsConnected() const noexcept
{
	return BTValidateModel(*this).empty();
}

nlohmann::json BTTreeModel::ToJson() const
{
	nlohmann::json nodes = nlohmann::json::array();
	for (const BTNodeDesc& node : Nodes)
	{
		nlohmann::json children = nlohmann::json::array();
		for (const std::string& child : node.Children) { children.push_back(child); }

		nodes.push_back({
			{"id",       node.Id},
			{"type",     node.Type},
			{"name",     node.Name},
			{"mode",     node.DecoratorMode},
			{"max_retry", node.MaxRetry},
			{"children",  children},
		});
	}

	return {
		{"root",  RootId},
		{"nodes", nodes},
	};
}

void BTTreeModel::FromJson(const nlohmann::json& Data)
{
	Nodes.clear();
	RootId.clear();

	RootId = Data.value("root", "");

	if (!Data.contains("nodes") || !Data["nodes"].is_array()) { return; }

	for (const nlohmann::json& entry : Data["nodes"])
	{
		BTNodeDesc node{};
		node.Id   = entry.value("id", "");
		node.Type = entry.value("type", "");
		node.Name = entry.value("name", "");
		node.DecoratorMode = entry.value("mode", 0);
		node.MaxRetry      = entry.value("max_retry", 1u);

		if (entry.contains("children") && entry["children"].is_array())
		{
			for (const nlohmann::json& child : entry["children"])
			{
				if (child.is_string()) { node.Children.push_back(child.get<std::string>()); }
			}
		}

		if (!node.Id.empty()) { Nodes.push_back(std::move(node)); }
	}
}
