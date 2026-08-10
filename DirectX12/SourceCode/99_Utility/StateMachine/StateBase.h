#pragma once

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/10.
* @brief     : ステートマシン(有限状態機械)の状態の基底クラス.
* @pattern   : State.
**********************************************************************************/

template<typename FSM_Owner>
class StateBase
{
public:
	explicit StateBase(FSM_Owner* pOwner) noexcept
		: m_pOwner { pOwner }
	{
	}

	virtual ~StateBase() = default;

	StateBase(const StateBase&)            = delete;
	StateBase& operator=(const StateBase&) = delete;
	StateBase(StateBase&&)                 = delete;
	StateBase& operator=(StateBase&&)      = delete;

	// 状態に入った時の処理.
	virtual void Enter() {}
	// 更新処理.
	virtual void Update() {}
	// Updateの後に呼ばれる更新処理.
	virtual void LateUpdate() {}
	// 描画処理.
	virtual void Draw() {}
	// 状態から出る時の処理.
	virtual void Exit() {}

	// ステート遷移を許可するか(既定では許可する).
	virtual bool CanChangeState() const { return true; }

protected:
	FSM_Owner* m_pOwner; // このステートの所有者(非所有).
};
