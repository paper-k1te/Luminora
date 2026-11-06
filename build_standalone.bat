@echo off
setlocal enabledelayedexpansion
title Build Luminora Standalone (流明洛原) Executable (Windows GUI)

echo =======================================================
echo   Luminora Standalone (流明洛原) - MinGW GCC Windows Builder
echo =======================================================
echo.

set PROJECT_DIR=%~dp0
cd /d "%PROJECT_DIR%"

where gcc >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [ERROR] MinGW GCC was not found in your PATH!
    echo Please make sure GCC is installed and accessible via system PATH.
    echo.
    pause
    exit /b 1
)

echo [+] GCC compiler detected:
gcc --version | findstr /i "gcc"

echo.
echo [+] Compiling Luminora_Standalone.exe ...
echo     Subsystem: Windows GUI (-mwindows)
echo     Libraries: opengl32, gdi32, winmm
echo.

gcc -O2 -std=c99 -Wall -Wextra -Wno-unused-parameter -Wno-unused-function ^
    standalone_src/main.c ^
    -o Luminora_Standalone.exe ^
    -lopengl32 -lgdi32 -lwinmm -mwindows

if %ERRORLEVEL% neq 0 (
    echo.
    echo [ERROR] Compilation failed with error code %ERRORLEVEL%!
    echo.
    pause
    exit /b %ERRORLEVEL%
)

echo.
echo =======================================================
echo [SUCCESS] Luminora_Standalone.exe built successfully!
echo =======================================================
echo.
echo [Key System Highlights]
echo  * Subsystem: Windows GUI (-mwindows) [Zero CMD console popup on launch!]
echo  * TrueType Font Renderer (font_render.h) [Win32 GDI DrawTextW with dynamic texture cache]
echo  * Procedural Audio Engine (sound_engine.h) [11 retro 2.5D sound effects via winmm waveOut]
echo.
