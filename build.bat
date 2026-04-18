@echo off
:: ╔══════════════════════════════════════════════════════════════════╗
:: ║   WoW 3.3.5a Emulator — Windows Build Script (Visual Studio)    ║
:: ║   Usage: build.bat [Debug|Release] [x86|x64]                    ║
:: ╚══════════════════════════════════════════════════════════════════╝
setlocal

set CONFIG=%1
set PLATFORM=%2
if "%CONFIG%"=="" set CONFIG=Release
if "%PLATFORM%"=="" set PLATFORM=x64

echo.
echo ========================================
echo   WoW 3.3.5a Emulator — Windows Build
echo ========================================
echo   Configuration: %CONFIG%
echo   Platform:      %PLATFORM%
echo.

:: Check for Visual Studio
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
    for /f "tokens=*" %%i in ('"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath') do set VSVARS=%%i
)
if not defined VSVARS (
    if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" (
        set VSVARS=%ProgramFiles%\Microsoft Visual Studio\2022\Community
    ) else if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" (
        set VSVARS=%ProgramFiles%\Microsoft Visual Studio\2022\Professional
    ) else if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" (
        set VSVARS=%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise
    ) else (
        echo [ERROR] Visual Studio 2022 not found.
        echo Install Visual Studio 2022 from: https://visualstudio.microsoft.com/
        echo Make sure to select "Desktop development with C++" workload.
        exit /b 1
    )
)

set VCVARS=%VSVARS%\VC\Auxiliary\Build\vcvarsall.bat

if "%PLATFORM%"=="x86" (
    call "%VCVARS%" x86
) else (
    call "%VCVARS%" x64
)

:: Build with MSBuild
echo.
echo [Build] Compiling wow-emulator (%CONFIG%\%PLATFORM%)...
msbuild wow-emulator.sln /p:Configuration=%CONFIG% /p:Platform=%PLATFORM% /t:Build /v:m /nologo

if errorlevel 1 (
    echo.
    echo [ERROR] Build failed. Check the errors above.
    exit /b 1
)

:: Check output
if "%PLATFORM%"=="x86" (
    set OUTDIR=bin\%CONFIG%\
) else (
    set OUTDIR=bin\%CONFIG%\
)

if exist "%OUTDIR%wow-emulator.exe" (
    echo.
    echo ========================================
    echo   BUILD SUCCESS
    echo ========================================
    echo   Output: %OUTDIR%wow-emulator.exe
    echo.
    echo   To run the server:
    echo     cd %OUTDIR%
    echo     wow-emulator.exe
    echo.
) else (
    echo.
    echo [WARN] Binary not found at %OUTDIR%wow-emulator.exe
    echo Check build output above for details.
)
endlocal
