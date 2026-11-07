-- The items a toybox can hold. target_type: 0 casts on the player, 1 on their target, 2
-- at their feet, for toys that would otherwise need a ground target.
-- Toybox.ExtraItems and Toybox.ExcludedItems in the conf adjust this list
CREATE TABLE IF NOT EXISTS `mod_toybox_item` (
  `item_id` INT UNSIGNED NOT NULL,
  `target_type` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`item_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- The whole list is replaced on purpose, so this file alone says what ships
DELETE FROM `mod_toybox_item`;
INSERT INTO `mod_toybox_item` (`item_id`, `target_type`) VALUES
(17712, 0), -- Winter Veil Disguise Kit
(17716, 0), -- Snowmaster 9000
(18660, 0), -- World Enlarger
(18984, 0), -- Dimensional Ripper - Everlook
(18986, 0), -- Ultrasafe Transporter: Gadgetzan
(21540, 0), -- Elune's Lantern
(23767, 0), -- Crashin' Thrashin' Robot
(30542, 0), -- Dimensional Ripper - Area 52
(30544, 0), -- Ultrasafe Transporter: Toshley's Station
(30690, 0), -- Power Converter
(32542, 0), -- Imp in a Ball
(32566, 0), -- Picnic Basket
(33079, 0), -- Murloc Costume
(33219, 2), -- Goblin Gumbo Kettle
(33223, 0), -- Fishing Chair
(33927, 2), -- Brewfest Pony Keg
(34480, 0), -- Romantic Picnic Basket
(34499, 0), -- Paper Flying Machine Kit
(34686, 0), -- Brazier of Dancing Flames
(35227, 0), -- Goblin Weather Machine - Prototype 01-B
(35275, 0), -- Orb of the Sin'dorei
(36862, 0), -- Worn Troll Dice
(36863, 0), -- Decahedral Dwarven Dice
(37460, 0), -- Rope Pet Leash
(37710, 0), -- Crashin' Thrashin' Racer Controller
(38301, 0), -- D.I.S.C.O.
(38578, 0), -- The Flag of Ownership
(40768, 0), -- MOLL-E
(43499, 0), -- Iron Boot Flask
(43824, 0), -- The Schools of Arcane Magic - Mastery
(44430, 0), -- Titanium Seal of Dalaran
(44606, 2), -- Toy Train Set
(44719, 0), -- Frenzyheart Brew
(44820, 0), -- Red Ribbon Pet Leash
(45011, 2), -- Stormwind Banner
(45013, 2), -- Thunder Bluff Banner
(45014, 2), -- Orgrimmar Banner
(45015, 2), -- Sen'jin Banner
(45016, 2), -- Undercity Banner
(45017, 2), -- Silvermoon City Banner
(45018, 2), -- Ironforge Banner
(45019, 2), -- Gnomeregan Banner
(45020, 2), -- Exodar Banner
(45021, 2), -- Darnassus Banner
(45057, 0), -- Wind-Up Train Wrecker
(45063, 2), -- Foam Sword Rack
(45984, 0), -- Unusual Compass
(46709, 0), -- MiniZep Controller
(46780, 0), -- Ogre Pinata
(46843, 2), -- Argent Crusader's Banner
(48933, 0), -- Wormhole Generator: Northrend
(49703, 0), -- Perpetual Purple Firework
(49704, 0), -- Carved Ogre Idol
(50471, 1), -- The Heartbreaker
(52201, 0), -- Muradin's Favor
(52253, 0), -- Sylvanas's Music Box
(54212, 0), -- Instant Statue Pedestal
(54343, 0), -- Blue Crashin' Thrashin' Racer Controller
(54437, 2), -- Tiny Green Ragdoll
(54438, 2), -- Tiny Blue Ragdoll
(54452, 0), -- Ethereal Portal
(54651, 0), -- Gnomeregan Pride
(54653, 0); -- Darkspear Pride
