#pragma once

#include <string>

#include "10_Ggraphic/PMX/AnimationClipTable.h"

class PMXActor;
class XActor;

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/10.
* @brief     : アニメーションEditor. 起動中はMain側でPMXActor::Update()の呼び出しを
*            : 止め、Stepボタンを押した時だけ1フレーム分進める(ゲーム内で起動できる
*            : ImGui製のツール. CutSceneEditorのタイムライン機能はまだ持たない).
*            : 再生範囲(開始/終了フレーム)・再生速度もここから調整できる.
*            : 名前付きクリップとしてAnimationClipTableへ保存・読込もできる
*            : (Character側の再生時にこのテーブルの値を参照する).
*            : XActor(.x)はVMDのような開始/終了フレーム編集の概念を持たない
*            : (名前付きクリップの切り替えのみ)ため、専用のDrawオーバーロードを持つ.
**********************************************************************************/

class AnimationEditor final
{
public:
	AnimationEditor();
	~AnimationEditor() = default;

	// 起動中かどうかを切り替える.
	void Toggle() noexcept { m_IsActive = !m_IsActive; }
	// 起動中かどうか.
	bool IsActive() const noexcept { return m_IsActive; }

	// 起動中のみUIを描画する. Stepボタンが押されたらtrueを返す.
	bool Draw(PMXActor& Actor);

	// 起動中のみUIを描画する(XActor用. クリップ切り替えボタンのみ、常時再生のためStepは無い).
	void Draw(XActor& Actor);

private:
	bool               m_IsActive = false;		// 起動中かどうか(trueの間、Mainはゲームの更新を止める).
	AnimationClipTable m_ClipTable;			// 名前付きクリップの保存先(ファイルへ永続化する).
	std::string        m_ClipName = "Idle";	// 現在編集中のクリップ名.
};
