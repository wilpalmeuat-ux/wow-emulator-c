#!/bin/bash
# ╔══════════════════════════════════════════════════════════════════╗
# ║   WoW 3.3.5a Emulator — All-In-One Install Script (Linux/macOS)  ║
# ║   Handles: deps → build → database → config → run             ║
# ╚══════════════════════════════════════════════════════════════════╝
#
# Usage:
#   curl -fsSL https://raw.githubusercontent.com/<user>/wow-emulator/main/install.sh | bash
#   OR just run from the repo:
#   ./install.sh
#
set -euo pipefail

# ── Detect OS ──────────────────────────────────────────────────────
OS="$(uname -s)"
DIST=""
if [ -f /etc/os-release ]; then
    DIST="$(. /etc/os-release 2>/dev/null; echo $ID)"
fi

# ── Colour codes ──────────────────────────────────────────────────
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
CYAN='\033[0;36m'; BOLD='\033[1m'; RESET='\033[0m'

log()  { printf "${CYAN}[install]${RESET} %s\n" "$*"; }
ok()   { printf "${GREEN}[OK]${RESET}    %s\n" "$*"; }
warn() { printf "${YELLOW}[WARN]${RESET}  %s\n" "$*"; }
fail() { printf "${RED}[FAIL]${RESET}  %s\n" "$*" >&2; }

need() {
    log "Installing $1..."
    case "$DIST" in
        ubuntu|debian|linuxmint|pop)
            sudo apt-get update -qq
            sudo apt-get install -y -qq "$2" ;;
        fedora)
            sudo dnf install -y "$2" ;;
        arch)
            sudo pacman -Sy --noconfirm "$2" ;;
        darwin)
            [ -z "$(command -v brew)" ] && /bin/bash -c "$(curl -fsSL https://brew.sh/install.sh)"
            brew install "$2" ;;
        alpine)
            sudo apk add --no-cache "$2" ;;
        *)
            fail "Unsupported distro: $DIST ($OS). Install dependencies manually."
            exit 1 ;;
    esac
    ok "$1 installed"
}

# ── 0. Pre-flight ─────────────────────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INSTALL_DIR="${INSTALL_DIR:-$SCRIPT_DIR}"
log "WoW 3.3.5a Emulator — All-In-One Installer"
log "Install directory: $INSTALL_DIR"

# ── 1. Build Dependencies ─────────────────────────────────────────
log "Checking build dependencies..."

check() { command -v "$1" >/dev/null 2>&1; }

if ! check cmake; then need "CMake" "cmake"; fi
if ! check g++;   then need "C/C++ compiler" "build-essential"; fi
if ! check make;  then need "Make" "make"; fi
if ! check pkg-config; then need "pkg-config" "pkg-config"; fi

# SQLite (build dependency)
if ! check sqlite3; then need "SQLite 3 CLI" "sqlite3"; fi
if ! pkg-config --exists sqlite3 2>/dev/null; then
    need "SQLite 3 dev" "libsqlite3-dev"
fi

# Threads (usually built-in on Linux)
if ! pkg-config --exists threads 2>/dev/null; then
    need "pthreads dev" "libpthreads-dev" || need "pthreads dev" "pthread"
fi

# ── 2. Create directory structure ──────────────────────────────────
log "Creating directory structure..."
mkdir -p "$INSTALL_DIR/build"
mkdir -p "$INSTALL_DIR/data"
mkdir -p "$INSTALL_DIR/logs"
mkdir -p "$INSTALL_DIR/scripts"
mkdir -p "$INSTALL_DIR/configs"

ok "Directories ready"

# ── 3. Build ───────────────────────────────────────────────────────
log "Building wow-emulator (this may take a minute)..."
cd "$INSTALL_DIR/build"

cmake .. \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_C_COMPILER=gcc \
    -DCMAKE_CXX_COMPILER=g++ \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    2>&1 | tail -5

MAKE_JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
make -j"$MAKE_JOBS" 2>&1

if [ ! -f wow-emulator ]; then
    fail "Build failed — binary not found"
    exit 1
fi
ok "Build complete: $INSTALL_DIR/build/wow-emulator"

# Symlink binary to install root for convenience
ln -sfn "$INSTALL_DIR/build/wow-emulator" "$INSTALL_DIR/wow-emulator"

