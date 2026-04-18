@echo off
:: ╔══════════════════════════════════════════════════════════════════╗
:: ║   WoW 3.3.5a Emulator — Windows Auto-Install (No MySQL needed) ║
:: ║   Downloads deps, builds, creates DB, generates config           ║
:: ╚══════════════════════════════════════════════════════════════════╝
setlocal enabledelayedexpansion

set EMU_DIR=%~dp0
set EMU_DIR=%EMU_DIR:~0,-1%

echo.
echo ================================================
echo   WoW 3.3.5a Emulator — Windows Installer
echo ================================================
echo.

:: ── Create directories ─────────────────────────────────
echo [1/6] Creating directory structure...
if not exist "%EMU_DIR%\build" mkdir "%EMU_DIR%\build"
if not exist "%EMU_DIR%\data" mkdir "%EMU_DIR%\data"
if not exist "%EMU_DIR%\logs" mkdir "%EMU_DIR%\logs"
if not exist "%EMU_DIR%\scripts" mkdir "%EMU_DIR%\scripts"
if not exist "%EMU_DIR%\scripts\core" mkdir "%EMU_DIR%\scripts\core"
if not exist "%EMU_DIR%\configs" mkdir "%EMU_DIR%\configs"
if not exist "%EMU_DIR%\bin" mkdir "%EMU_DIR%\bin"
echo       Done.

:: ── Check for SQLite DLL ─────────────────────────────────
echo [2/6] Checking SQLite3 runtime...
set SQLITE_FOUND=
where sqlite3 >nul 2>&1
if !ERRORLEVEL!==0 set SQLITE_FOUND=1
if exist "%EMU_DIR%\lib\sqlite3.dll" set SQLITE_FOUND=1

if not defined SQLITE_FOUND (
    echo       SQLite3 not found — downloading sqlite-dll-win64.zip...
    powershell -Command "Invoke-WebRequest -Uri 'https://www.sqlite.org/2024/sqlite-dll-win64.zip' -OutFile '%TEMP%\sqlite-dll-win64.zip' -UseBasicParsing"
    powershell -Command "Expand-Archive -Path '%TEMP%\sqlite-dll-win64.zip' -DestinationPath '%EMU_DIR%\lib' -Force"
    if exist "%EMU_DIR%\lib\sqlite3.dll" (
        echo       SQLite3 DLL extracted to lib\
    ) else (
        echo [WARN] Could not download SQLite DLL. Build may fail.
    )
) else (
    echo       Found sqlite3 in PATH.
)
echo       Done.

:: ── Create SQLite Database ─────────────────────────────────
echo [3/6] Creating SQLite database...
set DB_PATH=%EMU_DIR%\data\emulator.db
if exist "%DB_PATH%" (
    echo       Database already exists at data\emulator.db — skipping.
) else (
    where sqlite3 >nul 2>&1
    if !ERRORLEVEL!==0 (
        sqlite3 "%DB_PATH%" <<EOSQL
CREATE TABLE IF NOT EXISTS characters (
    guid INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL, race INTEGER DEFAULT 1, class INTEGER DEFAULT 1,
    level INTEGER DEFAULT 1, xp INTEGER DEFAULT 0, money INTEGER DEFAULT 0,
    position_x REAL DEFAULT 0, position_y REAL DEFAULT 0,
    position_z REAL DEFAULT 0, orientation REAL DEFAULT 0,
    map_id INTEGER DEFAULT 0, zone_id INTEGER DEFAULT 0,
    account_id INTEGER DEFAULT 0, online INTEGER DEFAULT 0,
    created_at INTEGER DEFAULT (strftime('%s', 'now'))
);
CREATE TABLE IF NOT EXISTS accounts (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT NOT NULL UNIQUE,
    password_hash TEXT NOT NULL,
    email TEXT DEFAULT '',
    last_ip TEXT DEFAULT '127.0.0.1',
    last_login INTEGER DEFAULT 0,
    expansion INTEGER DEFAULT 2,
    banned INTEGER DEFAULT 0
);
CREATE TABLE IF NOT EXISTS creature_templates (
    entry INTEGER PRIMARY KEY,
    name TEXT DEFAULT '',
    minlevel INTEGER DEFAULT 1, maxlevel INTEGER DEFAULT 1,
    health_min INTEGER DEFAULT 100, health_max INTEGER DEFAULT 100,
    mana_min INTEGER DEFAULT 0, mana_max INTEGER DEFAULT 0,
    faction INTEGER DEFAULT 0, scale REAL DEFAULT 1.0,
    display_id INTEGER DEFAULT 0, flags INTEGER DEFAULT 0,
    script_name TEXT DEFAULT ''
);
CREATE TABLE IF NOT EXISTS creature_spawns (
    guid INTEGER PRIMARY KEY AUTOINCREMENT,
    id INTEGER DEFAULT 0, map INTEGER DEFAULT 0,
    position_x REAL DEFAULT 0, position_y REAL DEFAULT 0,
    position_z REAL DEFAULT 0, orientation REAL DEFAULT 0,
    spawntime_secs INTEGER DEFAULT 120
);
CREATE TABLE IF NOT EXISTS world_state (
    var_name TEXT PRIMARY KEY, var_value TEXT
);
INSERT OR IGNORE INTO accounts (username, password_hash, expansion)
VALUES ('test', '5e884898da28047d165d9317a16d8b1a0d7c28b4', 2);
INSERT OR IGNORE INTO creature_templates
(entry, name, minlevel, maxlevel, health_min, health_max, faction, display_id)
VALUES
(1,'Orgrimmar Guard',1,5,80,100,85,1547),
(15,'Orgrimmar Grunt',5,10,150,200,85,1547),
(50,'Combat Trainer',50,55,5000,6000,35,357),
(999,'Stormwind Soldier',10,15,300,400,0,1547),
(9999,'World Boss',80,80,500000,500000,14,16946),
(1234,'KelThuzad',80,80,1000000,1000000,14,15928),
(5678,'Arcane Golem',60,65,50000,60000,14,16510),
(9001,'Dark Portal Guardian',70,75,200000,250000,14,16586);
INSERT OR IGNORE INTO creature_spawns
(id, map, position_x, position_y, position_z, orientation, spawntime_secs) VALUES
(1,0,-8949.0,-132.0,83.0,0.0,60),
(1,0,-8945.0,-130.0,83.0,1.0,60),
(15,0,-8920.0,-140.0,84.0,2.0,120),
(50,0,-8900.0,-145.0,85.0,0.5,300),
(999,1,-10806.0,284.0,35.0,0.0,60),
(9999,0,-8940.0,-120.0,83.5,0.0,3600),
(1234,0,-8945.0,-115.0,83.0,3.14,3600),
(5678,0,-8955.0,-125.0,83.0,1.5,600),
(9001,0,-8925.0,-135.0,83.0,0.0,600);
INSERT OR IGNORE INTO world_state (var_name, var_value) VALUES
('server_name','WoW 3.3.5a Custom Server'),
('max_players','1000'),
('motd','Welcome to the WoW Emulator!');
EOSQL
        echo       Database created: data\emulator.db
    ) else (
        echo [WARN] sqlite3 CLI not found — creating empty DB placeholder.
        echo placeholder > "%DB_PATH%"
    )
)
echo       Done.

