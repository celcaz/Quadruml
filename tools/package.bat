@echo off
rem Gera a versao Release em dist\QuadrumI, pronta para abrir com duplo clique.
rem Uso: tools\package.bat
setlocal

if "%QT_DIR%"=="" set "QT_DIR=C:\Qt\6.12.0\msvc2022_64"
set "ROOT=%~dp0.."
set "BUILD=%ROOT%\build\msvc-Release"
set "DIST=%ROOT%\dist\QuadrumI"
if "%VCINSTALLDIR%"=="" set "VCINSTALLDIR=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\"

call "%~dp0build.bat" Release || exit /b 1

if exist "%DIST%" rmdir /s /q "%DIST%"
mkdir "%DIST%" || exit /b 1

copy /y "%BUILD%\QuadrumI.exe" "%DIST%\" >nul || exit /b 1
copy /y "%BUILD%\SDL3.dll" "%DIST%\" >nul || exit /b 1
copy /y "%BUILD%\gamecontrollerdb.txt" "%DIST%\" >nul || exit /b 1

rem Bibliotecas e plugins do Qt usados pelo app (QML, multimidia, SQLite, rede...).
"%QT_DIR%\bin\windeployqt.exe" --release --qmldir "%ROOT%\qml" --no-translations --no-system-d3d-compiler --no-opengl-sw --no-compiler-runtime "%DIST%\QuadrumI.exe" || exit /b 1

rem Runtime do Visual C++ ao lado do exe (a Microsoft permite): abre mesmo em PCs
rem sem o "Visual C++ Redistributable" instalado.
for /d %%D in ("%VCINSTALLDIR%Redist\MSVC\*") do if exist "%%D\x64\Microsoft.VC143.CRT" set "CRT=%%D\x64\Microsoft.VC143.CRT"
if not defined CRT (echo Runtime do Visual C++ nao encontrado. & exit /b 1)
for %%F in (msvcp140.dll msvcp140_1.dll msvcp140_2.dll vcruntime140.dll vcruntime140_1.dll concrt140.dll) do copy /y "%CRT%\%%F" "%DIST%\" >nul
rem Temas locais (o tema PS5 pessoal fica so nesta maquina; nunca vai para o git).
if exist "%ROOT%\themes\ps5" xcopy /e /i /y /q "%ROOT%\themes\ps5" "%DIST%\themes\ps5" >nul

echo.
echo Release pronta em %DIST%
