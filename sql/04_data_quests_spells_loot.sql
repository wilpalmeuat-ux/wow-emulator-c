-- =====================================================
--  WoW 3.3.5a Emulator -- Quest, Spell, Loot Data
--  Part 4: quest_template + spell_template + loot tables
-- =====================================================
USE `wow_emulator`;

-- ===== QUESTS (Elwynn Forest starter chain) =====
INSERT IGNORE INTO `quest_template` (`id`, `title`, `details`, `objectives`, `completion_text`, `min_level`, `quest_level`, `reward_xp`, `reward_money`, `quest_giver`, `quest_ender`, `prev_quest`, `next_quest`, `zone_or_sort`) VALUES
(783,  'A Threat Within',       'The Stormwind Army has need of your help.', 'Report to Marshal McBride in Northshire Abbey.', 'Welcome, recruit!', 1, 1, 40, 10, 198, 197, 0, 7, 12),
(7,    'Kobold Camp Cleanup',   'The kobolds have been a menace.', 'Kill 10 Kobold Vermin.', 'Excellent work!', 1, 2, 250, 75, 197, 197, 783, 15, 12),
(15,   'Investigate Echo Ridge', 'The mine needs clearing.', 'Kill 10 Kobold Laborers in the mine.', 'The mine is safer now.', 2, 4, 450, 150, 197, 197, 7, 21, 12),
(21,   'Skirmish at Echo Ridge', 'The tunnelers must be stopped.', 'Kill 12 Kobold Tunnelers.', 'Victory!', 3, 5, 560, 200, 197, 197, 15, 0, 12),
(18,   'Brotherhood of Thieves','The Defias are plotting something.', 'Bring 12 Red Burlap Bandanas.', 'These will be useful evidence.', 2, 4, 450, 125, 198, 198, 783, 6, 12),
(6,    'Bounty on Garrick Padfoot', 'Garrick Padfoot must be eliminated.', 'Kill Garrick Padfoot and bring his head.', 'Justice is served.', 3, 5, 560, 200, 198, 198, 18, 0, 12),
-- Durotar quests
(788,  'Your Place In The World','Welcome to the Horde.', 'Speak to Gornek at the Den.', 'Lok''tar, young one.', 1, 1, 40, 0, 0, 3143, 0, 789, 14),
(789,  'Cutting Teeth',         'The boars are too many.', 'Kill 10 Mottled Boars.', 'Well done.', 1, 2, 250, 75, 3143, 3143, 788, 0, 14),
(791,  'Vile Familiars',        'The demons must be cleansed.', 'Kill 12 Vile Familiars.', 'The corruption recedes.', 1, 3, 360, 100, 3143, 3143, 788, 0, 14),
(790,  'Sting of the Scorpid',  'Scorpids threaten the camp.', 'Collect 10 Scorpid Worker Tails.', 'These tails will be useful.', 2, 4, 450, 125, 3143, 3143, 789, 0, 14);

-- Quest kill requirements
UPDATE `quest_template` SET `req_creature1`=6,   `req_creature1_count`=10 WHERE `id`=7;
UPDATE `quest_template` SET `req_creature1`=80,  `req_creature1_count`=10 WHERE `id`=15;
UPDATE `quest_template` SET `req_creature1`=257, `req_creature1_count`=12 WHERE `id`=21;
UPDATE `quest_template` SET `req_creature1`=103, `req_creature1_count`=1  WHERE `id`=6;
UPDATE `quest_template` SET `req_creature1`=3098,`req_creature1_count`=10 WHERE `id`=789;
UPDATE `quest_template` SET `req_creature1`=3100,`req_creature1_count`=12 WHERE `id`=791;

