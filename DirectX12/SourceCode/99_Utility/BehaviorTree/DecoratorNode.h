#pragma once

#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>

#include "99_Utility/BehaviorTree/NodeBase.h"
#include "99_Utility/BehaviorTree/NodeStatus.h"

/**********************************************************************************
* @brief     : デコレータノード(子1つをラップして結果を変換する).
*            : 条件反転・失敗吸収・リトライの3モードを持ち、条件判定ノードを
*            : ラムダActionとして子に持たせる使い方を想定。
* @pattern   : Decorator (Behavior Tree).
**********************************************************************************/

class DecoratorNode final : public NodeBase
{
public:
	// 変換種別.
	enum class Mode : std::uint8_t
	{
		Invert,        // 子のSuccess/Failureを反転する.
		AlwaysSucceed, // 子がFailureでもSuccessへ変換する(Runningはそのまま).
		Retry,         // 子がFailureでも上限回数まではRunningとして再試行する.
	};

	explicit DecoratorNode(Mode mode = Mode::Invert, std::uint32_t MaxRetry = 1u) noexcept
		: m_Mode      { mode }
		, m_MaxRetry  { MaxRetry }
	{
	}

	~DecoratorNode() override = default;

	// 子ノードを設定する(所有権を受け取る).
	void SetChild(std::unique_ptr<NodeBase> upChild) noexcept { m_upChild = std::move(upChild); }

	// 子ノードを生成して設定し、追加したノードへの参照を返す.
	template<typename T, typename... TArgs>
	T& AddChild(TArgs&&... Args)
	{
		static_assert(std::is_base_of_v<NodeBase, T>);

		m_upChild = std::make_unique<T>(std::forward<TArgs>(Args)...);

		return *static_cast<T*>(m_upChild.get());
	}

	// 子の評価結果をモードに応じて変換して返す.
	NodeStatus Tick() override
	{
		if (!m_upChild) { return NodeStatus::Failure; }

		const NodeStatus Status{ m_upChild->Tick() };

		switch (m_Mode)
		{
		case Mode::Invert:
			if (Status == NodeStatus::Success) { return NodeStatus::Failure; }
			if (Status == NodeStatus::Failure) { return NodeStatus::Success; }
			break;

		case Mode::AlwaysSucceed:
			if (Status == NodeStatus::Failure) { return NodeStatus::Success; }
			break;

		case Mode::Retry:
			if (Status == NodeStatus::Failure)
			{
				if (++m_RetryCount >= m_MaxRetry)
				{
					Reset();

					return NodeStatus::Failure;
				}

				return NodeStatus::Running; // 上限に達するまで再試行を継続.
			}
			break;

		default: break;
		}

		return Status;
	}

	// 子の評価状態とリトライ回数を初期化する.
	void Reset() override
	{
		if (m_upChild) { m_upChild->Reset(); }

		m_RetryCount = 0u;
	}

private:
	std::unique_ptr<NodeBase> m_upChild;   // ラップする子ノード(所有).
	Mode                      m_Mode;      // 変換種別.
	std::uint32_t             m_MaxRetry;  // Retry時の最大試行回数.
	std::uint32_t             m_RetryCount = 0u; // 現在の試行回数.
};
