@echo off
REM ===========================================================================
REM build_all.bat - builds everything, in order:
REM   1. extensions\kh_rv_extension        -> .hemttout\dev\kh_rv_extension_x64.dll
REM   2. extensions\kh_framework_teamspeak -> .hemttout\dev\kh_framework_teamspeak*.dll / .ts3_plugin
REM   3. extensions\kh_framework           -> .hemttout\dev\intercept\kh_framework_x64.dll
REM   4. the HEMTT mod (.hemtt\build.bat)   -> .hemttout\dev\addons\*.pbo
REM
REM The first step also fetches the binary dependencies (dependencies\manifest.json)
REM if they are missing. Each build.bat runs in its own cmd process so the Visual
REM Studio environment is set up fresh for each one; the first failure stops the run.
REM ===========================================================================
setlocal
cd /d "%~dp0"
set "KH_FAILED="
set "KH_START=%TIME%"
set "KH_BUILD_ALL=1"

echo.
echo ================= KH Framework: build everything =================

call :step "RV extension bridge"   "extensions\kh_rv_extension\build.bat"
call :step "TeamSpeak plugin"      "extensions\kh_framework_teamspeak\build.bat"
call :step "Framework extension"   "extensions\kh_framework\build.bat"
call :step "HEMTT mod (PBOs)"      ".hemtt\build.bat"

echo.
if defined KH_FAILED goto :failed
echo ================= ALL BUILDS SUCCEEDED (started %KH_START%, finished %TIME%) =================
echo Output: .hemttout\dev
echo.
REM Nothing failed: close on its own after a few seconds (failures pause below).
timeout /t 3 /nobreak >nul
exit /b 0

:failed
echo ================= BUILD STOPPED: %KH_FAILED% failed - see above =================
echo.
pause
exit /b 1

REM --- run one build script in a child cmd and record a failure ----------------
:step
if defined KH_FAILED exit /b 0
echo.
echo ----------------- [%~1]  %~2 -----------------
cmd /c call "%~dp0%~2"
if errorlevel 1 set "KH_FAILED=%~1"
exit /b 0
