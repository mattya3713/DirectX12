#pragma once
#include <crtdbg.h>//_ASSERT_EXPR()で必要.

//===========================================================
//	マクロ.
//===========================================================

//解放.
#define SAFE_RELEASE(p)	if(p!=nullptr){(p)->Release();(p)=nullptr;}
//破棄.
#define SAFE_DELETE(p) if(p!=nullptr){delete (p);(p)=nullptr;}
#define SAFE_DELETE_ARRAY(p)	\
{								\
	if(p!=nullptr){				\
		delete[] (p);			\
		(p) = nullptr;			\
	}							\
}

//ImGui　日本語対応.
#ifdef _DEBUG
#define IMGUI_JAPANESE(str) reinterpret_cast<const char*>(u8##str)
#endif

// HRESULT.
#define RETURN_IF_FAILED(result) if (FAILED(result)) { return result; } 

