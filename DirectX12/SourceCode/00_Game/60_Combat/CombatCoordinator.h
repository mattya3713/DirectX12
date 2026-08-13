#pragma once

#include <DirectXMath.h>

class Player;
class Boss;

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/13.
* @brief     : Player⇔Bossの戦闘演出(パリィ成立時の位置調整等)を仲介するクラス.
*            : Senzanの同名クラス(急ごしらえの設計だったため移植せず設計をやり直す
*            : とDESIGN.mdに記載済み)は、演出の計算とTransformへの書き込みを両方
*            : 自分で行っていた. こちらは責務を分離し、「いつ・どんな演出データに
*            : するか」の計算とトリガーだけを担当する. 実際にTransformへ書き込むのは
*            : 各アクター自身のステート(BossState::ParryReaction、Playerは既存の
*            : PlayerState::Parryが受け取って自分で消費する)に一任する
*            : (同じフレームに2箇所からTransformが書き換えられる競合を避けるため).
*            : CameraManagerと同じくMain.cppで構築しServiceLocatorへ登録するが、
*            : Player/Bossの実体はシーン側にしか無いため、シーンがInitialize()を
*            : 呼ぶまでは何も持たない空の状態で存在する.
**********************************************************************************/

class CombatCoordinator final
{
public:
	CombatCoordinator() = default;
	~CombatCoordinator() = default;

	CombatCoordinator(const CombatCoordinator&)            = delete;
	CombatCoordinator& operator=(const CombatCoordinator&) = delete;

	// Player/Bossの参照を設定する(両方揃って初めて演出をトリガーできる).
	void Initialize(Player* pPlayer, Boss* pBoss) noexcept;

	// シーン終了時に参照を手放す(寿命の切れたポインタを持ち続けないため).
	void Clear() noexcept;

	// パリィが成立した瞬間に呼ぶ. Player/Bossそれぞれの現在位置から演出データを計算し、
	// 各アクター自身のステートへ引き渡す(このメソッド自体はTransformを直接書き換えない).
	void OnParrySuccess() noexcept;

private:
	Player* m_pPlayer = nullptr; // 非所有.
	Boss*   m_pBoss   = nullptr; // 非所有.
};
