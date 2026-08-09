#pragma once

#include <string>
#include <vector>
#include <Windows.h>

#include "ImGui/imgui.h"
#include "99_Utility/ComPtr/ComPtr.h"

class DirectX12;

// ImGuiへ渡す日本語リテラル用マクロ.
// このプロジェクトは/utf-8を設定していないため、素の"日本語"リテラルは実行時文字コード(既定はANSI/CP932)へ
// 変換されてしまいUTF-8前提のImGuiでは文字化けする. u8プレフィックスを強制付与しUTF-8であることを保証する.
// (C++20ではu8""はchar8_t*型になるためconst char*へreinterpret_castする).
// NOTE: ImGuiManagerのラッパー関数(Text/Slider/Input/CheckBox/Combo/Tweak)はLabel引数を内部で
//       自動変換するため、このマクロは不要(素の日本語リテラルをそのまま渡してよい). 直接ImGui::を
//       呼ぶ場合(例: ImGui::Begin("日本語タイトル"))のみこのマクロで包む.
#define IMGUI_JP(str) reinterpret_cast<const char*>(u8##str)

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/10.
* @brief     : ImGui統合ラッパークラス(SenzanのCImGuiManagerを参考にDX12向けに再構築)。
*            : ServiceLocator経由で利用する。
**********************************************************************************/

class ImGuiManager final
{
public:
	ImGuiManager();
	~ImGuiManager();

	// 初期化(DirectX12構築後に1度だけ呼ぶ).
	HRESULT Init(HWND hWnd, DirectX12& Dx12);

	// 解放(DirectX12破棄前に1度だけ呼ぶ).
	void Shutdown();

	// フレーム開始(毎フレーム、Update()より前に呼ぶ).
	static void NewFrame();

	// 描画コマンドを積む(BeginDraw〜EndDrawの間、他の描画がすべて終わった後に呼ぶ).
	static void Render();

	// Win32メッセージの転送(MsgProcの先頭で呼ぶ). 未初期化なら何もせず0を返す.
	static LRESULT WndProcHandler(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

public: // 値調整用ラッパー(Senzan CImGuiManager方式: "##"でラベル表示とウィジェットIDを分離する).
	// いずれもLabel引数の日本語リテラルをそのまま渡してよい(内部でUTF-8へ自動変換する).

	// テキスト表示.
	static void Text(const char* InText);

	// スライダー表示(int/floatに対応).
	template<typename T>
	static void Slider(const char* Label, T& Value, T ValueMin, T ValueMax, bool IsLabel = true);

	// スライダー + 現在値をC++リテラル文字列としてクリップボードにコピーするボタン.
	// 調整した値をソースコードの初期値に貼り戻す用途(int/floatに対応).
	template<typename T>
	static void Tweak(const char* Label, T& Value, T ValueMin, T ValueMax, bool IsLabel = true);

	// 数値入力ボックス表示(int/float/std::stringに対応). 値が変化した場合trueを返す.
	template<typename T>
	static bool Input(const char* Label, T& Value, bool IsLabel = true, float Step = 0.0f, float StepFast = 0.0f, const char* Format = "%.3f");

	// チェックボックス表示. 値が変化した場合trueを返す.
	static bool CheckBox(const char* Label, bool& Flag, bool IsLabel = true);

	// コンボボックス表示. 選択中の項目名を返す.
	static std::string Combo(const char* Label, std::string& NowItem, const std::vector<std::string>& List, bool IsLabel = false, float Space = 100.0f);

private:
	// フォントSRV用のディスクリプタヒープを作成する.
	bool CreateSrvHeap(DirectX12& Dx12);

private:
	static constexpr float SAME_LINE_OFFSET = 100.0f;	// ラベルとウィジェットを横並びにするオフセット.
	static constexpr int   FRAME_COUNT      = 2;		// バックバッファ数(DirectX12::CreateSwapChainのBufferCountと一致させること).

private:
	DirectX12*                     m_pDx12;			// 描画に使うDirectX12への非所有参照.
	MyComPtr<ID3D12DescriptorHeap> m_cpSrvHeap;		// ImGui専用のSRVディスクリプタヒープ(フォント用).
	bool                           m_IsInitialized;	// Init()が成功済みか.
};
