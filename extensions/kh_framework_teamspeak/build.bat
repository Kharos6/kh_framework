@echo off
setlocal
echo Building KH Framework TeamSpeak Plugin...

REM Work from this script's own folder (extensions\kh_framework_teamspeak),
REM so relative paths work no matter where the script is invoked from.
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

REM Clean up previous build artifacts
echo Setting up output directory...
if exist output rd /s /q output
mkdir output
mkdir output\plugins

REM Build the DLL
echo Compiling TeamSpeak plugin...
cl /LD /arch:AVX /O2 /Ob3 /GL /MT /std:c++20 /EHsc /TP /Gy /Gw /GS- ^
    /I. ^
    kh_framework_teamspeak.cpp ^
    /Fe:output\plugins\kh_framework_teamspeak_win64.dll ^
    /Fo:output\ ^
    /Fd:output\kh_framework_teamspeak_win64.pdb ^
    /link /MACHINE:X64 ^
    /LTCG ^
    /OPT:REF /OPT:ICF /OPT:LBR ^
    /EXPORT:ts3plugin_name ^
    /EXPORT:ts3plugin_version ^
    /EXPORT:ts3plugin_apiVersion ^
    /EXPORT:ts3plugin_author ^
    /EXPORT:ts3plugin_description ^
    /EXPORT:ts3plugin_setFunctionPointers ^
    /EXPORT:ts3plugin_init ^
    /EXPORT:ts3plugin_shutdown ^
    /EXPORT:ts3plugin_offersConfigure ^
    /EXPORT:ts3plugin_onConnectStatusChangeEvent ^
    /EXPORT:ts3plugin_onEditCapturedVoiceDataEvent ^
    /EXPORT:ts3plugin_processCommand ^
    /EXPORT:ts3plugin_commandKeyword ^
    /EXPORT:ts3plugin_requestAutoload ^
    user32.lib kernel32.lib advapi32.lib shell32.lib

REM Check if build was successful
if not exist output\plugins\kh_framework_teamspeak_win64.dll (
    echo ================================
    echo BUILD FAILED!
    echo ================================
    pause
    exit /b 1
)

echo DLL compiled successfully.
echo.

REM Create package.ini for ts3_plugin
echo Creating package.ini...
echo Name = KH Framework Voice Modulation> output\package.ini
echo Type = Plugin>> output\package.ini
echo Author = KH Framework>> output\package.ini
echo Version = 1.0.0>> output\package.ini
echo Platforms = win64>> output\package.ini
echo Description = Voice modulation effects for Arma 3 KH Framework.>> output\package.ini

REM Create the .ts3_plugin package
echo Creating .ts3_plugin package...

REM Delete old file if exists
if exist output\kh_framework_teamspeak.ts3_plugin del output\kh_framework_teamspeak.ts3_plugin

REM Run PowerShell inline to create the package
powershell -NoProfile -ExecutionPolicy Bypass -Command "Add-Type -AssemblyName System.IO.Compression.FileSystem; $zip = [System.IO.Compression.ZipFile]::Open('output\kh_framework_teamspeak.ts3_plugin', 'Create'); [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile($zip, 'output\package.ini', 'package.ini'); [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile($zip, 'output\plugins\kh_framework_teamspeak_win64.dll', 'plugins/kh_framework_teamspeak_win64.dll'); $zip.Dispose(); Write-Host 'Package created successfully'"

echo.
echo PowerShell exit code: %ERRORLEVEL%

if exist output\kh_framework_teamspeak.ts3_plugin goto :package_success
goto :package_failed

:package_success
echo ================================
echo BUILD SUCCESS!
echo ================================
echo.
echo Output files:
echo   - output\kh_framework_teamspeak.ts3_plugin (installable package)
echo   - output\plugins\kh_framework_teamspeak_win64.dll (raw plugin)
echo.
echo File sizes:
for %%I in (output\kh_framework_teamspeak.ts3_plugin) do echo   Package: %%~zI bytes
for %%I in (output\plugins\kh_framework_teamspeak_win64.dll) do echo   DLL: %%~zI bytes
echo.
echo Installation:
echo   1. Double-click kh_framework_teamspeak.ts3_plugin to install
echo   2. Or copy to your mod folder for automatic installation via Arma 3
echo   3. Restart TeamSpeak 3 after installation
echo.
echo Deploying to %KH_DEPLOY% ...
if not exist "%KH_DEPLOY%" mkdir "%KH_DEPLOY%"
REM Brief settle delay before the copies (linker/AV file-handle release).
timeout /t 1 /nobreak >nul
copy /Y "output\kh_framework_teamspeak.ts3_plugin" "%KH_DEPLOY%\kh_framework_teamspeak.ts3_plugin" >nul
if errorlevel 1 (
    echo DEPLOY FAILED for the .ts3_plugin package - is a file lock held on "%KH_DEPLOY%"?
    echo.
    pause
    exit /b 1
)
copy /Y "output\plugins\kh_framework_teamspeak_win64.dll" "%KH_DEPLOY%\kh_framework_teamspeak_win64.dll" >nul
if errorlevel 1 (
    echo DEPLOY FAILED for the DLL - is TeamSpeak running with the plugin loaded?
    echo.
    pause
    exit /b 1
)
echo Deployed:
echo   - %KH_DEPLOY%\kh_framework_teamspeak.ts3_plugin
echo   - %KH_DEPLOY%\kh_framework_teamspeak_win64.dll
REM Success: give the banner a moment to be read, then close (return 0).
if not defined KH_BUILD_ALL timeout /t 2 /nobreak >nul
exit /b 0

:package_failed
echo ================================
echo PACKAGING FAILED!
echo ================================
echo The DLL was built but packaging failed.
echo.

:done
echo.
pause
endlocal
exit /b 1
