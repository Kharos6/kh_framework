@echo off
echo Building 32-bit kh_framework DLL (Cleaned Version)...

REM Setup Visual Studio 2019 x86 Native Tools environment
echo Setting up Visual Studio 2019 x86 environment...

REM Try different VS2019 installation paths and editions
set "VS2019_FOUND="

REM Check for Enterprise edition
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvars32.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvars32.bat"
    set "VS2019_FOUND=1"
    echo Found VS2019 Enterprise
    goto :build
)

REM Check for Professional edition
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars32.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars32.bat"
    set "VS2019_FOUND=1"
    echo Found VS2019 Professional
    goto :build
)

REM Check for Community edition
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars32.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars32.bat"
    set "VS2019_FOUND=1"
    echo Found VS2019 Community
    goto :build
)

REM Check for BuildTools edition
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars32.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars32.bat"
    set "VS2019_FOUND=1"
    echo Found VS2019 BuildTools
    goto :build
)

REM Check alternative installation location (Program Files instead of Program Files (x86))
if exist "C:\Program Files\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvars32.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvars32.bat"
    set "VS2019_FOUND=1"
    echo Found VS2019 Enterprise (alt location)
    goto :build
)

if exist "C:\Program Files\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars32.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars32.bat"
    set "VS2019_FOUND=1"
    echo Found VS2019 Professional (alt location)
    goto :build
)

if exist "C:\Program Files\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars32.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars32.bat"
    set "VS2019_FOUND=1"
    echo Found VS2019 Community (alt location)
    goto :build
)

REM If we get here, VS2019 wasn't found
echo ERROR: Visual Studio 2019 not found!
echo Please install Visual Studio 2019 with C++ tools or modify the paths in this script.
echo Checked locations:
echo   - C:\Program Files (x86)\Microsoft Visual Studio\2019\[Enterprise/Professional/Community/BuildTools]\VC\Auxiliary\Build\vcvars32.bat
echo   - C:\Program Files\Microsoft Visual Studio\2019\[Enterprise/Professional/Community]\VC\Auxiliary\Build\vcvars32.bat
pause
exit /b 1

:build
echo Visual Studio environment configured successfully.
echo.

REM Create output directory if it doesn't exist
echo Creating output directory...
if not exist "output_x86" mkdir output_x86

REM Clean up previous build artifacts
echo Cleaning previous build files...
if exist output_x86\kh_framework.dll del output_x86\kh_framework.dll
if exist output_x86\*.obj del output_x86\*.obj
if exist output_x86\*.lib del output_x86\*.lib
if exist output_x86\*.exp del output_x86\*.exp
if exist output_x86\*.pdb del output_x86\*.pdb

REM Build the DLL with all output going to the output directory
echo Compiling kh_framework.dll (Cleaned Version) for 32-bit...
cl /LD /O2 /Ox /Ot /GL /MT /TC kh_framework.c ^
   /Fe:output_x86\kh_framework.dll ^
   /Fo:output_x86\ ^
   /Fd:output_x86\kh_framework.pdb ^
   /link /MACHINE:X86 /OPT:REF /OPT:ICF /LTCG ^
   /IMPLIB:output_x86\kh_framework.lib ^
   shell32.lib ole32.lib user32.lib advapi32.lib

REM Check if build was successful
if exist output_x86\kh_framework.dll (
    echo.
    echo ================================
    echo BUILD SUCCESS!
    echo ================================
    dir output_x86\kh_framework.dll | find "kh_framework.dll"
    echo.
    echo Cleaned KH Framework DLL built successfully for 32-bit!
    echo Output location: output_x86\kh_framework.dll
) else (
    echo.
    echo ================================
    echo BUILD FAILED!
    echo ================================
    echo Check the compiler output above for errors.
)

echo.
pause