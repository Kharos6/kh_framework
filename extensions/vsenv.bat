@echo off
REM ===========================================================================
REM vsenv.bat - locate a Visual Studio 2022+ C++ toolchain and initialise the
REM x64 build environment (cl, link, rc on PATH). Called by every build.bat in
REM extensions\. No setlocal here on purpose: the environment must propagate
REM to the caller. Returns errorlevel 1 when nothing usable is found.
REM
REM Toolset pinning: MSVC occasionally hits internal compiler errors (C1001) on
REM this code base with particular compiler versions. KH_VCVARS_VER_DEFAULT below
REM is the toolset the maintainer builds with; if that exact toolset is installed
REM it is used (vcvars64 -vcvars_ver=...), otherwise the install's default toolset
REM is used and a note is printed. Override per machine with KH_VCVARS_VER
REM (e.g. set KH_VCVARS_VER=14.44) or disable pinning with KH_VCVARS_VER=none.
REM Extra toolsets are installed via the Visual Studio Installer -> Individual
REM components -> "MSVC v143 - VS 2022 C++ x64/x86 build tools (v14.xx)".
REM
REM Search order:
REM   0. KH_VCVARS     - explicit override: full path to a vcvars64.bat
REM   1. an already initialised x64 "Developer Command Prompt" (VCINSTALLDIR)
REM   2. vswhere.exe   - Microsoft's locator, installed by every VS 2017+ setup
REM                      at a fixed path; finds any edition (Community,
REM                      Professional, Enterprise, BuildTools) of any version
REM                      >= 17 (2022, 2026 ...) on any drive / folder, but only
REM                      if the C++ x64 tools component is installed. Stable
REM                      installs are preferred; a Preview is used only when no
REM                      stable install exists.
REM   3. the classic well-known folders, as a last resort
REM
REM Written with goto / call :sub instead of nested ( ) blocks on purpose:
REM paths like "Program Files (x86)" inside blocks are a classic cmd trap.
REM ===========================================================================

set "KH_VCVARS_FOUND="
set "KH_VSINSTALL="
set "KH_VSVER="
set "KH_VSMAJOR="
REM Known-good MSVC toolset (the maintainer's), e.g. 14.44. Empty = no pinning.
REM Per-machine override: the KH_VCVARS_VER environment variable ("none" disables).
set "KH_VCVARS_VER_DEFAULT="

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
REM Preference: a stable VS 2022 (17.x) first - the known-good compiler for this
REM code base (VS 2026's 14.50 compiler hits C1001 in lua_wrappers_sqf.hpp) -
REM then any stable 17+, then a Preview. vswhere lives in a path with "(x86)"
REM and the version range contains ")", so it is run via pushd + a temp file
REM instead of a FOR /F set (a ")" there breaks cmd's parser).
:vswhere
set "KH_VSWHERE_DIR=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer"
if not exist "%KH_VSWHERE_DIR%\vswhere.exe" set "KH_VSWHERE_DIR=%ProgramFiles%\Microsoft Visual Studio\Installer"
if not exist "%KH_VSWHERE_DIR%\vswhere.exe" goto :wellknown
pushd "%KH_VSWHERE_DIR%" || goto :wellknown
call :vsq -version [17.0,18.0)
if not defined KH_VSINSTALL call :vsq
if not defined KH_VSINSTALL call :vsq -prerelease
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
if %KH_VSMAJOR% GEQ 18 echo NOTE: this is Visual Studio 2026 or newer; its compiler is known to crash ^(C1001^) on this code - see the toolset note below if that happens.
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
set "KH_VER_WANT=%KH_VCVARS_VER%"
if not defined KH_VER_WANT set "KH_VER_WANT=%KH_VCVARS_VER_DEFAULT%"
if /i "%KH_VER_WANT%"=="none" set "KH_VER_WANT="
if not defined KH_VER_WANT goto :init_default
echo Requesting MSVC toolset %KH_VER_WANT% ...
call "%KH_VCVARS_FOUND%" -vcvars_ver=%KH_VER_WANT%
if not errorlevel 1 goto :check
echo.
echo NOTE: MSVC toolset %KH_VER_WANT% is not installed here - falling back to the default toolset.
echo       If the build hits an internal compiler error ^(C1001^), install that toolset via the
echo       Visual Studio Installer ^> Modify ^> Individual components ^> "MSVC v143 - VS 2022 C++
echo       x64/x86 build tools ^(v%KH_VER_WANT%^)" ^(available in the VS 2026 installer too^), then rebuild.
echo.
:init_default
call "%KH_VCVARS_FOUND%"
if not errorlevel 1 goto :check
echo ERROR: "%KH_VCVARS_FOUND%" failed to initialise the environment.
exit /b 1

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
REM Print the exact compiler version: the first thing to compare when a build fails on one machine only.
cl 2>&1 | findstr /c:"Compiler Version"
if defined VSCMD_ARG_TGT_ARCH if /i not "%VSCMD_ARG_TGT_ARCH%"=="x64" echo WARNING: the Visual Studio environment targets %VSCMD_ARG_TGT_ARCH%, not x64.
exit /b 0

REM --- subroutine: remember the first existing vcvars64.bat ----------------------------
:try
if defined KH_VCVARS_FOUND exit /b 0
if exist "%~1" set "KH_VCVARS_FOUND=%~1"
exit /b 0

REM --- subroutine: one vswhere query (extra args in %*), result in KH_VSINSTALL / KH_VSVER ---
:vsq
set "KH_VSINSTALL="
set "KH_VSVER="
vswhere.exe -latest -products * %* -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath > "%TEMP%\kh_vswhere.txt" 2>nul
set /p KH_VSINSTALL=<"%TEMP%\kh_vswhere.txt"
vswhere.exe -latest -products * %* -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationVersion > "%TEMP%\kh_vswhere.txt" 2>nul
set /p KH_VSVER=<"%TEMP%\kh_vswhere.txt"
del "%TEMP%\kh_vswhere.txt" >nul 2>nul
exit /b 0