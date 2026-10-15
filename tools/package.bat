@echo off
rem Gera a versao Release em dist\QuadrumI, pronta para abrir com duplo clique.
rem Uso: tools\package.bat
setlocal

if "%QT_DIR%"=="" set "QT_DIR=C:\Qt\6.12.0\msvc2022_64"
set "ROOT=%~dp0.."
set "BUILD=%ROOT%\build\msvc-Release"
set "DIST=%ROOT%\dist\QuadrumI"

call "%~dp0build.bat" Release || exit /b 1

if exist "%DIST%" rmdir /s /q "%DIST%"
mkdir "%DIST%" || exit /b 1

copy /y "%BUILD%\QuadrumI.exe" "%DIST%\" >nul || exit /b 1
copy /y "%BUILD%\SDL3.dll" "%DIST%\" >nul || exit /b 1
copy /y "%BUILD%\gamecontrollerdb.txt" "%DIST%\" >nul || exit /b 1

rem Bibliotecas e plugins do Qt usados pelo app (QML, multimidia, SQLite, rede...).
"%QT_DIR%\bin\windeployqt.exe" --release --qmldir "%ROOT%\qml" --no-translations --no-system-d3d-compiler --no-opengl-sw "%DIST%\QuadrumI.exe" || exit /b 1

rem Temas locais (o tema PS5 pessoal fica so nesta maquina; nunca vai para o git).
if exist "%ROOT%\themes\ps5" xcopy /e /i /y /q "%ROOT%\themes\ps5" "%DIST%\themes\ps5" >nul

echo.
echo Release pronta em %DIST%
