@echo off
rem PooledEnemyFactoryTest単体テストのビルド(実Enemyをリンクするため依存ソースを直接コンパイルする).
rem /utf-8 は必須(テスト埋め込みJSONの日本語をUTF-8バイト列として出力させるため.
rem 指定しないと実行時コードページへ変換され、nlohmannのUTF-8検証に失敗する).
rem /FIstdafx.h と include一式はゲーム本体(DirectX12.vcxproj)の設定踏襲.
setlocal enabledelayedexpansion
cd /d "%~dp0.."

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -property installationPath`) do set "VSROOT=%%i"
if not exist "%VSROOT%\VC\Auxiliary\Build\vcvars64.bat" (
	echo vcvars64.bat not found under "%VSROOT%"
	exit /b 1
)
call "%VSROOT%\VC\Auxiliary\Build\vcvars64.bat" >nul

set SRC=SourceCode
cl /nologo /EHsc /std:c++20 /MD /W3 /utf-8 /FIstdafx.h /D_UNICODE /DUNICODE ^
	/I %SRC% /I %SRC%\00_Game /I %SRC%\10_Ggraphic /I %SRC%\10_Ggraphic\10_Device /I %SRC%\99_Utility ^
	/I Data\Library /I Data\Library\DirectXTex\DirectXTex /I Data\Library\ImGui ^
	tests\PooledEnemyFactoryTest.cpp ^
	%SRC%\00_Game\10_Object\10_MeshObject\00_Character\10_Enemy\Enemy.cpp ^
	%SRC%\00_Game\10_Object\10_MeshObject\00_Character\10_Enemy\State\EnemyStateBase.cpp ^
	%SRC%\00_Game\10_Object\10_MeshObject\00_Character\10_Enemy\State\00_Idle\Idle.cpp ^
	%SRC%\00_Game\10_Object\10_MeshObject\00_Character\10_Enemy\State\10_Chase\Chase.cpp ^
	%SRC%\00_Game\10_Object\10_MeshObject\00_Character\10_Enemy\State\20_Attack\Attack.cpp ^
	%SRC%\00_Game\10_Object\10_MeshObject\00_Character\10_Enemy\State\30_Dead\Dead.cpp ^
	%SRC%\00_Game\10_Object\10_MeshObject\00_Character\Character.cpp ^
	%SRC%\00_Game\10_Object\10_MeshObject\MeshObject.cpp ^
	%SRC%\00_Game\10_Object\00_Base\GameObject.cpp ^
	%SRC%\00_Game\40_Collision\CollisionDetector.cpp ^
	%SRC%\00_Game\40_Collision\00_Core\ColliderBase.cpp ^
	%SRC%\00_Game\40_Collision\00_Core\CollisionMath.cpp ^
	%SRC%\00_Game\40_Collision\00_Capsule\CapsuleCollider.cpp ^
	%SRC%\00_Game\40_Collision\00_Box\BoxCollider.cpp ^
	%SRC%\00_Game\40_Collision\10_Sphere\SphereCollider.cpp ^
	%SRC%\99_Utility\HealthSystem\HealthSystem.cpp ^
	%SRC%\00_Game\00_GameLoop\Time\Time.cpp ^
	%SRC%\00_Game\60_Combat\CombatTuning.cpp ^
	%SRC%\10_Ggraphic\20_Render\Particle\ParticleSystem.cpp ^
	%SRC%\00_Game\30_Camera\00_Base\CameraBase.cpp ^
	%SRC%\00_Game\30_Camera\99_Manager\CameraManager.cpp ^
	%SRC%\00_Game\30_Camera\50_Keyframe\KeyframeCamera.cpp ^
	%SRC%\10_Ggraphic\10_Device\DirectX\DirectX12.cpp ^
	%SRC%\10_Ggraphic\20_Render\Rewind\FrameRewind.cpp ^
	%SRC%\99_Utility\Debug\Imgui\ImGuiManager.cpp ^
	%SRC%\99_Utility\String\String.cpp ^
	Data\Library\ImGui\imgui.cpp ^
	Data\Library\ImGui\imgui_demo.cpp ^
	Data\Library\ImGui\imgui_draw.cpp ^
	Data\Library\ImGui\imgui_impl_dx12.cpp ^
	Data\Library\ImGui\imgui_impl_win32.cpp ^
	Data\Library\ImGui\imgui_tables.cpp ^
	Data\Library\ImGui\imgui_widgets.cpp ^
	/Fe:tests\PooledEnemyFactoryTest.exe ^
	/link /LIBPATH:"Data\Library\DirectXTex\DirectXTex\Bin\Desktop_2022_Win10\x64\Release" ^
	d3d12.lib dxgi.lib d3dcompiler.lib DirectXTex.lib ole32.lib

endlocal & exit /b %ERRORLEVEL%
