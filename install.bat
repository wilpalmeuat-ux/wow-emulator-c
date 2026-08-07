@echo off
REM =====================================================
REM  WoW 3.3.5a Emulator -- Quick Install
REM  Sets up MySQL database and builds the project
REM =====================================================

echo.
echo  +==============================================+
echo  ^|  WoW 3.3.5a Emulator -- Quick Install         ^|
echo  +==============================================+
echo.

REM Step 1: Create database
echo [Step 1] Setting up MySQL database...
echo          (Enter your MySQL root password when prompted)
echo.

where mysql >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo [WARN] mysql client not in PATH.
    echo        Please run manually: mysql -u root -p ^< sql\create_database.sql
) else (
    mysql -u root -p < sql\create_database.sql
    if %ERRORLEVEL% neq 0 (
        echo [WARN] Database setup may have failed. Check MySQL connection.
    ) else (
        echo [OK] Database created successfully.
    )
)

echo.

REM Step 2: Build
echo [Step 2] Building the emulator...
call build.bat

echo.
echo  +==============================================+
echo  ^|  INSTALLATION COMPLETE                        ^|
echo  +==============================================+
echo.
echo  To start the server: run.bat
echo  To connect: set realmlist 127.0.0.1
echo  Test account: TEST (no password)
echo.
pause