-- ===== SPELLS =====
INSERT IGNORE INTO `spell_template` (`id`, `name`, `description`, `school`, `power_cost`, `cooldown`, `range_max`, `cast_time`, `effect_type1`, `effect_value1`, `duration`, `tick_interval`, `target_type`) VALUES
-- Warrior
(78,    'Heroic Strike',     'An aggressive melee attack.', 0, 0, 0, 5, 0, 1, 200, 0, 0, 1),
(100,   'Charge',            'Charge an enemy, stunning it.', 0, 0, 15000, 25, 0, 11, 0, 1000, 0, 1),
(6673,  'Battle Shout',      'Increases attack power of party.', 0, 0, 0, 0, 0, 3, 100, 120000, 0, 2),
(71,    'Defensive Stance',  'Assume a defensive posture.', 0, 0, 0, 0, 0, 3, 0, 0, 0, 0),
(2457,  'Battle Stance',     'Assume a battle posture.', 0, 0, 0, 0, 0, 3, 0, 0, 0, 0),
(772,   'Rend',              'Wounds the target causing bleed.', 0, 0, 0, 5, 0, 14, 42, 15000, 3000, 1),
-- Mage
(133,   'Fireball',          'Hurls a fiery ball at the target.', 2, 200, 0, 35, 3500, 1, 500, 0, 0, 1),
(116,   'Frostbolt',         'Launches a bolt of frost.', 4, 180, 0, 30, 3000, 1, 400, 0, 0, 1),
(5143,  'Arcane Missiles',   'Launches arcane missiles.', 6, 250, 0, 30, 0, 1, 200, 5000, 1000, 1),
(2136,  'Fire Blast',        'Blasts the enemy with fire.', 2, 150, 8000, 20, 0, 1, 300, 0, 0, 1),
(122,   'Frost Nova',        'Freezes all nearby enemies.', 4, 100, 25000, 0, 0, 12, 0, 8000, 0, 3),
(12051, 'Evocation',         'Restores mana rapidly.', 6, 0, 480000, 0, 0, 15, 500, 8000, 2000, 0),
-- Priest
(585,   'Smite',             'Smites the enemy with holy.', 1, 100, 0, 30, 2500, 1, 250, 0, 0, 1),
(2061,  'Flash Heal',        'Quick heal on a target.', 1, 300, 0, 40, 1500, 2, 700, 0, 0, 2),
(139,   'Renew',             'Heals target over time.', 1, 250, 0, 40, 0, 15, 100, 15000, 3000, 2),
(589,   'Shadow Word: Pain', 'Causes shadow damage over time.', 5, 200, 0, 30, 0, 14, 150, 18000, 3000, 1),
(8092,  'Mind Blast',        'Blasts the mind for shadow damage.', 5, 250, 8000, 30, 1500, 1, 600, 0, 0, 1),
(17,    'Power Word: Shield','Creates a damage-absorbing shield.', 1, 200, 4000, 40, 0, 3, 500, 30000, 0, 2),
-- Paladin
(635,   'Holy Light',        'Heals a friendly target.', 1, 350, 0, 40, 2500, 2, 900, 0, 0, 2),
(19750, 'Flash of Light',    'Quick heal.', 1, 200, 0, 40, 1500, 2, 500, 0, 0, 2),
(53563, 'Beacon of Light',   'Heals reflected from beacon.', 1, 300, 0, 60, 0, 3, 0, 60000, 0, 2),
(20925, 'Holy Shield',       'Increases block chance.', 1, 200, 8000, 0, 0, 3, 0, 10000, 0, 0),
-- Rogue
(53,    'Backstab',          'Stabs from behind.', 0, 0, 0, 5, 0, 1, 400, 0, 0, 1),
(1752,  'Sinister Strike',   'A sinister attack.', 0, 0, 0, 5, 0, 1, 150, 0, 0, 1),
(2098,  'Eviscerate',        'Powerful finishing move.', 0, 0, 0, 5, 0, 1, 500, 0, 0, 1),
(1784,  'Stealth',           'Become invisible.', 0, 0, 10000, 0, 0, 3, 0, 0, 0, 0),
-- Hunter
(75,    'Auto Shot',         'Fires your ranged weapon.', 0, 0, 0, 35, 0, 1, 100, 0, 0, 1),
(2643,  'Multi-Shot',        'Fires multiple arrows.', 0, 0, 10000, 35, 500, 1, 200, 0, 0, 3),
(3044,  'Arcane Shot',       'Fires an arcane arrow.', 6, 100, 6000, 35, 0, 1, 250, 0, 0, 1),
(5116,  'Concussive Shot',   'Dazes the target.', 0, 0, 12000, 35, 0, 11, 0, 4000, 0, 1),
-- Warlock
(686,   'Shadow Bolt',       'Hurls a bolt of shadow.', 5, 200, 0, 30, 3000, 1, 450, 0, 0, 1),
(172,   'Corruption',        'Corrupts the target.', 5, 150, 0, 30, 0, 14, 120, 18000, 3000, 1),
(348,   'Immolate',          'Burns the target.', 2, 150, 0, 30, 2000, 14, 100, 15000, 3000, 1),
(5782,  'Fear',              'Causes enemy to flee.', 5, 150, 0, 20, 1500, 3, 0, 10000, 0, 1),
-- Death Knight
(49998, 'Death Strike',      'Strike that heals you.', 0, 0, 0, 5, 0, 1, 300, 0, 0, 1),
(45477, 'Icy Touch',         'Fires ice at the target.', 4, 0, 0, 20, 0, 1, 250, 0, 0, 1),
(49143, 'Frost Strike',      'A powerful frost attack.', 4, 0, 0, 5, 0, 1, 450, 0, 0, 1);

