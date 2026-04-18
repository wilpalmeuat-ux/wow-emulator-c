-- WoW 3.3.5a Emulator MySQL Database Setup (Windows)
-- Run this from MySQL Command Line or MySQL Workbench:
--   mysql -u root -p < create_db.sql
-- Or import via MySQL Workbench / HeidiSQL

CREATE DATABASE IF NOT EXISTS wow_emulator CHARACTER SET utf8 COLLATE utf8_general_ci;
USE wow_emulator;

-- ── Accounts ──────────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS accounts (
    id           INT UNSIGNED    NOT NULL AUTO_INCREMENT PRIMARY KEY,
    username     VARCHAR(32)     NOT NULL UNIQUE,
    sha_pass_hash CHAR(40)        NOT NULL,
    gm_level     TINYINT UNSIGNED NOT NULL DEFAULT 0,
    last_ip      VARCHAR(32)      DEFAULT '127.0.0.1',
    locked       INT UNSIGNED     NOT NULL DEFAULT 0,
    created_at   DATETIME         NOT NULL DEFAULT CURRENT_TIMESTAMP,
    last_login   DATETIME,
    expansion    TINYINT UNSIGNED NOT NULL DEFAULT 3,
    INDEX idx_username (username)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- ── Characters ────────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS characters (
    guid         INT UNSIGNED    NOT NULL AUTO_INCREMENT PRIMARY KEY,
    account_id   INT UNSIGNED    NOT NULL,
    name         VARCHAR(32)    NOT NULL,
    race         INT UNSIGNED    NOT NULL,
    class        INT UNSIGNED    NOT NULL,
    level        INT UNSIGNED    NOT NULL DEFAULT 1,
    xp           INT UNSIGNED    NOT NULL DEFAULT 0,
    money        BIGINT UNSIGNED NOT NULL DEFAULT 0,
    pos_x        FLOAT          NOT NULL DEFAULT 0,
    pos_y        FLOAT          NOT NULL DEFAULT 0,
    pos_z        FLOAT          NOT NULL DEFAULT 0,
    ori          FLOAT          NOT NULL DEFAULT 0,
    map_id       INT UNSIGNED    NOT NULL DEFAULT 0,
    zone_id      INT UNSIGNED    NOT NULL DEFAULT 0,
    health       INT UNSIGNED    NOT NULL DEFAULT 100,
    mana         INT UNSIGNED    NOT NULL DEFAULT 100,
    online       TINYINT         NOT NULL DEFAULT 0,
    create_time  DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (account_id) REFERENCES accounts(id) ON DELETE CASCADE,
    INDEX idx_account (account_id),
    INDEX idx_name (name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- ── Creature Spawns ────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS creature_spawns (
    guid            INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    entry_id        INT UNSIGNED NOT NULL,
    map_id          INT UNSIGNED NOT NULL,
    pos_x           FLOAT       NOT NULL,
    pos_y           FLOAT       NOT NULL,
    pos_z           FLOAT       NOT NULL,
    ori             FLOAT       NOT NULL,
    spawntime_secs  INT UNSIGNED NOT NULL DEFAULT 300,
    INDEX idx_map (map_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- ── GameObject Spawns ───────────────────────────────────────────
CREATE TABLE IF NOT EXISTS gameobject_spawns (
    guid     INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    entry_id INT UNSIGNED NOT NULL,
    map_id   INT UNSIGNED NOT NULL,
    pos_x    FLOAT       NOT NULL,
    pos_y    FLOAT       NOT NULL,
    pos_z    FLOAT       NOT NULL,
    ori      FLOAT       NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- ── World State ────────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS world_state (
    var_name VARCHAR(64) NOT NULL PRIMARY KEY,
    value    VARCHAR(255) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- ── Default Test Account ────────────────────────────────────────
-- Username: test  /  Password: testpassword
-- (This is a placeholder hash — in production compute proper SRP6 verifier)
INSERT INTO accounts (username, sha_pass_hash, gm_level) VALUES
    ('test', '6F4728A4B6F21A1D4E60E5D5F5F5A5B5C5D5E5F5', 3),
    ('admin', '8A9B0C1D2E3F4A5B6C7D8E9F0A1B2C3D4E5F6A7B', 4);

-- ── Default Creatures ──────────────────────────────────────────
-- Orgrimmar (map=0) and Stormwind (map=1) spawns
INSERT INTO creature_spawns (entry_id, map_id, pos_x, pos_y, pos_z, ori) VALUES
    (1,    0,  -8949.0,  -132.0,   83.0,  0.0),   -- Orgrimmar Guard
    (1,    0,  -8945.0,  -130.0,   83.0,  1.0),   -- Orgrimmar Guard
    (15,   0,  -8920.0,  -140.0,   84.0,  2.0),   -- Orgrimmar Grunt
    (50,   0,  -8900.0,  -145.0,   85.0,  0.5),   -- Combat Trainer
    (999,  1, -10806.0,  284.0,    35.0,  0.0),   -- Stormwind Soldier
    (9999, 0,  -8940.0,  -120.0,   83.5,  0.0),   -- World Boss
    (1234, 0,  -8945.0,  -115.0,   83.0,  3.14),  -- Kel'Thuzad
    (5678, 0,  -8955.0,  -125.0,   83.0,  1.5),   -- Arcane Golem
    (9001, 0,  -8925.0,  -135.0,   83.0,  0.0);  -- Portal Guardian
