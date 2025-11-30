@echo off
echo Starting...

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
echo Checked locations:
echo   - C:\Program Files (x86)\Microsoft Visual Studio\2022\[Enterprise/Professional/Community/BuildTools]\VC\Auxiliary\Build\vcvars64.bat
echo   - C:\Program Files\Microsoft Visual Studio\2022\[Enterprise/Professional/Community]\VC\Auxiliary\Build\vcvars64.bat
pause
exit /b 1

:build

REM Clean up previous build artifacts
echo Setting up output directory...
if exist output_x64 rd /s /q output_x64
mkdir output_x64

REM Build the DLL
echo Compiling...
cl /LD /arch:AVX /O2 /Ob3 /GL /MT /std:c++20 /EHsc /TP /Gy /Gw /GS- ^
    /Iluajit\include ^
    /Iintercept\include ^
    /Isherpa\include ^
    /Illama\include ^
    /Iultralight\include ^
    /I. ^
    main.cpp ^
    /Fe:output_x64\kh_framework_x64.dll ^
    /Fo:output_x64\ ^
    /Fd:output_x64\kh_framework_x64.pdb ^
    /link /MACHINE:X64 ^
    /LTCG ^
    /OPT:REF /OPT:ICF /OPT:LBR ^
    /DELAYLOAD:lua51.dll ^
    /DELAYLOAD:cublas64_12.dll ^
    /DELAYLOAD:vulkan-1.dll ^
    /DELAYLOAD:sherpa-onnx-c-api.dll ^
    /DELAYLOAD:UltralightCore.dll ^
    /DELAYLOAD:Ultralight.dll ^
    /LIBPATH:luajit\lib ^
    /LIBPATH:intercept\lib ^
    /LIBPATH:sherpa\lib ^
    /LIBPATH:vulkan\lib ^
    /LIBPATH:llama\lib ^
    /LIBPATH:C:\Progra~1\NVIDIA~2\CUDA\v12.9\lib\x64 ^
    /LIBPATH:ultralight\lib ^
    /LIBPATH:minhook\lib ^
    intercept_client.lib lua51.lib ^
    sherpa-onnx-c-api.lib ^
    llama.lib common.lib ggml.lib ggml-base.lib ggml-cpu.lib ggml-cuda.lib ggml-vulkan.lib ^
    cuda.lib cudart_static.lib cublas.lib cublasLt.lib vulkan-1.lib ^
    Ultralight.lib UltralightCore.lib WebCore.lib ^
    libMinHook.x64.lib ^
    winmm.lib gdi32.lib shell32.lib ole32.lib user32.lib advapi32.lib delayimp.lib d3d11.lib dxgi.lib d3dcompiler.lib gdiplus.lib dwrite.lib

REM Check if build was successful
if exist output_x64\kh_framework_x64.dll (
    echo ================================
    echo BUILD SUCCESS!
    echo ================================
    echo.
    echo Output location: output_x64\kh_framework_x64.dll
    echo.
    echo File size:
    for %%I in (output_x64\kh_framework_x64.dll) do echo %%~zI bytes
) else (
    echo ================================
    echo BUILD FAILED!
    echo ================================
)

echo.
pause