#include "Main.h"
#include "10_Ggraphic/DirectX/DirectX12.h"
#include "Time/Time.h"
#include "20_Resource/ResourceManager/MeshManager/MeshManager.h"
#include "00_Game/30_Camera/99_Manager/CameraManager.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"
#include "99_Utility/Diagnostics/MemoryLeakDetector.h"
#include "00_Game/50_Input/KeyInput/KeyInput.h"
#include "00_Game/50_Input/Mouse/Mouse.h"
#include "00_Game/50_Input/Input.h"
#include "00_Game/50_Input/VirtualPad.h"
#include "99_Utility/Debug/Imgui/ImGuiManager.h"
#include "99_Utility/Debug/Imgui/DebugHud.h"
#include "99_Utility/Sound/SoundManager.h"
#include "00_Game/40_Collision/CollisionDetector.h"
#include "99_System/Scene/SceneManager.h"

#ifdef _DEBUG
#include <crtdbg.h>
#endif

// ウィンドウを画面中央で起動を有効にする.
#define ENABLE_WINDOWS_CENTERING

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
    , m_upCameraManager { nullptr }
    , m_upGameTime      { nullptr }
    , m_upMeshManager   { nullptr }
    , m_upKeyInput      { nullptr }
    , m_upMouse         { nullptr }
    , m_upInput         { nullptr }
    , m_upVirtualPad    { nullptr }
    , m_upImGuiManager  { nullptr }
    , m_upSceneManager  { nullptr }
    , m_upSoundManager  { nullptr }
    , m_upCollisionDetector { nullptr }
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

    // 入力系(キーボード/マウス/コントローラー/仮想パッド)の構築・登録.
    m_upKeyInput = std::make_unique<KeyInput>();
    ServiceLocator::Provide<KeyInput>(m_upKeyInput.get());

    m_upMouse = std::make_unique<Mouse>();
    ServiceLocator::Provide<Mouse>(m_upMouse.get());

    m_upInput = std::make_unique<Input>();
    ServiceLocator::Provide<Input>(m_upInput.get());
    Input::SethWnd(m_hWnd);

    m_upVirtualPad = std::make_unique<VirtualPad>();
    ServiceLocator::Provide<VirtualPad>(m_upVirtualPad.get());
    m_upVirtualPad->SetupDefaultBindings();

    m_pDx12 = std::make_shared<DirectX12>();
    m_pDx12->Create(m_hWnd);
    // 所有権はMainのまま、シーン側からも参照できるようサービスロケーターへ登録.
    ServiceLocator::Provide<DirectX12>(m_pDx12.get());

    // ImGuiの構築・登録(DirectX12構築後でないとデバイスが取得できない).
    m_upImGuiManager = std::make_unique<ImGuiManager>();
    ServiceLocator::Provide<ImGuiManager>(m_upImGuiManager.get());
    if (FAILED(m_upImGuiManager->Init(m_hWnd, *m_pDx12))) {
        _ASSERT_EXPR(false, _T("ImGuiの初期化に失敗しました"));
        return E_FAIL;
    }

    // カメラマネージャーを構築(カメラの登録・有効化は各シーンが行う).
    m_upCameraManager = std::make_unique<CameraManager>();
    ServiceLocator::Provide<CameraManager>(m_upCameraManager.get());

    // SE再生マネージャーを構築(Data\Sound以下のWAVを読み込む).
    m_upSoundManager = std::make_unique<SoundManager>();
    ServiceLocator::Provide<SoundManager>(m_upSoundManager.get());
    m_upSoundManager->LoadSounds();

    // 当たり判定検出器を構築(各シーン・GameObjectがコライダーを登録する).
    m_upCollisionDetector = std::make_unique<CollisionDetector>();
    ServiceLocator::Provide<CollisionDetector>(m_upCollisionDetector.get());

    // シーンマネージャーを構築し、最初のシーン(MainScene)を読み込む.
    m_upSceneManager = std::make_unique<SceneManager>();
    ServiceLocator::Provide<SceneManager>(m_upSceneManager.get());
    m_upSceneManager->LoadData(SceneManager::eList::MainScene);

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
    if (m_upSceneManager) {
        m_upSceneManager->Update();
    }

    // シーン更新後(位置が確定した後)に、この1フレーム分の当たり判定を実行する.
    if (m_upCollisionDetector) {
        m_upCollisionDetector->ExecuteCollisionDetection();
    }

    if (m_upSoundManager) {
        m_upSoundManager->Update();
    }
}

// 描画処理.
void Main::Draw()
{
    if (!m_pDx12) return;

    // 全体の描画準備.
    m_pDx12->BeginDraw();

    // デバッグHUD(FPS・デルタタイム・カメラ情報)を表示.
    DebugHud::Draw();

    if (m_upSceneManager) {
        m_upSceneManager->Draw();
    }

    // ImGuiの描画コマンドを積む(他の描画がすべて終わった後、EndDraw前).
    ImGuiManager::Render();

    // 終了処理.
    m_pDx12->EndDraw();
}

// 解放処理.
void Main::Release()
{
    // DirectX12/CameraManagerへの参照を各シーンが持ちうるため、それらより先に解放する.
    if (m_upSceneManager) {
        ServiceLocator::Provide<SceneManager>(nullptr);
        m_upSceneManager.reset();
    }

    if (m_upCollisionDetector) {
        ServiceLocator::Provide<CollisionDetector>(nullptr);
        m_upCollisionDetector.reset();
    }

    if (m_upSoundManager) {
        ServiceLocator::Provide<SoundManager>(nullptr);
        m_upSoundManager.reset();
    }

    if (m_upCameraManager) {
        // サービスロケーターへの登録を先に解除してから破棄する.
        ServiceLocator::Provide<CameraManager>(nullptr);
        m_upCameraManager.reset();
    }

    if (m_upMeshManager) {
        ServiceLocator::Provide<MeshManager>(nullptr);
        m_upMeshManager.reset();
    }

    if (m_upImGuiManager) {
        // ReportLiveDeviceObjects()より前に解放し、ImGuiが確保したD3D12リソースをリーク扱いさせない.
        ServiceLocator::Provide<ImGuiManager>(nullptr);
        m_upImGuiManager->Shutdown();
        m_upImGuiManager.reset();
    }

#if _DEBUG
    // オブジェクトの解放ミスを検出.
    MyComPtr<ID3D12DebugDevice> debugDevice;
    if (SUCCEEDED(m_pDx12->GetDevice()->QueryInterface(IID_PPV_ARGS(debugDevice.GetAddressOf())))) {
        debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
    }
#endif  // _DEBUG

    if (m_pDx12) {
        ServiceLocator::Provide<DirectX12>(nullptr);
        m_pDx12.reset();
    }

    if (m_upVirtualPad) {
        ServiceLocator::Provide<VirtualPad>(nullptr);
        m_upVirtualPad.reset();
    }

    if (m_upInput) {
        ServiceLocator::Provide<Input>(nullptr);
        m_upInput.reset();
    }

    if (m_upMouse) {
        ServiceLocator::Provide<Mouse>(nullptr);
        m_upMouse.reset();
    }

    if (m_upKeyInput) {
        ServiceLocator::Provide<KeyInput>(nullptr);
        m_upKeyInput.reset();
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
            Input::Update();
            ImGuiManager::NewFrame();

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

    // ImGuiへメッセージを転送(未初期化なら内部で何もしない).
    ImGuiManager::WndProcHandler(hWnd, uMsg, wParam, lParam);

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
