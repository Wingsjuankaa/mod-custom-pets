-- ============================================================
-- mod-custom-pets: Instalación completa
-- Base de datos: acore_world
-- ============================================================

-- ── Tabla de definiciones de mascotas ──────────────────────
DROP TABLE IF EXISTS `mod_custom_pets`;

CREATE TABLE `mod_custom_pets` (
  `id`              INT          AUTO_INCREMENT PRIMARY KEY,
  `name`            VARCHAR(64)  NOT NULL          COMMENT 'Nombre visible en comandos y mensajes',
  `type`            TINYINT      NOT NULL DEFAULT 1 COMMENT '1=Vendedor 2=Saqueador',
  `creature_entry`  INT UNSIGNED NOT NULL           COMMENT 'Entry en creature_template',
  `description`     VARCHAR(255) NOT NULL DEFAULT '' COMMENT 'Descripción corta',
  `speed`           FLOAT        NOT NULL DEFAULT 1.0 COMMENT 'Multiplicador de velocidad',
  `item_entry`      INT UNSIGNED NOT NULL DEFAULT 0  COMMENT 'Ítem tomo que enseña esta mascota (0 = solo admin)',
  `spell_id`        INT UNSIGNED NOT NULL DEFAULT 0  COMMENT 'Hechizo de compañero en spell_dbc (0 = sin hechizo)',
  `enabled`         TINYINT(1)   NOT NULL DEFAULT 1  COMMENT '0 = desactivado'
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ── Creature template: Mercader Ambulante ──────────────────
DELETE FROM `creature_template` WHERE `entry` = 601000;
INSERT INTO `creature_template`
  (`entry`, `name`, `subname`, `faction`, `npcflag`,
   `speed_walk`, `speed_run`, `scale`, `minlevel`, `maxlevel`,
   `unit_class`, `unit_flags`, `type`, `RegenHealth`, `flags_extra`,
   `ScriptName`, `VerifiedBuild`)
VALUES
  (601000, 'Mercader Ambulante', 'Mascota Vendedora',
   35, 128, 1.0, 1.14286, 1.0, 1, 1,
   1, 2, 7, 1, 2,
   'npc_custom_pet_vendor', 0);

-- ── Modelo visual del Mercader Ambulante ───────────────────
DELETE FROM `creature_template_model` WHERE `CreatureID` = 601000;
INSERT INTO `creature_template_model` (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`)
VALUES (601000, 0, 49, 1.0, 1.0);

-- ── Tienda del Mercader Ambulante ──────────────────────────
DELETE FROM `npc_vendor` WHERE `entry` = 601000;
INSERT INTO `npc_vendor` (`entry`, `slot`, `item`, `maxcount`, `incrtime`, `ExtendedCost`, `VerifiedBuild`)
VALUES
(601000,  1,   118, 0, 0, 0, 0), (601000,  2,   858, 0, 0, 0, 0),
(601000,  3,   929, 0, 0, 0, 0), (601000,  4,  1710, 0, 0, 0, 0),
(601000,  5,  3928, 0, 0, 0, 0), (601000,  6, 13446, 0, 0, 0, 0),
(601000,  7,  2455, 0, 0, 0, 0), (601000,  8,  3385, 0, 0, 0, 0),
(601000,  9,  3827, 0, 0, 0, 0), (601000, 10,  6149, 0, 0, 0, 0),
(601000, 11, 13443, 0, 0, 0, 0), (601000, 12,  1251, 0, 0, 0, 0),
(601000, 13,  3530, 0, 0, 0, 0), (601000, 14,  6450, 0, 0, 0, 0),
(601000, 15,  8544, 0, 0, 0, 0), (601000, 16, 14529, 0, 0, 0, 0),
(601000, 17, 21990, 0, 0, 0, 0), (601000, 18,   117, 0, 0, 0, 0),
(601000, 19,   159, 0, 0, 0, 0);

-- ============================================================
-- HECHIZOS DE COMPAÑERO (spell_dbc)
-- ============================================================
-- Estos hechizos permiten que las mascotas aparezcan en el libro
-- de hechizos del personaje y, en la mayoría de clientes 3.3.5a,
-- en la pestaña de Compañeros al recibir los datos del servidor.
--
-- Campos clave:
--   Effect_1 = 28  → SPELL_EFFECT_SUMMON
--   EffectMiscValue_1  = creature_entry (critter a invocar)
--   EffectMiscValueB_1 = 61 → SummonProperties ID para MINIPET
--   ImplicitTargetA_1  = 1  → TARGET_UNIT_CASTER
--   CastingTimeIndex   = 1  → instantáneo
--   RangeIndex         = 1  → distancia de self
--
-- El SpellScript 'spell_custom_pet_companion_summon' intercepta
-- SPELL_EFFECT_SUMMON y hace el summon manual + actualiza el tracker.
-- ============================================================

DELETE FROM `spell_dbc` WHERE `ID` IN (901000, 901001);

-- ── Hechizo 901000: Mercader Ambulante ─────────────────────
INSERT INTO `spell_dbc`
  (`ID`, `Category`, `DispelType`, `Mechanic`,
   `Attributes`, `AttributesEx`, `AttributesEx2`, `AttributesEx3`,
   `AttributesEx4`, `AttributesEx5`, `AttributesEx6`, `AttributesEx7`,
   `ShapeshiftMask`, `unk_320_2`, `ShapeshiftExclude`, `unk_320_3`,
   `Targets`, `TargetCreatureType`, `RequiresSpellFocus`, `FacingCasterFlags`,
   `CasterAuraState`, `TargetAuraState`, `ExcludeCasterAuraState`, `ExcludeTargetAuraState`,
   `CasterAuraSpell`, `TargetAuraSpell`, `ExcludeCasterAuraSpell`, `ExcludeTargetAuraSpell`,
   `CastingTimeIndex`, `RecoveryTime`, `CategoryRecoveryTime`,
   `InterruptFlags`, `AuraInterruptFlags`, `ChannelInterruptFlags`,
   `ProcTypeMask`, `ProcChance`, `ProcCharges`,
   `MaxLevel`, `BaseLevel`, `SpellLevel`, `DurationIndex`,
   `PowerType`, `ManaCost`, `ManaCostPerLevel`, `ManaPerSecond`, `ManaPerSecondPerLevel`,
   `RangeIndex`, `Speed`, `ModalNextSpell`, `CumulativeAura`,
   `Totem_1`, `Totem_2`,
   `Reagent_1`, `Reagent_2`, `Reagent_3`, `Reagent_4`,
   `Reagent_5`, `Reagent_6`, `Reagent_7`, `Reagent_8`,
   `ReagentCount_1`, `ReagentCount_2`, `ReagentCount_3`, `ReagentCount_4`,
   `ReagentCount_5`, `ReagentCount_6`, `ReagentCount_7`, `ReagentCount_8`,
   `EquippedItemClass`, `EquippedItemSubclass`, `EquippedItemInvTypes`,
   `Effect_1`, `Effect_2`, `Effect_3`,
   `EffectDieSides_1`, `EffectDieSides_2`, `EffectDieSides_3`,
   `EffectRealPointsPerLevel_1`, `EffectRealPointsPerLevel_2`, `EffectRealPointsPerLevel_3`,
   `EffectBasePoints_1`, `EffectBasePoints_2`, `EffectBasePoints_3`,
   `EffectMechanic_1`, `EffectMechanic_2`, `EffectMechanic_3`,
   `ImplicitTargetA_1`, `ImplicitTargetA_2`, `ImplicitTargetA_3`,
   `ImplicitTargetB_1`, `ImplicitTargetB_2`, `ImplicitTargetB_3`,
   `EffectRadiusIndex_1`, `EffectRadiusIndex_2`, `EffectRadiusIndex_3`,
   `EffectAura_1`, `EffectAura_2`, `EffectAura_3`,
   `EffectAuraPeriod_1`, `EffectAuraPeriod_2`, `EffectAuraPeriod_3`,
   `EffectMultipleValue_1`, `EffectMultipleValue_2`, `EffectMultipleValue_3`,
   `EffectChainTargets_1`, `EffectChainTargets_2`, `EffectChainTargets_3`,
   `EffectItemType_1`, `EffectItemType_2`, `EffectItemType_3`,
   `EffectMiscValue_1`, `EffectMiscValue_2`, `EffectMiscValue_3`,
   `EffectMiscValueB_1`, `EffectMiscValueB_2`, `EffectMiscValueB_3`,
   `EffectTriggerSpell_1`, `EffectTriggerSpell_2`, `EffectTriggerSpell_3`,
   `EffectPointsPerCombo_1`, `EffectPointsPerCombo_2`, `EffectPointsPerCombo_3`,
   `EffectSpellClassMaskA_1`, `EffectSpellClassMaskA_2`, `EffectSpellClassMaskA_3`,
   `EffectSpellClassMaskB_1`, `EffectSpellClassMaskB_2`, `EffectSpellClassMaskB_3`,
   `EffectSpellClassMaskC_1`, `EffectSpellClassMaskC_2`, `EffectSpellClassMaskC_3`,
   `SpellVisualID_1`, `SpellVisualID_2`,
   `SpellIconID`, `ActiveIconID`, `SpellPriority`,
   `Name_Lang_enUS`, `Name_Lang_enGB`, `Name_Lang_koKR`, `Name_Lang_frFR`,
   `Name_Lang_deDE`, `Name_Lang_enCN`, `Name_Lang_zhCN`, `Name_Lang_enTW`,
   `Name_Lang_zhTW`, `Name_Lang_esES`, `Name_Lang_esMX`, `Name_Lang_ruRU`,
   `Name_Lang_ptPT`, `Name_Lang_ptBR`, `Name_Lang_itIT`, `Name_Lang_Unk`,
   `Name_Lang_Mask`,
   `NameSubtext_Lang_enUS`, `NameSubtext_Lang_enGB`, `NameSubtext_Lang_koKR`,
   `NameSubtext_Lang_frFR`, `NameSubtext_Lang_deDE`, `NameSubtext_Lang_enCN`,
   `NameSubtext_Lang_zhCN`, `NameSubtext_Lang_enTW`, `NameSubtext_Lang_zhTW`,
   `NameSubtext_Lang_esES`, `NameSubtext_Lang_esMX`, `NameSubtext_Lang_ruRU`,
   `NameSubtext_Lang_ptPT`, `NameSubtext_Lang_ptBR`, `NameSubtext_Lang_itIT`,
   `NameSubtext_Lang_Unk`, `NameSubtext_Lang_Mask`,
   `Description_Lang_enUS`, `Description_Lang_enGB`, `Description_Lang_koKR`,
   `Description_Lang_frFR`, `Description_Lang_deDE`, `Description_Lang_enCN`,
   `Description_Lang_zhCN`, `Description_Lang_enTW`, `Description_Lang_zhTW`,
   `Description_Lang_esES`, `Description_Lang_esMX`, `Description_Lang_ruRU`,
   `Description_Lang_ptPT`, `Description_Lang_ptBR`, `Description_Lang_itIT`,
   `Description_Lang_Unk`, `Description_Lang_Mask`,
   `AuraDescription_Lang_enUS`, `AuraDescription_Lang_enGB`, `AuraDescription_Lang_koKR`,
   `AuraDescription_Lang_frFR`, `AuraDescription_Lang_deDE`, `AuraDescription_Lang_enCN`,
   `AuraDescription_Lang_zhCN`, `AuraDescription_Lang_enTW`, `AuraDescription_Lang_zhTW`,
   `AuraDescription_Lang_esES`, `AuraDescription_Lang_esMX`, `AuraDescription_Lang_ruRU`,
   `AuraDescription_Lang_ptPT`, `AuraDescription_Lang_ptBR`, `AuraDescription_Lang_itIT`,
   `AuraDescription_Lang_Unk`, `AuraDescription_Lang_Mask`,
   `ManaCostPct`, `StartRecoveryCategory`, `StartRecoveryTime`,
   `MaxTargetLevel`, `SpellClassSet`,
   `SpellClassMask_1`, `SpellClassMask_2`, `SpellClassMask_3`,
   `MaxTargets`, `DefenseType`, `PreventionType`, `StanceBarOrder`,
   `EffectChainAmplitude_1`, `EffectChainAmplitude_2`, `EffectChainAmplitude_3`,
   `MinFactionID`, `MinReputation`, `RequiredAuraVision`,
   `RequiredTotemCategoryID_1`, `RequiredTotemCategoryID_2`,
   `RequiredAreasID`, `SchoolMask`, `RuneCostID`, `SpellMissileID`, `PowerDisplayID`,
   `EffectBonusMultiplier_1`, `EffectBonusMultiplier_2`, `EffectBonusMultiplier_3`,
   `SpellDescriptionVariableID`, `SpellDifficultyID`)
VALUES
  (901000,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
   1,0,0, 0,0,0, 0,0,0, 0,0,0,0, 0,0,0,0,0, 1,0,0,0, 0,0,
   0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
   -1,0,0,
   28,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,
   1,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,
   601000,0,0, 61,0,0, 0,0,0, 0,0,0, 0,0,0,0,0,0,0,0,0,
   0,0, 1,0,0,
   'Mercader Ambulante','','','','','','','','','','','','','','','',16712190,
   'Mascota Vendedora','','','','','','','','','','','','','','','',16712190,
   'Invoca a tu Mercader Ambulante personal. Vuelve a lanzarlo para despedirlo.','','','','','','','','','','','','','','','',16712190,
   '','','','','','','','','','','','','','','','',16712188,
   0,0,0, 0,0, 0,0,0, 0,0,0,0, 1,1,1, 0,0,0, 0,0, 0,1,0,0,0, 0,0,0, 0,0);

-- ── Hechizo 901001: Recolector Fiel ────────────────────────
INSERT INTO `spell_dbc`
  (`ID`, `Category`, `DispelType`, `Mechanic`,
   `Attributes`, `AttributesEx`, `AttributesEx2`, `AttributesEx3`,
   `AttributesEx4`, `AttributesEx5`, `AttributesEx6`, `AttributesEx7`,
   `ShapeshiftMask`, `unk_320_2`, `ShapeshiftExclude`, `unk_320_3`,
   `Targets`, `TargetCreatureType`, `RequiresSpellFocus`, `FacingCasterFlags`,
   `CasterAuraState`, `TargetAuraState`, `ExcludeCasterAuraState`, `ExcludeTargetAuraState`,
   `CasterAuraSpell`, `TargetAuraSpell`, `ExcludeCasterAuraSpell`, `ExcludeTargetAuraSpell`,
   `CastingTimeIndex`, `RecoveryTime`, `CategoryRecoveryTime`,
   `InterruptFlags`, `AuraInterruptFlags`, `ChannelInterruptFlags`,
   `ProcTypeMask`, `ProcChance`, `ProcCharges`,
   `MaxLevel`, `BaseLevel`, `SpellLevel`, `DurationIndex`,
   `PowerType`, `ManaCost`, `ManaCostPerLevel`, `ManaPerSecond`, `ManaPerSecondPerLevel`,
   `RangeIndex`, `Speed`, `ModalNextSpell`, `CumulativeAura`,
   `Totem_1`, `Totem_2`,
   `Reagent_1`, `Reagent_2`, `Reagent_3`, `Reagent_4`,
   `Reagent_5`, `Reagent_6`, `Reagent_7`, `Reagent_8`,
   `ReagentCount_1`, `ReagentCount_2`, `ReagentCount_3`, `ReagentCount_4`,
   `ReagentCount_5`, `ReagentCount_6`, `ReagentCount_7`, `ReagentCount_8`,
   `EquippedItemClass`, `EquippedItemSubclass`, `EquippedItemInvTypes`,
   `Effect_1`, `Effect_2`, `Effect_3`,
   `EffectDieSides_1`, `EffectDieSides_2`, `EffectDieSides_3`,
   `EffectRealPointsPerLevel_1`, `EffectRealPointsPerLevel_2`, `EffectRealPointsPerLevel_3`,
   `EffectBasePoints_1`, `EffectBasePoints_2`, `EffectBasePoints_3`,
   `EffectMechanic_1`, `EffectMechanic_2`, `EffectMechanic_3`,
   `ImplicitTargetA_1`, `ImplicitTargetA_2`, `ImplicitTargetA_3`,
   `ImplicitTargetB_1`, `ImplicitTargetB_2`, `ImplicitTargetB_3`,
   `EffectRadiusIndex_1`, `EffectRadiusIndex_2`, `EffectRadiusIndex_3`,
   `EffectAura_1`, `EffectAura_2`, `EffectAura_3`,
   `EffectAuraPeriod_1`, `EffectAuraPeriod_2`, `EffectAuraPeriod_3`,
   `EffectMultipleValue_1`, `EffectMultipleValue_2`, `EffectMultipleValue_3`,
   `EffectChainTargets_1`, `EffectChainTargets_2`, `EffectChainTargets_3`,
   `EffectItemType_1`, `EffectItemType_2`, `EffectItemType_3`,
   `EffectMiscValue_1`, `EffectMiscValue_2`, `EffectMiscValue_3`,
   `EffectMiscValueB_1`, `EffectMiscValueB_2`, `EffectMiscValueB_3`,
   `EffectTriggerSpell_1`, `EffectTriggerSpell_2`, `EffectTriggerSpell_3`,
   `EffectPointsPerCombo_1`, `EffectPointsPerCombo_2`, `EffectPointsPerCombo_3`,
   `EffectSpellClassMaskA_1`, `EffectSpellClassMaskA_2`, `EffectSpellClassMaskA_3`,
   `EffectSpellClassMaskB_1`, `EffectSpellClassMaskB_2`, `EffectSpellClassMaskB_3`,
   `EffectSpellClassMaskC_1`, `EffectSpellClassMaskC_2`, `EffectSpellClassMaskC_3`,
   `SpellVisualID_1`, `SpellVisualID_2`,
   `SpellIconID`, `ActiveIconID`, `SpellPriority`,
   `Name_Lang_enUS`, `Name_Lang_enGB`, `Name_Lang_koKR`, `Name_Lang_frFR`,
   `Name_Lang_deDE`, `Name_Lang_enCN`, `Name_Lang_zhCN`, `Name_Lang_enTW`,
   `Name_Lang_zhTW`, `Name_Lang_esES`, `Name_Lang_esMX`, `Name_Lang_ruRU`,
   `Name_Lang_ptPT`, `Name_Lang_ptBR`, `Name_Lang_itIT`, `Name_Lang_Unk`,
   `Name_Lang_Mask`,
   `NameSubtext_Lang_enUS`, `NameSubtext_Lang_enGB`, `NameSubtext_Lang_koKR`,
   `NameSubtext_Lang_frFR`, `NameSubtext_Lang_deDE`, `NameSubtext_Lang_enCN`,
   `NameSubtext_Lang_zhCN`, `NameSubtext_Lang_enTW`, `NameSubtext_Lang_zhTW`,
   `NameSubtext_Lang_esES`, `NameSubtext_Lang_esMX`, `NameSubtext_Lang_ruRU`,
   `NameSubtext_Lang_ptPT`, `NameSubtext_Lang_ptBR`, `NameSubtext_Lang_itIT`,
   `NameSubtext_Lang_Unk`, `NameSubtext_Lang_Mask`,
   `Description_Lang_enUS`, `Description_Lang_enGB`, `Description_Lang_koKR`,
   `Description_Lang_frFR`, `Description_Lang_deDE`, `Description_Lang_enCN`,
   `Description_Lang_zhCN`, `Description_Lang_enTW`, `Description_Lang_zhTW`,
   `Description_Lang_esES`, `Description_Lang_esMX`, `Description_Lang_ruRU`,
   `Description_Lang_ptPT`, `Description_Lang_ptBR`, `Description_Lang_itIT`,
   `Description_Lang_Unk`, `Description_Lang_Mask`,
   `AuraDescription_Lang_enUS`, `AuraDescription_Lang_enGB`, `AuraDescription_Lang_koKR`,
   `AuraDescription_Lang_frFR`, `AuraDescription_Lang_deDE`, `AuraDescription_Lang_enCN`,
   `AuraDescription_Lang_zhCN`, `AuraDescription_Lang_enTW`, `AuraDescription_Lang_zhTW`,
   `AuraDescription_Lang_esES`, `AuraDescription_Lang_esMX`, `AuraDescription_Lang_ruRU`,
   `AuraDescription_Lang_ptPT`, `AuraDescription_Lang_ptBR`, `AuraDescription_Lang_itIT`,
   `AuraDescription_Lang_Unk`, `AuraDescription_Lang_Mask`,
   `ManaCostPct`, `StartRecoveryCategory`, `StartRecoveryTime`,
   `MaxTargetLevel`, `SpellClassSet`,
   `SpellClassMask_1`, `SpellClassMask_2`, `SpellClassMask_3`,
   `MaxTargets`, `DefenseType`, `PreventionType`, `StanceBarOrder`,
   `EffectChainAmplitude_1`, `EffectChainAmplitude_2`, `EffectChainAmplitude_3`,
   `MinFactionID`, `MinReputation`, `RequiredAuraVision`,
   `RequiredTotemCategoryID_1`, `RequiredTotemCategoryID_2`,
   `RequiredAreasID`, `SchoolMask`, `RuneCostID`, `SpellMissileID`, `PowerDisplayID`,
   `EffectBonusMultiplier_1`, `EffectBonusMultiplier_2`, `EffectBonusMultiplier_3`,
   `SpellDescriptionVariableID`, `SpellDifficultyID`)
VALUES
  (901001,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
   1,0,0, 0,0,0, 0,0,0, 0,0,0,0, 0,0,0,0,0, 1,0,0,0, 0,0,
   0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
   -1,0,0,
   28,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,
   1,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,
   601001,0,0, 61,0,0, 0,0,0, 0,0,0, 0,0,0,0,0,0,0,0,0,
   0,0, 1,0,0,
   'Recolector Fiel','','','','','','','','','','','','','','','',16712190,
   'Mascota Saqueadora','','','','','','','','','','','','','','','',16712190,
   'Invoca a tu Recolector Fiel. Saqueará los cadáveres cercanos automáticamente. Vuelve a lanzarlo para despedirlo.','','','','','','','','','','','','','','','',16712190,
   '','','','','','','','','','','','','','','','',16712188,
   0,0,0, 0,0, 0,0,0, 0,0,0,0, 1,1,1, 0,0,0, 0,0, 0,1,0,0,0, 0,0,0, 0,0);

-- ── Vincular hechizos al SpellScript ───────────────────────
-- El nombre 'spell_custom_pet_companion_summon' debe coincidir
-- con el nombre de clase en CustomPetCompanionSpell.cpp
DELETE FROM `spell_script_names` WHERE `spell_id` IN (901000, 901001)
  AND `ScriptName` = 'spell_custom_pet_companion_summon';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
VALUES
  (901000, 'spell_custom_pet_companion_summon'),
  (901001, 'spell_custom_pet_companion_summon');

-- ============================================================
-- MASCOTA VENDEDORA (tipo 1 – CUSTOM_PET_VENDOR)
-- ============================================================

-- ── Ítem tomo: Tomo del Mercader Ambulante ─────────────────
DELETE FROM `item_template` WHERE `entry` = 601100;
INSERT INTO `item_template`
    (`entry`, `class`, `subclass`, `SoundOverrideSubclass`, `name`, `displayid`,
     `Quality`, `Flags`, `FlagsExtra`, `BuyCount`, `BuyPrice`, `SellPrice`,
     `InventoryType`, `AllowableClass`, `AllowableRace`, `ItemLevel`, `RequiredLevel`,
     `MaxCount`, `stackable`, `ContainerSlots`,
     `spellid_1`, `spelltrigger_1`, `spellcharges_1`, `spellcooldown_1`, `spellcategory_1`, `spellcategorycooldown_1`,
     `bonding`, `description`, `ScriptName`)
VALUES
    (601100, 15, 0, -1, 'Tomo del Mercader Ambulante', 8840,
     4, 0, 0, 1, 0, 0,
     0, -1, -1, 1, 0,
     1, 1, 0,
     5001, 0, 0, -1, 0, -1,
     4, 'Te enseña a invocar a tu propio Mercader Ambulante portátil.', 'item_custom_pet_tome');

-- ── Registro de la primera mascota ─────────────────────────
INSERT INTO `mod_custom_pets`
  (`id`, `name`, `type`, `creature_entry`, `description`, `speed`, `item_entry`, `spell_id`, `enabled`)
VALUES
  (1, 'Mercader Ambulante', 1, 601000,
   'Mercader portátil: pociones, vendajes y provisiones básicas.',
   1.0, 601100, 901000, 1);

-- ============================================================
-- MASCOTA SAQUEADORA (tipo 2 – CUSTOM_PET_LOOTER)
-- ============================================================

-- ── Creature template: Recolector Fiel ─────────────────────
DELETE FROM `creature_template` WHERE `entry` = 601001;
INSERT INTO `creature_template`
  (`entry`, `name`, `subname`, `faction`, `npcflag`,
   `speed_walk`, `speed_run`, `scale`, `minlevel`, `maxlevel`,
   `unit_class`, `unit_flags`, `type`, `RegenHealth`, `flags_extra`,
   `ScriptName`, `VerifiedBuild`)
VALUES
  (601001, 'Recolector Fiel', 'Mascota Saqueadora',
   35, 0, 1.0, 1.14286, 0.6, 1, 1,
   1, 33554434, 7, 1, 2,
   'npc_custom_pet_looter', 0);

-- ── Modelo visual del Recolector Fiel ──────────────────────
DELETE FROM `creature_template_model` WHERE `CreatureID` = 601001;
INSERT INTO `creature_template_model` (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`)
VALUES (601001, 0, 55, 1.0, 1.0);

-- ── Ítem tomo: Tomo del Recolector Fiel ────────────────────
DELETE FROM `item_template` WHERE `entry` = 601101;
INSERT INTO `item_template`
    (`entry`, `class`, `subclass`, `SoundOverrideSubclass`, `name`, `displayid`,
     `Quality`, `Flags`, `FlagsExtra`, `BuyCount`, `BuyPrice`, `SellPrice`,
     `InventoryType`, `AllowableClass`, `AllowableRace`, `ItemLevel`, `RequiredLevel`,
     `MaxCount`, `stackable`, `ContainerSlots`,
     `spellid_1`, `spelltrigger_1`, `spellcharges_1`, `spellcooldown_1`, `spellcategory_1`, `spellcategorycooldown_1`,
     `bonding`, `description`, `ScriptName`)
VALUES
    (601101, 15, 0, -1, 'Tomo del Recolector Fiel', 8840,
     4, 0, 0, 1, 0, 0,
     0, -1, -1, 1, 0,
     1, 1, 0,
     5001, 0, 0, -1, 0, -1,
     4, 'Te enseña a invocar a tu propio Recolector Fiel, que saqueará los cadáveres cercanos.', 'item_custom_pet_tome');

-- ── Registro de la mascota saqueadora ──────────────────────
INSERT INTO `mod_custom_pets`
  (`id`, `name`, `type`, `creature_entry`, `description`, `speed`, `item_entry`, `spell_id`, `enabled`)
VALUES
  (2, 'Recolector Fiel', 2, 601001,
   'Saquea automáticamente los cadáveres en un radio de 40 unidades.',
   1.8, 601101, 901001, 1);
