@echo off
echo Starting...

REM Work from this script's own folder (extensions\kh_framework), so the
REM relative compile paths work no matter where the script is invoked from.
cd /d "%~dp0"

REM Project root = two levels up from this script (...\kh_framework\).
REM Derived from the script location, so it works on any drive/user folder.
for %%I in ("%~dp0..\..") do set "KH_ROOT=%%~fI"
set "KH_DEPLOY=%KH_ROOT%\.hemttout\dev\intercept"

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

REM Compile the shader resources (26884: the HLSL ships inside the DLL as
REM RCDATA, see kh_shaders.rc beside rendering_integration.hpp). rc.exe is
REM on PATH after vcvars64; the .res is linked in below. Fail here rather
REM than ship a DLL whose every shader compile reports hlslResMissing.
echo Compiling shader resources...
rc /nologo /fo output_x64\kh_shaders.res kh_shaders.rc
if not exist output_x64\kh_shaders.res (
    echo ================================
    echo RESOURCE COMPILE FAILED - kh_shaders.rc / hlsl\*.hlsl
    echo ================================
    pause
    exit /b 1
)

REM Build the DLL
echo Compiling...
cl /LD /arch:AVX /O2 /Ob3 /GL /MT /std:c++20 /EHsc /TP /Gy /Gw /GS- ^
    /Isol ^
    /Iluajit\include ^
    /Iintercept\include ^
    /Isherpa\include ^
    /Illama\include ^
    /Iultralight\include ^
    /Ilz4\include ^
    /I. ^
    main.cpp ^
    lz4\include\lz4.c ^
    /Fe:output_x64\kh_framework_x64.dll ^
    /Fo:output_x64\ ^
    /Fd:output_x64\kh_framework_x64.pdb ^
    /link /MACHINE:X64 ^
    output_x64\kh_shaders.res ^
    /LTCG ^
    /OPT:REF /OPT:ICF /OPT:LBR ^
    /DELAYLOAD:vcomp140.dll ^
    /DELAYLOAD:lua51.dll ^
    /DELAYLOAD:nvcuda.dll ^
    /DELAYLOAD:cublas64_12.dll ^
    /DELAYLOAD:vulkan-1.dll ^
    /DELAYLOAD:sherpa-onnx-c-api.dll ^
    /DELAYLOAD:UltralightCore.dll ^
    /DELAYLOAD:WebCore.dll ^
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
    winmm.lib gdi32.lib shell32.lib ole32.lib user32.lib advapi32.lib delayimp.lib d3d11.lib dxgi.lib d3dcompiler.lib gdiplus.lib dwrite.lib ws2_32.lib winhttp.lib

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
    echo.
    echo Deploying to intercept folder...
    if not exist "%KH_DEPLOY%" mkdir "%KH_DEPLOY%"
    REM Brief settle delay before the copy (linker/AV file-handle release).
    timeout /t 1 /nobreak >nul
    copy /Y "output_x64\kh_framework_x64.dll" "%KH_DEPLOY%\kh_framework_x64.dll" >nul
    if errorlevel 1 (
        echo DEPLOY FAILED to "%KH_DEPLOY%" - is the game running with the DLL loaded?
        echo.
        pause
        exit
    )
    echo Deployed to: %KH_DEPLOY%\kh_framework_x64.dll
    REM Fully successful build + deploy: close the window automatically.
    exit
) else (
    echo ================================
    echo BUILD FAILED!
    echo ================================
)

echo.
pause