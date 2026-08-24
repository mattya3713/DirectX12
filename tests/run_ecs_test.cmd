@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++20 /I DirectX12\SourceCode tests\ECSTest.cpp /Fe:tests\ECSTest.exe
if errorlevel 1 exit /b 1
cd tests
ECSTest.exe