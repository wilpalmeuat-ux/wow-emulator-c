@echo off
REM =====================================================
REM  WoW 3.3.5a Emulator -- Windows Build Script
REM  Requires: Visual Studio 2019+, CMake, MySQL Server
REM =====================================================

echo.
echo  +==============================================+
echo  ^|  WoW 3.3.5a Emulator Build Script            ^|
echo  ^|  Windows + MSVC + MySQL                       ^|
echo  +==============================================+
echo.

REM Check for CMake
where cmake >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake not found. Install CMake and add to PATH.
    echo         https://cmake.org/download/
    pause
    exit /b 1
)

REM Detect MySQL
set MYSQL_PATH=
if exist "C:\Program Files\MySQL\MySQL Server 8.0" (
    set MYSQL_PATH=C:\Program Files\MySQL\MySQL Server 8.0
) else if exist "C:\Program Files\MySQL\MySQL Server 5.7" (
    set MYSQL_PATH=C:\Program Files\MySQL\MySQL Server 5.7
) else if exist "C:\mysql" (
    set MYSQL_PATH=C:\mysql
)

if "%MYSQL_PATH%"=="" (
    echo [WARN] MySQL not auto-detected. Set MYSQL_DIR manually.
    set MYSQL_PATH=C:\mysql
)

echo [INFO] MySQL path: %MYSQL_PATH%

REM Create build directory
if not exist build mkdir build
cd build

REM Configure with CMake
echo [INFO] Running CMake...
cmake .. -G "Visual Studio 17 2022" -A x64 -DMYSQL_DIR="%MYSQL_PATH%"
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake configuration failed.
    echo         Try: cmake .. -G "Visual Studio 16 2019" -A x64 -DMYSQL_DIR="%MYSQL_PATH%"
    cd ..
    pause
    exit /b 1
)

REM Build
echo [INFO] Building...
cmake --build . --config Release
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed.
    cd ..
    pause
    exit /b 1
)

cd ..

REM Copy MySQL DLL
if exist "%MYSQL_PATH%\lib\libmysql.dll" (
    echo [INFO] Copying libmysql.dll to bin/
    copy "%MYSQL_PATH%\lib\libmysql.dll" bin\ >nul 2>nul
)

REM Copy configs
if not exist bin\configs mkdir bin\configs
copy configs\worldserver.conf bin\configs\ >nul 2>nul

REM Copy scripts
if not exist bin\scripts mkdir bin\scripts
xcopy scripts bin\scripts /E /Y /Q >nul 2>nul

REM Copy SQL
if not exist bin\sql mkdir bin\sql
copy sql\*.sql bin\sql\ >nul 2>nul

echo.
echo  +==============================================+
echo  ^|  BUILD COMPLETE                               ^|
echo  +==============================================+
echo.
echo  Output: bin\wow-emulator.exe
echo.
echo  Before running:
echo    1. Install MySQL Server 8.0
echo    2. Run: mysql -u root -p ^< sql\create_database.sql
echo    3. Edit: bin\configs\worldserver.conf
echo    4. Run:  bin\wow-emulator.exe
echo.
pause
