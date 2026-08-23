#pragma once

#include <memory>
#include <type_traits>
#include <vector>

#include "99_Utility/BehaviorTree/NodeBase.h"

/**********************************************************************************
* @brief     : 子ノードを複数持つ複合ノードの基底クラス(抽象).
*            : Selector/Sequenceがこれを継承し、子の順次評価とRunning再開位置の
*            : 記憶を共通実装する。
* @pattern   : Composite.
**********************************************************************************/

class CompositeNodeBase : public NodeBase
{
public:
	~CompositeNodeBase() override = default;

	// 生成済みの子ノードを末尾へ接続する(BT Editor等、外部で生成したノードの取り込み用).
	void AdoptChild(std::unique_ptr<NodeBase> upChild)
	{
		if (upChild) { m_upChildren.push_back(std::move(upChild)); }
	}

	// 子ノードを末尾へ生成追加し、追加したノードへの参照を返す.
	template<typename T, typename... TArgs>
	T& AddChild(TArgs&&... Args)
	{
		static_assert(std::is_base_of_v<NodeBase, T>);

		std::unique_ptr<T> upChild{ std::make_unique<T>(std::forward<TArgs>(Args)...) };
		T& refChild{ *upChild };
		m_upChildren.push_back(std::move(upChild));

		return refChild;
	}

	// 子ノード数を取得する.
	std::size_t GetChildCount() const noexcept { return m_upChildren.size(); }

	// 全子ノードの評価状態と再開位置を初期化する.
	void Reset() override
	{
		for (const std::unique_ptr<NodeBase>& upChild : m_upChildren) { upChild->Reset(); }

		m_ActiveIndex = 0u;
	}

protected:
	// 現在評価中の子のインデックス(Running時の再開位置).
	std::size_t m_ActiveIndex = 0u;

	// 子ノード列(所有).
	std::vector<std::unique_ptr<NodeBase>> m_upChildren;
};
