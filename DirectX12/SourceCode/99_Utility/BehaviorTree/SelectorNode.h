#pragma once

#include "99_Utility/BehaviorTree/CompositeNodeBase.h"
#include "99_Utility/BehaviorTree/NodeStatus.h"

/**********************************************************************************
* @brief     : セレクタノード(OR的評価).
*            : 子を先頭から順に評価し、最初にSuccessを返した子で確定する。
*            : 全子がFailureならFailure。Runningを返した場合は次Tickもその子から
*            : 再開する(それ以前の子は再評価しない)。
* @pattern   : Behavior Tree.
**********************************************************************************/

class SelectorNode final : public CompositeNodeBase
{
public:
	~SelectorNode() override = default;

	// 子を先頭から試し、最初のSuccessで確定する.
	NodeStatus Tick() override
	{
		while (m_ActiveIndex < m_upChildren.size())
		{
			const NodeStatus Status{ m_upChildren[m_ActiveIndex]->Tick() };

			if (Status == NodeStatus::Running) { return NodeStatus::Running; } // 再開位置はm_ActiveIndexのまま保持.
			if (Status == NodeStatus::Success)
			{
				m_ActiveIndex = 0u;
				return NodeStatus::Success;
			}

			++m_ActiveIndex;
		}

		m_ActiveIndex = 0u;

		return NodeStatus::Failure;
	}
};
