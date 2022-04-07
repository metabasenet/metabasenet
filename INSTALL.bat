@echo off

set old_dir=%cd%

cd %~dp0

REM create build directory
if not exist build (
	mkdir build
	if "%errorlevel%" NEQ "0" goto end
)

REM go to build
cd build
if "%errorlevel%" NEQ "0" goto end

REM cmake
set flagdebug="off"
if "%1%"=="debug" set flagdebug="on"
if "%2%"=="debug" set flagdebug="on"
if %flagdebug%=="on" (
	set flagdebug="-DCMAKE_BUILD_TYPE=Debug"
) else (
	set flagdebug="-DCMAKE_BUILD_TYPE=Release"
)

echo 'cmake .. -G "Ninja" %flagdebug%
cmake .. -G "Ninja" %flagdebug%
if "%errorlevel%" NEQ "0" goto end

REM make
ninja
if "%errorlevel%" NEQ "0" goto end

REM install
mkdir bin
copy src\metabasenet\metabasenet.exe bin\
copy src\metabasenet\*.dll bin\

echo %%~dp0\metabasenet.exe console %%* > bin\metabasenet-cli.bat
echo %%~dp0\metabasenet.exe -daemon %%* > bin\metabasenet-server.bat

echo 'Installed to build\bin\'
echo ''
echo 'Usage:'
echo 'Run build\bin\metabasenet.exe to launch metabasenet'
echo 'Run build\bin\metabasenet-cli.bat to launch metabasenet RPC console'
echo 'Run build\bin\metabasenet-server.bat to launch metabasenet server on background'
echo 'Run build\test\test_big.exe to launch test program.'
echo 'Default `-datadir` is the same folder of metabasenet.exe'

echo 'Installed to build\bin\'
echo ''
echo 'Usage:'
echo 'Run build\bin\metabasenet.exe to launch metabasenet'
echo 'Run build\bin\metabasenet-console.bat to launch metabasenet-console'
echo 'Run build\bin\metabasenet-server.bat to launch metabasenet server on background'
echo 'Run build\test\test_big.exe to launch test program.'

:end

cd %old_dir%
