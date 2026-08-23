#pragma once

#include <memory>
#include <type_traits>
#include <utility>

#include "99_Utility/BehaviorTree/NodeBase.h"
#include "99_Utility/BehaviorTree/NodeStatus.h"

/**********************************************************************************
* @brief     : ルートノード(Behavior Tree全体の入口).
*            : 単一の子ツリーを保持し、Tick()/Reset()を委譲する。
* @pattern   : Behavior Tree.
**********************************************************************************/

class RootNode final : public NodeBase
{
public:
	~RootNode() override = default;

	// 子ツリーを設定する(所有権を受け取る).
	void SetChild(std::unique_ptr<NodeBase> upChild) noexcept { m_upChild = std::move(upChild); }

	// 子ツリーを生成して設定し、追加したノードへの参照を返す.
	template<typename T, typename... TArgs>
	T& AddChild(TArgs&&... Args)
	{
		static_assert(std::is_base_of_v<NodeBase, T>);

		m_upChild = std::make_unique<T>(std::forward<TArgs>(Args)...);

		return *static_cast<T*>(m_upChild.get());
	}

	// 子ツリーの評価結果をそのまま返す(未接続ならFailure).
	NodeStatus Tick() override
	{
		if (!m_upChild) { return NodeStatus::Failure; }

		return m_upChild->Tick();
	}

	// 子ツリーの評価状態を初期化する.
	void Reset() override
	{
		if (m_upChild) { m_upChild->Reset(); }
	}

private:
	std::unique_ptr<NodeBase> m_upChild; // ルート直下の子ノード(所有).
};
