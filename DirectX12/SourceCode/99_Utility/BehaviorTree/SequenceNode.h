#pragma once

#include "99_Utility/BehaviorTree/CompositeNodeBase.h"
#include "99_Utility/BehaviorTree/NodeStatus.h"

/**********************************************************************************
* @brief     : シーケンスノード(AND的評価).
*            : 子を先頭から順に評価し、1つでもFailureを返した子があればその場で
*            : 中断してFailure。全子がSuccessならSuccess。Runningを返した場合は
*            : 次Tickもその子から再開する(それ以前の子は再評価しない)。
* @pattern   : Behavior Tree.
**********************************************************************************/

class SequenceNode final : public CompositeNodeBase
{
public:
	~SequenceNode() override = default;

	// 子を先頭から順に実行し、1つでも失敗したら中断する.
	NodeStatus Tick() override
	{
		while (m_ActiveIndex < m_upChildren.size())
		{
			const NodeStatus Status{ m_upChildren[m_ActiveIndex]->Tick() };

			if (Status == NodeStatus::Running) { return NodeStatus::Running; } // 再開位置はm_ActiveIndexのまま保持.
			if (Status == NodeStatus::Failure)
			{
				m_ActiveIndex = 0u;
				return NodeStatus::Failure;
			}

			++m_ActiveIndex;
		}

		m_ActiveIndex = 0u;

		return NodeStatus::Success;
	}
};
