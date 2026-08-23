#pragma once

#include <cstdint>

/**********************************************************************************
* @brief     : Behavior Treeのノード評価結果.
*            : Success/Failureは即確定、Runningは処理継続中(次Tickで再評価)を表す。
**********************************************************************************/

enum class NodeStatus : std::uint8_t
{
	Success, // 成功(条件成立・行動完了).
	Failure, // 失敗(条件不成立・行動不能).
	Running, // 継続中(次のTickでも同じノードを再評価する).
};
