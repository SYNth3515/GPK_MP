
@echo off
chcp 65001 >nul
setlocal

title GPK_MP - Build Installer

set "SOURCE=D:\Repo\GPK_MP\GPK_MP"
set "OUTPUT=D:\Repo\GPK_MP\Installer"
set "ISS=%~dp0GPK_MP.iss"
set "VERSION=16.11.26 ALPHA"

echo ==========================================
echo        GPK_MP INSTALLER BUILDER
echo ==========================================
echo.

REM Проверка исходников
if not exist "%SOURCE%\" (
    echo ERROR: Source folder not found!
    pause
    exit /b 1
)

if not exist "%ISS%" (
    echo ERROR: GPK_MP.iss not found!
    pause
    exit /b 1
)

REM Поиск Inno Setup Compiler
set "ISCC="

for %%P in (
    "%ProgramFiles%\Inno Setup 7\ISCC.exe"
    "%ProgramFiles(x86)%\Inno Setup 7\ISCC.exe"
    "%ProgramFiles%\Inno Setup 6\ISCC.exe"
    "%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
    "%ProgramFiles%\Inno Setup 5\ISCC.exe"
    "%ProgramFiles(x86)%\Inno Setup 5\ISCC.exe"
) do (
    if exist "%%~P" set "ISCC=%%~P"
)

REM Если не найден стандартный путь, пробуем PATH
if not defined ISCC (
    for %%P in (ISCC.exe) do set "ISCC=%%~$PATH:P"
)

if not defined ISCC (
    echo ERROR: Inno Setup Compiler not found!
    echo Check installation path.
    pause
    exit /b 1
)

if not exist "%OUTPUT%\" mkdir "%OUTPUT%"

echo Compiler: "%ISCC%"
echo Source:   "%SOURCE%"
echo Output:   "%OUTPUT%"
echo Version:  %VERSION%
echo.
echo Building installer...
echo.

"%ISCC%" /DAppVersion="%VERSION%" "%ISS%"

if errorlevel 1 (
    echo.
    echo BUILD FAILED!
    pause
    exit /b 1
)

echo.
echo ==========================================
echo BUILD SUCCESS!
echo.
echo Installer:
echo %OUTPUT%\GPK_MP_Setup_%VERSION%.exe
echo ==========================================

pause
