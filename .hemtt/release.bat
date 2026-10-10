@echo off
cd /d "%~dp0.."
set "KH_DEP_NOPAUSE=1"
call dependencies\get_dependencies.bat

if errorlevel 1 (
    echo ERROR: could not obtain the binary dependencies - see above.
    pause
    exit /b 1
)

hemtt release