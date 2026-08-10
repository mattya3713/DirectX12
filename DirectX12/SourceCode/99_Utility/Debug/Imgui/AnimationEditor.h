#pragma once

class PMXActor;

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/10.
* @brief     : アニメーションEditor. 起動中はMain側でPMXActor::Update()の呼び出しを
*            : 止め、Stepボタンを押した時だけ1フレーム分進める(ゲーム内で起動できる
*            : ImGui製のツール. CutSceneEditorのタイムライン機能はまだ持たない).
*            : 再生範囲(開始/終了フレーム)・再生速度もここから調整できる.
**********************************************************************************/

class AnimationEditor final
{
public:
	AnimationEditor() = default;
	~AnimationEditor() = default;

	// 起動中かどうかを切り替える.
	void Toggle() noexcept { m_IsActive = !m_IsActive; }
	// 起動中かどうか.
	bool IsActive() const noexcept { return m_IsActive; }

	// 起動中のみUIを描画する. Stepボタンが押されたらtrueを返す.
	bool Draw(PMXActor& Actor);

private:
	bool m_IsActive = false; // 起動中かどうか(trueの間、Mainはゲームの更新を止める).
};
