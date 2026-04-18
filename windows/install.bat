@echo off
setlocal enabledelayedexpansion

echo.
echo  ================================================
echo   WoW 3.3.5a Emulator  -  Windows Install
echo   Sets up config, database, and directory structure
echo  ================================================
echo.

set EMULATOR_ROOT=%~dp0..
set CONFIG_DIR=%EMULATOR_ROOT%\configs
set DATA_DIR=%EMULATOR_ROOT%\data
set SCRIPTS_DIR=%EMULATOR_ROOT%\scripts
set SCRIPTS_CORE_DIR=%EMULATOR_ROOT%\scripts\core
set LOGS_DIR=%EMULATOR_ROOT%\logs

:: Create directories
echo [Install] Creating directory structure...
if not exist "%CONFIG_DIR%" mkdir "%CONFIG_DIR%"
if not exist "%DATA_DIR%" mkdir "%DATA_DIR%"
if not exist "%SCRIPTS_DIR%" mkdir "%SCRIPTS_DIR%"
if not exist "%SCRIPTS_CORE_DIR%" mkdir "%SCRIPTS_CORE_DIR%"
if not exist "%LOGS_DIR%" mkdir "%LOGS_DIR%"

:: Copy config if not exists
if not exist "%CONFIG_DIR%\worldserver.conf" (
    echo [Install] Creating worldserver.conf...
    (
        echo # WoW 3.3.5a Emulator Configuration (Windows)
        echo # This file is read on startup
        echo.
        echo [authserver]
        echo AuthServerPort=3724
        echo.
        echo [worldserver]
        echo WorldServerPort=8085
        echo.
        echo [database]
        echo DBType=sqlite
        echo # For MySQL, uncomment below and install MySQL Connector/C:
        echo # DBType=mysql
        echo # DBHost=127.0.0.1
        echo # DBPort=3306
        echo # DBUser=root
        echo # DBPassword=YOUR_PASSWORD
        echo # DBDatabase=wow_emulator
    ) > "%CONFIG_DIR%\worldserver.conf"
    echo [Install]   Created: %CONFIG_DIR%\worldserver.conf
) else (
    echo [Install]   worldserver.conf already exists, skipping.
)

:: Create default .wss scripts
if not exist "%SCRIPTS_DIR%\startup.wss" (
    echo [Install] Creating startup.wss script...
    (
        echo # WoW 3.3.5a WSS Startup Script
        echo # Word Statement Script — https://your-wiki/docs/wss
        echo.
        echo var world_name = "WoW 3.3.5a Emulator"
        echo var max_players = 100
        echo.
        echo when server starts do
        echo     print "================================"
        echo     print "  " + world_name + " online"
        echo     print "  Max players: " + max_players
        echo     print "================================"
        echo end
    ) > "%SCRIPTS_DIR%\startup.wss"
)

if not exist "%SCRIPTS_DIR%\npcs.wss" (
    echo [Install] Creating npcs.wss script...
    (
        echo # NPC behavior scripts
        echo.
        echo when creature spawns do
        echo     print "Creature spawned: " + creature.name
        echo     set creature.max_health = creature.level * 100 + 500
        echo     set creature.health = creature.max_health
        echo end
    ) > "%SCRIPTS_DIR%\npcs.wss"
)

if not exist "%SCRIPTS_CORE_DIR%\combat.wss" (
    echo [Install] Creating combat.wss script...
    (
        echo # Combat system core
        echo.
        echo var phase = 1
        echo.
        echo when player attacks do
        echo     set creature.health = creature.health - damage
        echo     if creature.health is less than 5000 then
        echo         broadcast "Phase 2!"
        echo         set phase = 2
        echo     end
        echo end
    ) > "%SCRIPTS_CORE_DIR%\combat.wss"
)

:: Check MySQL
echo.
echo [Install] Checking MySQL...
mysql --version >nul 2>&1
if errorlevel 1 (
    echo [Install] MySQL not found in PATH.
    echo [Install] SQLite will be used as the default database.
    echo [Install] To use MySQL, install MySQL Server 8.0 and add it to your PATH.
    echo [Install] Download: https://dev.mysql.com/downloads/mysql/
) else (
    echo [Install] MySQL found.
    echo [Install] You can use MySQL by editing configs\worldserver.conf
)

:: Create database
if exist "C:\mysql\bin\mysql.exe" (
    echo.
    echo [Install] Creating MySQL database...
    C:\mysql\bin\mysql.exe -u root -p < "%CONFIG_DIR%\create_db.sql" >nul 2>&1
    if errorlevel 1 (
        echo [Install] Could not create database. Check your MySQL password in create_db.sql
    ) else (
        echo [Install]   Database created successfully.
    )
)

echo.
echo  ================================================
echo   Install complete!
echo  ================================================
echo.
echo  Directory structure created:
echo    configs\   - server configuration
echo    data\      - runtime data
echo    scripts\   - WSS script files
echo    logs\      - server logs
echo.
echo  To run:
echo    cd build
echo    wow-emulator.exe
echo.
echo  Or run from root:
echo    build\wow-emulator.exe
echo.
pause