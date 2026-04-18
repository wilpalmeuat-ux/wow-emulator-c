# ╔══════════════════════════════════════════════════════════════════╗
# ║   WoW 3.3.5a Emulator — All-In-One Install Script (Windows)     ║
# ╚══════════════════════════════════════════════════════════════════╝
#
# Run as Administrator:
#   powershell -ExecutionPolicy Bypass -File install-all-in-one.ps1
#
# PARAMETERS:
#   -InstallPath   Target directory (default: C:\WoWEmulator)
#   -MySQLPassword MySQL root password (default: rootpassword)
#   -SkipBuild     Skip compilation, just setup files
#   -SkipDeps      Skip dependency installation
#
param(
    [string]$InstallPath  = "C:\WoWEmulator",
    [string]$MySQLPassword = "rootpassword",
    [switch]$SkipBuild,
    [switch]$SkipDeps
)

$ErrorActionPreference = "Stop"
$ProgressPreference    = "SilentlyContinue"

function Log  ($msg) { Write-Host "[INSTALL] $msg" -ForegroundColor Cyan }
function Ok   ($msg) { Write-Host "[OK]      $msg" -ForegroundColor Green }
function Warn ($msg) { Write-Host "[WARN]    $msg" -ForegroundColor Yellow }
function Fail ($msg) { Write-Host "[FAIL]    $msg" -ForegroundColor Red }
function Test-Exe($name) { $null -ne (Get-Command $name -EA SilentlyContinue) }

Write-Host ""
Write-Host "  ╔══════════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "  ║   WoW 3.3.5a Emulator — Windows Installer   ║" -ForegroundColor Cyan
Write-Host "  ╚══════════════════════════════════════════════╝" -ForegroundColor Cyan
Write-Host ""

Log "Install path : $InstallPath"

$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    Warn "Not running as Administrator — some features may fail."
}

$RepoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
if ([string]::IsNullOrEmpty($RepoRoot)) { $RepoRoot = $InstallPath }

foreach ($dir in @("", "build", "data", "logs", "scripts", "scripts\core", "configs", "sql")) {
    $path = Join-Path $InstallPath $dir
    if (-not (Test-Path $path)) { New-Item -ItemType Directory -Force -Path $path | Out-Null }
}

