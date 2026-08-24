@echo off
setlocal

if "%~2"=="" exit /b 2

set "PROJECT_DIR=%~1"
set "SHADER_DIR=%PROJECT_DIR%Data\Shader"
set "OUTPUT_DIR=%~2Data\Shader"

for %%D in (PMX Mstc PostProcess Rewind Sprite Particle Compute) do (
	if not exist "%OUTPUT_DIR%\%%D" mkdir "%OUTPUT_DIR%\%%D" || exit /b 1
)

call :compile "PMX\Vertex.hlsl" "PMX\Vertex.cso" VS vs_5_0 || exit /b 1
call :compile "PMX\Pixel.hlsl" "PMX\Pixel.cso" PS ps_5_0 || exit /b 1
call :compile "PMX\ShadowVertex.hlsl" "PMX\ShadowVertex.cso" VS vs_5_0 || exit /b 1
call :compile "Mstc\Vertex.hlsl" "Mstc\Vertex.cso" VS vs_5_0 || exit /b 1
call :compile "Mstc\Pixel.hlsl" "Mstc\Pixel.cso" PS ps_5_0 || exit /b 1
call :compile "PostProcess\Vertex.hlsl" "PostProcess\Vertex.cso" VS vs_5_0 || exit /b 1
call :compile "PostProcess\BrightExtract_PS.hlsl" "PostProcess\BrightExtract_PS.cso" PS ps_5_0 || exit /b 1
call :compile "PostProcess\GaussianBlur_PS.hlsl" "PostProcess\GaussianBlur_PS.cso" PS ps_5_0 || exit /b 1
call :compile "PostProcess\Composite_PS.hlsl" "PostProcess\Composite_PS.cso" PS ps_5_0 || exit /b 1
call :compile "Rewind\Rewind.hlsl" "Rewind\Rewind_VS.cso" VS vs_5_0 || exit /b 1
call :compile "Rewind\Rewind.hlsl" "Rewind\Rewind_PS.cso" PS ps_5_0 || exit /b 1
call :compile "Sprite\Sprite2D.hlsl" "Sprite\Sprite2D_VS.cso" VS vs_5_0 || exit /b 1
call :compile "Sprite\Sprite2D.hlsl" "Sprite\Sprite2D_PS.cso" PS ps_5_0 || exit /b 1
call :compile "Sprite\Sprite3D.hlsl" "Sprite\Sprite3D_VS.cso" VS vs_5_0 || exit /b 1
call :compile "Sprite\Sprite3D.hlsl" "Sprite\Sprite3D_PS.cso" PS ps_5_0 || exit /b 1
call :compile "Particle\ParticleVS.hlsl" "Particle\ParticleVS.cso" main vs_5_0 || exit /b 1
call :compile "Particle\ParticlePS.hlsl" "Particle\ParticlePS.cso" main ps_5_0 || exit /b 1
call :compile "Compute\TestCompute.hlsl" "Compute\TestCompute.cso" main cs_5_0 || exit /b 1

exit /b 0

:compile
fxc.exe /nologo /T %4 /E %3 /Fo "%OUTPUT_DIR%\%~2" "%SHADER_DIR%\%~1"
exit /b %errorlevel%
