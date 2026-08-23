#pragma once

#include <functional>
#include <utility>

#include "99_Utility/BehaviorTree/NodeBase.h"
#include "99_Utility/BehaviorTree/NodeStatus.h"

/**********************************************************************************
* @brief     : アクションノード(末端の実行ノード).
*            : 任意の処理をstd::function<NodeStatus()>として渡して使う。
*            : 所有側(Enemy等)の状態へラムダキャプチャすることでFSMと同等の
*            : 行動を表現できる。
* @pattern   : Behavior Tree.
**********************************************************************************/

class ActionNode final : public NodeBase
{
public:
	explicit ActionNode(std::function<NodeStatus()> Func) noexcept
		: m_Func{ std::move(Func) }
	{
	}

	~ActionNode() override = default;

	// 登録された処理を実行し、その結果を返す(未登録ならFailure).
	NodeStatus Tick() override
	{
		if (!m_Func) { return NodeStatus::Failure; }

		return m_Func();
	}

private:
	std::function<NodeStatus()> m_Func; // 末端で実行する処理.
};