-- ===== CREATURE LOOT TABLES =====
INSERT IGNORE INTO `creature_loot` (`entry`, `item_entry`, `chance`, `min_count`, `max_count`) VALUES
-- Kobold Vermin
(6, 117, 50.0, 1, 1),     -- Tough Jerky
(6, 2589, 20.0, 1, 2),    -- Linen Cloth
(6, 118, 5.0, 1, 1),      -- Minor Healing Potion
-- Defias Thug
(38, 117, 40.0, 1, 1),
(38, 2589, 30.0, 1, 2),
(38, 2074, 1.0, 1, 1),    -- Solid Shortblade
-- Kobold Miner
(40, 2589, 35.0, 1, 3),
(40, 2770, 15.0, 1, 2),   -- Copper Ore
(40, 2835, 10.0, 1, 2),   -- Rough Stone
-- Kobold Laborer
(80, 2589, 40.0, 1, 3),
(80, 2770, 20.0, 1, 3),
(80, 2835, 15.0, 1, 2),
(80, 118, 8.0, 1, 1),
-- Garrick Padfoot (quest boss)
(103, 2074, 25.0, 1, 1),
(103, 2059, 10.0, 1, 1),  -- Sentry Cloak
(103, 118, 50.0, 1, 2),
-- Wolves
(299, 117, 60.0, 1, 1),
(69, 117, 50.0, 1, 1),
(69, 2589, 15.0, 1, 1),
-- Durotar creatures
(3098, 117, 50.0, 1, 1),
(3100, 2589, 20.0, 1, 1),
(3100, 118, 5.0, 1, 1),
(3101, 117, 40.0, 1, 1),
-- Dungeon bosses
(639, 9425, 1.0, 1, 1),   -- Pendulum of Doom
(639, 9423, 5.0, 1, 1),   -- The Jackhammer
(639, 2080, 15.0, 1, 1),
-- World Boss
(9999, 49623, 5.0, 1, 1), -- Shadowmourne
(9999, 50000, 100.0, 5, 20), -- Server Tokens
(9999, 33447, 100.0, 3, 5),
(9999, 33448, 100.0, 3, 5),
-- ICC Bosses
(36597, 49623, 10.0, 1, 1), -- Shadowmourne from Lich King
(36853, 50000, 100.0, 10, 20); -- Tokens from Sindragosa

