#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "00_Game/30_Camera/00_Base/CameraBase.h"
#include "00_Game/30_Camera/50_Keyframe/KeyframeCamera.h"

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/07.
* @brief     : 登録したカメラを名前で管理し、アクティブカメラを切り替えるマネージャー.
*            : カメラは分割画面などで将来複数持ちうるため、シングルトンにはせず
*            : Main(将来的にはScene)がメンバとして所有する想定.
**********************************************************************************/

class CameraManager final
{
public:
	CameraManager();
	~CameraManager();

	CameraManager(const CameraManager&)            = delete;
	CameraManager& operator=(const CameraManager&) = delete;

	// カメラを登録する.
	void Register(std::string_view Name, std::unique_ptr<CameraBase> upCamera);

	// アクティブカメラを切り替える.
	void SetActive(std::string_view Name);

	// アクティブカメラを取得(未設定ならnullptr).
	CameraBase* GetActive() const noexcept;

	// アクティブカメラの登録名を取得(デバッグ表示用).
	const std::string& GetActiveName() const noexcept { return m_ActiveName; }

	// アクティブカメラのUpdateを呼び出す.
	void Update();

	// Keyframesを一度だけ再生する演出用カメラにNameで切り替える. 再生が終わると
	// 自動的に、この呼び出し時点でアクティブだったカメラへ戻る(呼び出し元はタイマー等を持たなくてよい).
	// IsRelativeToFirst: trueなら先頭以外のキーフレームを先頭からの相対座標として扱う(KeyframeCamera参照).
	void PlayOneShot(std::string_view Name, std::vector<CameraKeyframe> Keyframes, bool IsRelativeToFirst = false);

private:
	std::unordered_map<std::string, std::unique_ptr<CameraBase>> m_Cameras;	// 登録済みカメラ.
	CameraBase* m_pActiveCamera;	// アクティブカメラ(非所有).
	std::string m_ActiveName;		// アクティブカメラの登録名(PlayOneShotの戻り先解決用).
};
