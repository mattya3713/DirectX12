#pragma once
#include <Windows.h>

//�N���X�̑O���錾.
class CDirectX12;
class CPMDActor;
class CPMXActor;
class CPMDRenderer;
class CPMXRenderer;
class CGame;

/**************************************************
*	���C���N���X.
**/
class CMain
{
public:
	CMain();	// �R���X�g���N�^.
	~CMain();	// �f�X�g���N�^.

	void Update();		// �X�V����.
	void Draw();		// �`�揈��.
	HRESULT Create();	// �\�z����.
	HRESULT LoadData();	// �f�[�^���[�h����.
	void Release();		// �������.

	void Loop();		// ���C�����[�v.

	//�E�B���h�E�������֐�.
	HRESULT InitWindow(
		HINSTANCE hInstance,
		INT x, INT y,
		INT width, INT height );

private:
	//�E�B���h�E�֐��i���b�Z�[�W���̏����j.
	static LRESULT CALLBACK MsgProc(
		HWND hWnd, UINT uMsg,
		WPARAM wParam, LPARAM lParam );

private:
	

	HWND			m_hWnd;	 // �E�B���h�E�n���h��.

	std::shared_ptr<CDirectX12>		m_pDx12;			// DirectX12�Z�b�g�A�b�v�N���X.
	std::shared_ptr<CPMDRenderer>	m_pPMDRenderer;
	std::shared_ptr<CPMDActor>		m_pPmdActor;

	std::shared_ptr<CPMXRenderer>	m_pPMXRenderer;
	std::shared_ptr<CPMXActor>		m_pPMXActor;
};