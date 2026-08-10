#pragma once
#include <Windows.h>

//クラスの前方宣言.
class DirectX12;
class CGame;
class CameraManager;
class GameTime;
class MeshManager;
class KeyInput;
class Mouse;
class Input;
class VirtualPad;
class ImGuiManager;
class SceneManager;
class SoundManager;
class CollisionDetector;

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

	std::unique_ptr<CameraManager>	m_upCameraManager;	// カメラマネージャー(各シーンがカメラを登録・切り替える).
	std::unique_ptr<GameTime>		m_upGameTime;		// ゲーム全体の時計.
	std::unique_ptr<MeshManager>	m_upMeshManager;	// メッシュマネージャー.

	std::unique_ptr<KeyInput>		m_upKeyInput;		// キー入力.
	std::unique_ptr<Mouse>			m_upMouse;			// マウス入力.
	std::unique_ptr<Input>			m_upInput;			// 入力機のラッパー(キーボード/マウス/コントローラー).
	std::unique_ptr<VirtualPad>	m_upVirtualPad;		// 仮想パッド(アクションマッピング).
	std::unique_ptr<ImGuiManager>	m_upImGuiManager;	// ImGui統合ラッパー.
	std::unique_ptr<SceneManager>	m_upSceneManager;	// シーンマネージャー.
	std::unique_ptr<SoundManager>	m_upSoundManager;	// SE再生マネージャー.
	std::unique_ptr<CollisionDetector> m_upCollisionDetector; // 当たり判定検出器.
};
