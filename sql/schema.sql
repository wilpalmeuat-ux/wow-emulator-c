-- WoW 3.3.5a Emulator SQL Schema (SQLite)

CREATE TABLE IF NOT EXISTS characters (
    guid INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    race INTEGER DEFAULT 1,
    class INTEGER DEFAULT 1,
    level INTEGER DEFAULT 1,
    xp INTEGER DEFAULT 0,
    money INTEGER DEFAULT 0,
    position_x REAL DEFAULT 0,
    position_y REAL DEFAULT 0,
    position_z REAL DEFAULT 0,
    orientation REAL DEFAULT 0,
    map_id INTEGER DEFAULT 0,
    zone_id INTEGER DEFAULT 0,
    account_id INTEGER DEFAULT 0,
    online INTEGER DEFAULT 0,
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
    banned INTEGER DEFAULT 0,
    ban_reason TEXT DEFAULT ''
);

CREATE TABLE IF NOT EXISTS creature_spawns (
    guid INTEGER PRIMARY KEY AUTOINCREMENT,
    id INTEGER DEFAULT 0,
    map INTEGER DEFAULT 0,
    position_x REAL DEFAULT 0,
    position_y REAL DEFAULT 0,
    position_z REAL DEFAULT 0,
    orientation REAL DEFAULT 0,
    spawntime_secs INTEGER DEFAULT 120
);

CREATE TABLE IF NOT EXISTS creature_templates (
    entry INTEGER PRIMARY KEY,
    name TEXT DEFAULT '',
    minlevel INTEGER DEFAULT 1,
    maxlevel INTEGER DEFAULT 1,
    health_min INTEGER DEFAULT 100,
    health_max INTEGER DEFAULT 100,
    mana_min INTEGER DEFAULT 0,
    mana_max INTEGER DEFAULT 0,
    faction INTEGER DEFAULT 0,
    scale REAL DEFAULT 1.0,
    display_id INTEGER DEFAULT 0,
    flags INTEGER DEFAULT 0,
    script_name TEXT DEFAULT ''
);

CREATE TABLE IF NOT EXISTS gameobject_spawns (
    guid INTEGER PRIMARY KEY AUTOINCREMENT,
    id INTEGER DEFAULT 0,
    map INTEGER DEFAULT 0,
    position_x REAL DEFAULT 0,
    position_y REAL DEFAULT 0,
    position_z REAL DEFAULT 0,
    orientation REAL DEFAULT 0,
    spawntime_secs INTEGER DEFAULT 120
);

CREATE TABLE IF NOT EXISTS world_state (
    var_name TEXT PRIMARY KEY,
    var_value TEXT
);

CREATE TABLE IF NOT EXISTS scripts (
    name TEXT PRIMARY KEY,
    code TEXT,
    loaded_at INTEGER DEFAULT (strftime('%s', 'now'))
);

-- Insert default account (test / test)
INSERT OR IGNORE INTO accounts (username, password_hash, email, expansion)
VALUES ('test', '5e884898da28047d165d9317a16d8b1a0d7c28b4', 'test@localhost', 2);

-- Insert some creature templates
INSERT OR IGNORE INTO creature_templates (entry, name, minlevel, maxlevel, health_min, health_max, faction, display_id, script_name) VALUES
(1, 'Orgrimmar Guard', 1, 5, 80, 100, 85, 1547, ''),
(15, 'Orgrimmar Grunt', 5, 10, 150, 200, 85, 1547, ''),
(50, 'Combat Trainer', 50, 55, 5000, 6000, 35, 357, ''),
(999, 'Stormwind Soldier', 10, 15, 300, 400, 0, 1547, ''),
(9999, 'World Boss', 80, 80, 500000, 500000, 14, 16946, 'boss_dragon'),
(1234, 'Kel''Thuzad', 80, 80, 1000000, 1000000, 14, 15928, 'kel_thuzad'),
(5678, 'Arcane Golem', 60, 65, 50000, 60000, 14, 16510, ''),
(9001, 'Dark Portal Guardian', 70, 75, 200000, 250000, 14, 15686, '');

-- Spawn creatures from templates
INSERT OR IGNORE INTO creature_spawns (id, map, position_x, position_y, position_z, orientation, spawntime_secs) VALUES
(1, 0, -8949.0, -132.0, 83.0, 0.0, 60),
(1, 0, -8945.0, -130.0, 83.0, 1.0, 60),
(15, 0, -8920.0, -140.0, 84.0, 2.0, 120),
(50, 0, -8900.0, -145.0, 85.0, 0.5, 300),
(999, 1, -10806.0, 284.0, 35.0, 0.0, 60),
(9999, 0, -8940.0, -120.0, 83.5, 0.0, 3600),
(1234, 0, -8945.0, -115.0, 83.0, 3.14, 3600),
(5678, 0, -8955.0, -125.0, 83.0, 1.5, 600),
(9001, 0, -8925.0, -135.0, 83.0, 0.0, 600);

-- Default world state
INSERT OR IGNORE INTO world_state (var_name, var_value) VALUES
('server_name', 'WoW 3.3.5a Custom Server'),
('max_players', '1000'),
('motd', 'Welcome to the WoW Emulator!');
