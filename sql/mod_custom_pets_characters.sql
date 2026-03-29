-- ============================================================
-- mod-custom-pets: Tabla de mascotas aprendidas por jugador
-- Base de datos: acore_characters
-- ============================================================

-- ── Tabla mod_custom_pets_learned ──────────────────────────
-- Registra qué mascotas ha aprendido cada personaje.
-- player_guid : guid del personaje (characters.guid)
-- pet_id      : id de la mascota  (mod_custom_pets.id  en acore_world)
-- learned_date: fecha en que se aprendió
-- ──────────────────────────────────────────────────────────
DROP TABLE IF EXISTS `mod_custom_pets_learned`;
CREATE TABLE IF NOT EXISTS `mod_custom_pets_learned` (
  `player_guid`  INT UNSIGNED NOT NULL COMMENT 'GUID del personaje',
  `pet_id`       INT UNSIGNED NOT NULL COMMENT 'ID de la mascota en mod_custom_pets',
  `learned_date` DATETIME     NOT NULL DEFAULT NOW() COMMENT 'Fecha de aprendizaje',
  PRIMARY KEY (`player_guid`, `pet_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
  COMMENT='Mascotas custom aprendidas por cada personaje';