# ── 4. Database Setup ─────────────────────────────────────────────
DB_PATH="$INSTALL_DIR/data/emulator.db"
SCHEMA="$INSTALL_DIR/sql/schema.sql"

if [ -f "$DB_PATH" ]; then
    warn "Database already exists at $DB_PATH — skipping creation"
else
    log "Creating SQLite database at $DB_PATH..."
    if [ -f "$SCHEMA" ]; then
        sqlite3 "$DB_PATH" < "$SCHEMA"
    else
        # Inline minimal schema
        sqlite3 "$DB_PATH" <<'EOSQL'
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
(1234,"Kel'Thuzad",80,80,1000000,1000000,14,15928),
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
    fi
    ok "Database created at $DB_PATH"
fi

# ── 5. Config file ────────────────────────────────────────────────
CONF="$INSTALL_DIR/configs/worldserver.conf"
if [ -f "$CONF" ]; then
    ok "Config already exists"
else
    log "Creating worldserver.conf..."
    cat > "$CONF" <<'EOCONF'
# WoW 3.3.5a Emulator — Server Configuration
# Generated by install.sh

[worldserver]
Port = 8085
WorldName = "WoW 3.3.5a Custom Server"
MaxPlayers = 5000
ThreadCount = 4
DataPath = "data/"

[authserver]
Port = 3724
BindIP = "0.0.0.0"

[database]
Type = "sqlite"
Path = "data/emulator.db"

[scripting]
LoadScripts = true
ScriptPath = "scripts/"
AutoLoad = true
EOCONF
    ok "Config written to $CONF"
fi

# ── 6. Sample WSS scripts (if missing) ───────────────────────────
SAMPLE_WSS="$INSTALL_DIR/scripts/hello_world.wss"
if [ ! -f "$SAMPLE_WSS" ]; then
    log "Creating sample WSS scripts..."
    cat > "$INSTALL_DIR/scripts/hello_world.wss" <<'EOWSS'
# Hello World — sample WSS script
ON PLAYER_JOIN:
    PRINT "Welcome to the server, {PLAYER_NAME}!"
    SET GLOBAL server_players_count = GLOBAL server_players_count + 1
    IF GLOBAL server_players_count > 10:
        PRINT "Server is getting crowded!"
    END
EOWSS
    cat > "$INSTALL_DIR/scripts/boss_kelthuzad.wss" <<'EOWSS'
# Kel'Thuzad Boss Script
ON NPCDeath:
    IF NPC_ID == 1234:
        PRINT "Kel'Thuzad has been slain!"
        SPAWN Creature 5678 at -8955 -125 83
        BROADCAST "Arcane Golem awakens..."
        GRANT_GOLD 50000
        GRANT_XP 100000
    END
EOWSS
    ok "Sample scripts created"
fi

# ── 7. Start script ───────────────────────────────────────────────
START="$INSTALL_DIR/start-server.sh"
log "Creating start script..."
cat > "$START" <<EOSCRIPT
#!/bin/bash
# WoW 3.3.5a Server Start Script
cd "$INSTALL_DIR"
./wow-emulator 8085
EOSCRIPT
chmod +x "$START"
ok "Start script: $START"

# ── Done ─────────────────────────────────────────────────────────
echo ""
echo "  ${GREEN}╔══════════════════════════════════════════════╗${RESET}"
echo "  ${GREEN}║   INSTALLATION COMPLETE!                     ║${RESET}"
echo "  ${GREEN}╚══════════════════════════════════════════════╝${RESET}"
echo ""
echo "  Server binary : $INSTALL_DIR/wow-emulator"
echo "  Database      : $DB_PATH"
echo "  Config        : $CONF"
echo "  Scripts       : $INSTALL_DIR/scripts/"
echo ""
echo "  ${YELLOW}Quick Start:${RESET}"
echo "    cd $INSTALL_DIR"
echo "    ./start-server.sh"
echo ""
echo "  ${YELLOW}Connect with WoW 3.3.5a client:${RESET}"
echo "    Edit realmlist.wtf:"
echo "      set realmlist 127.0.0.1:8085"
echo ""
echo "  Default account: ${BOLD}test${RESET} / ${BOLD}testpassword${RESET}"
echo ""
