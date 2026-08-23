#include "Main.h"
#include "10_Ggraphic/10_Device/DirectX/DirectX12.h"
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
#include "99_Utility/Debug/Imgui/DebugDockSpace.h"
#include "99_Utility/Debug/Imgui/DebugHud.h"
#include "99_Utility/Debug/Log/DebugLog.h"
#include "99_Utility/Debug/Imgui/DebugConsole.h"
#include "99_Utility/Debug/Imgui/SceneView.h"
#include "99_Utility/Sound/SoundManager.h"
#include "99_Utility/Profiling/Profiler.h"
#include "00_Game/40_Collision/CollisionDetector.h"
#include "00_Game/60_Combat/CombatCoordinator.h"
#include "99_Utility/Event/EventBus.h"
#include "99_Utility/DebugBridge/DebugBridgeServer.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/Player.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/Boss.h"
#include "00_Game/00_Scene/SceneManager.h"
#if _DEBUG
#include "10_Ggraphic/20_Render/Debug/DebugColliderRenderer.h"
#endif

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
    , m_upDebugLog      { nullptr }
    , m_upSceneManager  { nullptr }
    , m_upSoundManager  { nullptr }
    , m_upCollisionDetector { nullptr }
    , m_upCombatCoordinator { nullptr }
#if _DEBUG
    , m_upDebugColliderRenderer { nullptr }
#endif
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

    // ゲーム全体の時間.
    m_upGameTime = std::make_unique<GameTime>();
    ServiceLocator::Provide<GameTime>(m_upGameTime.get());

    // メッシュ生成.
    m_upMeshManager = std::make_unique<MeshManager>();
    ServiceLocator::Provide<MeshManager>(m_upMeshManager.get());

	// 入力系の構築.
    m_upKeyInput = std::make_unique<KeyInput>();
    ServiceLocator::Provide<KeyInput>(m_upKeyInput.get());
    
	// マウス入力の構築.
    m_upMouse = std::make_unique<Mouse>();
    ServiceLocator::Provide<Mouse>(m_upMouse.get());

	// 入力ラッパーの構築(キーボード/マウス/コントローラー).
    m_upInput = std::make_unique<Input>();
    ServiceLocator::Provide<Input>(m_upInput.get());
    Input::SethWnd(m_hWnd);

	// 仮想パッドの構築(アクションマッピング).
    m_upVirtualPad = std::make_unique<VirtualPad>();
    ServiceLocator::Provide<VirtualPad>(m_upVirtualPad.get());
    m_upVirtualPad->SetupDefaultBindings();

	// DirectX12の構築.
    m_pDx12 = std::make_shared<DirectX12>();
    m_pDx12->Create(m_hWnd);
    ServiceLocator::Provide<DirectX12>(m_pDx12.get());

#if _DEBUG
    // 当たり判定コライダーのワイヤーフレーム描画(Debugビルドのみ. Character/Playerが毎フレーム利用する).
    m_upDebugColliderRenderer = std::make_unique<DebugColliderRenderer>(*m_pDx12);
    ServiceLocator::Provide<DebugColliderRenderer>(m_upDebugColliderRenderer.get());
