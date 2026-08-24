@echo off
chcp 65001 >nul
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++20 /W4 /DNOMINMAX tests\ComputeQueueSyncTest.cpp /Fe:tests\ComputeQueueSyncTest.exe
if errorlevel 1 exit /b 1
tests\ComputeQueueSyncTest.exe
