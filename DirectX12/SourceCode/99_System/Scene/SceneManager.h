#pragma once

#include <memory>

class SceneBase;

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/10.
* @brief     : シーンマネージャー(Senzanの`SceneManager`を移植).
*            : Senzanはシングルトンだったが、このプロジェクトの方針(マネージャーは
*            : サービスロケーター経由)に合わせてMainが所有しServiceLocatorへ登録する形にした.
*            : フェード/当たり判定連携はこのプロジェクトにまだ無いため未対応(導入時に追加).
**********************************************************************************/

class SceneManager final
{
public:
	// シーン一覧.
	enum class eList
	{
		MainScene,	// 通常のメインシーン.

#if _DEBUG
		AnimationTuning,	// アニメーション調整用シーン(デバッグ専用).
#endif // _DEBUG.

		MAX,
	};

public:
	SceneManager();
	~SceneManager();	// unique_ptr<SceneBase>がここでSceneBaseの完全な型を要求するため.cpp側で定義する.

	SceneManager(const SceneManager&)            = delete;
	SceneManager& operator=(const SceneManager&) = delete;

	// 最初のシーンを読み込む(即時).
	void LoadData(eList InitialScene);

	// シーンの切り替えを予約する. 実際の切り替えは次のUpdate()の先頭で行う
	// (シーン自身のUpdate()実行中に呼ばれても、自分自身を即座に破棄しないようにするため).
	void LoadScene(eList Scene);

	// 毎フレーム更新(シーンのUpdate→LateUpdateの順で呼ぶ).
	void Update();
	// 描画.
	void Draw();

private:
	// シーンを生成する.
	void MakeScene(eList Scene);

private:
	std::unique_ptr<SceneBase> m_upScene;				// 現在のシーン.
	eList                      m_NextSceneID = eList::MAX;	// 予約中の切り替え先(MAXなら無し).

#if _DEBUG
	eList m_CurrentSceneID = eList::MAX;	// デバッグ表示用.
#endif // _DEBUG.
};
