@echo off
rem CombatSEEventTest unit test build (links real Character/CombatCoordinator dependency sources directly).
rem /utf-8 is mandatory (Japanese PASS labels must be emitted as UTF-8 bytes).
rem /FIstdafx.h and include set follow the game project (DirectX12.vcxproj) settings.
rem Boss/Enemy states share file names (Idle/Attack/Dead) so their objs collide; compile them
rem separately into uniquely named objs first, then link everything in one go.
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
set FLAGS=/nologo /c /EHsc /std:c++20 /MD /W3 /utf-8 /FIstdafx.h /D_UNICODE /DUNICODE
set INCLUDES=/I %SRC% /I %SRC%\00_Game /I %SRC%\10_Ggraphic /I %SRC%\10_Ggraphic\10_Device /I %SRC%\99_Utility /I Data\Library /I Data\Library\DirectXTex\DirectXTex /I Data\Library\ImGui

set BOSS=%SRC%\00_Game\10_Object\10_MeshObject\00_Character\20_Boss
set ENEMY=%SRC%\00_Game\10_Object\10_MeshObject\00_Character\10_Enemy

cl %FLAGS% %INCLUDES% /Fotests\obj_boss_idle.obj   %BOSS%\State\00_Idle\Idle.cpp || exit /b 1
cl %FLAGS% %INCLUDES% /Fotests\obj_boss_attack.obj %BOSS%\State\20_Attack\Attack.cpp || exit /b 1
cl %FLAGS% %INCLUDES% /Fotests\obj_boss_dead.obj   %BOSS%\State\30_Dead\Dead.cpp || exit /b 1
cl %FLAGS% %INCLUDES% /Fotests\obj_enemy_idle.obj  %ENEMY%\State\00_Idle\Idle.cpp || exit /b 1
cl %FLAGS% %INCLUDES% /Fotests\obj_enemy_attack.obj %ENEMY%\State\20_Attack\Attack.cpp || exit /b 1
cl %FLAGS% %INCLUDES% /Fotests\obj_enemy_dead.obj  %ENEMY%\State\30_Dead\Dead.cpp || exit /b 1

cl /nologo /EHsc /std:c++20 /MD /W3 /utf-8 /FIstdafx.h /D_UNICODE /DUNICODE ^
	/I %SRC% /I %SRC%\00_Game /I %SRC%\10_Ggraphic /I %SRC%\10_Ggraphic\10_Device /I %SRC%\99_Utility ^
	/I Data\Library /I Data\Library\DirectXTex\DirectXTex /I Data\Library\ImGui ^
	tests\CombatSEEventTest.cpp ^
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
	%SRC%\00_Game\60_Combat\CombatCoordinator.cpp ^
	%SRC%\00_Game\60_Combat\PlayerCombatView.cpp ^
	%SRC%\00_Game\60_Combat\BossCombatView.cpp ^
	%BOSS%\Boss.cpp ^
	%BOSS%\State\BossStateBase.cpp ^
	tests\obj_boss_idle.obj ^
	tests\obj_boss_attack.obj ^
	tests\obj_boss_dead.obj ^
	%BOSS%\State\10_Move\Move.cpp ^
	%BOSS%\State\21_Attack2\Attack2.cpp ^
	%BOSS%\State\22_BeamAttack\BeamAttack.cpp ^
	%BOSS%\State\23_JumpAttack\JumpAttack.cpp ^
	%BOSS%\State\24_SpinAttack\SpinAttack.cpp ^
	%BOSS%\State\40_ParryReaction\ParryReaction.cpp ^
	%ENEMY%\Enemy.cpp ^
	%ENEMY%\State\EnemyStateBase.cpp ^
	tests\obj_enemy_idle.obj ^
	tests\obj_enemy_attack.obj ^
	tests\obj_enemy_dead.obj ^
	%ENEMY%\State\10_Chase\Chase.cpp ^
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
	/Fe:tests\CombatSEEventTest.exe ^
	/link /LIBPATH:"Data\Library\DirectXTex\DirectXTex\Bin\Desktop_2022_Win10\x64\Release" ^
	d3d12.lib dxgi.lib d3dcompiler.lib DirectXTex.lib ole32.lib xaudio2.lib

endlocal & exit /b %ERRORLEVEL%
