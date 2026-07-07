@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

echo ════════════════════════════════════════════════════
echo  KnotHub — Build & Package
echo ════════════════════════════════════════════════════
echo.

set "ROOT=%~dp0..\.."
set "STAGING=%~dp0..\staging"
set "BIN=%~dp0..\bin"

rem ── Paths to build artifacts ────────────────────────────
set "CORE_SRC=%ROOT%\KnotHubCore\src\build-KnotHubCore-static-Release\release\KnotHubCore.exe"
set "DASH_SRC=%ROOT%\KnotHubDash\knot-hub-dash\src-tauri\target\release\knot-hub-dash.exe"
rem ── Fallback: try debug build if release not found ──
set "DASH_DBG=%ROOT%\KnotHubDash\knot-hub-dash\src-tauri\target\debug\knot-hub-dash.exe"

rem ── 1. KnotHubCore (static, no Qt DLLs needed) ──────────
echo [1/3] KnotHubCore (static)...
if exist "%CORE_SRC%" (
    echo   Found: %CORE_SRC%
) else (
    echo   ERROR: Static build not found.
    echo   Build it with: Qt Creator → build-KnotHubCore-static-Release
    exit /b 1
)

rem ── 2. KnotHubDash (Tauri) ─────────────────────────────
echo [2/3] KnotHubDash (Tauri)...
if exist "%DASH_SRC%" (
    echo   Found: %DASH_SRC%
) else if exist "%DASH_DBG%" (
    set "DASH_SRC=%DASH_DBG%"
    echo   WARNING: Using DEBUG build.
    echo   For release: cd KnotHubDash\knot-hub-dash ^&^& npx tauri build
) else (
    echo   ERROR: Tauri binary not found.
    echo   Build it with: cd KnotHubDash\knot-hub-dash ^&^& npx tauri build
    exit /b 1
)

rem ── 3. Stage & Package ─────────────────────────────────
echo [3/3] Packaging...

if not exist "%STAGING%" mkdir "%STAGING%"
if not exist "%BIN%"      mkdir "%BIN%"

copy /Y "%CORE_SRC%" "%STAGING%\KnotHubCore.exe"   >nul
copy /Y "%DASH_SRC%" "%STAGING%\knot-hub-dash.exe"  >nul

echo   Staged to %STAGING%

rem ── Run NSIS ────────────────────────────────────────────
where makensis >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo   ERROR: makensis not found in PATH.
    echo   Install NSIS from https://nsis.sourceforge.io/
    echo   Then add it to your PATH, e.g. C:\Program Files (x86)\NSIS
    exit /b 1
)

makensis "%~dp0KnotHub.nsi"
if %ERRORLEVEL% EQU 0 (
    echo.
    echo   ========================================
    echo     Done: %BIN%\KnotHub-*-Setup.exe
    echo   ========================================
) else (
    echo   NSIS build failed.
    exit /b 1
)
