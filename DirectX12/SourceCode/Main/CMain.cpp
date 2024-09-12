#include "CMain.h"
#include "..\DirectX\CDirectX12.h"

#ifdef _DEBUG
	#include <crtdbg.h>
#endif

//�E�B���h�E����ʒ����ŋN����L���ɂ���.
#define ENABLE_WINDOWS_CENTERING

//=================================================
//	�萔.
//=================================================
const TCHAR WND_TITLE[] = _T( "�䂫�䂫���킲�낲��" );
const TCHAR APP_NAME[]	= _T( "�䂫�䂫���킲�낲��" );


/********************************************************************************
*	���C���N���X.
**/
//=================================================
//	�R���X�g���N�^.
//=================================================
CMain::CMain()
	//���������X�g.
	: m_hWnd	( nullptr )
	, m_pDx12	( nullptr )
{
	m_pDx12 = new CDirectX12();
}


//=================================================
//	�f�X�g���N�^.
//=================================================
CMain::~CMain()
{
	SAFE_DELETE(m_pDx12);

	DeleteObject( m_hWnd );
}


//�X�V����.
void CMain::Update()
{
	// �o�b�N�o�b�t�@���N���A�ɂ���.
	//m_pDx12->ClearBackBuffer();
}

// �`�揈��.
void CMain::Draw()
{
	//��ʂɕ\��.
//m_pDx12->Present();
}


//�\�z����.
HRESULT CMain::Create()
{
	// DirectX12�\�z.
	if(m_pDx12->Create( m_hWnd ) )
	{
		return E_FAIL;
	}
	return S_OK;
}

//�f�[�^���[�h����.
HRESULT CMain::LoadData()
{
	return S_OK;
}


//�������.
void CMain::Release()
{

	if (m_pDx12 != nullptr) {
		//m_pDx12->Release();
	}
}


//���b�Z�[�W���[�v.
void CMain::Loop()
{
	//------------------------------------------------
	//	�t���[�����[�g��������.
	//------------------------------------------------
	float Rate = 0.0f;	//���[�g.
	DWORD sync_old = timeGetTime();			//�ߋ�����.
	DWORD sync_now;							//���ݎ���.
	
	//���b�Z�[�W���[�v.
	MSG msg = { 0 };
	ZeroMemory( &msg, sizeof( msg ) );

	while( msg.message != WM_QUIT )
	{
		sync_now = timeGetTime();	//���݂̎��Ԃ��擾.

		if( PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ) )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		else if( sync_now - sync_old >= Rate )
		{
			sync_old = sync_now;	//���ݎ��Ԃɒu������.

			//�X�V����.
			Update();
			Draw();
		}
	}


	//�A�v���P�[�V�����̏I��.
	Release();
}

