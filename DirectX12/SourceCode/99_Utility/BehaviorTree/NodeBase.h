#pragma once

#include "99_Utility/BehaviorTree/NodeStatus.h"

/**********************************************************************************
* @brief     : Behavior Treeの全ノードの基底クラス(抽象).
*            : ツリーは毎フレームTick()を評価し、その戻り値で意思決定を表現する。
* @pattern   : Behavior Tree / Composite.
**********************************************************************************/

class NodeBase
{
public:
	NodeBase() = default;
	virtual ~NodeBase() = default;

	NodeBase(const NodeBase&)            = delete;
	NodeBase& operator=(const NodeBase&) = delete;
	NodeBase(NodeBase&&)                 = delete;
	NodeBase& operator=(NodeBase&&)      = delete;

	// ツリーを1ステップ評価し、その結果を返す.
	virtual NodeStatus Tick() = 0;

	// 評価状態(Running中の子の記憶等)を初期状態へ戻す.
	virtual void Reset() {}
};
