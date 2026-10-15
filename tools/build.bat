@echo off
rem Compila o Quadrum I com MSVC fora do Qt Creator.
rem Uso: tools\build.bat [Debug|Release]
setlocal

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Debug"
if "%QT_DIR%"=="" set "QT_DIR=C:/Qt/6.12.0/msvc2022_64"
if "%QT_TOOLS%"=="" set "QT_TOOLS=C:\Qt\Tools"
if "%VCVARS%"=="" set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if "%VCPKG_ROOT%"=="" set "VCPKG_ROOT=%USERPROFILE%\vcpkg"

call "%VCVARS%" >nul 2>nul
where cl >nul 2>nul || (echo MSVC nao encontrado. Ajuste a variavel VCVARS. & exit /b 1)

set "PATH=%QT_TOOLS%\Ninja;%QT_TOOLS%\CMake_64\bin;%PATH%"
set "BUILD_DIR=%~dp0..\build\msvc-%CONFIG%"

cmake -S "%~dp0.." -B "%BUILD_DIR%" -G Ninja -DCMAKE_BUILD_TYPE=%CONFIG% -DCMAKE_PREFIX_PATH=%QT_DIR% ^
    "-DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" || exit /b 1
cmake --build "%BUILD_DIR%" || exit /b 1
