@echo off
echo ============================================
echo KH RVExtension Bridge - Build Script
echo ============================================
echo.

REM Try different VS2022 installation paths and editions
set "VS2022_FOUND="

if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
    set "VS2022_FOUND=1"
    echo Found VS2022 Enterprise
    goto :build
)

if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
    set "VS2022_FOUND=1"
    echo Found VS2022 Professional
    goto :build
)

if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    set "VS2022_FOUND=1"
    echo Found VS2022 Community
    goto :build
)

if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    set "VS2022_FOUND=1"
    echo Found VS2022 BuildTools
    goto :build
)

if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    set "VS2022_FOUND=1"
    echo Found VS2022 Community (alt location)
    goto :build
)

echo ERROR: Visual Studio 2022 not found!
pause
exit /b 1

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
) else (
    echo.
    echo BUILD FAILED!
)

echo.
pause