-- ===== PLAYER CREATION DATA =====
INSERT IGNORE INTO `player_create_info` (`race`, `class`, `map_id`, `zone_id`, `pos_x`, `pos_y`, `pos_z`, `ori`) VALUES
-- Human (race=1)
(1, 1, 0, 12, -8949.95, -132.49, 83.53, 0),   -- Warrior
(1, 2, 0, 12, -8949.95, -132.49, 83.53, 0),   -- Paladin
(1, 4, 0, 12, -8949.95, -132.49, 83.53, 0),   -- Rogue
(1, 5, 0, 12, -8949.95, -132.49, 83.53, 0),   -- Priest
(1, 8, 0, 12, -8949.95, -132.49, 83.53, 0),   -- Mage
(1, 9, 0, 12, -8949.95, -132.49, 83.53, 0),   -- Warlock
(1, 6, 0, 12, -8949.95, -132.49, 83.53, 0),   -- Death Knight
-- Orc (race=2)
(2, 1, 1, 14, -618.52, -4251.67, 38.72, 0),
(2, 3, 1, 14, -618.52, -4251.67, 38.72, 0),   -- Hunter
(2, 4, 1, 14, -618.52, -4251.67, 38.72, 0),
(2, 7, 1, 14, -618.52, -4251.67, 38.72, 0),   -- Shaman
(2, 9, 1, 14, -618.52, -4251.67, 38.72, 0),
(2, 6, 1, 14, -618.52, -4251.67, 38.72, 0),
-- Dwarf (race=3)
(3, 1, 0, 1, -6240.32, 331.03, 382.76, 6.17),
(3, 2, 0, 1, -6240.32, 331.03, 382.76, 6.17),
(3, 3, 0, 1, -6240.32, 331.03, 382.76, 6.17),
(3, 4, 0, 1, -6240.32, 331.03, 382.76, 6.17),
(3, 5, 0, 1, -6240.32, 331.03, 382.76, 6.17),
-- Night Elf (race=4)
(4, 1, 1, 141, 10311.3, 832.46, 1326.41, 5.69),
(4, 3, 1, 141, 10311.3, 832.46, 1326.41, 5.69),
(4, 4, 1, 141, 10311.3, 832.46, 1326.41, 5.69),
(4, 5, 1, 141, 10311.3, 832.46, 1326.41, 5.69),
(4, 11, 1, 141, 10311.3, 832.46, 1326.41, 5.69), -- Druid
-- Undead (race=5)
(5, 1, 0, 85, 1676.71, 1678.31, 121.67, 2.70),
(5, 4, 0, 85, 1676.71, 1678.31, 121.67, 2.70),
(5, 5, 0, 85, 1676.71, 1678.31, 121.67, 2.70),
(5, 8, 0, 85, 1676.71, 1678.31, 121.67, 2.70),
(5, 9, 0, 85, 1676.71, 1678.31, 121.67, 2.70),
-- Tauren (race=6)
(6, 1, 1, 215, -2917.58, -257.98, 52.99, 0),
(6, 3, 1, 215, -2917.58, -257.98, 52.99, 0),
(6, 7, 1, 215, -2917.58, -257.98, 52.99, 0),
(6, 11, 1, 215, -2917.58, -257.98, 52.99, 0),
-- Gnome (race=7)
(7, 1, 0, 1, -6240.32, 331.03, 382.76, 6.17),
(7, 4, 0, 1, -6240.32, 331.03, 382.76, 6.17),
(7, 8, 0, 1, -6240.32, 331.03, 382.76, 6.17),
(7, 9, 0, 1, -6240.32, 331.03, 382.76, 6.17),
-- Troll (race=8)
(8, 1, 1, 14, -618.52, -4251.67, 38.72, 0),
(8, 3, 1, 14, -618.52, -4251.67, 38.72, 0),
(8, 4, 1, 14, -618.52, -4251.67, 38.72, 0),
(8, 5, 1, 14, -618.52, -4251.67, 38.72, 0),
(8, 7, 1, 14, -618.52, -4251.67, 38.72, 0),
(8, 8, 1, 14, -618.52, -4251.67, 38.72, 0),
-- Blood Elf (race=10)
(10, 2, 530, 3431, 10349.6, -6357.29, 33.40, 5.31),
(10, 3, 530, 3431, 10349.6, -6357.29, 33.40, 5.31),
(10, 4, 530, 3431, 10349.6, -6357.29, 33.40, 5.31),
(10, 5, 530, 3431, 10349.6, -6357.29, 33.40, 5.31),
(10, 8, 530, 3431, 10349.6, -6357.29, 33.40, 5.31),
(10, 9, 530, 3431, 10349.6, -6357.29, 33.40, 5.31),
-- Draenei (race=11)
(11, 1, 530, 3526, -3961.64, -13931.2, 100.62, 2.08),
(11, 2, 530, 3526, -3961.64, -13931.2, 100.62, 2.08),
(11, 3, 530, 3526, -3961.64, -13931.2, 100.62, 2.08),
(11, 5, 530, 3526, -3961.64, -13931.2, 100.62, 2.08),
(11, 7, 530, 3526, -3961.64, -13931.2, 100.62, 2.08),
(11, 8, 530, 3526, -3961.64, -13931.2, 100.62, 2.08);

