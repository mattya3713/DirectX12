#pragma once
#include <crtdbg.h>//_ASSERT_EXPR()�ŕK�v.

//===========================================================
//	�}�N��.
//===========================================================

//���.
#define SAFE_RELEASE(p)	if(p!=nullptr){(p)->Release();(p)=nullptr;}
//�j��.
#define SAFE_DELETE(p) if(p!=nullptr){delete (p);(p)=nullptr;}
#define SAFE_DELETE_ARRAY(p)	\
{								\
	if(p!=nullptr){				\
		delete[] (p);			\
		(p) = nullptr;			\
	}							\
}

//ImGui�@���{��Ή�.
#ifdef _DEBUG
#define IMGUI_JAPANESE(str) reinterpret_cast<const char*>(u8##str)
#endif

// HRESULT.
#define RETURN_IF_FAILED(result) if (FAILED(result)) { return result; } 

