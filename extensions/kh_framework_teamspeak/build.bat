@echo off
setlocal
echo Building KH Framework TeamSpeak Plugin...

REM Try different VS2022 installation paths and editions
set "VS2022_FOUND="

REM Check for Enterprise edition
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
    set "VS2022_FOUND=1"
    echo Found VS2022 Enterprise
    goto :build
)

REM Check for Professional edition
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
    set "VS2022_FOUND=1"
    echo Found VS2022 Professional
    goto :build
)

REM Check for Community edition
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    set "VS2022_FOUND=1"
    echo Found VS2022 Community
    goto :build
)

REM Check for BuildTools edition
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    set "VS2022_FOUND=1"
    echo Found VS2022 BuildTools
    goto :build
)

REM Check alternative installation location (Program Files instead of Program Files (x86))
if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
    set "VS2022_FOUND=1"
    echo Found VS2022 Enterprise (alt location)
    goto :build
)

if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
    set "VS2022_FOUND=1"
    echo Found VS2022 Professional (alt location)
    goto :build
)

if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    set "VS2022_FOUND=1"
    echo Found VS2022 Community (alt location)
    goto :build
)

echo ERROR: Visual Studio 2022 not found!
echo Please install Visual Studio 2022 with C++ tools or modify the paths in this script.
pause
exit /b 1

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
    /EXPORT:ts3plugin_onTalkStatusChangeEvent ^
    /EXPORT:ts3plugin_processCommand ^
    /EXPORT:ts3plugin_commandKeyword ^
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
goto :done

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