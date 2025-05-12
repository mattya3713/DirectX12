#pragma once

//警告についてのｺｰﾄﾞ分析を無効にする.4005:再定義.
#pragma warning(disable:4005)


// MEMO : Windowライブラリに入っていmin,maxが邪魔なので無効にする.
//		: Windows.hより先に定義するすることが推奨されているためこの位置.

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <crtdbg.h>

#include <iostream>
#include <vector>       // 可変長配列.	
#include <array>        // 定数長配列.
#include <algorithm>    // アルゴリズム.
#include <map>          // マップ.
#include <unordered_map>// マップ.
#include <cmath>        // 数学関数.
#include <memory>       // メモリ管理.
#include <string>       // 文字列.
#include <fstream>		// ファイル入出力.	

#include "Utility/Assert/Assert.inl"	// HRESULTのtrycatchをする.
#include "Utility/Math/Math.h"			// 算数系.	

#include "Utility/ComPtr/ComPtr.h"		// Microsoft::WRL::ComPtrのようなもの.	

// DirectX12.
#include <D3D12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>

// DirectSound.
#include <dsound.h>


//ライブラリ読み込み.
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
//DirectSound.
#pragma comment( lib, "dsound.lib" )

// ライブラリを設定しなくてはならない.
// $(DXTEX_DIR)\Bin\Desktop_2022_Win10\x64\Debugをリンカー>全般>追加のライブラリディテクトリに追加.
#pragma comment( lib,"DirectXTex.lib" )


//=================================================
//	定数.
//=================================================
static constexpr int	WND_W	= 1280;		// ウィンドウの幅.
static constexpr float	WND_WF	= 1280.f;	// ウィンドウの幅.
static constexpr int	WND_H	= 720;		// ウィンドウの高さ.
static constexpr float	WND_HF	= 720.f;	// ウィンドウの高さ.
static constexpr int	FPS		= 60;			// フレームレート.
