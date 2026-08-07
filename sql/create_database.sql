-- =====================================================
--  WoW 3.3.5a Emulator -- MySQL Database Schema
--  Run this file to set up the database from scratch:
--    mysql -u root -p < sql/create_database.sql
-- =====================================================

CREATE DATABASE IF NOT EXISTS `wow_emulator`
    CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci;
USE `wow_emulator`;

-- =====================================================
--  accounts -- Login credentials and account metadata
-- =====================================================
CREATE TABLE IF NOT EXISTS `accounts` (
    `id`           INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    `username`     VARCHAR(32)  NOT NULL UNIQUE,
    `salt`         CHAR(64)     NOT NULL DEFAULT '',
    `verifier`     CHAR(128)    NOT NULL DEFAULT '',
    `session_key`  CHAR(80)     NOT NULL DEFAULT '',
    `gmlevel`      TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `banned`       TINYINT      NOT NULL DEFAULT 0,
    `last_ip`      VARCHAR(45)  NOT NULL DEFAULT '',
    `last_login`   DATETIME     NULL,
    `created_at`   DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- =====================================================
--  characters -- Player characters
-- =====================================================
CREATE TABLE IF NOT EXISTS `characters` (
    `guid`         INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    `account_id`   INT UNSIGNED NOT NULL,
    `name`         VARCHAR(32)  NOT NULL,
    `race`         TINYINT UNSIGNED NOT NULL,
    `class`        TINYINT UNSIGNED NOT NULL,
    `gender`       TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `level`        SMALLINT UNSIGNED NOT NULL DEFAULT 1,
    `xp`           INT UNSIGNED NOT NULL DEFAULT 0,
    `money`        BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `skin`         TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `face`         TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `hairstyle`    TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `haircolor`    TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `facialstyle`  TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `pos_x`        FLOAT NOT NULL DEFAULT 0,
    `pos_y`        FLOAT NOT NULL DEFAULT 0,
    `pos_z`        FLOAT NOT NULL DEFAULT 0,
    `ori`          FLOAT NOT NULL DEFAULT 0,
    `map_id`       INT UNSIGNED NOT NULL DEFAULT 0,
    `zone_id`      INT UNSIGNED NOT NULL DEFAULT 0,
    `health`       INT NOT NULL DEFAULT 100,
    `mana`         INT NOT NULL DEFAULT 100,
    `online`       TINYINT NOT NULL DEFAULT 0,
    `flags`        INT UNSIGNED NOT NULL DEFAULT 0,
    INDEX `idx_account` (`account_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- =====================================================
--  creature_template -- NPC definitions
-- =====================================================
CREATE TABLE IF NOT EXISTS `creature_template` (
    `entry`        INT UNSIGNED NOT NULL PRIMARY KEY,
    `name`         VARCHAR(100) NOT NULL DEFAULT 'Unknown',
    `subname`      VARCHAR(100) NOT NULL DEFAULT '',
    `min_level`    TINYINT UNSIGNED NOT NULL DEFAULT 1,
    `max_level`    TINYINT UNSIGNED NOT NULL DEFAULT 1,
    `display_id`   INT UNSIGNED NOT NULL DEFAULT 0,
    `faction`      INT UNSIGNED NOT NULL DEFAULT 0,
    `health`       INT UNSIGNED NOT NULL DEFAULT 100,
    `mana`         INT UNSIGNED NOT NULL DEFAULT 0,
    `armor`        INT UNSIGNED NOT NULL DEFAULT 0,
    `attack_power` INT UNSIGNED NOT NULL DEFAULT 10,
    `damage_min`   FLOAT NOT NULL DEFAULT 1,
    `damage_max`   FLOAT NOT NULL DEFAULT 5,
    `speed_walk`   FLOAT NOT NULL DEFAULT 1.0,
    `speed_run`    FLOAT NOT NULL DEFAULT 1.14,
    `npc_flags`    INT UNSIGNED NOT NULL DEFAULT 0,
    `unit_flags`   INT UNSIGNED NOT NULL DEFAULT 0,
    `type`         TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `loot_id`      INT UNSIGNED NOT NULL DEFAULT 0,
    `script`       VARCHAR(64) NOT NULL DEFAULT ''
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- =====================================================
--  creature_spawns -- Where NPCs appear in the world
-- =====================================================
CREATE TABLE IF NOT EXISTS `creature_spawns` (
    `guid`         INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    `entry_id`     INT UNSIGNED NOT NULL,
    `map_id`       INT UNSIGNED NOT NULL DEFAULT 0,
    `pos_x`        FLOAT NOT NULL DEFAULT 0,
    `pos_y`        FLOAT NOT NULL DEFAULT 0,
    `pos_z`        FLOAT NOT NULL DEFAULT 0,
    `ori`          FLOAT NOT NULL DEFAULT 0,
    `spawntime`    INT UNSIGNED NOT NULL DEFAULT 300,
    INDEX `idx_map` (`map_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- =====================================================
--  gameobject_spawns -- World objects (chests, doors, etc.)
-- =====================================================
CREATE TABLE IF NOT EXISTS `gameobject_spawns` (
    `guid`         INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    `entry_id`     INT UNSIGNED NOT NULL,
    `map_id`       INT UNSIGNED NOT NULL DEFAULT 0,
    `pos_x`        FLOAT NOT NULL DEFAULT 0,
    `pos_y`        FLOAT NOT NULL DEFAULT 0,
    `pos_z`        FLOAT NOT NULL DEFAULT 0,
    `ori`          FLOAT NOT NULL DEFAULT 0,
    `rot0`         FLOAT NOT NULL DEFAULT 0,
    `rot1`         FLOAT NOT NULL DEFAULT 0,
    `rot2`         FLOAT NOT NULL DEFAULT 0,
    `rot3`         FLOAT NOT NULL DEFAULT 0,
    `state`        INT UNSIGNED NOT NULL DEFAULT 1,
    INDEX `idx_map` (`map_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- =====================================================
--  world_state -- Key/value store for server variables
-- =====================================================
CREATE TABLE IF NOT EXISTS `world_state` (
    `var_name`     VARCHAR(64) NOT NULL PRIMARY KEY,
    `value`        VARCHAR(255) NOT NULL DEFAULT ''
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- =====================================================
--  realmlist -- Available realms
-- =====================================================
CREATE TABLE IF NOT EXISTS `realmlist` (
    `id`           INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    `name`         VARCHAR(64)  NOT NULL DEFAULT 'WoW Emulator',
    `address`      VARCHAR(128) NOT NULL DEFAULT '127.0.0.1',
    `port`         INT UNSIGNED NOT NULL DEFAULT 8085,
    `type`         TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `flags`        TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `population`   FLOAT NOT NULL DEFAULT 0,
    `timezone`     TINYINT UNSIGNED NOT NULL DEFAULT 1
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- =====================================================
--  item_template -- Item definitions (stub)
-- =====================================================
CREATE TABLE IF NOT EXISTS `item_template` (
    `entry`        INT UNSIGNED NOT NULL PRIMARY KEY,
    `name`         VARCHAR(100) NOT NULL DEFAULT 'Unknown Item',
    `display_id`   INT UNSIGNED NOT NULL DEFAULT 0,
    `quality`      TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `inventory_type` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `item_level`   SMALLINT UNSIGNED NOT NULL DEFAULT 1,
    `required_level` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `stat_type1`   TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `stat_value1`  INT NOT NULL DEFAULT 0,
    `armor`        INT UNSIGNED NOT NULL DEFAULT 0,
    `damage_min`   FLOAT NOT NULL DEFAULT 0,
    `damage_max`   FLOAT NOT NULL DEFAULT 0,
    `buy_price`    INT UNSIGNED NOT NULL DEFAULT 0,
    `sell_price`   INT UNSIGNED NOT NULL DEFAULT 0,
    `max_count`    INT UNSIGNED NOT NULL DEFAULT 1
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- =====================================================
--  spell_template -- Spell definitions (stub)
-- =====================================================
CREATE TABLE IF NOT EXISTS `spell_template` (
    `id`           INT UNSIGNED NOT NULL PRIMARY KEY,
    `name`         VARCHAR(100) NOT NULL DEFAULT 'Unknown Spell',
    `description`  TEXT,
    `power_cost`   INT UNSIGNED NOT NULL DEFAULT 0,
    `cooldown`     INT UNSIGNED NOT NULL DEFAULT 0,
    `range_min`    FLOAT NOT NULL DEFAULT 0,
    `range_max`    FLOAT NOT NULL DEFAULT 30,
    `cast_time`    INT UNSIGNED NOT NULL DEFAULT 0,
    `effect_type`  TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `effect_value` INT NOT NULL DEFAULT 0
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- =====================================================
--  Default data
-- =====================================================

-- Default realm
INSERT IGNORE INTO `realmlist` (`id`, `name`, `address`, `port`)
VALUES (1, 'WoW 3.3.5a Emulator', '127.0.0.1', 8085);

-- Default test account (username: TEST, no password)
INSERT IGNORE INTO `accounts` (`id`, `username`, `gmlevel`)
VALUES (1, 'TEST', 3);

-- Sample creature templates
INSERT IGNORE INTO `creature_template` (`entry`, `name`, `subname`, `min_level`, `max_level`, `health`, `faction`, `display_id`) VALUES
(1,    'Stormwind Guard',       'City Guard',    60, 60, 50000, 11,  3167),
(15,   'Orgrimmar Grunt',       'Grunt',         60, 60, 45000, 29,  4590),
(50,   'Combat Trainer',        'Trainer',       60, 60, 99999, 35,  5887),
(999,  'Stormwind Soldier',     'Alliance',      40, 42, 10000, 11,  2765),
(1234, 'Kel''Thuzad',           'Archlich',      83, 83, 999999, 21, 15945),
(5678, 'Arcane Golem',          'Construct',     72, 72, 80000, 14,  17612),
(9001, 'Portal Guardian',       'Ethereal',      80, 80, 150000, 35, 17612),
(9999, 'World Boss',            'Raid Boss',     83, 83, 9999999, 14, 11686);

-- Sample creature spawns
INSERT IGNORE INTO `creature_spawns` (`guid`, `entry_id`, `map_id`, `pos_x`, `pos_y`, `pos_z`, `ori`) VALUES
(1, 1,    0, -8949.0, -132.0, 83.0, 0.0),
(2, 1,    0, -8945.0, -130.0, 83.0, 1.0),
(3, 15,   0, -8920.0, -140.0, 84.0, 2.0),
(4, 50,   0, -8900.0, -145.0, 85.0, 0.5),
(5, 999,  1, -10806.0, 284.0, 35.0, 0.0);

-- Sample items
INSERT IGNORE INTO `item_template` (`entry`, `name`, `quality`, `inventory_type`, `item_level`) VALUES
(49623, 'Shadowmourne',           5, 17, 284),
(50000, 'Server Token',           4, 0,  1),
(50001, 'Welcome Gift',           2, 0,  1);
