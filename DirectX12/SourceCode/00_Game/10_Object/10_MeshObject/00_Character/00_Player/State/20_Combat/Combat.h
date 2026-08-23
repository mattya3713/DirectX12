#pragma once

#include <string>
#include <vector>

#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/PlayerStateBase.h"

namespace PlayerState {

	// 攻撃判定を有効にする時間帯(Enterからの経過時間(秒)基準).
	struct ColliderWindow
	{
		float Start    = 0.0f;
		float Duration = 0.1f;
		bool  IsAct    = false; // 現在有効化中か.
		bool  IsEnd    = false; // 有効→無効を1度終えたか.
	};

	/**********************************************************************************
	* @author    : mattya3713.
	* @date      : 2026/08/11.
	* @brief     : 攻撃系(コンボ攻撃・パリィ)ステートの基底. 経過時間に応じて攻撃判定
	*            : (Character::m_AttackCollider)のON/OFFを切り替えるColliderWindowの
	*            : 仕組みを持つ. タイミング設定はJSON(FileManager)から読み込む.
	*            : SenzanのTargetPos(ボス位置)を使った旋回・接近移動は、対象となる
	*            : Enemy/Bossがまだ実装されていないため今回は含めていない
	*            : (その場で攻撃する形. Enemy実装時に追加を検討する).
	**********************************************************************************/

	class Combat : public PlayerStateBase
	{
	public:
		explicit Combat(Player* pOwner) noexcept;
		~Combat() override = default;

		void Enter() override;
		void Update() override;
		void Exit() override;

		// 突進移動(攻撃中に方向へ距離を詰める. コンボフロー用).
		void LateUpdate() override;

		// 次のステートを使うJSON設定ファイルのパス(派生クラスで上書き. 空文字なら読み込まない).
		virtual std::string GetSettingsFileName() const { return {}; }

	protected:
		// JSON設定ファイルから攻撃パラメータ・ColliderWindowsを読み込む.
		void LoadSettings();

		// 攻撃ウィンドウを追加する.
		void AddColliderWindow(float Start, float Duration);

		// 経過時間に応じて攻撃判定をON/OFFを切り替える.
		void ProcessColliderWindows();

		// コンボ受付(攻撃ボタン)の受付・判定処理. 次のコンボへ遷移してよいならtrueを返す
		// (呼び出し元がtrueを返したら次のComboStateへChangeStateしてreturnすること).
		bool UpdateComboInput();

		// コンボ数に応じたアニメーション再生速度を適用する(勢い制. 上限付き).
		void ApplyComboSpeedToAnimation();

		// 突進方向を確定する(AllowInputRedirect=trueなら移動入力を優先、無ければターゲット方向).
		void DecideRushDirection(bool AllowInputRedirect);

		// 突進移動(LateUpdateから呼ぶ. 攻撃時間全体でRushDistance分だけ詰める).
		void ProcessRushMovement();

	protected:
		float m_MinComboTransTime = 0.0f; // 以降でないとコンボ入力を受け付けない.
		float m_ComboStartTime    = 0.0f; // コンボ入力の受付開始時刻.
		float m_ComboEndTime      = 1.0f; // このステートの終了時刻(コンボ入力が間に合わなければIdleへ).
		float m_CurrentTime       = 0.0f; // 攻撃クリップの現在の再生位置(秒).
		bool  m_IsComboAccepted   = false; // コンボ入力(次の攻撃ボタン)を受け付け済みか.

		std::vector<ColliderWindow> m_ColliderWindows;

		DirectX::XMFLOAT3 m_RushDirection{ 0.0f, 0.0f, 1.0f }; // 突進方向(Enter時に確定).
		bool              m_IsRushEnabled = false;             // 突進有効フラグ(攻撃系のみtrue).
	};

} // namespace PlayerState
