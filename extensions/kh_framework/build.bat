@echo off
echo Starting...

REM Work from this script's own folder (extensions\kh_framework), so the
REM relative compile paths work no matter where the script is invoked from.
cd /d "%~dp0"

REM Project root = two levels up from this script (...\kh_framework\).
REM Derived from the script location, so it works on any drive/user folder.
for %%I in ("%~dp0..\..") do set "KH_ROOT=%%~fI"
set "KH_DEPLOY=%KH_ROOT%\.hemttout\dev\intercept"
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

REM Build the DLL. Library search order: the folders delivered by
REM dependencies\get_dependencies.bat first (cuda\lib carries the four CUDA
REM import libs, so a CUDA Toolkit install is NOT needed to link); the CUDA
REM Toolkit path stays as a fallback for machines that have it.
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
    /LIBPATH:cuda\lib ^
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
        exit /b 1
    )
    echo Deployed to: %KH_DEPLOY%\kh_framework_x64.dll
    REM Success: give the banner a moment to be read, then close (return 0).
    if not defined KH_BUILD_ALL timeout /t 2 /nobreak >nul
    exit /b 0
) else (
    echo ================================
    echo BUILD FAILED!
    echo ================================
)

echo.
pause
exit /b 1
