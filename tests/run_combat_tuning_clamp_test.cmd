@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++20 /I DirectX12\SourceCode /I DirectX12\Data\Library tests\CombatTuningClampTest.cpp DirectX12\SourceCode\00_Game\60_Combat\CombatTuning.cpp /Fe:tests\CombatTuningClampTest.exe
if errorlevel 1 exit /b 1
cd tests
CombatTuningClampTest.exe
cd ..
