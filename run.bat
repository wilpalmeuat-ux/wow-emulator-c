@echo off
REM =====================================================
REM  WoW 3.3.5a Emulator -- Run Script
REM =====================================================
echo.
echo  Starting WoW 3.3.5a Emulator...
echo.

cd bin
if not exist wow-emulator.exe (
    echo [ERROR] wow-emulator.exe not found in bin/
    echo         Run build.bat first.
    cd ..
    pause
    exit /b 1
)

wow-emulator.exe
cd ..
pause