:: ── Create Config File ─────────────────────────────────
echo [4/6] Creating worldserver.conf...
set CONF=%EMU_DIR%\configs\worldserver.conf
if exist "%CONF%" (
    echo       Config already exists — skipping.
) else (
    (
        echo # WoW 3.3.5a Emulator — Server Configuration
        echo # Generated by install.bat
        echo.
        echo [worldserver]
        echo Port = 8085
        echo WorldName = "WoW 3.3.5a Custom Server"
        echo MaxPlayers = 5000
        echo ThreadCount = 4
        echo DataPath = "data/"
        echo.
        echo [authserver]
        echo Port = 3724
        echo BindIP = "0.0.0.0"
        echo.
        echo [database]
        echo Type = "sqlite"
        echo Path = "data/emulator.db"
        echo.
        echo [scripting]
        echo LoadScripts = true
        echo ScriptPath = "scripts/"
        echo AutoLoad = true
    ) > "%CONF%"
    echo       Config written: configs\worldserver.conf
)
echo       Done.

:: ── Create Sample WSS Scripts ─────────────────────────────────
echo [5/6] Creating sample WSS scripts...
if not exist "%EMU_DIR%\scripts\hello_world.wss" (
    (
        echo # Hello World — sample WSS script
        echo ON PLAYER_JOIN:
        echo     PRINT "Welcome to the server, {PLAYER_NAME}!"
        echo     SET GLOBAL server_players_count = GLOBAL server_players_count + 1
        echo     IF GLOBAL server_players_count ^> 10:
        echo         PRINT "Server is getting crowded!"
        echo     END
    ) > "%EMU_DIR%\scripts\hello_world.wss"
)
if not exist "%EMU_DIR%\scripts\boss_kelthuzad.wss" (
    (
        echo # Kel'Thuzad Boss Script
        echo ON NPCDeath:
        echo     IF NPC_ID == 1234:
        echo         PRINT "Kel'Thuzad has been slain!"
        echo         SPAWN Creature 5678 at -8955 -125 83
        echo         BROADCAST "Arcane Golem awakens..."
        echo         GRANT_GOLD 50000
        echo         GRANT_XP 100000
        echo     END
    ) > "%EMU_DIR%\scripts\boss_kelthuzad.wss"
)
echo       Done.

:: ── Build ─────────────────────────────────
echo [6/6] Building...
echo.
call "%EMU_DIR%\build.bat" Release x64
if errorlevel 1 (
    echo.
    echo [ERROR] Build failed. See errors above.
    echo [HINT] Make sure Visual Studio 2022 is installed with "Desktop development with C++".
    exit /b 1
)

:: ── Done ─────────────────────────────────
echo.
echo ================================================
echo   INSTALLATION COMPLETE!
echo ================================================
echo.
echo   Server binary : bin\Release\wow-emulator.exe
echo   Database      : data\emulator.db
echo   Config        : configs\worldserver.conf
echo   Scripts       : scripts\
echo.
echo   Default account: test / testpassword
echo.
echo   Connect with WoW 3.3.5a client:
echo   Edit realmlist.wtf:  set realmlist 127.0.0.1
echo.
echo   Run: bin\Release\wow-emulator.exe
echo.
endlocal
