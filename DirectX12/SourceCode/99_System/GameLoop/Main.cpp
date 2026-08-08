#include "Main.h"
#include "10_Ggraphic/DirectX/DirectX12.h"
#include "10_Ggraphic/PMD/PMDActor.h"
#include "10_Ggraphic/PMD/PMDRenderer.h"
#include "10_Ggraphic/PMX/PMXActor.h"
#include "10_Ggraphic/PMX/PMXRenderer.h"
#include "Time/Time.h"
#include "20_Resource/ResourceManager/MeshManager/MeshManager.h"
#include "00_Game/31_Camera/99_Manager/CameraManager.h"
#include "00_Game/31_Camera/30_Debug/DebugCamera.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"
#include "99_Utility/Diagnostics/MemoryLeakDetector.h"

#ifdef _DEBUG
#include <crtdbg.h>
#endif

// ウィンドウを画面中央で起動を有効にする.
#define ENABLE_WINDOWS_CENTERING

#define ISOMX 0

//=================================================
// 定数.
//=================================================
const TCHAR WND_TITLE[] = _T("DirectX12");
const TCHAR APP_NAME[]  = _T("DirectX12");

//=================================================
// コンストラクタ.
//=================================================
Main::Main()
    : m_hWnd            { nullptr }
    , m_pDx12           { nullptr }
    , m_pPmdActor       { nullptr }
    , m_pPMDRenderer    { nullptr }
    , m_pPMXActor       { nullptr }
    , m_pPMXRenderer    { nullptr }
    , m_upCameraManager { nullptr }
    , m_upGameTime      { nullptr }
    , m_upMeshManager   { nullptr }
{
}

//=================================================
// デストラクタ.
//=================================================
Main::~Main()
{
}

// 構築処理.
HRESULT Main::Create()
{
    // ここから先で増えたメモリ確保をリーク検知の対象にする.
    Diagnostics::BeginMemoryLeakCheck();

    // GameTimeは毎フレーム最初に使われるため、他の何よりも先に構築・登録する.
    m_upGameTime = std::make_unique<GameTime>();
    ServiceLocator::Provide<GameTime>(m_upGameTime.get());

    m_upMeshManager = std::make_unique<MeshManager>();
    ServiceLocator::Provide<MeshManager>(m_upMeshManager.get());

    m_pDx12 = std::make_shared<DirectX12>();
    m_pDx12->Create(m_hWnd);

    // カメラマネージャーを構築し、デバッグカメラをデフォルトで有効化.
    m_upCameraManager = std::make_unique<CameraManager>();
    m_upCameraManager->Register("Debug", std::make_unique<DebugCamera>());
    m_upCameraManager->SetActive("Debug");

    // 所有権はMainのまま、他クラスからも参照できるようサービスロケーターへ登録.
    ServiceLocator::Provide<CameraManager>(m_upCameraManager.get());

    try {
        //m_pPMDRenderer = std::make_shared<CPMDRenderer>(*m_pDx12);
        //m_pPmdActor = std::make_shared<CPMDActor>("Data\\Model\\PMD\\Cube\\Cube.pmd", *m_pPMDRenderer);
#if ISOMX
        m_pPMDRenderer = std::make_shared<PMDRenderer>(*m_pDx12);
        m_pPmdActor = std::make_shared<PMDActor>("Data\\Model\\PMD\\Defelt\\初音ミクVer2.pmd", *m_pPMDRenderer);

#else
        m_pPMXRenderer = std::make_shared<PMXRenderer>(*m_pDx12);
        m_pPMXActor = std::make_shared<PMXActor>("Data\\Model\\PMX\\Hatune\\REM式プロセカ風初音ミクN25.pmx", *m_pPMXRenderer);

        //m_pPMXActor->LoadVMDFile("Data\\Model\\PMX\\Hatune\\Anim\\Anim.vmd");
        m_pPMXActor->PlayAnimation();
        // Data\\Model\\PMX\\Hatune\\REM式プロセカ風初音ミクN25.pmx
        // Data\\Model\\PMX\\HatuneVer2\\初音ミクVer2.pmx
        // Data\\Model\\PMX\\Cube\\Cube.pmx"
        // "Data\\Model\\PMX\\Hatune\\Anim\\Anim.vmd"
        // "Data\\Model\\PMX\\HatuneVer2\\motion\\swing.vmd"
#endif
    }
    catch (const std::runtime_error& Msg) {
        // エラーメッセージを表示(未捕捉のまま伝播させてabortするのを防ぐ).
        std::wstring WStr = MyString::StringToWString(Msg.what());
        _ASSERT_EXPR(false, WStr.c_str());
        return E_FAIL;
    }

    return S_OK;
}

