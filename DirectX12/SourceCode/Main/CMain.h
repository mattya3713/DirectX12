#pragma once
#include <Windows.h>

//クラスの前方宣言.
class CDirectX12;
class CGame;

/**************************************************
*	メインクラス.
**/
class CMain
{
public:
	CMain();	// コンストラクタ.
	~CMain();	// デストラクタ.

	void Update();		// 更新処理.
	void Draw();		// 描画処理.
	HRESULT Create();	// 構築処理.
	HRESULT LoadData();	// データロード処理.
	void Release();		// 解放処理.

	void Loop();		// メインループ.

	//ウィンドウ初期化関数.
	HRESULT InitWindow(
		HINSTANCE hInstance,
		INT x, INT y,
		INT width, INT height );

private:
	//ウィンドウ関数（メッセージ毎の処理）.
	static LRESULT CALLBACK MsgProc(
		HWND hWnd, UINT uMsg,
		WPARAM wParam, LPARAM lParam );

private:
	

	HWND			m_hWnd;	 // ウィンドウハンドル.
	CDirectX12*		m_pDx12; // DirectX12セットアップクラス.

};