#endif

    // ImGuiの構築・登録(DirectX12構築後でないとデバイスが取得できない).
    m_upImGuiManager = std::make_unique<ImGuiManager>();
    ServiceLocator::Provide<ImGuiManager>(m_upImGuiManager.get());
    if (FAILED(m_upImGuiManager->Init(m_hWnd, *m_pDx12))) {
        _ASSERT_EXPR(false, _T("ImGuiの初期化に失敗しました"));
        return E_FAIL;
    }

	// 3Dシーンをオフスクリーンへレンダリングするためのカラーバッファを作成する
	// (Scene ViewパネルがImGui::Image()で表示する. ImGuiManager初期化済みである必要がある).
	m_pDx12->CreateSceneColorTarget(*m_upImGuiManager);

	// ログ管理を構築し、DebugConsoleから参照できるよう登録する.
	m_upDebugLog = std::make_unique<DebugLog>();
	ServiceLocator::Provide<DebugLog>(m_upDebugLog.get());

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

    // 戦闘演出の仲介役を構築(Player/Bossの実体はまだ無いため、シーン側がInitialize()を呼ぶまで空のまま).
    m_upCombatCoordinator = std::make_unique<CombatCoordinator>();
    ServiceLocator::Provide<CombatCoordinator>(m_upCombatCoordinator.get());

    // 汎用イベントバスを構築(UI/サウンド等がゲーム内イベントを購読するための共通基盤).
    m_upEventBus = std::make_unique<EventBus>();
    ServiceLocator::Provide<EventBus>(m_upEventBus.get());

#if _DEBUG
    // DebugBridgeサーバーを起動(外部EditorとのNamed Pipe通信. _DEBUG限定).
    m_upDebugBridgeServer = std::make_unique<DebugBridgeServer>();
    m_upDebugBridgeServer->SetRuntimeInfoResolver([]() -> nlohmann::json {
        nlohmann::json info{};
        if (Player* p_player = ServiceLocator::Get<Player>()) {
            info["player"] = {
                { "hp", p_player->GetHealth().GetHP() },
                { "maxHp", p_player->GetHealth().GetMaxHP() },
                { "stateId", static_cast<int>(p_player->GetCurrentStateID()) }, // State名はCombatDebugHudの対応表参照.
                { "combo", p_player->GetCombo() },
            };
        }
        if (Boss* p_boss = ServiceLocator::Get<Boss>()) {
            info["boss"] = {
                { "hp", p_boss->GetHealth().GetHP() },
                { "maxHp", p_boss->GetHealth().GetMaxHP() },
                { "stateId", static_cast<int>(p_boss->GetCurrentStateID()) },
            };
        }
        info["timeScale"] = GameTime::GetTimeScale();
        info["paused"] = GameTime::IsPaused();
        return info;
    });
    m_upDebugBridgeServer->Start(L"\\\\.\\pipe\\senzan.debugbridge.control.v1");
#endif

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
    Profiler::ScopedTimer cpu_timer("CPU:Update");

#if _DEBUG
    // DebugBridge: 受信済み要求を実行し応答を積む(メインスレッド上でのみゲーム状態へ触れる).
    if (m_upDebugBridgeServer) { m_upDebugBridgeServer->Pump(); }
#endif

#if _DEBUG
    if (m_upSceneManager && m_upSceneManager->IsAnimationTuningActive()) {
        // ドッキング対象のBegin()より先にホストを提出し、ImGuiのドッキング処理順を保証する.
        DebugDockSpace::Draw();
    }
#endif

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

    Profiler::ScopedTimer cpu_timer("CPU:Draw");
    Profiler::Instance().GpuBegin("GPU:Frame");

#if _DEBUG
    const bool is_editor_scene = m_upSceneManager && m_upSceneManager->IsAnimationTuningActive();
#else
    constexpr bool is_editor_scene = false;
#endif

    // 全体の描画準備.
    m_pDx12->BeginDraw(is_editor_scene);

    // デバッグHUD(FPS・デルタタイム・カメラ情報)を表示.
    DebugHud::Draw();
	DebugConsole::Draw();
	Profiler::Instance().DrawImGui();
	if (is_editor_scene) {
		SceneView::Draw();
	}
	if (m_upSceneManager) {
        Profiler::Instance().GpuBegin("GPU:Scene");
        m_upSceneManager->Draw();
        Profiler::Instance().GpuEnd("GPU:Scene");
    }

	if (is_editor_scene) {
		// 3DシーンをオフスクリーンからPIXEL_SHADER_RESOURCEへ、実際のバックバッファをImGui用のRENDER_TARGETへ.
		m_pDx12->PrepareUIRenderTarget();
	}

    // ImGuiの描画コマンドを積む(他の描画がすべて終わった後、EndDraw前).
    Profiler::Instance().GpuBegin("GPU:UI");
    ImGuiManager::Render();
    Profiler::Instance().GpuEnd("GPU:UI");

    // 終了処理.
    Profiler::Instance().GpuEnd("GPU:Frame");
    m_pDx12->EndDraw();
}

