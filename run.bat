@echo off
:: WoW 3.3.5a Emulator — Windows Run Script
:: Just double-click this to start the server
cd /d %~dp0
if exist "bin\Release\wow-emulator.exe" (
    bin\Release\wow-emulator.exe
) else if exist "wow-emulator.exe" (
    wow-emulator.exe
) else (
    echo [ERROR] wow-emulator.exe not found!
    echo Run install.bat first to build the server.
    pause
)
