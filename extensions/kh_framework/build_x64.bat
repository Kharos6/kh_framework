@echo off
echo Building 64-bit kh_framework DLL...

REM Setup Visual Studio 2019 x64 Native Tools environment
echo Setting up Visual Studio 2019 x64 environment...

REM Try different VS2019 installation paths and editions
set "VS2019_FOUND="

REM Check for Enterprise edition
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
    set "VS2019_FOUND=1"
    echo Found VS2019 Enterprise
    goto :build
)

REM Check for Professional edition
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat"
    set "VS2019_FOUND=1"
    echo Found VS2019 Professional
    goto :build
)

REM Check for Community edition
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
    set "VS2019_FOUND=1"
    echo Found VS2019 Community
    goto :build
)

REM Check for BuildTools edition
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    set "VS2019_FOUND=1"
    echo Found VS2019 BuildTools
    goto :build
)

REM Check alternative installation location (Program Files instead of Program Files (x86))
if exist "C:\Program Files\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
    set "VS2019_FOUND=1"
    echo Found VS2019 Enterprise (alt location)
    goto :build
)

if exist "C:\Program Files\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat"
    set "VS2019_FOUND=1"
    echo Found VS2019 Professional (alt location)
    goto :build
)

if exist "C:\Program Files\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
    set "VS2019_FOUND=1"
    echo Found VS2019 Community (alt location)
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

REM Clean up previous build artifacts
echo Cleaning previous build files...
if exist kh_framework_x64.dll del kh_framework_x64.dll
if exist *.obj del *.obj
if exist *.lib del *.lib
if exist *.exp del *.exp
if exist *.pdb del *.pdb

REM Build the DLL
echo Compiling kh_framework_x64.dll...
cl /LD /O2 /Ox /Ot /GL /MT /TC kh_framework.cpp /Fe:kh_framework_x64.dll /link /MACHINE:X64 /OPT:REF /OPT:ICF /LTCG shell32.lib ole32.lib user32.lib advapi32.lib

REM Check if build was successful
if exist kh_framework_x64.dll (
    echo.
    echo ================================
    echo BUILD SUCCESS!
    echo ================================
    dir kh_framework_x64.dll | find "kh_framework_x64.dll"
    echo.
    echo DLL built successfully and ready for use in Arma 3.
) else (
    echo.
    echo ================================
    echo BUILD FAILED!
    echo ================================
    echo Check the compiler output above for errors.
    echo Make sure all .hpp files are in the same directory.
)

echo.
pause