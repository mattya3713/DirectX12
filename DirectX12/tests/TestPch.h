// スタンドアロンテスト用の強制インクルード(/FI指定で使用).
// ゲーム本体のstdafx.h相当(Global.h経由でMyAssert/_T等)を提供する.
#pragma once

#include <Windows.h>
#include <tchar.h>

#include "../SourceCode/Global.h"
