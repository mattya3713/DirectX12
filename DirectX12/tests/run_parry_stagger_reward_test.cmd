@echo off
chcp 65001 >nul
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++20 /W4 /DNOMINMAX /I Data\Library /I SourceCode tests\ParryStaggerRewardTest.cpp SourceCode\00_Game\60_Combat\CombatTuning.cpp /Fe:tests\ParryStaggerRewardTest.exe
if errorlevel 1 exit /b 1
tests\ParryStaggerRewardTest.exe
