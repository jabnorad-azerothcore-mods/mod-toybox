-- Toyboxes live here rather than in acore_characters so they span realms. An account's
-- toybox ID is its own account ID unless mod_toybox_account links it to another
CREATE TABLE IF NOT EXISTS `mod_toybox_collection` (
  `toybox_id` INT UNSIGNED NOT NULL,
  `item_id` INT UNSIGNED NOT NULL,
  `added_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`toybox_id`, `item_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `mod_toybox_account` (
  `account_id` INT UNSIGNED NOT NULL,
  `toybox_id` INT UNSIGNED NOT NULL,
  PRIMARY KEY (`account_id`),
  KEY `toybox_id` (`toybox_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
