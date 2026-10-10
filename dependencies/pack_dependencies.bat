@echo off
REM AUTHOR SIDE: builds the dependency zip into dependencies\upload\ and
REM records its sha256 / size in manifest.json. Arguments pass through, e.g.:
REM     pack_dependencies.bat -Check
REM     pack_dependencies.bat -Url https://github.com/Kharos6/kh_framework/releases/download/Dependency/kh_framework_deps.zip
setlocal
set "KH_DEP_DIR=%~dp0"
where powershell >nul 2>nul
if errorlevel 1 (
    echo ERROR: powershell.exe not found on PATH. It ships with every Windows 10/11.
    exit /b 1
)
powershell -NoProfile -ExecutionPolicy Bypass -File "%KH_DEP_DIR%pack_dependencies.ps1" %*
set "RC=%ERRORLEVEL%"
if not "%~1"=="" goto :end
set "KH_CMDLINE=%CMDCMDLINE:"=%"
echo "%KH_CMDLINE%" | find /i " /c " >nul
if not errorlevel 1 pause
:end
endlocal & exit /b %RC%
