#include "CMain.h"
#include <crtdbg.h>	//_ASSERT_EXPR()�ŕK�v.


//================================================
//	���C���֐�.
//================================================
INT WINAPI WinMain(
	_In_ HINSTANCE hInstance,	//�C���X�^���X�ԍ��i�E�B���h�E�̔ԍ��j.
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ PSTR lpCmdLine,
	_In_ INT nCmdShow)
{
	// ���������[�N���o
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	CMain* pCMain = new CMain();	//���������N���X�錾.

	if (pCMain != nullptr)
	{
		//�E�B���h�E�쐬����������.
		if( SUCCEEDED(
			pCMain->InitWindow(
				hInstance,
				0, 0,
				WND_W, WND_H)))
		{
			//Dx11�p�̏�����.
			if( SUCCEEDED( pCMain->Create() ))
			{
				//���b�Z�[�W���[�v.
				pCMain->Loop();
			}
		}
		//�I��.
		pCMain->Release();	//Direct3D�̉��.

		SAFE_DELETE( pCMain );	//�N���X�̔j��.
	}

	return 0;
}



