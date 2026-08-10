#pragma once

#include <Windows.h>

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/10.
* @brief     : シーンの抽象基底クラス(Senzanの`SceneBase`を移植).
*            : SceneManagerが`std::unique_ptr<SceneBase>`で1つだけ所有し、
*            : シーン切り替え時に破棄→生成し直す.
**********************************************************************************/

class SceneBase
{
public:
	SceneBase() = default;
	virtual ~SceneBase() = default;

	SceneBase(const SceneBase&)            = delete;
	SceneBase& operator=(const SceneBase&) = delete;
	SceneBase(SceneBase&&)                 = delete;
	SceneBase& operator=(SceneBase&&)      = delete;

	// サブオブジェクトの構築等、軽い初期化.
	virtual void Initialize() = 0;
	// GPUリソースの生成等、重い初期化.
	virtual void Create() = 0;
	// 毎フレーム更新.
	virtual void Update() = 0;
	// Updateの後に呼ばれる更新(将来の当たり判定挿入用に分離).
	virtual void LateUpdate() = 0;
	// 描画.
	virtual void Draw() = 0;

protected:
	HWND m_hWnd = nullptr; // ウィンドウハンドル.
};
