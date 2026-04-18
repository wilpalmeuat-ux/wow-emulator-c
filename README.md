/* ╔══════════════════════════════════════════════════════════════╗
   ║  WoW 3.3.5 Private Server Emulator                         ║
   ║  Fully custom C codebase with word/statement scripting      ║
   ╚══════════════════════════════════════════════════════════════╝
   
   Build:  mkdir build && cd build && cmake .. && make -j$(nproc)
   Run:    ./bin/authserver    (terminal 1)
           ./bin/worldserver   (terminal 2)
   
   Scripting:  All game logic is driven by .wss script files.
   See scripts/core/core.wss for the standard event handlers.
   
   ─── Architecture ────────────────────────────────────────────
   
   authserver/   ── Client authentication, account DB, realm list
   worldserver/   ── Game world simulation, maps, sessions
   shared/        ── Types, logging, database helpers, config
   scripting/     ─── The entire scripting engine (explained below)
   
   ─── Custom Scripting Engine (scripts/) ────────────────────
   
   The scripting engine is NOT Lua, NOT Python, NOT a DB table.
   It is a purpose-built plain-English word/statement interpreter
   that reads *.wss files (WoW Script Statements).
   
   Each line of a .wss file is a declarative English statement:
   
       on player join
           broadcast "Welcome to the server!"
           set global server_players + 1
           if global server_players == 10
               broadcast "Server is getting full!"
           end
       end
   
   Supported statement keywords:
       on / when          → event binding
       set / give / take   → state manipulation
       say / whisper      → player output
       teleport / spawn   → world interaction
       if / else_if / end  → branching
       for / end          → looping
       broadcast          → server-wide announcements
       set_flag / clear_flag → unit flags
       spawn / despawn    → NPC / gameobject lifecycle
       timer / wait       → delays
       countdown          → timed announcements
   
   The engine is fully reloadable at runtime — edit a .wss file
   and type  reloadscripts  in the worldserver console.
   
   ─── Key Source Files ────────────────────────────────────────
   
   include/scripting/
     wss_engine.h       Engine public API + event dispatch
     wss_scanner.h      Lexer: text → tokens
     wss_value.h        Tagged union value type
     wss_objectstore.h  Persistent key-value game state store
     wss_scriptdb.h     Script file registry
     wss_chunk.h        Bytecode chunk + opcode enum
   
   src/scripting/
     wss_engine.c       Event dispatch + command registration
     wss_scanner.c      Lexer implementation
     wss_value.c        Value methods
     wss_objectstore.c  Hash-map storage
     wss_scriptdb.c     Script registry
     wss_chunk.c        Bytecode serialization
     wss_compiler.c     AST + bytecode compiler (recursive descent)
     wss_vm.c           Stack-based bytecode interpreter
   
   src/shared/
     Types.h            Shared header (from your open file)
     AuthSession.c      Authentication logic
     WorldSession.c     Player session + packet handlers
     ObjectAccessor.c    GUID → object pointer lookup
     ChatHandler.c      Command routing for players
     Log.h              Logging macros
   
   src/authserver/
     main.c             Entry point, listen loop
     AuthSocket.c       Accept connections, handle packets
     AuthDatabase.c     Account table CRUD
   
   src/worldserver/
     main.c             Entry point, listen loop
     WorldSocket.c      Accept connections, handle packets
     WorldDatabase.c    Character DB, world state DB
     MapMgr.c           Instance/map management
     WorldSession.c     Per-player session state
     Player.c           Player object update
     Unit.c             Unit base class (NPCs + players)
     GossipCommands.c   NPC gossip menu handling
     Battleground.c     BG scaffolding
     Arena.c            Arena scaffolding
*/

/* ─── Layout ─────────────────────────────────────────────────── */
/*
   authserver/          → TCP port 3724
   worldserver/         → TCP port 8085 (client)  + 8129 (auth link)
   scripts/core/        → core.wss, combat.wss, quests.wss, etc.
   scripts/events/      → custom per-event handler .wss files
   scripts/npc/        → NPC-specific .wss scripts (loaded by entry ID)
   sql/                 → auth + world SQL schema (full .sql files)
*/
