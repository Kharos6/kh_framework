@echo off
echo ============================================
echo KH RVExtension Bridge - Build Script
echo ============================================
echo.

REM Work from this script's own folder (extensions\kh_rv_extension), so
REM relative paths work no matter where the script is invoked from.
cd /d "%~dp0"

REM Project root = two levels up from this script (...\kh_framework\).
REM Derived from the script location, so it works on any drive/user folder.
for %%I in ("%~dp0..\..") do set "KH_ROOT=%%~fI"
set "KH_DEPLOY=%KH_ROOT%\.hemttout\dev"
REM ---------------------------------------------------------------------------
REM Fetch / verify the prebuilt binary dependencies (.lib, .dll, hemtt.exe, the
REM Ultralight resources, ...) listed in dependencies\manifest.json. They are
REM not in git; this downloads whatever is missing and is instant otherwise.
REM ---------------------------------------------------------------------------
set "KH_DEP_NOPAUSE=1"
call "%KH_ROOT%\dependencies\get_dependencies.bat"
if errorlevel 1 (
    echo.
    echo ERROR: could not obtain the binary dependencies - see the messages above.
    echo        Re-run dependencies\get_dependencies.bat once the problem is fixed.
    pause
    exit /b 1
)


REM Locate Visual Studio 2022+ and initialise the x64 toolchain (cl / link / rc).
REM Shared logic lives in extensions\vsenv.bat (vswhere-based: any edition, any
REM drive, VS 2022 / 2026, Build Tools; KH_VCVARS overrides; works inside a
REM Developer Command Prompt too).
call "%~dp0..\vsenv.bat"
if errorlevel 1 (
    pause
    exit /b 1
)

:build

echo.
echo Setting up output directory...
if exist output_x64 rd /s /q output_x64
mkdir output_x64

echo.
echo Compiling 64-bit DLL...
cl /LD /arch:AVX /O2 /Ob3 /GL /MT /std:c++20 /EHsc /TP /Gy /Gw /GS- ^
    main.cpp ^
    /Fe:output_x64\kh_rv_extension_x64.dll ^
    /Fo:output_x64\ ^
    /Fd:output_x64\kh_rv_extension_x64.pdb ^
    /link /MACHINE:X64 ^
    /LTCG ^
    /OPT:REF /OPT:ICF /OPT:LBR ^
    kernel32.lib user32.lib d3d11.lib

if exist output_x64\kh_rv_extension_x64.dll (
    echo.
    echo ================================
    echo BUILD SUCCESS!
    echo ================================
    echo.
    for %%I in (output_x64\kh_rv_extension_x64.dll) do echo Size: %%~zI bytes
    echo.
    echo Deploying to %KH_DEPLOY% ...
    if not exist "%KH_DEPLOY%" mkdir "%KH_DEPLOY%"
    REM Brief settle delay before the copy (linker/AV file-handle release).
    timeout /t 1 /nobreak >nul
    copy /Y "output_x64\kh_rv_extension_x64.dll" "%KH_DEPLOY%\kh_rv_extension_x64.dll" >nul
    if errorlevel 1 (
        echo DEPLOY FAILED to "%KH_DEPLOY%" - is the game running with the extension loaded?
        echo.
        pause
        exit /b 1
    )
    echo Deployed to: %KH_DEPLOY%\kh_rv_extension_x64.dll
    REM Success: give the banner a moment to be read, then close (return 0).
    if not defined KH_BUILD_ALL timeout /t 2 /nobreak >nul
    exit /b 0
) else (
    echo.
    echo BUILD FAILED!
)

echo.
pause
exit /b 1
