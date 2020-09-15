
cd %~dp0
mkdir .build.2019
cd .build.2019
cmake -G "Visual Studio 16 2019" -D EWAM_NODE_HOST=ON -D EWAM_CORE=ON "%~dp0"
