@echo off
echo Building 64-bit kh_framework DLL with LuaJIT (Simplified)...

REM Setup Visual Studio 2022 x64 Native Tools environment
echo Setting up Visual Studio 2022 x64 environment...

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

REM If we get here, VS2019 wasn't found
echo ERROR: Visual Studio 2019 not found!
echo Please install Visual Studio 2019 with C++ tools or modify the paths in this script.
echo Checked locations:
echo   - C:\Program Files (x86)\Microsoft Visual Studio\2019\[Enterprise/Professional/Community/BuildTools]\VC\Auxiliary\Build\vcvars64.bat
echo   - C:\Program Files\Microsoft Visual Studio\2019\[Enterprise/Professional/Community]\VC\Auxiliary\Build\vcvars64.bat
pause
exit /b 1

:build
echo Visual Studio environment configured successfully.
echo.

REM Create output directory if it doesn't exist
echo Creating output directory...
if not exist "output_x64" mkdir output_x64

REM Clean up previous build artifacts
echo Cleaning previous build files...
if exist output_x64\kh_framework_x64.dll del output_x64\kh_framework_x64.dll
if exist output_x64\*.obj del output_x64\*.obj
if exist output_x64\*.lib del output_x64\*.lib
if exist output_x64\*.exp del output_x64\*.exp
if exist output_x64\*.pdb del output_x64\*.pdb

REM Build the DLL with LuaJIT linked
echo Compiling kh_framework_x64.dll
cl /LD /O2 /Ox /Ot /GL /MT /TC ^
   kh_framework.c ^
   /Fe:output_x64\kh_framework_x64.dll ^
   /Fo:output_x64\ ^
   /Fd:output_x64\kh_framework_x64.pdb ^
   /link /MACHINE:X64 /OPT:REF /OPT:ICF /LTCG ^
   /IMPLIB:output_x64\kh_framework_x64.lib ^
   shell32.lib ole32.lib user32.lib advapi32.lib

REM Check if build was successful
if exist output_x64\kh_framework_x64.dll (
    echo.
    echo ================================
    echo BUILD SUCCESS!
    echo ================================
    dir output_x64\kh_framework_x64.dll | find "kh_framework_x64.dll"
    echo.
    echo File size:
    for %%I in (output_x64\kh_framework_x64.dll) do echo %%~zI bytes
) else (
    echo.
    echo ================================
    echo BUILD FAILED!
    echo ================================
    echo Check the compiler output above for errors.
)

echo.
pause