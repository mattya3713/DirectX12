#pragma once

#include <memory>

#include "99_Utility/StateMachine/StateBase.h"

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/10.
* @brief     : ステートマシン(有限状態機械)の管理クラス.
* @pattern   : State.
**********************************************************************************/

template<typename FSM_Owner>
class StateMachine final
{
public:
	explicit StateMachine(FSM_Owner* pOwner) noexcept
		: m_pOwner         { pOwner }
		, m_spCurrentState {}
	{
	}

	~StateMachine() = default;

	StateMachine(const StateMachine&)            = delete;
	StateMachine& operator=(const StateMachine&) = delete;
	StateMachine(StateMachine&&)                 = delete;
	StateMachine& operator=(StateMachine&&)      = delete;

	// 状態を変更する.
	void ChangeState(std::shared_ptr<StateBase<FSM_Owner>> spNewState)
	{
		// 現在のステートが存在し、遷移を許可していない場合は変更を拒否する.
		if (m_spCurrentState && !m_spCurrentState->CanChangeState())
		{
			return;
		}

		if (m_spCurrentState)
		{
			m_spCurrentState->Exit();
			m_spCurrentState = nullptr;
		}

		m_spCurrentState = spNewState;

		if (m_spCurrentState)
		{
			m_spCurrentState->Enter();
		}
	}

	// 更新処理.
	void Update()
	{
		if (m_spCurrentState) { m_spCurrentState->Update(); }
	}

	// Updateの後に呼ばれる更新処理.
	void LateUpdate()
	{
		if (m_spCurrentState) { m_spCurrentState->LateUpdate(); }
	}

	// 描画処理.
	void Draw()
	{
		if (m_spCurrentState) { m_spCurrentState->Draw(); }
	}

	// 現在ステートが存在するか.
	operator bool() const noexcept { return m_spCurrentState != nullptr; }

private:
	FSM_Owner*                            m_pOwner;			// 所有者(非所有).
	std::shared_ptr<StateBase<FSM_Owner>> m_spCurrentState;	// 現在のステート.
};
