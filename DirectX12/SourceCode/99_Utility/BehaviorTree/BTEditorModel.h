#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "json/json.hpp"

/**********************************************************************************
* @date      : 2026-08-23.
* @brief     : Behavior Tree Editorが扱うツリーのデータモデル(JSON可逆).
*            : ランタイム(NodeBase派生)から独立した純データであり、エディタ上の
*            : 配置・接続・削除・複製はすべてこのモデルへの操作として行う.
*            : ノードはフラット配列+ID参照で持ち、循環・遊離は作らせない.
**********************************************************************************/

struct BTNodeDesc
{
	std::string Id;                      // 一意ID("n1"等. エディタが採番).
	std::string Type;                    // "Selector" / "Sequence" / "Decorator" / "Action".
	std::string Name;                    // 表示名(Actionは実行関数の識別子を兼ねる).
	int         DecoratorMode = 0;       // Decorator専用(0=Invert, 1=AlwaysSucceed, 2=Retry).
	std::uint32_t MaxRetry = 1;          // Decorator(Retry)専用.
	std::vector<std::string> Children;   // 子ノードID(Decorator/Rootは最大1件).

	bool operator==(const BTNodeDesc& Other) const noexcept
	{
		return Id == Other.Id && Type == Other.Type && Name == Other.Name &&
		       DecoratorMode == Other.DecoratorMode && MaxRetry == Other.MaxRetry &&
		       Children == Other.Children;
	}
};

// 子を持てる上限(Composite=複数, Decorator/Root=1).
int BTMaxChildren(const std::string& Type);

// モデル全体の妥当性検証(全ノード到達可能・子数上限・種別不明等). 問題の説明を返す(無ければ空).
std::string BTValidateModel(const class BTTreeModel& Model);

class BTTreeModel
{
public:
	std::vector<BTNodeDesc> Nodes;
	std::string             RootId;

	// ノード検索(見つからなければnullptr).
	const BTNodeDesc* Find(const std::string& Id) const;
	BTNodeDesc*       Find(const std::string& Id);

	// 新規ノードを追加する(IDは自動採番). 追加したIDを返す.
	std::string AddNode(const std::string& Type, const std::string& Name);

	// 指定ノードを親の子リストへ接続する(子数上限に達していたらfalse).
	bool Attach(const std::string& ParentId, const std::string& ChildId);

	// 親の子リストから外す(ノード自体は消さない). 親が見つからなければfalse.
	bool Detach(const std::string& ChildId);

	// 指定ノードの親IDを返す(根または未接続は空文字).
	std::string FindParentId(const std::string& ChildId) const;

	// 指定ノードとその部分木を削除する(根は削除不可→false).
	bool DeleteSubtree(const std::string& Id);

	// 指定ノードの部分木を複製して同じ親へ追加する(新IDを採番). 成功時は複製した根のIDを返す.
	std::string DuplicateSubtree(const std::string& Id);

	// 根が存在し、全ノードが根から到達可能か.
	bool IsConnected() const noexcept;

	// ---- JSON変換 ----
	nlohmann::json ToJson() const;
	void           FromJson(const nlohmann::json& Data);
};
