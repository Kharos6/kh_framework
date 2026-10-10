@echo off
REM Downloads / verifies the binary dependencies listed in manifest.json.
REM Double-click it, or let the build scripts call it. Arguments are passed
REM through to get_dependencies.ps1, e.g.:
REM     get_dependencies.bat -Force
REM     get_dependencies.bat -Profile release
setlocal
set "KH_DEP_DIR=%~dp0"
where powershell >nul 2>nul
if errorlevel 1 (
    echo ERROR: powershell.exe not found on PATH. It ships with every Windows 10/11.
    exit /b 1
)
powershell -NoProfile -ExecutionPolicy Bypass -File "%KH_DEP_DIR%get_dependencies.ps1" %*
set "RC=%ERRORLEVEL%"
REM Double-clicked (no arguments, launched by Explorer as "cmd /c ...")? Then keep
REM the window open. Build scripts set KH_DEP_NOPAUSE so they are never held up.
if not "%~1"=="" goto :end
if defined KH_DEP_NOPAUSE goto :end
set "KH_CMDLINE=%CMDCMDLINE:"=%"
echo "%KH_CMDLINE%" | find /i " /c " >nul
if not errorlevel 1 pause
:end
endlocal & exit /b %RC%