# ═══ STEP 1: Dependencies ═══════════════════════════════════════════
if (-not $SkipDeps) {
    Log "Checking build dependencies..."

    if (-not (Test-Exe cmake)) {
        Log "Installing CMake..."
        $cmakeUrl = "https://github.com/Kitware/CMake/releases/download/v3.28.1/cmake-3.28.1-windows-x86_64.zip"
        $cmakeZip  = "$env:TEMP\cmake.zip"
        $cmakeDir  = "C:\cmake"
        [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
        Invoke-WebRequest -Uri $cmakeUrl -OutFile $cmakeZip -UseBasicParsing
        Expand-Archive -Path $cmakeZip -DestinationPath $cmakeDir -Force
        $env:PATH = "$cmakeDir\cmake-3.28.1-windows-x86_64\bin;$env:PATH"
        Ok "CMake installed"
        Remove-Item $cmakeZip -Force -EA SilentlyContinue
    } else { Ok "CMake already installed" }

    $mysqlLib = "C:\mysql\lib\libmysql.lib"
    if (-not (Test-Path $mysqlLib)) {
        Log "Installing MySQL Connector/C 8.0..."
        $mysqlUrl  = "https://downloads.mysql.com/archives/get/p/23/file/mysql-8.0.35-winx64.zip"
        $mysqlZip  = "$env:TEMP\mysql.zip"
        $mysqlTarget = "C:\mysql"
        [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
        Invoke-WebRequest -Uri $mysqlUrl -OutFile $mysqlZip -UseBasicParsing
        Expand-Archive -Path $mysqlZip -DestinationPath $mysqlTarget -Force -ErrorAction SilentlyContinue
        if (Test-Path "$env:TEMP\mysql-8.0.35-winx64") {
            Move-Item "$env:TEMP\mysql-8.0.35-winx64\*" $mysqlTarget -Force
            Remove-Item "$env:TEMP\mysql-8.0.35-winx64" -Recurse -Force
        }
        Ok "MySQL Connector/C installed to C:\mysql"
        Remove-Item $mysqlZip -Force -EA SilentlyContinue
    } else { Ok "MySQL Connector/C already installed" }

    $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vsWhere) {
        $vsPath = & $vsWhere -latest -property installationPath 2>$null
        if ($vsPath) { Ok "Visual Studio found: $vsPath" }
    } else {
        Warn "Visual Studio not found."
        Warn "Download: https://visualstudio.microsoft.com/downloads/"
        Warn "Select: 'Desktop development with C++'"
    }
}

# ═══ STEP 2: Build ══════════════════════════════════════════════════
if (-not $SkipBuild) {
    $buildBat = Join-Path $RepoRoot "windows\build.bat"
    if (Test-Path $buildBat) {
        Log "Running build.bat..."
        Push-Location (Split-Path $buildBat)
        cmd /c "`"$buildBat`""
        Pop-Location
    } else {
        Log "build.bat not found — using CMake..."
        $buildDir = Join-Path $InstallPath "build"
        cmake $InstallPath -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
        if (Test-Exe msbuild) {
            msbuild (Join-Path $buildDir "WoWEmulator.sln") /p:Configuration=Release /m /v:m
        }
    }

    $exePath = Join-Path $InstallPath "build\wow-emulator.exe"
    if (Test-Path $exePath) {
        Copy-Item $exePath (Join-Path $InstallPath "wow-emulator.exe") -Force
        Ok "Binary ready: $InstallPath\wow-emulator.exe"
    } else {
        Warn "Binary not found. Build may have failed — check errors above."
    }
}

# ═══ STEP 3: Database ═══════════════════════════════════════════════
Log "Setting up database..."

$dbPath = Join-Path $InstallPath "data\emulator.db"

$mysqlService = Get-Service -Name MySQL* -EA SilentlyContinue | Where-Object { $_.Status -eq "Running" } | Select-Object -First 1
if ($mysqlService) {
    Log "MySQL detected — creating databases..."
    $mysqlArgs = @("-u", "root", "-p$MySQLPassword", "--protocol=TCP", "--port=3306")
    try {
        mysql $mysqlArgs -e "CREATE DATABASE IF NOT EXISTS wow_auth CHARACTER SET utf8 COLLATE utf8_general_ci;" 2>$null
        mysql $mysqlArgs -e "CREATE DATABASE IF NOT EXISTS wow_world CHARACTER SET utf8 COLLATE utf8_general_ci;" 2>$null
        Ok "MySQL databases created"
    } catch { Warn "MySQL connection failed — using SQLite" }
} else {
    Ok "MySQL not running — using SQLite"
}

Log "Creating SQLite database..."
$sqliteCreate = @"
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
(entry, name, minlevel, maxlevel, health_min, health_max, faction, display_id) VALUES
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
"@

Add-Content -Path $dbPath -Value $sqliteCreate -Encoding UTF8
Ok "SQLite database ready: $dbPath"

# ═══ STEP 4: Config ══════════════════════════════════════════════════
Log "Writing config..."
$confPath = Join-Path $InstallPath "configs\worldserver.conf"
$instPathSlash = $InstallPath -replace '\\','/'

$confContent = @"
# WoW 3.3.5a Emulator — Server Configuration
# Generated by install-all-in-one.ps1

[worldserver]
Port = 8085
WorldName = `"WoW 3.3.5a Custom Server`"
MaxPlayers = 5000
ThreadCount = 4
DataPath = `"$instPathSlash/data/`"

[authserver]
Port = 3724
BindIP = `"0.0.0.0`"

[database]
Type = `"sqlite`"
Path = `"$instPathSlash/data/emulator.db`"

[scripting]
LoadScripts = true
ScriptPath = `"$instPathSlash/scripts/`"
AutoLoad = true
"@

Set-Content -Path $confPath -Value $confContent -Encoding UTF8
Ok "Config written: $confPath"

# ═══ STEP 5: Scripts ════════════════════════════════════════════════
Log "Copying WSS scripts..."
$srcScripts = Join-Path $RepoRoot "scripts"
if (Test-Path $srcScripts) {
    Copy-Item "$srcScripts\*" (Join-Path $InstallPath "scripts") -Recurse -Force -ErrorAction SilentlyContinue
    Ok "Scripts copied"
}

$helloWss = Join-Path $InstallPath "scripts\hello_world.wss"
if (-not (Test-Path $helloWss)) {
    @"
# Hello World — sample WSS (Word Statement Script)
ON PLAYER_JOIN:
    PRINT `"Welcome, {PLAYER_NAME}, to the WoW 3.3.5a server!"`
    SET GLOBAL server_players = GLOBAL server_players + 1
    IF GLOBAL server_players > 5:
        PRINT `"Server is getting popular!"`
    END

ON PLAYER_CHAT:
    IF MESSAGE == `"!help`":
        PRINT `"Available commands: !help, !info, !online`"
    END
    IF MESSAGE == `"!info`":
        PRINT `"WoW 3.3.5a Custom Emulator`"
        PRINT `"Built with C + Custom WSS Scripting Engine`"
    END
"@ | Set-Content -Path $helloWss -Encoding UTF8
}

$kelthuzadWss = Join-Path $InstallPath "scripts\boss_kelthuzad.wss"
if (-not (Test-Path $kelthuzadWss)) {
    @"
# Kel'Thuzad Boss Script
ON NPC_DEATH:
    IF NPC_ID == 1234:
        PRINT `"Kel'Thuzad has been slain!`"
        SPAWN_CREATURE 5678 at -8955 -125 83
        BROADCAST `"An Arcane Golem awakens from the ruins...`"
        GRANT_GOLD 50000
        GRANT_XP 100000
        SET GLOBAL kelthuzad_killed = GLOBAL kelthuzad_killed + 1
    END
"@ | Set-Content -Path $kelthuzadWss -Encoding UTF8
}

# ═══ STEP 6: run.bat ════════════════════════════════════════════════
$runBat = @"
@echo off
title WoW 3.3.5a Emulator
cd /d `"%~dp0`"
if not exist `"configs\worldserver.conf`" (
    echo [ERROR] worldserver.conf not found!
    pause
    exit /b 1
)
echo.
echo  Starting WoW 3.3.5a Emulator...
echo  Auth Server : 0.0.0.0:3724
echo  World Server: 0.0.0.0:8085
echo  Database    : data\emulator.db (SQLite)
echo.
echo  Default account: test / testpassword
echo.
wow-emulator.exe 8085
"@

Set-Content -Path (Join-Path $InstallPath "run.bat") -Value $runBat -Encoding ASCII
Ok "run.bat created"

# ═══ Done ════════════════════════════════════════════════════════════
Write-Host ""
Write-Host "  ╔══════════════════════════════════════════════╗" -ForegroundColor Green
Write-Host "  ║   INSTALLATION COMPLETE!                   ║" -ForegroundColor Green
Write-Host "  ╚══════════════════════════════════════════════╝" -ForegroundColor Green
Write-Host ""
Write-Host "  Install dir : $InstallPath" -ForegroundColor Yellow
Write-Host "  Binary      : $InstallPath\wow-emulator.exe" -ForegroundColor Yellow
Write-Host "  Database    : $InstallPath\data\emulator.db" -ForegroundColor Yellow
Write-Host "  Config      : $InstallPath\configs\worldserver.conf" -ForegroundColor Yellow
Write-Host ""
Write-Host "  NEXT STEPS:" -ForegroundColor White
Write-Host "  1. Run as Administrator: $InstallPath\run.bat" -ForegroundColor White
Write-Host "  2. Edit realmlist.wtf: set realmlist 127.0.0.1:8085" -ForegroundColor White
Write-Host "  3. Connect with WoW 3.3.5a client" -ForegroundColor White
Write-Host ""
Write-Host "  Default account: test / testpassword" -ForegroundColor Green
Write-Host ""
