set GENERATE_OPTIONS= -G "Visual Studio 16 2019" -A x64 ^
 -D ENABLE_TESTS=ON ^
 -D DEVMODE=ON

cd %~dp0
set GENERATE_PROJECT_NAME=.build-2019
mkdir %GENERATE_PROJECT_NAME%
cd %GENERATE_PROJECT_NAME%
cmake %GENERATE_OPTIONS% -D "PROJECT_DEV_DIR=%~dp0.output" "%~dp0"