// データロード処理.
HRESULT Main::LoadData()
{
    // 必要に応じてデータロード処理を追加.
    return S_OK;
}

// 更新処理.
void Main::Update()
{
    if (m_upCameraManager) {
        m_upCameraManager->Update();

        // アクティブカメラの行列をDirectX12側へ反映.
        if (CameraBase* active_camera = m_upCameraManager->GetActive()) {
            m_pDx12->SetCamera(
                active_camera->GetViewMatrix(),
                active_camera->GetProjMatrix(),
                active_camera->GetPosition());
        }
    }

    if (m_pDx12) {
        m_pDx12->Update();
    }

    if (m_pPmdActor) {
        m_pPmdActor->Update();
    }


    if (m_pPMXActor) {
        m_pPMXActor->Update();
    }

}

// 描画処理.
void Main::Draw()
{
    if (!m_pDx12) return;

    // 全体の描画準備.
    m_pDx12->BeginDraw();

#if ISOMX
	//PMD用の描画パイプラインに合わせる
    m_pDx12->GetCommandList()->SetPipelineState(m_pPMDRenderer->GetPipelineState());
    //ルートシグネチャもPMD用に合わせる
    m_pDx12->GetCommandList()->SetGraphicsRootSignature(m_pPMDRenderer->GetRootSignature());
#else 
    //PMD用の描画パイプラインに合わせる
    m_pDx12->GetCommandList()->SetPipelineState(m_pPMXRenderer->GetPipelineState());
    //ルートシグネチャもPMX用に合わせる
    m_pDx12->GetCommandList()->SetGraphicsRootSignature(m_pPMXRenderer->GetRootSignature());
#endif
    m_pDx12->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    if (m_pPmdActor) {
        m_pPmdActor->Draw();
    }

    if (m_pPMXActor) {
        m_pPMXActor->Draw();
    }

    // 終了処理.
    m_pDx12->EndDraw();
}

// 解放処理.
void Main::Release()
{
    if (m_upCameraManager) {
        // サービスロケーターへの登録を先に解除してから破棄する.
        ServiceLocator::Provide<CameraManager>(nullptr);
        m_upCameraManager.reset();
    }

    if (m_upMeshManager) {
        ServiceLocator::Provide<MeshManager>(nullptr);
        m_upMeshManager.reset();
    }

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

#if _DEBUG
    // オブジェクトの解放ミスを検出.
    MyComPtr<ID3D12DebugDevice> debugDevice;
    if (SUCCEEDED(m_pDx12->GetDevice()->QueryInterface(IID_PPV_ARGS(debugDevice.GetAddressOf())))) {
        debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
    }
#endif  // _DEBUG

    if (m_pDx12) {
        m_pDx12.reset();
    }

    // GameTimeは一番最後に破棄する(他の解放処理がデルタタイムを参照する可能性があるため).
    if (m_upGameTime) {
        ServiceLocator::Provide<GameTime>(nullptr);
        m_upGameTime.reset();
    }

    // BeginMemoryLeakCheck()以降のリークがあればここで報告される.
    Diagnostics::EndMemoryLeakCheck();
}

// メッセージループ.

void Main::Loop()
{
    MSG msg = {};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            GameTime::Update();
            GameTime::MaintainFPS();

            Update();
            Draw();
        }
    }
}

// ウィンドウ初期化関数.
HRESULT Main::InitWindow(HINSTANCE hInstance, int x, int y, int width, int height)
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

    int winWidth = rect.right - rect.left;
    int winHeight = rect.bottom - rect.top;
    int winX = (GetSystemMetrics(SM_CXSCREEN) - winWidth) / 2;
    int winY = (GetSystemMetrics(SM_CYSCREEN) - winHeight) / 2;

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
LRESULT CALLBACK Main::MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // hWndに関連付けられたCMainを取得.
    // MEMO : ウィンドウが作成されるまでは nullptr になる可能性がある.
    Main* pMain = reinterpret_cast<Main*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

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
