-- WoW 3.3.5a Database Schema (SQLite)
-- This defines the canonical data layout used by the emulator.

CREATE TABLE IF NOT EXISTS characters (
    guid            INTEGER PRIMARY KEY,
    name            TEXT NOT NULL,
    race            INTEGER NOT NULL,
    class           INTEGER NOT NULL,
    gender          INTEGER NOT NULL,
    level           INTEGER NOT NULL DEFAULT 1,
    experience      INTEGER NOT NULL DEFAULT 0,
    gold            INTEGER NOT NULL DEFAULT 0,
    map_id          INTEGER NOT NULL DEFAULT 0,
    zone_id         INTEGER NOT NULL DEFAULT 0,
    pos_x           REAL NOT NULL DEFAULT 0,
    pos_y           REAL NOT NULL DEFAULT 0,
    pos_z           REAL NOT NULL DEFAULT 0,
    facing          REAL NOT NULL DEFAULT 0,
    health          INTEGER NOT NULL DEFAULT 1,
    power           INTEGER NOT NULL DEFAULT 1,
    strength        INTEGER NOT NULL DEFAULT 20,
    agility         INTEGER NOT NULL DEFAULT 20,
    stamina         INTEGER NOT NULL DEFAULT 20,
    intellect       INTEGER NOT NULL DEFAULT 20,
    spirit          INTEGER NOT NULL DEFAULT 20,
    online          INTEGER NOT NULL DEFAULT 0,
    last_login      INTEGER NOT NULL DEFAULT 0,
    played_time     INTEGER NOT NULL DEFAULT 0,
    logout_time     INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS items (
    guid            INTEGER PRIMARY KEY AUTOINCREMENT,
    owner_guid      INTEGER NOT NULL,
    entry           INTEGER NOT NULL,
    slot            INTEGER NOT NULL,
    count           INTEGER NOT NULL DEFAULT 1,
    enchant_id      INTEGER NOT NULL DEFAULT 0,
    durability      INTEGER NOT NULL DEFAULT 100,
    charges         INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS world_objects (
    guid            INTEGER PRIMARY KEY,
    entry           INTEGER NOT NULL,
    name            TEXT NOT NULL,
    type            INTEGER NOT NULL,  -- 0=npc, 1=object
    model_id        INTEGER NOT NULL DEFAULT 0,
    map_id          INTEGER NOT NULL DEFAULT 0,
    pos_x           REAL NOT NULL DEFAULT 0,
    pos_y           REAL NOT NULL DEFAULT 0,
    pos_z           REAL NOT NULL DEFAULT 0,
    facing          REAL NOT NULL DEFAULT 0,
    faction         INTEGER NOT NULL DEFAULT 0,
    npc_flags       INTEGER NOT NULL DEFAULT 0,
    level           INTEGER NOT NULL DEFAULT 1,
    health          INTEGER NOT NULL DEFAULT 1,
    max_health      INTEGER NOT NULL DEFAULT 1,
    script_name     TEXT
);

CREATE TABLE IF NOT EXISTS quests (
    entry           INTEGER PRIMARY KEY,
    title           TEXT NOT NULL,
    description     TEXT,
    quest_flags     INTEGER NOT NULL DEFAULT 0,
    min_level       INTEGER NOT NULL DEFAULT 1,
    required_race   INTEGER NOT NULL DEFAULT 0,
    required_class  INTEGER NOT NULL DEFAULT 0,
    reward_gold     INTEGER NOT NULL DEFAULT 0,
    reward_xp       INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS quest_status (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    player_guid     INTEGER NOT NULL,
    quest_entry     INTEGER NOT NULL,
    status          INTEGER NOT NULL DEFAULT 0,  -- 0=not taken, 1=active, 2=complete
    time            INTEGER NOT NULL DEFAULT 0,
    UNIQUE(player_guid, quest_entry)
);

CREATE TABLE IF NOT EXISTS gossip_menus (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    npc_guid        INTEGER NOT NULL,
    text            TEXT NOT NULL,
    option_text     TEXT,
    next_menu_id    INTEGER
);

CREATE TABLE IF NOT EXISTS chat_commands (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    command         TEXT NOT NULL UNIQUE,
    permission_level INTEGER NOT NULL DEFAULT 0,
    handler_script  TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS server_config (
    key             TEXT PRIMARY KEY,
    value           TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS world_scripts (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    script_name     TEXT NOT NULL UNIQUE,
    source_code     TEXT NOT NULL,
    loaded_at       INTEGER NOT NULL DEFAULT 0,
    enabled         INTEGER NOT NULL DEFAULT 1
);

CREATE INDEX IF NOT EXISTS idx_char_name ON characters(name);
CREATE INDEX IF NOT EXISTS idx_char_online ON characters(online);
CREATE INDEX IF NOT EXISTS idx_items_owner ON items(owner_guid);
CREATE INDEX IF NOT EXISTS idx_quest_status_player ON quest_status(player_guid);