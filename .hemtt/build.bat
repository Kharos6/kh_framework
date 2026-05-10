@echo off
cd ..
hemtt build

timeout /t 1 /nobreak >nul
echo Copying build addons to dev...
xcopy ".hemttout\build\addons\*" ".hemttout\dev\addons\" /Y /E
echo Done.