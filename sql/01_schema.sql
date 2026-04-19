-- =====================================================
--  WoW 3.3.5a Emulator -- Complete Database Schema
--  Part 1: All table definitions
-- =====================================================

CREATE DATABASE IF NOT EXISTS `wow_emulator`
    CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci;
USE `wow_emulator`;

-- =====================================================
--  ACCOUNT / AUTH TABLES
-- =====================================================

CREATE TABLE IF NOT EXISTS `accounts` (
    `id`           INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    `username`     VARCHAR(32)  NOT NULL UNIQUE,
    `salt`         CHAR(64)     NOT NULL DEFAULT '',
    `verifier`     CHAR(128)    NOT NULL DEFAULT '',
    `session_key`  CHAR(80)     NOT NULL DEFAULT '',
    `gmlevel`      TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `banned`       TINYINT      NOT NULL DEFAULT 0,
    `ban_reason`   VARCHAR(255) NOT NULL DEFAULT '',
    `ban_expires`  DATETIME     NULL,
    `last_ip`      VARCHAR(45)  NOT NULL DEFAULT '',
    `last_login`   DATETIME     NULL,
    `locale`       TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `os`           VARCHAR(16)  NOT NULL DEFAULT '',
    `created_at`   DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `realmlist` (
    `id`           INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    `name`         VARCHAR(64)  NOT NULL DEFAULT 'WoW Emulator',
    `address`      VARCHAR(128) NOT NULL DEFAULT '127.0.0.1',
    `port`         INT UNSIGNED NOT NULL DEFAULT 8085,
    `type`         TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `flags`        TINYINT UNSIGNED NOT NULL DEFAULT 2,
    `population`   FLOAT NOT NULL DEFAULT 0,
    `timezone`     TINYINT UNSIGNED NOT NULL DEFAULT 1,
    `allowed_builds` VARCHAR(64) NOT NULL DEFAULT '12340'
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `account_banned` (
    `id`           INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    `account_id`   INT UNSIGNED NOT NULL,
    `banned_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `expires_at`   DATETIME NULL,
    `banned_by`    VARCHAR(64) NOT NULL DEFAULT 'System',
    `reason`       VARCHAR(255) NOT NULL DEFAULT '',
    `active`       TINYINT NOT NULL DEFAULT 1,
    INDEX `idx_account` (`account_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `ip_banned` (
    `ip`           VARCHAR(45) NOT NULL PRIMARY KEY,
    `banned_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `expires_at`   DATETIME NULL,
    `reason`       VARCHAR(255) NOT NULL DEFAULT ''
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- =====================================================
--  CHARACTER TABLES
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
    `at_login`     INT UNSIGNED NOT NULL DEFAULT 0,
    `played_time`  INT UNSIGNED NOT NULL DEFAULT 0,
    `rest_state`   TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `taxi_mask`    TEXT,
    `talent_spec`  TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `stable_slots` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `death_expire` INT UNSIGNED NOT NULL DEFAULT 0,
    `extra_flags`  INT UNSIGNED NOT NULL DEFAULT 0,
    `created_at`   DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    INDEX `idx_account` (`account_id`),
    INDEX `idx_name` (`name`),
    INDEX `idx_online` (`online`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `character_inventory` (
    `id`           INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    `guid`         INT UNSIGNED NOT NULL,
    `bag`          TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `slot`         TINYINT UNSIGNED NOT NULL,
    `item_guid`    INT UNSIGNED NOT NULL,
    `item_entry`   INT UNSIGNED NOT NULL,
    `stack_count`  INT UNSIGNED NOT NULL DEFAULT 1,
    `durability`   INT NOT NULL DEFAULT -1,
    `enchantments` VARCHAR(255) NOT NULL DEFAULT '',
    `flags`        INT UNSIGNED NOT NULL DEFAULT 0,
    INDEX `idx_guid` (`guid`),
    UNIQUE KEY `unique_slot` (`guid`, `bag`, `slot`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `character_spells` (
    `guid`         INT UNSIGNED NOT NULL,
    `spell_id`     INT UNSIGNED NOT NULL,
    `active`       TINYINT NOT NULL DEFAULT 1,
    PRIMARY KEY (`guid`, `spell_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `character_talents` (
    `guid`         INT UNSIGNED NOT NULL,
    `talent_id`    INT UNSIGNED NOT NULL,
    `current_rank` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `spec`         TINYINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`guid`, `talent_id`, `spec`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `character_quests` (
    `guid`         INT UNSIGNED NOT NULL,
    `quest_id`     INT UNSIGNED NOT NULL,
    `status`       TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `explored`     TINYINT NOT NULL DEFAULT 0,
    `timer`        INT UNSIGNED NOT NULL DEFAULT 0,
    `mob_count1`   INT UNSIGNED NOT NULL DEFAULT 0,
    `mob_count2`   INT UNSIGNED NOT NULL DEFAULT 0,
    `mob_count3`   INT UNSIGNED NOT NULL DEFAULT 0,
    `mob_count4`   INT UNSIGNED NOT NULL DEFAULT 0,
    `item_count1`  INT UNSIGNED NOT NULL DEFAULT 0,
    `item_count2`  INT UNSIGNED NOT NULL DEFAULT 0,
    `item_count3`  INT UNSIGNED NOT NULL DEFAULT 0,
    `item_count4`  INT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`guid`, `quest_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `character_achievements` (
    `guid`         INT UNSIGNED NOT NULL,
    `achievement`  INT UNSIGNED NOT NULL,
    `date`         INT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`guid`, `achievement`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `character_skills` (
    `guid`         INT UNSIGNED NOT NULL,
    `skill_id`     INT UNSIGNED NOT NULL,
    `value`        INT UNSIGNED NOT NULL DEFAULT 0,
    `max_value`    INT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`guid`, `skill_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `character_auras` (
    `guid`         INT UNSIGNED NOT NULL,
    `spell_id`     INT UNSIGNED NOT NULL,
    `remaining`    INT UNSIGNED NOT NULL DEFAULT 0,
    `stacks`       INT UNSIGNED NOT NULL DEFAULT 1,
    INDEX `idx_guid` (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `character_social` (
    `guid`         INT UNSIGNED NOT NULL,
    `friend_guid`  INT UNSIGNED NOT NULL,
    `flags`        TINYINT UNSIGNED NOT NULL DEFAULT 1,
    `note`         VARCHAR(128) NOT NULL DEFAULT '',
    PRIMARY KEY (`guid`, `friend_guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- =====================================================
--  GUILD TABLES
-- =====================================================

CREATE TABLE IF NOT EXISTS `guilds` (
    `id`           INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    `name`         VARCHAR(64)  NOT NULL UNIQUE,
    `leader_guid`  INT UNSIGNED NOT NULL,
    `motd`         VARCHAR(256) NOT NULL DEFAULT '',
    `info`         TEXT,
    `created_at`   DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `bank_money`   BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `emblem_style` INT NOT NULL DEFAULT 0,
    `emblem_color` INT NOT NULL DEFAULT 0,
    `border_style` INT NOT NULL DEFAULT 0,
    `border_color` INT NOT NULL DEFAULT 0,
    `bg_color`     INT NOT NULL DEFAULT 0
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `guild_members` (
    `guild_id`     INT UNSIGNED NOT NULL,
    `guid`         INT UNSIGNED NOT NULL,
    `rank_id`      TINYINT UNSIGNED NOT NULL DEFAULT 4,
    `public_note`  VARCHAR(128) NOT NULL DEFAULT '',
    `officer_note` VARCHAR(128) NOT NULL DEFAULT '',
    PRIMARY KEY (`guild_id`, `guid`),
    INDEX `idx_guid` (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `guild_ranks` (
    `guild_id`     INT UNSIGNED NOT NULL,
    `rank_id`      TINYINT UNSIGNED NOT NULL,
    `name`         VARCHAR(32) NOT NULL,
    `rights`       INT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`guild_id`, `rank_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- =====================================================
--  MAIL TABLE
-- =====================================================

CREATE TABLE IF NOT EXISTS `mail` (
    `id`           INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    `sender_guid`  INT UNSIGNED NOT NULL,
    `receiver_guid` INT UNSIGNED NOT NULL,
    `subject`      VARCHAR(128) NOT NULL DEFAULT '',
    `body`         TEXT,
    `money`        BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `cod`          BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `has_items`    TINYINT NOT NULL DEFAULT 0,
    `sent_time`    INT UNSIGNED NOT NULL,
    `expire_time`  INT UNSIGNED NOT NULL,
    `checked`      TINYINT NOT NULL DEFAULT 0,
    INDEX `idx_receiver` (`receiver_guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `mail_items` (
    `mail_id`      INT UNSIGNED NOT NULL,
    `item_guid`    INT UNSIGNED NOT NULL,
    `item_entry`   INT UNSIGNED NOT NULL,
    `stack_count`  INT UNSIGNED NOT NULL DEFAULT 1,
    INDEX `idx_mail` (`mail_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- =====================================================
--  AUCTION TABLE
-- =====================================================

CREATE TABLE IF NOT EXISTS `auctionhouse` (
    `id`           INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    `house_id`     TINYINT UNSIGNED NOT NULL DEFAULT 7,
    `owner_guid`   INT UNSIGNED NOT NULL,
    `item_entry`   INT UNSIGNED NOT NULL,
    `item_count`   INT UNSIGNED NOT NULL DEFAULT 1,
    `start_bid`    BIGINT UNSIGNED NOT NULL,
    `buyout`       BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `current_bid`  BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `bidder_guid`  INT UNSIGNED NOT NULL DEFAULT 0,
    `deposit`      BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `start_time`   INT UNSIGNED NOT NULL,
    `expire_time`  INT UNSIGNED NOT NULL,
    INDEX `idx_owner` (`owner_guid`),
    INDEX `idx_house` (`house_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- =====================================================
--  WORLD TABLES
-- =====================================================

CREATE TABLE IF NOT EXISTS `creature_template` (
    `entry`        INT UNSIGNED NOT NULL PRIMARY KEY,
    `name`         VARCHAR(100) NOT NULL DEFAULT 'Unknown',
    `subname`      VARCHAR(100) NOT NULL DEFAULT '',
    `min_level`    TINYINT UNSIGNED NOT NULL DEFAULT 1,
    `max_level`    TINYINT UNSIGNED NOT NULL DEFAULT 1,
    `display_id`   INT UNSIGNED NOT NULL DEFAULT 0,
    `display_id2`  INT UNSIGNED NOT NULL DEFAULT 0,
    `display_id3`  INT UNSIGNED NOT NULL DEFAULT 0,
    `display_id4`  INT UNSIGNED NOT NULL DEFAULT 0,
    `faction`      INT UNSIGNED NOT NULL DEFAULT 0,
    `health`       INT UNSIGNED NOT NULL DEFAULT 100,
    `mana`         INT UNSIGNED NOT NULL DEFAULT 0,
    `armor`        INT UNSIGNED NOT NULL DEFAULT 0,
    `attack_power` INT UNSIGNED NOT NULL DEFAULT 10,
    `ranged_attack_power` INT UNSIGNED NOT NULL DEFAULT 0,
    `damage_min`   FLOAT NOT NULL DEFAULT 1,
    `damage_max`   FLOAT NOT NULL DEFAULT 5,
    `speed_walk`   FLOAT NOT NULL DEFAULT 1.0,
    `speed_run`    FLOAT NOT NULL DEFAULT 1.14,
    `npc_flags`    INT UNSIGNED NOT NULL DEFAULT 0,
    `unit_flags`   INT UNSIGNED NOT NULL DEFAULT 0,
    `unit_flags2`  INT UNSIGNED NOT NULL DEFAULT 0,
    `dynamic_flags` INT UNSIGNED NOT NULL DEFAULT 0,
    `type`         TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `type_flags`   INT UNSIGNED NOT NULL DEFAULT 0,
    `rank`         TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `family`       TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `trainer_type` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `trainer_spell` INT UNSIGNED NOT NULL DEFAULT 0,
    `trainer_class` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `loot_id`      INT UNSIGNED NOT NULL DEFAULT 0,
    `skinning_loot_id` INT UNSIGNED NOT NULL DEFAULT 0,
    `pickpocket_loot_id` INT UNSIGNED NOT NULL DEFAULT 0,
    `gold_min`     INT UNSIGNED NOT NULL DEFAULT 0,
    `gold_max`     INT UNSIGNED NOT NULL DEFAULT 0,
    `mechanic_immune_mask` INT UNSIGNED NOT NULL DEFAULT 0,
    `resistance_holy` INT NOT NULL DEFAULT 0,
    `resistance_fire` INT NOT NULL DEFAULT 0,
    `resistance_nature` INT NOT NULL DEFAULT 0,
    `resistance_frost` INT NOT NULL DEFAULT 0,
    `resistance_shadow` INT NOT NULL DEFAULT 0,
    `resistance_arcane` INT NOT NULL DEFAULT 0,
    `script`       VARCHAR(64) NOT NULL DEFAULT '',
    `ai_name`      VARCHAR(64) NOT NULL DEFAULT ''
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `creature_spawns` (
    `guid`         INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    `entry_id`     INT UNSIGNED NOT NULL,
    `map_id`       INT UNSIGNED NOT NULL DEFAULT 0,
    `pos_x`        FLOAT NOT NULL DEFAULT 0,
    `pos_y`        FLOAT NOT NULL DEFAULT 0,
    `pos_z`        FLOAT NOT NULL DEFAULT 0,
    `ori`          FLOAT NOT NULL DEFAULT 0,
    `spawntime`    INT UNSIGNED NOT NULL DEFAULT 300,
    `phase_mask`   INT UNSIGNED NOT NULL DEFAULT 1,
    `movement_type` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `spawn_dist`   FLOAT NOT NULL DEFAULT 0,
    INDEX `idx_map` (`map_id`),
    INDEX `idx_entry` (`entry_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `creature_waypoints` (
    `entry_id`     INT UNSIGNED NOT NULL,
    `point_id`     INT UNSIGNED NOT NULL,
    `pos_x`        FLOAT NOT NULL,
    `pos_y`        FLOAT NOT NULL,
    `pos_z`        FLOAT NOT NULL,
    `wait_time`    INT UNSIGNED NOT NULL DEFAULT 0,
    `emote`        INT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`entry_id`, `point_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

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
    `phase_mask`   INT UNSIGNED NOT NULL DEFAULT 1,
    INDEX `idx_map` (`map_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `gameobject_template` (
    `entry`        INT UNSIGNED NOT NULL PRIMARY KEY,
    `type`         TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `display_id`   INT UNSIGNED NOT NULL DEFAULT 0,
    `name`         VARCHAR(100) NOT NULL DEFAULT '',
    `cast_bar_caption` VARCHAR(100) NOT NULL DEFAULT '',
    `faction`      INT UNSIGNED NOT NULL DEFAULT 0,
    `flags`        INT UNSIGNED NOT NULL DEFAULT 0,
    `size`         FLOAT NOT NULL DEFAULT 1,
    `data0`        INT NOT NULL DEFAULT 0,
    `data1`        INT NOT NULL DEFAULT 0,
    `data2`        INT NOT NULL DEFAULT 0,
    `data3`        INT NOT NULL DEFAULT 0,
    `script`       VARCHAR(64) NOT NULL DEFAULT ''
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- =====================================================
--  ITEM TABLES
-- =====================================================

CREATE TABLE IF NOT EXISTS `item_template` (
    `entry`        INT UNSIGNED NOT NULL PRIMARY KEY,
    `name`         VARCHAR(100) NOT NULL DEFAULT 'Unknown Item',
    `display_id`   INT UNSIGNED NOT NULL DEFAULT 0,
    `quality`      TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `flags`        INT UNSIGNED NOT NULL DEFAULT 0,
    `buy_count`    INT UNSIGNED NOT NULL DEFAULT 1,
    `buy_price`    BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `sell_price`   BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `inventory_type` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `allowable_class` INT NOT NULL DEFAULT -1,
    `allowable_race` INT NOT NULL DEFAULT -1,
    `item_level`   SMALLINT UNSIGNED NOT NULL DEFAULT 1,
    `required_level` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `required_skill` INT UNSIGNED NOT NULL DEFAULT 0,
    `required_skill_rank` INT UNSIGNED NOT NULL DEFAULT 0,
    `max_count`    INT UNSIGNED NOT NULL DEFAULT 0,
    `stackable`    INT UNSIGNED NOT NULL DEFAULT 1,
    `container_slots` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `stat_type1`   TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `stat_value1`  INT NOT NULL DEFAULT 0,
    `stat_type2`   TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `stat_value2`  INT NOT NULL DEFAULT 0,
    `stat_type3`   TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `stat_value3`  INT NOT NULL DEFAULT 0,
    `stat_type4`   TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `stat_value4`  INT NOT NULL DEFAULT 0,
    `stat_type5`   TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `stat_value5`  INT NOT NULL DEFAULT 0,
    `armor`        INT UNSIGNED NOT NULL DEFAULT 0,
    `dmg_min1`     FLOAT NOT NULL DEFAULT 0,
    `dmg_max1`     FLOAT NOT NULL DEFAULT 0,
    `dmg_type1`    TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `delay`        INT UNSIGNED NOT NULL DEFAULT 0,
    `bonding`      TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `description`  TEXT,
    `page_text`    INT UNSIGNED NOT NULL DEFAULT 0,
    `start_quest`  INT UNSIGNED NOT NULL DEFAULT 0,
    `spell_id_1`   INT UNSIGNED NOT NULL DEFAULT 0,
    `spell_trigger_1` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `spell_charges_1` INT NOT NULL DEFAULT 0,
    `spell_cooldown_1` INT NOT NULL DEFAULT -1,
    `material`     TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `sheath`       TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `max_durability` INT UNSIGNED NOT NULL DEFAULT 0,
    `socket_color_1` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `socket_color_2` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `socket_color_3` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `socket_bonus`  INT UNSIGNED NOT NULL DEFAULT 0,
    `gem_properties` INT UNSIGNED NOT NULL DEFAULT 0,
    `disenchant_id` INT UNSIGNED NOT NULL DEFAULT 0
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- =====================================================
--  QUEST TABLES
-- =====================================================

CREATE TABLE IF NOT EXISTS `quest_template` (
    `id`           INT UNSIGNED NOT NULL PRIMARY KEY,
    `title`        VARCHAR(128) NOT NULL DEFAULT '',
    `details`      TEXT,
    `objectives`   TEXT,
    `completion_text` TEXT,
    `offer_reward_text` TEXT,
    `min_level`    TINYINT UNSIGNED NOT NULL DEFAULT 1,
    `quest_level`  SMALLINT NOT NULL DEFAULT -1,
    `type`         INT UNSIGNED NOT NULL DEFAULT 0,
    `required_races` INT NOT NULL DEFAULT 0,
    `required_classes` INT NOT NULL DEFAULT 0,
    `suggested_players` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `time_limit`   INT UNSIGNED NOT NULL DEFAULT 0,
    `flags`        INT UNSIGNED NOT NULL DEFAULT 0,
    `reward_xp`    INT UNSIGNED NOT NULL DEFAULT 0,
    `reward_money` INT NOT NULL DEFAULT 0,
    `reward_money_at_max` INT UNSIGNED NOT NULL DEFAULT 0,
    `reward_spell` INT UNSIGNED NOT NULL DEFAULT 0,
    `reward_honor` INT UNSIGNED NOT NULL DEFAULT 0,
    `reward_title` INT UNSIGNED NOT NULL DEFAULT 0,
    `reward_item1` INT UNSIGNED NOT NULL DEFAULT 0,
    `reward_item1_count` INT UNSIGNED NOT NULL DEFAULT 0,
    `reward_item2` INT UNSIGNED NOT NULL DEFAULT 0,
    `reward_item2_count` INT UNSIGNED NOT NULL DEFAULT 0,
    `reward_item3` INT UNSIGNED NOT NULL DEFAULT 0,
    `reward_item3_count` INT UNSIGNED NOT NULL DEFAULT 0,
    `reward_item4` INT UNSIGNED NOT NULL DEFAULT 0,
    `reward_item4_count` INT UNSIGNED NOT NULL DEFAULT 0,
    `req_creature1` INT UNSIGNED NOT NULL DEFAULT 0,
    `req_creature1_count` INT UNSIGNED NOT NULL DEFAULT 0,
    `req_creature2` INT UNSIGNED NOT NULL DEFAULT 0,
    `req_creature2_count` INT UNSIGNED NOT NULL DEFAULT 0,
    `req_creature3` INT UNSIGNED NOT NULL DEFAULT 0,
    `req_creature3_count` INT UNSIGNED NOT NULL DEFAULT 0,
    `req_creature4` INT UNSIGNED NOT NULL DEFAULT 0,
    `req_creature4_count` INT UNSIGNED NOT NULL DEFAULT 0,
    `req_item1`    INT UNSIGNED NOT NULL DEFAULT 0,
    `req_item1_count` INT UNSIGNED NOT NULL DEFAULT 0,
    `req_item2`    INT UNSIGNED NOT NULL DEFAULT 0,
    `req_item2_count` INT UNSIGNED NOT NULL DEFAULT 0,
    `quest_giver`  INT UNSIGNED NOT NULL DEFAULT 0,
    `quest_ender`  INT UNSIGNED NOT NULL DEFAULT 0,
    `prev_quest`   INT NOT NULL DEFAULT 0,
    `next_quest`   INT UNSIGNED NOT NULL DEFAULT 0,
    `zone_or_sort` INT NOT NULL DEFAULT 0,
    `source_item`  INT UNSIGNED NOT NULL DEFAULT 0,
    `source_item_count` INT UNSIGNED NOT NULL DEFAULT 0
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- =====================================================
--  LOOT TABLES
-- =====================================================

CREATE TABLE IF NOT EXISTS `creature_loot` (
    `entry`        INT UNSIGNED NOT NULL,
    `item_entry`   INT UNSIGNED NOT NULL,
    `chance`       FLOAT NOT NULL DEFAULT 0,
    `quest_required` TINYINT NOT NULL DEFAULT 0,
    `min_count`    INT UNSIGNED NOT NULL DEFAULT 1,
    `max_count`    INT UNSIGNED NOT NULL DEFAULT 1,
    `group_id`     TINYINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`entry`, `item_entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `gameobject_loot` (
    `entry`        INT UNSIGNED NOT NULL,
    `item_entry`   INT UNSIGNED NOT NULL,
    `chance`       FLOAT NOT NULL DEFAULT 0,
    `min_count`    INT UNSIGNED NOT NULL DEFAULT 1,
    `max_count`    INT UNSIGNED NOT NULL DEFAULT 1,
    PRIMARY KEY (`entry`, `item_entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `skinning_loot` (
    `entry`        INT UNSIGNED NOT NULL,
    `item_entry`   INT UNSIGNED NOT NULL,
    `chance`       FLOAT NOT NULL DEFAULT 0,
    `min_count`    INT UNSIGNED NOT NULL DEFAULT 1,
    `max_count`    INT UNSIGNED NOT NULL DEFAULT 1,
    PRIMARY KEY (`entry`, `item_entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- =====================================================
--  VENDOR / TRAINER / GOSSIP TABLES
-- =====================================================

CREATE TABLE IF NOT EXISTS `npc_vendor` (
    `entry`        INT UNSIGNED NOT NULL,
    `item_entry`   INT UNSIGNED NOT NULL,
    `max_count`    INT UNSIGNED NOT NULL DEFAULT 0,
    `incrtime`     INT UNSIGNED NOT NULL DEFAULT 0,
    `extended_cost` INT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`entry`, `item_entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `npc_trainer` (
    `entry`        INT UNSIGNED NOT NULL,
    `spell_id`     INT UNSIGNED NOT NULL,
    `spell_cost`   INT UNSIGNED NOT NULL DEFAULT 0,
    `required_level` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `required_skill` INT UNSIGNED NOT NULL DEFAULT 0,
    `required_skill_value` INT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`entry`, `spell_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `gossip_menu` (
    `entry`        INT UNSIGNED NOT NULL,
    `text_id`      INT UNSIGNED NOT NULL DEFAULT 0,
    `condition_id` INT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`entry`, `text_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `gossip_menu_option` (
    `menu_id`      INT UNSIGNED NOT NULL,
    `option_id`    TINYINT UNSIGNED NOT NULL,
    `option_icon`  TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `option_text`  VARCHAR(128) NOT NULL DEFAULT '',
    `action_menu_id` INT NOT NULL DEFAULT 0,
    `npc_option_npcflag` INT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`menu_id`, `option_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `npc_text` (
    `id`           INT UNSIGNED NOT NULL PRIMARY KEY,
    `text0_0`      TEXT,
    `text0_1`      TEXT,
    `probability0` FLOAT NOT NULL DEFAULT 1
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- =====================================================
--  SPELL / TALENT / PROFESSION TABLES
-- =====================================================

CREATE TABLE IF NOT EXISTS `spell_template` (
    `id`           INT UNSIGNED NOT NULL PRIMARY KEY,
    `name`         VARCHAR(100) NOT NULL DEFAULT 'Unknown Spell',
    `description`  TEXT,
    `school`       TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `power_type`   TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `power_cost`   INT UNSIGNED NOT NULL DEFAULT 0,
    `cooldown`     INT UNSIGNED NOT NULL DEFAULT 0,
    `range_min`    FLOAT NOT NULL DEFAULT 0,
    `range_max`    FLOAT NOT NULL DEFAULT 30,
    `cast_time`    INT UNSIGNED NOT NULL DEFAULT 0,
    `effect_type1` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `effect_value1` INT NOT NULL DEFAULT 0,
    `effect_type2` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `effect_value2` INT NOT NULL DEFAULT 0,
    `effect_type3` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `effect_value3` INT NOT NULL DEFAULT 0,
    `duration`     INT UNSIGNED NOT NULL DEFAULT 0,
    `tick_interval` INT UNSIGNED NOT NULL DEFAULT 0,
    `is_channeled` TINYINT NOT NULL DEFAULT 0,
    `target_type`  TINYINT UNSIGNED NOT NULL DEFAULT 0
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `recipe_template` (
    `id`           INT UNSIGNED NOT NULL PRIMARY KEY,
    `skill_id`     INT UNSIGNED NOT NULL,
    `skill_required` INT UNSIGNED NOT NULL DEFAULT 1,
    `result_item`  INT UNSIGNED NOT NULL DEFAULT 0,
    `result_count` INT UNSIGNED NOT NULL DEFAULT 1,
    `reagent1`     INT UNSIGNED NOT NULL DEFAULT 0,
    `reagent1_count` INT UNSIGNED NOT NULL DEFAULT 0,
    `reagent2`     INT UNSIGNED NOT NULL DEFAULT 0,
    `reagent2_count` INT UNSIGNED NOT NULL DEFAULT 0,
    `reagent3`     INT UNSIGNED NOT NULL DEFAULT 0,
    `reagent3_count` INT UNSIGNED NOT NULL DEFAULT 0,
    `reagent4`     INT UNSIGNED NOT NULL DEFAULT 0,
    `reagent4_count` INT UNSIGNED NOT NULL DEFAULT 0,
    `orange_level` INT UNSIGNED NOT NULL DEFAULT 0,
    `yellow_level` INT UNSIGNED NOT NULL DEFAULT 0,
    `green_level`  INT UNSIGNED NOT NULL DEFAULT 0,
    `gray_level`   INT UNSIGNED NOT NULL DEFAULT 0
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- =====================================================
--  INSTANCE / BATTLEGROUND TABLES
-- =====================================================

CREATE TABLE IF NOT EXISTS `instance_template` (
    `map_id`       INT UNSIGNED NOT NULL PRIMARY KEY,
    `name`         VARCHAR(64) NOT NULL DEFAULT '',
    `parent`       INT UNSIGNED NOT NULL DEFAULT 0,
    `max_players`  TINYINT UNSIGNED NOT NULL DEFAULT 5,
    `reset_delay`  INT UNSIGNED NOT NULL DEFAULT 86400,
    `access_id`    INT UNSIGNED NOT NULL DEFAULT 0,
    `script`       VARCHAR(64) NOT NULL DEFAULT ''
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `instance_boss` (
    `map_id`       INT UNSIGNED NOT NULL,
    `boss_order`   TINYINT UNSIGNED NOT NULL,
    `creature_entry` INT UNSIGNED NOT NULL,
    `name`         VARCHAR(64) NOT NULL DEFAULT '',
    PRIMARY KEY (`map_id`, `boss_order`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `battleground_template` (
    `id`           INT UNSIGNED NOT NULL PRIMARY KEY,
    `name`         VARCHAR(64) NOT NULL DEFAULT '',
    `map_id`       INT UNSIGNED NOT NULL,
    `min_level`    TINYINT UNSIGNED NOT NULL DEFAULT 10,
    `max_level`    TINYINT UNSIGNED NOT NULL DEFAULT 80,
    `min_players`  TINYINT UNSIGNED NOT NULL DEFAULT 10,
    `max_players`  TINYINT UNSIGNED NOT NULL DEFAULT 40,
    `alliance_start_x` FLOAT NOT NULL DEFAULT 0,
    `alliance_start_y` FLOAT NOT NULL DEFAULT 0,
    `alliance_start_z` FLOAT NOT NULL DEFAULT 0,
    `horde_start_x` FLOAT NOT NULL DEFAULT 0,
    `horde_start_y` FLOAT NOT NULL DEFAULT 0,
    `horde_start_z` FLOAT NOT NULL DEFAULT 0
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- =====================================================
--  WORLD STATE
-- =====================================================

CREATE TABLE IF NOT EXISTS `world_state` (
    `var_name`     VARCHAR(64) NOT NULL PRIMARY KEY,
    `value`        VARCHAR(255) NOT NULL DEFAULT ''
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `world_event` (
    `event_id`     INT UNSIGNED NOT NULL PRIMARY KEY,
    `name`         VARCHAR(64) NOT NULL DEFAULT '',
    `start_time`   INT UNSIGNED NOT NULL DEFAULT 0,
    `end_time`     INT UNSIGNED NOT NULL DEFAULT 0,
    `occurrence`   INT UNSIGNED NOT NULL DEFAULT 0,
    `length`       INT UNSIGNED NOT NULL DEFAULT 0,
    `description`  VARCHAR(255) NOT NULL DEFAULT ''
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `player_create_info` (
    `race`         TINYINT UNSIGNED NOT NULL,
    `class`        TINYINT UNSIGNED NOT NULL,
    `map_id`       INT UNSIGNED NOT NULL DEFAULT 0,
    `zone_id`      INT UNSIGNED NOT NULL DEFAULT 0,
    `pos_x`        FLOAT NOT NULL DEFAULT 0,
    `pos_y`        FLOAT NOT NULL DEFAULT 0,
    `pos_z`        FLOAT NOT NULL DEFAULT 0,
    `ori`          FLOAT NOT NULL DEFAULT 0,
    PRIMARY KEY (`race`, `class`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `player_create_items` (
    `race`         TINYINT UNSIGNED NOT NULL,
    `class`        TINYINT UNSIGNED NOT NULL,
    `item_entry`   INT UNSIGNED NOT NULL,
    `amount`       INT UNSIGNED NOT NULL DEFAULT 1,
    PRIMARY KEY (`race`, `class`, `item_entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `player_create_spells` (
    `race`         TINYINT UNSIGNED NOT NULL,
    `class`        TINYINT UNSIGNED NOT NULL,
    `spell_id`     INT UNSIGNED NOT NULL,
    PRIMARY KEY (`race`, `class`, `spell_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `player_levelup_stats` (
    `race`         TINYINT UNSIGNED NOT NULL,
    `class`        TINYINT UNSIGNED NOT NULL,
    `level`        TINYINT UNSIGNED NOT NULL,
    `hp`           INT UNSIGNED NOT NULL DEFAULT 0,
    `mana`         INT UNSIGNED NOT NULL DEFAULT 0,
    `str`          INT UNSIGNED NOT NULL DEFAULT 0,
    `agi`          INT UNSIGNED NOT NULL DEFAULT 0,
    `sta`          INT UNSIGNED NOT NULL DEFAULT 0,
    `int_`         INT UNSIGNED NOT NULL DEFAULT 0,
    `spi`          INT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`race`, `class`, `level`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `xp_for_level` (
    `level`        TINYINT UNSIGNED NOT NULL PRIMARY KEY,
    `xp_required`  INT UNSIGNED NOT NULL DEFAULT 0
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
