@echo off
setlocal enabledelayedexpansion
title Build Terraria 2.5D Windows Executable

echo =======================================================
echo   Terraria 2.5D Sandbox - Unity Windows Standalone Builder
echo =======================================================
echo.

set PROJECT_PATH=%~dp0
set OUTPUT_PATH=%PROJECT_PATH%Build\Windows\Terraria25D_Sandbox.exe
set UNITY_EXE=

:: Check common Unity Hub / Unity Editor locations
for /d %%D in ("C:\Program Files\Unity\Hub\Editor\*") do (
    if exist "%%D\Editor\Unity.exe" (
        set UNITY_EXE=%%D\Editor\Unity.exe
        goto :FOUND_UNITY
    )
)

for /d %%D in ("C:\Program Files\Unity\*") do (
    if exist "%%D\Editor\Unity.exe" (
        set UNITY_EXE=%%D\Editor\Unity.exe
        goto :FOUND_UNITY
    )
)

if exist "C:\Program Files\Unity\Editor\Unity.exe" (
    set UNITY_EXE=C:\Program Files\Unity\Editor\Unity.exe
    goto :FOUND_UNITY
)

:SEARCH_PATH
where Unity.exe >nul 2>&1
if %ERRORLEVEL% equ 0 (
    for /f "delims=" %%I in ('where Unity.exe') do (
        set UNITY_EXE=%%I
        goto :FOUND_UNITY
    )
)

echo [!] Notice: Unity Editor (Unity.exe) was not detected in standard system paths.
echo.
echo To build directly from Unity:
echo 1. Open Unity Hub
echo 2. Add and open the project at: %PROJECT_PATH%
echo 3. In the Unity top menu, click: "Sandbox 2.5D" -> "Build Windows Executable (.exe)"
echo.
echo Alternatively, you can run the pre-compiled standalone game executable:
echo   %PROJECT_PATH%Terraria25D_Standalone.exe
echo.
pause
exit /b 1

:FOUND_UNITY
echo [+] Found Unity Editor: "%UNITY_EXE%"
echo [+] Building Standalone Player to: "%OUTPUT_PATH%"
echo.
"%UNITY_EXE%" -quit -batchmode -projectPath "%PROJECT_PATH%" -executeMethod Editor.BuildScript.BuildWindowsPlayer -logFile "%PROJECT_PATH%build.log"

if %ERRORLEVEL% equ 0 (
    echo.
    echo [OK] Build completed successfully!
    echo Executable located at: "%OUTPUT_PATH%"
    start "" "%PROJECT_PATH%Build\Windows"
) else (
    echo.
    echo [ERR] Build encountered an error. Check build.log for details.
)

pause