// 解放処理.
void Main::Release()
{
    // 通信スレッドがシーン/サービスへ触れないよう、最初にサーバーだけ停止する.
    if (m_upDebugBridgeServer) {
        m_upDebugBridgeServer->Stop();
        m_upDebugBridgeServer.reset();
    }

    // DirectX12/CameraManagerへの参照を各シーンが持ちうるため、それらより先に解放する.
    if (m_upSceneManager) {
        ServiceLocator::Provide<SceneManager>(nullptr);
        m_upSceneManager.reset();
    }

    if (m_upCollisionDetector) {
        ServiceLocator::Provide<CollisionDetector>(nullptr);
        m_upCollisionDetector.reset();
    }

#if _DEBUG
    if (m_upDebugColliderRenderer) {
        ServiceLocator::Provide<DebugColliderRenderer>(nullptr);
        m_upDebugColliderRenderer.reset();
    }
#endif

    if (m_upCombatCoordinator) {
        m_upCombatCoordinator->Clear();
        ServiceLocator::Provide<CombatCoordinator>(nullptr);
        m_upCombatCoordinator.reset();
    }

    if (m_upEventBus) {
        ServiceLocator::Provide<EventBus>(nullptr);
        m_upEventBus.reset();
    }

    if (m_upSoundManager) {
        ServiceLocator::Provide<SoundManager>(nullptr);
        m_upSoundManager.reset();
    }

    if (m_upCameraManager) {
        ServiceLocator::Provide<CameraManager>(nullptr);
        m_upCameraManager.reset();
    }

    if (m_upMeshManager) {
        ServiceLocator::Provide<MeshManager>(nullptr);
        m_upMeshManager.reset();
    }

    if (m_upImGuiManager) {
        ServiceLocator::Provide<ImGuiManager>(nullptr);
        m_upImGuiManager->Shutdown();
        m_upImGuiManager.reset();
    }

	if (m_upDebugLog) {
		ServiceLocator::Provide<DebugLog>(nullptr);
		m_upDebugLog.reset();
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

            Profiler::Instance().BeginFrame();

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

        case WM_SIZE:
            if (pMain->m_pDx12) {
                const UINT width = LOWORD(lParam);
                const UINT height = HIWORD(lParam);
                pMain->m_pDx12->OnWindowResize(width, height);
            }
            break;

        case WM_MOUSEWHEEL:
            // 1フレーム中に複数ノッチ分のメッセージが届くことがあるため、上書きではなく積算する
            // (DebugCamera側が1フレームに1回読み取ってリセットする).
            Input::SetWheelDirection(Input::GetWheelDirection() + (GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? 1 : -1));
            break;

            // キーボードが押されたとき.
        case WM_KEYDOWN:
            // ESCはゲーム内ポーズ(VirtualPad::Pause)と兼用のため、単押しでは終了しない.
            // 0.5秒以内の2連打(ダブルタップ)で終了する. 自動リピート押下は除外する
            // (旧実装はESCを押すたびモーダル確認が出てポーズ操作と衝突していた).
            if (wParam == VK_ESCAPE && (lParam & 0x40000000) == 0) {
                static DWORD s_last_esc_tick = 0;
                const DWORD now_tick = GetTickCount();
                if (s_last_esc_tick != 0 && now_tick - s_last_esc_tick <= 500) {
                    DestroyWindow(hWnd);
                }
                s_last_esc_tick = now_tick;
            }
            break;

        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
        }
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
