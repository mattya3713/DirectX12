#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "00_Game/30_Camera/00_Base/CameraBase.h"

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

	// アクティブカメラのUpdateを呼び出す.
	void Update();

private:
	std::unordered_map<std::string, std::unique_ptr<CameraBase>> m_Cameras;	// 登録済みカメラ.
	CameraBase* m_pActiveCamera;	// アクティブカメラ(非所有).
};
