#pragma once
#include <Windows.h>

//クラスの前方宣言.
class DirectX12;
class PMDActor;
class PMXActor;
class PMDRenderer;
class PMXRenderer;
class CGame;
class CameraManager;
class GameTime;
class MeshManager;

/**************************************************
*	メインクラス.
**/
class Main
{
public:
	Main();	// コンストラクタ.
	~Main();	// デストラクタ.

	void Update();		// 更新処理.
	void Draw();		// 描画処理.
	HRESULT Create();	// 構築処理.
	HRESULT LoadData();	// データロード処理.
	void Release();		// 解放処理.

	void Loop();		// メインループ.

	//ウィンドウ初期化関数.
	HRESULT InitWindow(
		HINSTANCE hInstance,
		int x, int y,
		int width, int height );

private:
	//ウィンドウ関数（メッセージ毎の処理）.
	static LRESULT CALLBACK MsgProc(
		HWND hWnd, UINT uMsg,
		WPARAM wParam, LPARAM lParam );

private:
	

	HWND			m_hWnd;	 // ウィンドウハンドル.

	std::shared_ptr<DirectX12>		m_pDx12;			// DirectX12セットアップクラス.
	std::shared_ptr<PMDRenderer>	m_pPMDRenderer;
	std::shared_ptr<PMDActor>		m_pPmdActor;

	std::shared_ptr<PMXRenderer>	m_pPMXRenderer;
	std::shared_ptr<PMXActor>		m_pPMXActor;

	std::unique_ptr<CameraManager>	m_upCameraManager;	// カメラマネージャー.
	std::unique_ptr<GameTime>		m_upGameTime;		// ゲーム全体の時計.
	std::unique_ptr<MeshManager>	m_upMeshManager;	// メッシュマネージャー.
};