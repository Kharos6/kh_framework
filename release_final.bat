@echo off
REM ===========================================================================
REM release_final.bat - full release build:
REM   1. dependencies\get_dependencies.bat  (binary dependencies present + verified)
REM   2. the three extensions, freshly built (rv extension, TeamSpeak plugin,
REM      framework extension), each in its own cmd process like build_all.bat
REM   3. .hemtt\release.bat                 (`hemtt release`; HEMTT wipes
REM                                          .hemttout\release and fills it with PBOs)
REM   4. copies everything from .hemttout\dev into .hemttout\release, EXCEPT the
REM      "addons" and "keys" folders and "mod.cpp" (HEMTT already produced those)
REM   5. copies <Documents>\Arma 3\kh_framework\cache into .hemttout\release\cache
REM
REM Note: HEMTT zips the release into releases\ at the end of step 3, i.e. BEFORE
REM steps 4-5, so that archive holds the PBOs only; .hemttout\release is the
REM complete folder. The first failure stops the run.
REM ===========================================================================
setlocal
cd /d "%~dp0"
set "KH_DEV=%~dp0.hemttout\dev"
set "KH_REL=%~dp0.hemttout\release"
set "KH_BUILD_ALL=1"
set "KH_DEP_NOPAUSE=1"
set "KH_FAILED="

echo.
echo ================= KH Framework: release =================

REM --- 1. dependencies ----------------------------------------------------------------
echo.
echo ----------------- [dependencies] -----------------
call "%~dp0dependencies\get_dependencies.bat"
if errorlevel 1 goto :fail_deps

REM --- 2. extensions, fresh ------------------------------------------------------------
call :step "RV extension bridge"   "extensions\kh_rv_extension\build.bat"
call :step "TeamSpeak plugin"      "extensions\kh_framework_teamspeak\build.bat"
call :step "Framework extension"   "extensions\kh_framework\build.bat"
if defined KH_FAILED goto :fail_step

REM --- 3. HEMTT release ------------------------------------------------------------------
call :step "HEMTT release"         ".hemtt\release.bat"
if defined KH_FAILED goto :fail_step
if not exist "%KH_REL%\" goto :fail_norel

REM --- 4. dev -> release, minus addons\, keys\, mod.cpp ---------------------------------
echo.
echo ----------------- [dev -^> release] -----------------
if not exist "%KH_DEV%\" goto :fail_nodev
robocopy "%KH_DEV%" "%KH_REL%" /E /XD addons keys /XF mod.cpp /NJH /NJS /NDL /NP
REM robocopy: 0-7 = success (bits: 1 copied, 2 extras, 4 mismatches), 8+ = failures
if errorlevel 8 goto :fail_copy

REM --- 5. Documents\Arma 3\kh_framework\cache -> release\cache ------------------------
echo.
echo ----------------- [cache -^> release\cache] -----------------
REM Resolve the real Documents folder (it may be redirected, e.g. into OneDrive).
set "KH_DOCS="
powershell -NoProfile -Command "[Environment]::GetFolderPath('MyDocuments')" > "%TEMP%\kh_docs_path.txt"
set /p KH_DOCS=<"%TEMP%\kh_docs_path.txt"
del "%TEMP%\kh_docs_path.txt" >nul 2>nul
if not defined KH_DOCS set "KH_DOCS=%USERPROFILE%\Documents"
set "KH_CACHE=%KH_DOCS%\Arma 3\kh_framework\cache"
if not exist "%KH_CACHE%\" goto :fail_nocache
robocopy "%KH_CACHE%" "%KH_REL%\cache" /E /NJH /NJS /NDL /NP
if errorlevel 8 goto :fail_copy

echo.
echo ================= RELEASE READY: %KH_REL% =================
echo.
timeout /t 3 /nobreak >nul
exit /b 0

:fail_deps
echo.
echo ================= RELEASE STOPPED: could not obtain the binary dependencies - see above =================
pause
exit /b 1
:fail_step
echo.
echo ================= RELEASE STOPPED: %KH_FAILED% failed - see above =================
pause
exit /b 1
:fail_norel
echo.
echo ================= RELEASE STOPPED: "%KH_REL%" was not created by hemtt release =================
pause
exit /b 1
:fail_nodev
echo.
echo ================= RELEASE STOPPED: "%KH_DEV%" does not exist - run build_all.bat first =================
pause
exit /b 1
:fail_copy
echo.
echo ================= RELEASE STOPPED: a copy step failed ^(robocopy exit code %ERRORLEVEL%^) - see above =================
pause
exit /b 1
:fail_nocache
echo.
echo ================= RELEASE STOPPED: cache folder not found: "%KH_CACHE%" =================
echo Run the game with the dev build once so it generates the cache, then run this again.
pause
exit /b 1

REM --- run one build script in a child cmd and record a failure ----------------
:step
if defined KH_FAILED exit /b 0
echo.
echo ----------------- [%~1]  %~2 -----------------
cmd /c call "%~dp0%~2"
if errorlevel 1 set "KH_FAILED=%~1"
exit /b 0
