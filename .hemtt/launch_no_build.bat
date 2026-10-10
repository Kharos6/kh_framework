@echo off
cd /d "%~dp0.."
set "KH_DEP_NOPAUSE=1"
call dependencies\get_dependencies.bat

if errorlevel 1 (
    echo ERROR: could not obtain the binary dependencies - see above.
    pause
    exit /b 1
)

REM Make sure <Arma 3>\x\kh is a junction to .hemttout\dev. `hemtt launch -Q`
REM refuses to start without that link; .hemtt\link_dev.ps1 finds the game via
REM Steam's library list / the Bohemia registry key (override: KH_ARMA3_DIR) and
REM creates the link if it is missing or points elsewhere. It never deletes a
REM real folder - if x\kh is one, it stops and tells you.
powershell -NoProfile -ExecutionPolicy Bypass -File ".hemtt\link_dev.ps1"

if errorlevel 1 (
    echo ERROR: could not set up the Arma 3 link - see above.
    pause
    exit /b 1
)

hemtt launch -Q

if errorlevel 1 (
    echo HEMTT LAUNCH FAILED - see above.
    pause
    exit /b 1
)

exit /b 0