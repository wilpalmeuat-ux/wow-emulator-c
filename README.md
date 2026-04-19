# WoW 3.3.5a Private Server Emulator

A fully custom World of Warcraft 3.3.5a (WotLK) private server emulator written entirely in C, featuring a unique **Word Statement Scripting (WSS)** engine where all game logic is defined using plain-English statements.

**Windows only. MySQL required.**

---

## Features

- **Pure C codebase** -- no C++, no Lua, no external scripting languages
- **MySQL database** -- accounts, characters, creatures, items, spells
- **Custom WSS scripting engine** -- game logic written as English word/statement scripts
- **Auth server** -- SRP6-based login handshake on port 3724
- **World server** -- game world simulation on port 8085
- **WoW 3.3.5a protocol** -- handles authentication, character enum, login, chat, movement, gossip, creature queries
- **Hot-reloadable scripts** -- edit `.wss` files and type `.reloadscripts` in-game
- **Windows native** -- WinSock2, Windows threads, MSVC build

---

## Architecture

```
wow-emulator-c/
  src/
    main.c                  -- Entry point (starts auth + world servers)
    authserver/
      authserver.c          -- Login protocol (challenge, proof, realm list)
      auth_main.c           -- Standalone auth entry point (optional)
    worldserver/
      worldserver.c         -- World server core (connections, spawning, maps)
      worldsession.c        -- Packet dispatcher (auth, char, login, chat, movement)
      player.c              -- Player object management
      worldsocket.c         -- Low-level socket send/recv helpers
      packets.c             -- Opcode table + packet builder utilities
    shared/
      database.c            -- MySQL database layer (accounts, characters, CRUD)
      Config/Config.c       -- INI configuration file parser
      BigNumber.c           -- Big number arithmetic (for SRP6)
      SRP6.c                -- SRP6 authentication protocol
      ByteBuffer.c          -- Binary packet read/write buffer
      log.c                 -- Logging system
    scripting/
      script_engine.c       -- WSS engine core (lexer, parser, executor)
      wss_lexer.c           -- Tokenizer for WSS scripts
      wss_vm.c              -- Bytecode virtual machine
      wss_compiler.c        -- Script compiler (recursive descent)
      wss_scanner.c         -- File scanner for .wss loading
      wss_value.c           -- Tagged union value type
      wss_chunk.c           -- Bytecode chunk management
      wss_objectstore.c     -- Persistent key-value game state
      wss_wow_hooks.c       -- WoW-specific script hooks
    scripts/core/
      builtin_statements.c  -- All built-in WSS statement handlers
  include/                  -- Header files for all modules
  configs/
    worldserver.conf        -- Server configuration (MySQL, ports, paths)
  scripts/                  -- WSS script files (.wss)
    core/core.wss           -- Core event handlers
    core/welcome.wss        -- Welcome messages
    creatures/guard.wss     -- NPC behavior scripts
    creatures/boss.wss      -- Boss encounter scripts
    events/pvp_event.wss    -- Event system examples
    commands/gm_commands.wss-- GM command extensions
  sql/
    create_database.sql     -- Full MySQL schema + sample data
```

---

## WSS Scripting Engine

The **Word Statement Scripting (WSS)** engine is the core innovation of this emulator. Instead of Lua tables or C++ hooks, all game behavior is defined using plain-English statements in `.wss` files.

### How It Works

Each line in a `.wss` file is a declarative English statement. The engine tokenizes the words, matches them against registered statement handlers, and executes them.

### Example Script

```wss
// When a player logs in
onLogin player {
    broadcast "Welcome to the server!"
    givegold 10000
    setlevel 10
}

// When the World Boss spawns
onSpawn World_Boss {
    sethealth 9999999
    setmaxhealth 9999999
    setlevel 83
    broadcast "WARNING: A World Boss has appeared!"
}

// When the World Boss dies
onDeath World_Boss {
    broadcast "The World Boss has been defeated!"
}

// NPC gossip interaction
onGossip Combat_Trainer {
    say "Ready to train, adventurer?"
}
```

### Available Statement Keywords

