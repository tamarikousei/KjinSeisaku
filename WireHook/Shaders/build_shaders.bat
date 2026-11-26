@echo off
rem Compile HLSL pixel shaders into Assets\Shaders\*.pso using fxc.exe from the Windows SDK.
rem If fxc.exe is not found, the precompiled .pso files are used as they are.
setlocal
set FXC=
for /f "delims=" %%F in ('dir /b /s "%ProgramFiles(x86)%\Windows Kits\10\bin\*fxc.exe" 2^>nul ^| findstr /i "\\x64\\"') do set FXC=%%F
if "%FXC%"=="" (
  echo fxc.exe not found. Using precompiled shaders.
  exit /b 0
)
set SRC=%~dp0
set OUT=%~dp0..\Assets\Shaders
if not exist "%OUT%" mkdir "%OUT%"
for %%S in (down up final) do (
  "%FXC%" /nologo /T ps_4_0 /E main /O3 /Fo "%OUT%\%%S.pso" "%SRC%%%S.hlsl" >nul || exit /b 1
)
echo Shaders compiled.
exit /b 0
