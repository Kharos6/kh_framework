@echo off
cd /d "%~dp0.."
set "KH_DEP_NOPAUSE=1"
call dependencies\get_dependencies.bat

if errorlevel 1 (
    echo ERROR: could not obtain the binary dependencies - see above.
    pause
    exit /b 1
)

hemtt build

if errorlevel 1 (
    echo HEMTT BUILD FAILED
    pause
    exit /b 1
)

timeout /t 1 /nobreak >nul
echo Copying build addons to dev...
xcopy ".hemttout\build\addons\*" ".hemttout\dev\addons\" /Y /E /I

if errorlevel 1 (
    echo COPY TO DEV FAILED
    pause
    exit /b 1
)

echo Done.
if not defined KH_BUILD_ALL timeout /t 2 /nobreak >nul
exit /b 0