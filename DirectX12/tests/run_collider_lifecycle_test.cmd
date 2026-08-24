@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++20 /W4 /DNOMINMAX /FI"%~dp0TestPreinclude.h" /I SourceCode tests\ColliderLifecycleTest.cpp SourceCode\00_Game\40_Collision\CollisionDetector.cpp SourceCode\00_Game\40_Collision\00_Core\ColliderBase.cpp SourceCode\00_Game\40_Collision\00_Core\CollisionMath.cpp SourceCode\00_Game\40_Collision\00_Capsule\CapsuleCollider.cpp SourceCode\00_Game\40_Collision\00_Box\BoxCollider.cpp SourceCode\00_Game\40_Collision\10_Sphere\SphereCollider.cpp /Fe:tests\ColliderLifecycleTest.exe
if errorlevel 1 exit /b 1
tests\ColliderLifecycleTest.exe
