#include "CMain.h"
#include "Ggraphic/DirectX/CDirectX12.h"
#include "Ggraphic/PMD/CPMDActor.h"
#include "Ggraphic/PMD/CPMDRenderer.h"
#include "Ggraphic/PMX/CPMXActor.h"
#include "Ggraphic/PMX/CPMXRenderer.h"

#ifdef _DEBUG
#include <crtdbg.h>
#endif

// ウィンドウを画面中央で起動を有効にする.
#define ENABLE_WINDOWS_CENTERING

//=================================================
// 定数.
//=================================================
const TCHAR WND_TITLE[] = _T("ゆきゆき合戦ごろごろ");
const TCHAR APP_NAME[] = _T("ゆきゆき合戦ごろごろ");

//=================================================
// コンストラクタ.
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
// デストラクタ.
//=================================================
CMain::~CMain()
{
    Release();
}

// 更新処理.
void CMain::Update()
{
    if (m_pPmdActor) {
        m_pPmdActor->Update();
    }

    if (m_pPMXActor) {
        m_pPMXActor->Update();
    }
}

// 描画処理.
void CMain::Draw()
{
    if (!m_pDx12) return;

    // 全体の描画準備.
    m_pDx12->BeginDraw();

	//PMD用の描画パイプラインに合わせる
    m_pDx12->GetCommandList()->SetPipelineState(m_pPMDRenderer->GetPipelineState());
    //ルートシグネチャもPMD用に合わせる
    m_pDx12->GetCommandList()->SetGraphicsRootSignature(m_pPMDRenderer->GetRootSignature());

    m_pDx12->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    m_pDx12->SetScene();

    if (m_pPmdActor) {
        m_pPmdActor->Draw();
    }

    if (m_pPMXActor) {
        m_pPMXActor->Draw();
    }

    // 終了処理.
    m_pDx12->EndDraw();

    // フリップ.
    m_pDx12->GetSwapChain()->Present(1, 0);
}

// 構築処理.
HRESULT CMain::Create()
{
    m_pDx12 = std::make_shared<CDirectX12>();
    m_pDx12->Create(m_hWnd);
    m_pPMDRenderer = std::make_shared<CPMDRenderer>(*m_pDx12);
    m_pPmdActor = std::make_shared<CPMDActor>("Data\\Model\\PMD\\初音ミクVer2.pmd", *m_pPMDRenderer);

    m_pPMXRenderer = std::make_shared<CPMXRenderer>(*m_pDx12);
    m_pPMXActor = std::make_shared<CPMXActor>("Data/Model/PMX/NK Miku Hatsune/NK Miku 1.0.pmx", *m_pPMXRenderer);


    return S_OK;
}

// データロード処理.
HRESULT CMain::LoadData()
{
    // 必要に応じてデータロード処理を追加.
    return S_OK;
}

// 解放処理.
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

// メッセージループ.
void CMain::Loop()
{
    float rate = 0.0f;   // フレームレート制御用.
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

// ウィンドウ初期化関数.
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

// ウィンドウ関数（メッセージ毎の処理）.
LRESULT CALLBACK CMain::MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // hWndに関連付けられたCMainを取得.
    // MEMO : ウィンドウが作成されるまでは nullptr になる可能性がある.
    CMain* pMain = reinterpret_cast<CMain*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    // ウィンドウが初めて作成された時.
    if (uMsg == WM_NCCREATE) {
        // CREATESTRUCT構造体からCMainのポインタを取得.
        CREATESTRUCT* pCreateStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
        // SetWindowLongPtrを使用しhWndにCMainインスタンスを関連付ける.
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
        // デフォルトのウィンドウプロシージャを呼び出して処理を進める.
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    if (pMain) {
        switch (uMsg) {
            // ウィンドウが破棄されるとき.
        case WM_DESTROY:
            // GPUの終了を待ってからウィンドウを閉じる.
            pMain->m_pDx12->WaitForGPU();
            PostQuitMessage(0);
            break;

            // キーボードが押されたとき.
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                if (MessageBox(hWnd, _T("ゲームを終了しますか？"), _T("警告"), MB_YESNO) == IDYES) {
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
