#include "CMain.h"
#include "Ggraphic/DirectX/CDirectX12.h"
#include "Ggraphic/PMD/CPMDActor.h"
#include "Ggraphic/PMD/CPMDRenderer.h"
#include "Ggraphic/PMX/CPMXActor.h"
#include "Ggraphic/PMX/CPMXRenderer.h"

#ifdef _DEBUG
#include <crtdbg.h>
#endif

// �E�B���h�E����ʒ����ŋN����L���ɂ���.
#define ENABLE_WINDOWS_CENTERING

//=================================================
// �萔.
//=================================================
const TCHAR WND_TITLE[] = _T("�䂫�䂫���킲�낲��");
const TCHAR APP_NAME[] = _T("�䂫�䂫���킲�낲��");

//=================================================
// �R���X�g���N�^.
//=================================================
CMain::CMain()
    : m_hWnd            ( nullptr )
    , m_pDx12           ( nullptr )
    , m_pPmdActor       ( nullptr )
    , m_pPMDRenderer    ( nullptr )
    , m_pPMXActor       ( nullptr )
    , m_pPMXRenderer    ( nullptr )
{
}

//=================================================
// �f�X�g���N�^.
//=================================================
CMain::~CMain()
{
    Release();
}

// �X�V����.
void CMain::Update()
{
    if (m_pPmdActor) {
        m_pPmdActor->Update();
    }

    if (m_pPMXActor) {
        m_pPMXActor->Update();
    }
}

// �`�揈��.
void CMain::Draw()
{
    if (!m_pDx12) return;

    // �S�̂̕`�揀��.
    m_pDx12->BeginDraw();

	//PMD�p�̕`��p�C�v���C���ɍ��킹��
    m_pDx12->GetCommandList()->SetPipelineState(m_pPMDRenderer->GetPipelineState());
    //���[�g�V�O�l�`����PMD�p�ɍ��킹��
    m_pDx12->GetCommandList()->SetGraphicsRootSignature(m_pPMDRenderer->GetRootSignature());

    m_pDx12->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    m_pDx12->SetScene();

    if (m_pPmdActor) {
        m_pPmdActor->Draw();
    }

    if (m_pPMXActor) {
        m_pPMXActor->Draw();
    }

    // �I������.
    m_pDx12->EndDraw();

    // �t���b�v.
    m_pDx12->GetSwapChain()->Present(1, 0);
}

// �\�z����.
HRESULT CMain::Create()
{
    m_pDx12 = std::make_shared<CDirectX12>();
    m_pDx12->Create(m_hWnd);
    m_pPMDRenderer = std::make_shared<CPMDRenderer>(*m_pDx12);
    m_pPmdActor = std::make_shared<CPMDActor>("Data\\Model\\PMD\\�����~�NVer2.pmd", *m_pPMDRenderer);

    m_pPMXRenderer = std::make_shared<CPMXRenderer>(*m_pDx12);
    m_pPMXActor = std::make_shared<CPMXActor>("Data/Model/PMX/NK Miku Hatsune/NK Miku 1.0.pmx", *m_pPMXRenderer);


    return S_OK;
}

// �f�[�^���[�h����.
HRESULT CMain::LoadData()
{
    // �K�v�ɉ����ăf�[�^���[�h������ǉ�.
    return S_OK;
}

// �������.
void CMain::Release()
{
    if (m_pPmdActor) {
        m_pPmdActor.reset();
    }

    if (m_pPMDRenderer) {
        m_pPMDRenderer.reset();
    }
    if (m_pPMXActor) {
        m_pPMXActor.reset();
    }

    if (m_pPMXRenderer) {
        m_pPMXRenderer.reset();
    }

    if (m_pDx12) {
        m_pDx12.reset();
    }
}

// ���b�Z�[�W���[�v.
void CMain::Loop()
{
    float rate = 0.0f;   // �t���[�����[�g����p.
    DWORD syncOld = timeGetTime();
    DWORD syncNow;

    MSG msg = {};
    while (msg.message != WM_QUIT) {
        syncNow = timeGetTime();

        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else if (syncNow - syncOld >= rate) {
            syncOld = syncNow;

            Update();
            Draw();
        }
    }

    Release();
}

// �E�B���h�E�������֐�.
HRESULT CMain::InitWindow(HINSTANCE hInstance, INT x, INT y, INT width, INT height)
{
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MsgProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
    wc.lpszClassName = APP_NAME;

    if (!RegisterClassEx(&wc)) {
        return E_FAIL;
    }

    RECT rect = { 0, 0, width, height };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    INT winWidth = rect.right - rect.left;
    INT winHeight = rect.bottom - rect.top;
    INT winX = (GetSystemMetrics(SM_CXSCREEN) - winWidth) / 2;
    INT winY = (GetSystemMetrics(SM_CYSCREEN) - winHeight) / 2;

    m_hWnd = CreateWindow(
        APP_NAME, WND_TITLE,
        WS_OVERLAPPEDWINDOW,
        winX, winY, winWidth, winHeight,
        nullptr, nullptr, hInstance, this
    );

    if (!m_hWnd) {
        return E_FAIL;
    }

    ShowWindow(m_hWnd, SW_SHOW);
    UpdateWindow(m_hWnd);

    return S_OK;
}

// �E�B���h�E�֐��i���b�Z�[�W���̏����j.
LRESULT CALLBACK CMain::MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // hWnd�Ɋ֘A�t����ꂽCMain���擾.
    // MEMO : �E�B���h�E���쐬�����܂ł� nullptr �ɂȂ�\��������.
    CMain* pMain = reinterpret_cast<CMain*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    // �E�B���h�E�����߂č쐬���ꂽ��.
    if (uMsg == WM_NCCREATE) {
        // CREATESTRUCT�\���̂���CMain�̃|�C���^���擾.
        CREATESTRUCT* pCreateStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
        // SetWindowLongPtr���g�p��hWnd��CMain�C���X�^���X���֘A�t����.
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
        // �f�t�H���g�̃E�B���h�E�v���V�[�W�����Ăяo���ď�����i�߂�.
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    if (pMain) {
        switch (uMsg) {
            // �E�B���h�E���j�������Ƃ�.
        case WM_DESTROY:
            // GPU�̏I����҂��Ă���E�B���h�E�����.
            pMain->m_pDx12->WaitForGPU();
            PostQuitMessage(0);
            break;

            // �L�[�{�[�h�������ꂽ�Ƃ�.
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                if (MessageBox(hWnd, _T("�Q�[�����I�����܂����H"), _T("�x��"), MB_YESNO) == IDYES) {
                    DestroyWindow(hWnd);
                }
            }
            break;

        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
        }
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
