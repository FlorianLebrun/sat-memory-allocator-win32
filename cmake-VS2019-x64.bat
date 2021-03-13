
cd %~dp0
mkdir .build.2019
cd .build.2019
cmake -G "Visual Studio 16 2019" -D "PROJECT_DEV_DIR=%~dp0.output" -D "DEVMODE=ON" "%~dp0"
