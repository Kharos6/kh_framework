@echo off
REM ===========================================================================
REM vsenv.bat - locate a Visual Studio 2022+ C++ toolchain and initialise the
REM x64 build environment (cl, link, rc on PATH). Called by every build.bat in
REM extensions\. No setlocal here on purpose: the environment must propagate
REM to the caller. Returns errorlevel 1 when nothing usable is found.
REM
REM Search order:
REM   0. KH_VCVARS     - explicit override: full path to a vcvars64.bat
REM   1. an already initialised x64 "Developer Command Prompt" (VCINSTALLDIR)
REM   2. vswhere.exe   - Microsoft's locator, installed by every VS 2017+ setup
REM                      at a fixed path; finds any edition (Community,
REM                      Professional, Enterprise, BuildTools, Preview) of any
REM                      version >= 17 (2022, 2026 ...) on any drive / folder,
REM                      but only if the C++ x64 tools component is installed
REM   3. the classic well-known folders, as a last resort
REM
REM Written with goto / call :sub instead of nested ( ) blocks on purpose:
REM paths like "Program Files (x86)" inside blocks are a classic cmd trap.
REM ===========================================================================

set "KH_VCVARS_FOUND="
set "KH_VSINSTALL="
set "KH_VSVER="
set "KH_VSMAJOR="

REM --- 0. explicit override ---------------------------------------------------
if not defined KH_VCVARS goto :devprompt
if exist "%KH_VCVARS%" goto :override_ok
echo WARNING: KH_VCVARS is set but "%KH_VCVARS%" does not exist - ignoring it.
goto :devprompt
:override_ok
echo Using KH_VCVARS: %KH_VCVARS%
set "KH_VCVARS_FOUND=%KH_VCVARS%"
goto :init

REM --- 1. already inside an x64 developer prompt ------------------------------
:devprompt
if not defined VCINSTALLDIR goto :vswhere
if /i not "%VSCMD_ARG_TGT_ARCH%"=="x64" goto :vswhere
echo Using the already initialised Visual Studio x64 environment: %VSINSTALLDIR%
goto :check

REM --- 2. vswhere -----------------------------------------------------------------
REM vswhere lives in a path with "(x86)" in it. A ")" inside a FOR /F set breaks
REM cmd's parser even when quoted, so we pushd into its folder and run it by
REM bare name - no quotes, no parentheses in the FOR set.
:vswhere
set "KH_VSWHERE_DIR=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer"
if not exist "%KH_VSWHERE_DIR%\vswhere.exe" set "KH_VSWHERE_DIR=%ProgramFiles%\Microsoft Visual Studio\Installer"
if not exist "%KH_VSWHERE_DIR%\vswhere.exe" goto :wellknown
pushd "%KH_VSWHERE_DIR%" || goto :wellknown
for /f "usebackq delims=" %%I in (`vswhere.exe -latest -prerelease -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "KH_VSINSTALL=%%I"
for /f "usebackq delims=" %%I in (`vswhere.exe -latest -prerelease -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationVersion`) do set "KH_VSVER=%%I"
popd
if not defined KH_VSINSTALL goto :wellknown
for /f "tokens=1 delims=." %%M in ("%KH_VSVER%") do set "KH_VSMAJOR=%%M"
if not defined KH_VSMAJOR set "KH_VSMAJOR=0"
if %KH_VSMAJOR% GEQ 17 goto :vswhere_ok
echo vswhere found Visual Studio %KH_VSVER% at "%KH_VSINSTALL%" but 2022 ^(17.x^) or newer is required.
goto :wellknown
:vswhere_ok
if not exist "%KH_VSINSTALL%\VC\Auxiliary\Build\vcvars64.bat" goto :wellknown
echo Found Visual Studio %KH_VSVER% via vswhere: %KH_VSINSTALL%
set "KH_VCVARS_FOUND=%KH_VSINSTALL%\VC\Auxiliary\Build\vcvars64.bat"
goto :init

REM --- 3. well-known folders ------------------------------------------------------
:wellknown
for %%V in (2022 18 2026) do for %%E in (Enterprise Professional Community BuildTools Preview) do call :try "%ProgramFiles%\Microsoft Visual Studio\%%V\%%E\VC\Auxiliary\Build\vcvars64.bat"
for %%V in (2022 18 2026) do for %%E in (Enterprise Professional Community BuildTools Preview) do call :try "%ProgramFiles(x86)%\Microsoft Visual Studio\%%V\%%E\VC\Auxiliary\Build\vcvars64.bat"
if not defined KH_VCVARS_FOUND goto :notfound
echo Found Visual Studio at: %KH_VCVARS_FOUND%
goto :init

:notfound
echo.
echo ERROR: no Visual Studio 2022 or newer C++ toolchain was found.
echo.
echo   Install Visual Studio 2022 (Community is free) or the "Build Tools for
echo   Visual Studio" and tick the workload "Desktop development with C++"
echo   (that includes the x64 compiler, the Windows SDK and rc.exe).
echo.
echo   If it IS installed somewhere unusual, either run this script from an
echo   "x64 Native Tools Command Prompt for VS", or set the environment variable
echo   KH_VCVARS to the full path of its VC\Auxiliary\Build\vcvars64.bat.
echo.
echo   Looked via: %%ProgramFiles(x86)%%\Microsoft Visual Studio\Installer\vswhere.exe
echo   and in:     %%ProgramFiles%%\Microsoft Visual Studio\^<2022^|18^>\^<edition^>\VC\Auxiliary\Build\vcvars64.bat
exit /b 1

REM --- initialise -------------------------------------------------------------------
:init
call "%KH_VCVARS_FOUND%"
if errorlevel 1 (
    echo ERROR: "%KH_VCVARS_FOUND%" failed to initialise the environment.
    exit /b 1
)

:check
where cl >nul 2>nul
if errorlevel 1 (
    echo ERROR: cl.exe is not on PATH after initialising Visual Studio.
    echo        The "Desktop development with C++" workload ^(MSVC v143+ x64/x86 build tools^) is probably missing.
    exit /b 1
)
where rc >nul 2>nul
if errorlevel 1 (
    echo ERROR: rc.exe ^(the resource compiler^) is not on PATH.
    echo        A Windows 10/11 SDK is missing - add it in the Visual Studio Installer.
    exit /b 1
)
if defined VSCMD_ARG_TGT_ARCH if /i not "%VSCMD_ARG_TGT_ARCH%"=="x64" echo WARNING: the Visual Studio environment targets %VSCMD_ARG_TGT_ARCH%, not x64.
exit /b 0

REM --- subroutine: remember the first existing vcvars64.bat ----------------------------
:try
if defined KH_VCVARS_FOUND exit /b 0
if exist "%~1" set "KH_VCVARS_FOUND=%~1"
exit /b 0