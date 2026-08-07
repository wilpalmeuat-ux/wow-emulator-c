-- =====================================================
--  WoW 3.3.5a Emulator -- Item Data
--  Part 3: item_template
-- =====================================================
USE `wow_emulator`;

-- Quality: 0=Poor(gray), 1=Common(white), 2=Uncommon(green), 3=Rare(blue),
--          4=Epic(purple), 5=Legendary(orange), 6=Artifact(red), 7=Heirloom(gold)
-- InvType: 0=NonEquip, 1=Head, 2=Neck, 3=Shoulders, 4=Body, 5=Chest,
--          6=Waist, 7=Legs, 8=Feet, 9=Wrists, 10=Hands, 11=Finger,
--          12=Trinket, 13=Weapon, 14=Shield, 15=Ranged, 16=Cloak,
--          17=2HWeapon, 18=Bag, 19=Tabard, 21=MainHand, 22=OffHand
-- Bonding: 0=None, 1=BoP, 2=BoE, 3=BoU

INSERT IGNORE INTO `item_template` (`entry`, `name`, `quality`, `inventory_type`, `item_level`, `required_level`, `armor`, `dmg_min1`, `dmg_max1`, `delay`, `buy_price`, `sell_price`, `stackable`, `bonding`, `max_durability`, `description`) VALUES
-- ===== STARTER WEAPONS =====
(25,    'Worn Shortsword',        0, 21, 2, 1, 0, 1, 3, 1900, 10, 2, 1, 0, 20, 'A dull blade.'),
(35,    'Bent Staff',             0, 17, 2, 1, 0, 2, 4, 2900, 10, 2, 1, 0, 25, 'A crooked staff.'),
(36,    'Worn Mace',              0, 21, 2, 1, 0, 1, 3, 2200, 10, 2, 1, 0, 20, ''),
(37,    'Worn Axe',               0, 21, 2, 1, 0, 1, 4, 2000, 10, 2, 1, 0, 20, ''),
(2092,  'Worn Dagger',            0, 13, 2, 1, 0, 1, 2, 1600, 10, 2, 1, 0, 16, ''),
-- ===== STARTER ARMOR =====
(6948,  'Hearthstone',            1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'Use: Returns you to your home inn.'),
(38,    'Recruit''s Shirt',       1, 4, 1, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, ''),
(39,    'Recruit''s Pants',       0, 7, 1, 1, 2, 0, 0, 0, 5, 1, 1, 0, 25, ''),
(40,    'Recruit''s Boots',       0, 8, 1, 1, 1, 0, 0, 0, 5, 1, 1, 0, 20, ''),
(44,    'Squire''s Pants',        0, 7, 1, 1, 3, 0, 0, 0, 5, 1, 1, 0, 25, ''),
(45,    'Squire''s Boots',        0, 8, 1, 1, 2, 0, 0, 0, 5, 1, 1, 0, 20, ''),
(43,    'Squire''s Shirt',        1, 4, 1, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, ''),
(53,    'Neophyte''s Robe',       0, 20, 1, 1, 3, 0, 0, 0, 5, 1, 1, 0, 30, ''),
(52,    'Neophyte''s Pants',      0, 7, 1, 1, 2, 0, 0, 0, 5, 1, 1, 0, 25, ''),
(51,    'Neophyte''s Boots',      0, 8, 1, 1, 1, 0, 0, 0, 5, 1, 1, 0, 20, ''),
-- ===== LEVEL 10-20 GREENS =====
(2059,  'Sentry Cloak',           2, 16, 14, 9, 12, 0, 0, 0, 172, 43, 1, 2, 30, ''),
(2080,  'Hillman''s Cloak',       2, 16, 19, 14, 16, 0, 0, 0, 355, 88, 1, 2, 35, ''),
(2960,  'Journeyman''s Vest',     2, 5, 15, 10, 45, 0, 0, 0, 448, 112, 1, 2, 60, ''),
(2961,  'Journeyman''s Pants',    2, 7, 15, 10, 38, 0, 0, 0, 372, 93, 1, 2, 50, ''),
(2965,  'Warrior''s Shield',      2, 14, 15, 10, 300, 0, 0, 0, 600, 150, 1, 2, 55, ''),
(2074,  'Solid Shortblade',       2, 13, 13, 8, 0, 8, 16, 1600, 320, 80, 1, 2, 40, ''),
(2075,  'Heavy Shortblade',       2, 13, 18, 13, 0, 12, 24, 1700, 640, 160, 1, 2, 50, ''),
-- ===== LEVEL 40-60 BLUES =====
(7786,  'Heliotrope Cloak',       3, 16, 42, 37, 28, 0, 0, 0, 6400, 1600, 1, 2, 55, ''),
(7945,  'Windreaver Greaves',     3, 8, 44, 39, 142, 0, 0, 0, 12000, 3000, 1, 2, 60, ''),
(9423,  'The Jackhammer',         3, 17, 42, 37, 0, 74, 112, 3600, 18000, 4500, 1, 2, 90, 'Hits like a truck.'),
(9425,  'Pendulum of Doom',       4, 17, 43, 38, 0, 124, 187, 3800, 55000, 13750, 1, 2, 100, 'Swings with devastating force.'),
-- ===== EPIC/LEGENDARY ITEMS =====
(17182, 'Sulfuras, Hand of Ragnaros', 5, 17, 80, 60, 0, 223, 372, 3700, 0, 0, 1, 1, 145, 'The legendary hammer of Ragnaros the Firelord.'),
(19019, 'Thunderfury, Blessed Blade of the Windseeker', 5, 21, 80, 60, 0, 115, 213, 1900, 0, 0, 1, 1, 125, ''),
(22691, 'Corrupted Ashbringer',   4, 17, 78, 60, 0, 259, 389, 3500, 0, 0, 1, 1, 120, 'The corruption runs deep.'),
(30312, 'Infinity Blade',         4, 21, 115, 70, 0, 105, 196, 1800, 0, 0, 1, 1, 105, ''),
(32837, 'Warglaive of Azzinoth',  5, 21, 156, 70, 0, 214, 398, 2600, 0, 0, 1, 1, 150, ''),
(34334, 'Thori''dal, the Stars'' Fury', 5, 26, 164, 70, 0, 201, 374, 3000, 0, 0, 1, 1, 125, ''),
(46017, 'Val''anyr, Hammer of Ancient Kings', 5, 21, 245, 80, 0, 167, 310, 1800, 0, 0, 1, 1, 125, ''),
(49623, 'Shadowmourne',           5, 17, 284, 80, 0, 954, 1592, 3600, 0, 0, 1, 1, 200, 'The darkness calls.'),
-- ===== CONSUMABLES =====
(117,   'Tough Jerky',            1, 0, 5, 1, 0, 0, 0, 0, 25, 1, 20, 0, 0, 'Use: Restores 61 health over 18 sec.'),
(159,   'Refreshing Spring Water',1, 0, 5, 1, 0, 0, 0, 0, 25, 1, 20, 0, 0, 'Use: Restores 151 mana over 18 sec.'),
(118,   'Minor Healing Potion',   1, 0, 3, 1, 0, 0, 0, 0, 30, 1, 5, 0, 0, 'Use: Restores 70 to 90 health.'),
(2455,  'Minor Mana Potion',      1, 0, 7, 1, 0, 0, 0, 0, 50, 2, 5, 0, 0, 'Use: Restores 70 to 90 mana.'),
(3928,  'Superior Healing Potion', 1, 0, 45, 35, 0, 0, 0, 0, 2000, 100, 5, 0, 0, 'Use: Restores 700 to 900 health.'),
(33447, 'Runic Healing Potion',   1, 0, 80, 70, 0, 0, 0, 0, 12000, 600, 5, 0, 0, 'Use: Restores 2700 to 4500 health.'),
(33448, 'Runic Mana Potion',      1, 0, 80, 70, 0, 0, 0, 0, 12000, 600, 5, 0, 0, 'Use: Restores 4300 to 4500 mana.'),
-- ===== TRADE MATERIALS =====
(2589,  'Linen Cloth',            1, 0, 10, 0, 0, 0, 0, 0, 50, 3, 20, 0, 0, ''),
(2592,  'Wool Cloth',             1, 0, 25, 0, 0, 0, 0, 0, 200, 10, 20, 0, 0, ''),
(4306,  'Silk Cloth',             1, 0, 40, 0, 0, 0, 0, 0, 500, 25, 20, 0, 0, ''),
(4338,  'Mageweave Cloth',        1, 0, 50, 0, 0, 0, 0, 0, 1000, 50, 20, 0, 0, ''),
(14047, 'Runecloth',              1, 0, 58, 0, 0, 0, 0, 0, 1500, 75, 20, 0, 0, ''),
(21877, 'Netherweave Cloth',      1, 0, 60, 0, 0, 0, 0, 0, 2000, 100, 20, 0, 0, ''),
(33470, 'Frostweave Cloth',       1, 0, 72, 0, 0, 0, 0, 0, 5000, 250, 20, 0, 0, ''),
(2835,  'Rough Stone',            1, 0, 7, 0, 0, 0, 0, 0, 15, 0, 20, 0, 0, ''),
(2836,  'Coarse Stone',           1, 0, 15, 0, 0, 0, 0, 0, 50, 2, 20, 0, 0, ''),
(2770,  'Copper Ore',             1, 0, 10, 0, 0, 0, 0, 0, 25, 1, 20, 0, 0, ''),
(2771,  'Tin Ore',                1, 0, 20, 0, 0, 0, 0, 0, 100, 5, 20, 0, 0, ''),
(2447,  'Peacebloom',             1, 0, 5, 0, 0, 0, 0, 0, 10, 0, 20, 0, 0, ''),
(765,   'Silverleaf',             1, 0, 5, 0, 0, 0, 0, 0, 10, 0, 20, 0, 0, ''),
(2449,  'Earthroot',              1, 0, 10, 0, 0, 0, 0, 0, 25, 1, 20, 0, 0, ''),
(785,   'Mageroyal',              1, 0, 15, 0, 0, 0, 0, 0, 50, 2, 20, 0, 0, ''),
-- ===== BAGS =====
(4498,  'Brown Leather Satchel',  1, 18, 15, 0, 0, 0, 0, 0, 250, 62, 1, 0, 0, ''),
(4500,  'Traveler''s Backpack',   1, 18, 45, 0, 0, 0, 0, 0, 10000, 2500, 1, 0, 0, ''),
(14156, 'Bottomless Bag',         4, 18, 55, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, ''),
-- ===== FUN SERVER TOKENS =====
(50000, 'Server Token',           4, 0, 1, 0, 0, 0, 0, 0, 0, 100, 200, 1, 0, 'Currency for the fun server shop.'),
(50001, 'Welcome Gift',           2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'A gift from the server admins.');
