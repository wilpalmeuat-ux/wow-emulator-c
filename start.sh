#!/bin/bash
# WoW 3.3.5a Emulator — Startup Script
set -e

cd "$(dirname "$0")"

echo ""
echo "  ╔══════════════════════════════════════════════╗"
echo "  ║   WoW 3.3.5a Emulator — C + WSS Scripts ║"
echo "  ╚══════════════════════════════════════════════╝"
echo ""

# Check for built binary
if [ ! -f "./wow-emulator" ]; then
    echo "[Setup] Binary not found, running cmake build..."
    mkdir -p build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make -j$(nproc)
    cd ..
fi

# Initialize database if needed
if [ ! -f "world.db" ]; then
    echo "[DB] Creating SQLite database..."
    sqlite3 world.db < sql/schema.sql
    echo "[DB] Database created at world.db"
fi

# Detect auth server port (default 3724)
AUTH_PORT=3724
WORLD_PORT=8085

# Allow override via environment
WORLD_PORT=${WORLD_PORT:-8085}

echo "[Info] Auth Server:  0.0.0.0:$AUTH_PORT"
echo "[Info] World Server: 0.0.0.0:$WORLD_PORT"
echo "[Info] Database:     world.db (SQLite)"
echo "[Info] Scripts:      scripts/"
echo ""
echo "  Default account: test / testpassword"
echo ""
echo "[Ready] Starting server..."
echo ""

./wow-emulator $WORLD_PORT