-- ===== XP PER LEVEL TABLE (WoW 3.3.5) =====
INSERT IGNORE INTO `xp_for_level` (`level`, `xp_required`) VALUES
(1,400),(2,900),(3,1400),(4,2100),(5,2800),(6,3600),(7,4500),(8,5400),
(9,6500),(10,7600),(11,8800),(12,10100),(13,11400),(14,12900),(15,14400),
(16,16000),(17,17700),(18,19400),(19,21300),(20,23200),(21,25200),(22,27300),
(23,29400),(24,31700),(25,34000),(26,36400),(27,38900),(28,41400),(29,44300),
(30,47400),(31,50800),(32,54500),(33,58600),(34,62800),(35,67100),(36,71600),
(37,76100),(38,80800),(39,85700),(40,90700),(41,95800),(42,101000),(43,106300),
(44,111800),(45,117500),(46,123200),(47,129100),(48,135100),(49,141200),(50,147500),
(51,153900),(52,160400),(53,167100),(54,173900),(55,180800),(56,187900),(57,195000),
(58,202300),(59,209800),(60,494000),(61,574700),(62,614400),(63,650300),(64,682300),
(65,710200),(66,734100),(67,753700),(68,768900),(69,779700),(70,1523800),(71,1539600),
(72,1555700),(73,1571800),(74,1587900),(75,1604200),(76,1620700),(77,1637400),(78,1653900),
(79,1670800);

-- ===== DEFAULT ACCOUNTS =====
INSERT IGNORE INTO `accounts` (`id`, `username`, `gmlevel`) VALUES
(1, 'TEST', 3),
(2, 'ADMIN', 3),
(3, 'PLAYER', 0);

-- ===== DEFAULT REALM =====
INSERT IGNORE INTO `realmlist` (`id`, `name`, `address`, `port`) VALUES
(1, 'WoW 3.3.5a Emulator', '127.0.0.1', 8085);

-- ===== INSTANCE TEMPLATES =====
INSERT IGNORE INTO `instance_template` (`map_id`, `name`, `max_players`, `reset_delay`) VALUES
(33,  'Shadowfang Keep', 5, 86400),
(36,  'The Deadmines', 5, 86400),
(34,  'The Stockade', 5, 86400),
(43,  'Wailing Caverns', 5, 86400),
(47,  'Razorfen Kraul', 5, 86400),
(90,  'Gnomeregan', 5, 86400),
(209, 'Zul''Farrak', 5, 86400),
(229, 'Blackrock Spire', 10, 86400),
(249, 'Onyxia''s Lair', 25, 604800),
(409, 'Molten Core', 40, 604800),
(469, 'Blackwing Lair', 40, 604800),
(509, 'Ruins of Ahn''Qiraj', 20, 259200),
(531, 'Temple of Ahn''Qiraj', 40, 604800),
(533, 'Naxxramas', 25, 604800),
(603, 'Ulduar', 25, 604800),
(615, 'The Obsidian Sanctum', 25, 604800),
(616, 'The Eye of Eternity', 25, 604800),
(624, 'Vault of Archavon', 25, 604800),
(631, 'Icecrown Citadel', 25, 604800),
(649, 'Trial of the Crusader', 25, 604800),
(724, 'The Ruby Sanctum', 25, 604800);

-- ===== BATTLEGROUND TEMPLATES =====
INSERT IGNORE INTO `battleground_template` (`id`, `name`, `map_id`, `min_level`, `max_level`, `min_players`, `max_players`) VALUES
(1, 'Alterac Valley', 30, 51, 80, 20, 40),
(2, 'Warsong Gulch', 489, 10, 80, 5, 10),
(3, 'Arathi Basin', 529, 20, 80, 8, 15),
(7, 'Eye of the Storm', 566, 61, 80, 8, 15),
(9, 'Strand of the Ancients', 607, 65, 80, 8, 15),
(30, 'Isle of Conquest', 628, 71, 80, 20, 40);