| Keyword | Description | Example |
|---------|-------------|---------|
| `print` / `log` | Output to server console | `print "Hello world"` |
| `say` | NPC chat bubble | `say "Greetings!"` |
| `whisper` | Private message | `whisper "Secret info"` |
| `broadcast` | Server-wide message | `broadcast "Server restart in 5 min"` |
| `announce` | Broadcast with prefix | `announce "Maintenance soon"` |
| `spawn` | Create a creature | `spawn 1234 at 100 200 300 on map 0` |
| `despawn` | Remove a creature | `despawn target` |
| `sethealth` | Set unit health | `sethealth 5000` |
| `setmaxhealth` | Set max health | `setmaxhealth 10000` |
| `setlevel` | Set unit level | `setlevel 80` |
| `teleport` | Move player | `teleport player to 0 -8949 -132 83 0` |
| `givexp` | Award XP | `givexp 1000` |
| `givegold` | Award gold (copper) | `givegold 50000` |
| `giveitem` | Add item to inventory | `giveitem 49623 count 1` |
| `kill` | Instantly kill target | `kill target` |
| `resurrect` | Revive target | `resurrect target` |
| `morph` | Change display model | `morph 12345` |
| `cast` | Cast a spell | `cast 12345 on target` |
| `aura` | Apply a buff | `aura 12345 on target` |
| `removeaura` | Remove a buff | `removeaura 12345` |
| `emote` | Play animation | `emote dance` |
| `set_flag` | Set unit flag | `set_flag pvp` |
| `clear_flag` | Remove unit flag | `clear_flag combat` |
| `setfaction` | Change faction | `setfaction 35` |
| `setspeed` | Change move speed | `setspeed run 2.5` |
| `wait` | Delay execution | `wait 5000` |
| `countdown` | Timed announcements | `countdown 10 "Event starts in"` |
| `timer` | Recurring timer | `timer 60000 "onMinute"` |
| `set` | Set a variable | `set global server_players + 1` |
| `if` / `while` / `for` | Control flow | `if level > 10 { ... }` |

### Event Hooks

Scripts bind to game events using these keywords:

| Hook | Trigger |
|------|---------|
| `onLogin` | Player enters the world |
| `onLogout` | Player leaves |
| `onSay` | Player sends chat |
| `onDeath` | Unit dies |
| `onSpawn` | Unit enters the world |
| `onGossip` | Player interacts with NPC |
| `onKill` | Unit gets a kill |
| `onHit` | Unit takes damage |
| `onEnter` | Player enters area |
| `onLeave` | Player leaves area |
| `onTimer` | Timer fires |
| `onUse` | Object is used |

---

## Requirements

- **Windows 10/11** (64-bit)
- **Visual Studio 2019 or 2022** (with C/C++ workload)
- **CMake 3.16+**
- **MySQL Server 8.0** (or 5.7)
- **WoW 3.3.5a client** (build 12340)

---

## Quick Start

### 1. Install Prerequisites

1. Install [Visual Studio 2022](https://visualstudio.microsoft.com/) with "Desktop development with C++"
2. Install [CMake](https://cmake.org/download/)
3. Install [MySQL Server 8.0](https://dev.mysql.com/downloads/mysql/)

### 2. Set Up Database

```bat
mysql -u root -p < sql\create_database.sql
```

This creates the `wow_emulator` database with all required tables and sample data.

### 3. Build

```bat
build.bat
```

Or manually:

```bat
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DMYSQL_DIR="C:\Program Files\MySQL\MySQL Server 8.0"
cmake --build . --config Release
cd ..
```

### 4. Configure

Edit `configs\worldserver.conf`:

```ini
[database]
Host = 127.0.0.1
Port = 3306
User = root
Password = yourpassword
Database = wow_emulator
```

### 5. Run

```bat
run.bat
```

Or:

```bat
bin\wow-emulator.exe
```

### 6. Connect

In your WoW 3.3.5a client, set:

```
set realmlist 127.0.0.1
```

Login with username `TEST` (no password needed for the demo auth).

---

## Configuration

The server reads `configs/worldserver.conf`:

```ini
[worldserver]
Port = 8085              # World server port
WorldName = My Server    # Realm name
MaxPlayers = 5000        # Max concurrent players

[authserver]
Port = 3724              # Auth/login server port
BindIP = 0.0.0.0         # Listen address

[database]
Type = mysql
Host = 127.0.0.1
Port = 3306
User = root
Password =
Database = wow_emulator

[scripting]
LoadScripts = 1          # Enable WSS engine
ScriptPath = scripts/    # Path to .wss files
```

---

## In-Game Commands

| Command | Description |
|---------|-------------|
| `.info` | Show server uptime and player count |
| `.reloadscripts` | Hot-reload all WSS scripts |
| `.spawn <entry>` | Spawn a creature at your position |

---

## Database Schema

The MySQL database (`wow_emulator`) contains:

| Table | Purpose |
|-------|---------|
| `accounts` | Login credentials, GM levels |
| `characters` | Player characters (name, race, class, position) |
| `creature_template` | NPC definitions (stats, display, faction) |
| `creature_spawns` | Where NPCs appear in the world |
| `gameobject_spawns` | World objects (chests, doors) |
| `world_state` | Server key/value variables |
| `realmlist` | Available realms |
| `item_template` | Item definitions |
| `spell_template` | Spell definitions |

---

## Adding Custom Scripts

1. Create a new `.wss` file in `scripts/` (any subdirectory)
2. Write your event handlers using the WSS keywords
3. Restart the server or type `.reloadscripts` in-game

Example -- create `scripts/creatures/vendor.wss`:

```wss
onGossip Vendor_NPC {
    say "Browse my wares, traveler!"
}

onSpawn Vendor_NPC {
    sethealth 99999
    setlevel 60
    setfaction 35
    log "Vendor NPC ready for business"
}
```

---

## License

This project is for educational purposes. World of Warcraft is a trademark of Blizzard Entertainment.