//�E�B���h�E�������֐�.
HRESULT CMain::InitWindow(
	HINSTANCE hInstance,	//�C���X�^���X.
	INT x, INT y,			//�E�B���h�Ex,y���W.
	INT width, INT height)	//�E�B���h�E��,����.
{
	//�E�B���h�E�̒�`.
	WNDCLASSEX wc;
	ZeroMemory( &wc, sizeof( wc ) );//������(0��ݒ�).

	wc.cbSize			= sizeof( wc );
	wc.style			= CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc		= MsgProc;//WndProc;
	wc.hInstance		= hInstance;
	wc.hIcon			= LoadIcon( nullptr, IDI_APPLICATION );
	wc.hCursor			= LoadCursor( nullptr, IDC_ARROW );
	wc.hbrBackground	= (HBRUSH)GetStockObject( LTGRAY_BRUSH );
	wc.lpszClassName	= APP_NAME;
	wc.hIconSm			= LoadIcon( nullptr, IDI_APPLICATION );

	//�E�B���h�E�N���X��Windows�ɓo�^.
	if( !RegisterClassEx( &wc ) ) {
		_ASSERT_EXPR( false, _T( "�E�B���h�E�N���X�̓o�^�Ɏ��s" ) );
		return E_FAIL;
	}

	//--------------------------------------.
	//	�E�B���h�E�\���ʒu�̒���.
	//--------------------------------------.
	//���̊֐����ł̂ݎg�p����\���̂������Œ�`.
	struct RECT_WND
	{
		INT x, y, w, h;
		RECT_WND() : x(), y(), w(), h() {}
	} rectWindow;//�����ɕϐ��錾������.

#ifdef ENABLE_WINDOWS_CENTERING
	//�f�B�X�v���C�̕��A�������擾.
	HWND hDeskWnd = nullptr;
	RECT recDisplay;
	hDeskWnd = GetDesktopWindow();
	GetWindowRect( hDeskWnd, &recDisplay );

	//�Z���^�����O.
	rectWindow.x = ( recDisplay.right - width ) / 2;	//�\���ʒux���W.
	rectWindow.y = ( recDisplay.bottom - height ) / 2;	//�\���ʒuy���W.
#endif//ENABLE_WINDOWS_CENTERING

	//--------------------------------------.
	//	�E�B���h�E�̈�̒���.
	//--------------------------------------.
	RECT	rect;		//��`�\����.
	DWORD	dwStyle;	//�E�B���h�E�X�^�C��.
	rect.top = 0;			//��.
	rect.left = 0;			//��.
	rect.right = width;		//�E.
	rect.bottom = height;	//��.
	dwStyle = WS_OVERLAPPEDWINDOW;	//�E�B���h�E���.

	if( AdjustWindowRect(
		&rect,			//(in)��ʃT�C�Y����������`�\����.(out)�v�Z����.
		dwStyle,		//�E�B���h�E�X�^�C��.
		FALSE ) == 0 )	//���j���[�������ǂ����̎w��.
	{
		MessageBox(
			nullptr,
			_T( "�E�B���h�E�̈�̒����Ɏ��s" ),
			_T( "�G���[���b�Z�[�W" ),
			MB_OK );
		return 0;
	}

	//�E�B���h�E�̕���������.
	rectWindow.w = rect.right - rect.left;
	rectWindow.h = rect.bottom - rect.top;

	//�E�B���h�E�̍쐬.
	m_hWnd = CreateWindow(
		APP_NAME,					//�A�v����.
		WND_TITLE,					//�E�B���h�E�^�C�g��.
		dwStyle,					//�E�B���h�E���(����).
		rectWindow.x, rectWindow.y,	//�\���ʒux,y���W.
		rectWindow.w, rectWindow.h,	//�E�B���h�E��,����.
		nullptr,					//�e�E�B���h�E�n���h��.
		nullptr,					//���j���[�ݒ�.
		hInstance,					//�C���X�^���X�ԍ�.
		nullptr );					//�E�B���h�E�쐬���ɔ�������C�x���g�ɓn���f�[�^.
	if( !m_hWnd ) {
		_ASSERT_EXPR( false, _T( "�E�B���h�E�쐬���s" ) );
		return E_FAIL;
	}

	//�E�B���h�E�̕\��.
	ShowWindow( m_hWnd, SW_SHOW );
	UpdateWindow( m_hWnd );

	return S_OK;
}

//�E�B���h�E�֐��i���b�Z�[�W���̏����j.
LRESULT CALLBACK CMain::MsgProc(
	HWND hWnd, UINT uMsg,
	WPARAM wParam, LPARAM lParam )
{
	switch( uMsg ) {
	case WM_DESTROY://�E�B���h�E���j�����ꂽ�Ƃ�.
		//�A�v���P�[�V�����̏I����Windows�ɒʒm����.
		PostQuitMessage( 0 );
		break;

	case WM_KEYDOWN://�L�[�{�[�h�������ꂽ�Ƃ�.
		//�L�[�ʂ̏���.
		switch( static_cast<char>( wParam ) ) {
		case VK_ESCAPE:	//ESC��.
			if( MessageBox( nullptr,
				_T( "�Q�[�����I�����܂����H" ),
				_T( "�x��" ), MB_YESNO ) == IDYES )
			{
				//�E�B���h�E��j������.
				DestroyWindow( hWnd );
			}
			break;
		}
		break;
	}

	//���C���ɕԂ����.
	return DefWindowProc( hWnd, uMsg, wParam, lParam );
}