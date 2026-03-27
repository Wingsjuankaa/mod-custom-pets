-- ============================================================
-- mod-custom-pets: Instalación completa
-- Base de datos: acore_world
-- ============================================================

-- ── Tabla de definiciones de mascotas ──────────────────────
DROP TABLE IF EXISTS `mod_custom_pets`;

CREATE TABLE `mod_custom_pets` (
  `id`              INT          AUTO_INCREMENT PRIMARY KEY,
  `name`            VARCHAR(64)  NOT NULL          COMMENT 'Nombre visible en comandos y mensajes',
  `type`            TINYINT      NOT NULL DEFAULT 1 COMMENT '1=Vendedor (más tipos en el futuro)',
  `creature_entry`  INT UNSIGNED NOT NULL           COMMENT 'Entry en creature_template',
  `description`     VARCHAR(255) NOT NULL DEFAULT '' COMMENT 'Descripción corta para .custompet list',
  `enabled`         TINYINT(1)   NOT NULL DEFAULT 1  COMMENT '0 = desactivado, no invocable'
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ── Creature template: Mercader Ambulante ──────────────────
-- entry      : 601000  (rango libre, ajusta si colisiona con otros módulos)
-- npcflag    : 128 = UNIT_NPC_FLAG_VENDOR  (requerido para SendListInventory)
-- faction    : 35  = Friendly to all
-- type       : 7   = CREATURE_TYPE_HUMANOID
-- unit_flags : 2   = UNIT_FLAG_NON_ATTACKABLE
-- flags_extra: 2   = CREATURE_FLAG_EXTRA_CIVILIAN (no contraataca)
-- ScriptName : 'npc_custom_pet_vendor'  ← debe coincidir con el C++
-- NOTA: los modelos van en creature_template_model (ver más abajo)
-- ──────────────────────────────────────────────────────────
DELETE FROM `creature_template` WHERE `entry` = 601000;
INSERT INTO `creature_template`
  (`entry`, `name`, `subname`,
   `faction`, `npcflag`,
   `speed_walk`, `speed_run`, `scale`,
   `minlevel`, `maxlevel`,
   `unit_class`, `unit_flags`, `type`,
   `RegenHealth`, `flags_extra`,
   `ScriptName`, `VerifiedBuild`)
VALUES
  (601000, 'Mercader Ambulante', 'Mascota Vendedora',
   35, 128,     -- faction=Friendly, npcflag=VENDOR
   1.0, 1.14286, 1.0,
   1, 1,
   1, 2, 7,     -- unit_class=Warrior, unit_flags=NON_ATTACKABLE, type=HUMANOID
   1, 2,        -- RegenHealth=true, flags_extra=CIVILIAN
   'npc_custom_pet_vendor', 0);

-- ── Modelo visual del Mercader Ambulante ───────────────────
-- En esta versión de AzerothCore los modelos se almacenan en
-- creature_template_model, separados del template principal.
-- CreatureDisplayID 49 = Human Male. Cámbialo por cualquier
-- ID válido de creature_display_info.
-- ──────────────────────────────────────────────────────────
DELETE FROM `creature_template_model` WHERE `CreatureID` = 601000;
INSERT INTO `creature_template_model`
  (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`)
VALUES
  (601000, 0, 49, 1.0, 1.0);

-- ── Tienda del Mercader Ambulante ──────────────────────────
-- maxcount=0 → stock ilimitado
-- ExtendedCost=0 → sólo precio en oro (definido en item_template)
-- Agrega o quita filas según tus necesidades.
-- ──────────────────────────────────────────────────────────
DELETE FROM `npc_vendor` WHERE `entry` = 601000;
INSERT INTO `npc_vendor`
  (`entry`, `slot`, `item`, `maxcount`, `incrtime`, `ExtendedCost`, `VerifiedBuild`)
VALUES
-- Pociones de Vida ─────────────────────────────────────────
(601000,  1,   118, 0, 0, 0, 0),   -- Minor Healing Potion
(601000,  2,   858, 0, 0, 0, 0),   -- Lesser Healing Potion
(601000,  3,   929, 0, 0, 0, 0),   -- Healing Potion
(601000,  4,  1710, 0, 0, 0, 0),   -- Greater Healing Potion
(601000,  5,  3928, 0, 0, 0, 0),   -- Superior Healing Potion
(601000,  6, 13446, 0, 0, 0, 0),   -- Major Healing Potion
-- Pociones de Maná ─────────────────────────────────────────
(601000,  7,  2455, 0, 0, 0, 0),   -- Minor Mana Potion
(601000,  8,  3385, 0, 0, 0, 0),   -- Lesser Mana Potion
(601000,  9,  3827, 0, 0, 0, 0),   -- Mana Potion
(601000, 10,  6149, 0, 0, 0, 0),   -- Greater Mana Potion
(601000, 11, 13443, 0, 0, 0, 0),   -- Major Mana Potion
-- Vendajes ─────────────────────────────────────────────────
(601000, 12,  1251, 0, 0, 0, 0),   -- Linen Bandage
(601000, 13,  3530, 0, 0, 0, 0),   -- Wool Bandage
(601000, 14,  6450, 0, 0, 0, 0),   -- Silk Bandage
(601000, 15,  8544, 0, 0, 0, 0),   -- Mageweave Bandage
(601000, 16, 14529, 0, 0, 0, 0),   -- Runecloth Bandage
(601000, 17, 21990, 0, 0, 0, 0),   -- Netherweave Bandage
-- Comida y Agua ────────────────────────────────────────────
(601000, 18,   117, 0, 0, 0, 0),   -- Tough Jerky
(601000, 19,   159, 0, 0, 0, 0);   -- Refreshing Spring Water

-- ── Registro de la primera mascota ─────────────────────────
-- type=1 → CUSTOM_PET_VENDOR  (debe coincidir con el enum en C++)
-- ──────────────────────────────────────────────────────────
INSERT INTO `mod_custom_pets`
  (`id`, `name`, `type`, `creature_entry`, `description`, `enabled`)
VALUES
  (1, 'Mercader Ambulante', 1, 601000,
   'Mercader portátil: pociones, vendajes y provisiones básicas.', 1);

-- ============================================================
-- MASCOTA SAQUEADORA (tipo 2 – CUSTOM_PET_LOOTER)
-- ============================================================

-- ── Creature template: Recolector Fiel ─────────────────────
-- entry      : 601001
-- npcflag    : 0   = sin interacción (no es un vendedor ni gossip)
-- unit_flags : 33554434 = NON_ATTACKABLE (2) | NOT_SELECTABLE (0x2000000)
--              El jugador no puede hacer clic sobre él.
-- flags_extra: 2   = CREATURE_FLAG_EXTRA_CIVILIAN (no contraataca)
-- ScriptName : 'npc_custom_pet_looter'
-- ──────────────────────────────────────────────────────────
DELETE FROM `creature_template` WHERE `entry` = 601001;
INSERT INTO `creature_template`
  (`entry`, `name`, `subname`,
   `faction`, `npcflag`,
   `speed_walk`, `speed_run`, `scale`,
   `minlevel`, `maxlevel`,
   `unit_class`, `unit_flags`, `type`,
   `RegenHealth`, `flags_extra`,
   `ScriptName`, `VerifiedBuild`)
VALUES
  (601001, 'Recolector Fiel', 'Mascota Saqueadora',
   35, 0,
   1.0, 1.14286, 0.6,  -- escala 0.6: más pequeño que un NPC normal
   1, 1,
   1, 33554434, 7,      -- NON_ATTACKABLE | NOT_SELECTABLE, HUMANOID
   1, 2,
   'npc_custom_pet_looter', 0);

-- ── Modelo visual del Recolector Fiel ──────────────────────
-- DisplayID 55 = Gnome Male (pequeño y ágil, aspecto de ayudante)
-- Cámbialo por cualquier ID válido de creature_display_info.
-- ──────────────────────────────────────────────────────────
DELETE FROM `creature_template_model` WHERE `CreatureID` = 601001;
INSERT INTO `creature_template_model`
  (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`)
VALUES
  (601001, 0, 55, 1.0, 1.0);

-- ── Registro de la mascota saqueadora ──────────────────────
-- type=2 → CUSTOM_PET_LOOTER  (debe coincidir con el enum en C++)
-- ──────────────────────────────────────────────────────────
INSERT INTO `mod_custom_pets`
  (`id`, `name`, `type`, `creature_entry`, `description`, `enabled`)
VALUES
  (2, 'Recolector Fiel', 2, 601001,
   'Saquea automáticamente los cadáveres en un radio de 40 unidades.', 1);

