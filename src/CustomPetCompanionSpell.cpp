#include "CustomPets.h"
#include "ScriptMgr.h"
#include "SpellScript.h"
#include "Chat.h"
#include "Map.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ──────────────────────────────────────────────────────────────────────────────
// spell_custom_pet_companion_summon
//
// SpellScript para los hechizos de compañero definidos en spell_dbc.
// Se registra en spell_script_names con el nombre 'spell_custom_pet_companion_summon'
// y se vincula a los spell_id de cada mascota (ej: 901000, 901001…).
//
// Flujo:
//   1. Jugador aprende la mascota (ítem tomo → OnUse → learnSpell(spell_id))
//   2. El hechizo aparece en el libro de hechizos / pestaña de Compañeros
//   3. Al lanzarlo, este SpellScript intercepta SPELL_EFFECT_SUMMON:
//      - Si la mascota ya está activa → la despide (toggle)
//      - Si no está activa → la invoca con la lógica de CustomPets
//   4. El tracker se actualiza para que el resto del módulo funcione
//      (looter, dismiss-on-mount, dismiss-on-death, etc.)
// ──────────────────────────────────────────────────────────────────────────────
class spell_custom_pet_companion_summon : public SpellScript
{
    PrepareSpellScript(spell_custom_pet_companion_summon);

    void HandleSummon(SpellEffIndex effIndex)
    {
        // Cancelamos el efecto de montura/summon por defecto del engine;
        // nosotros lo manejamos manualmente para actualizar el tracker.
        PreventHitDefaultEffect(effIndex);

        Player* player = GetCaster()->ToPlayer();
        if (!player)
            return;

        uint32 spellId   = GetSpellInfo()->Id;
        uint32 playerKey = player->GetGUID().GetCounter();

        // Buscar la mascota correspondiente a este hechizo
        CustomPetData petData;
        if (!sCustomPetsMgr->GetPetBySpellId(spellId, petData))
            return;

        // ── TOGGLE: si ya hay una mascota activa, despedirla ─────────────
        if (sCustomPetsTracker->Has(playerKey))
        {
            DismissActivePet(player);
            return;
        }

        // ── Invocar la mascota a espaldas del jugador ─────────────────────
        float o      = player->GetOrientation();
        float spawnX = player->GetPositionX() + 2.0f * std::cos(o + (float)M_PI);
        float spawnY = player->GetPositionY() + 2.0f * std::sin(o + (float)M_PI);
        float spawnZ = player->GetPositionZ();

        Creature* pet = player->SummonCreature(
            petData.creature_entry,
            spawnX, spawnY, spawnZ, o,
            TEMPSUMMON_MANUAL_DESPAWN, 0);

        if (!pet)
        {
            ChatHandler(player->GetSession()).PSendSysMessage(
                "|cffff4444Error al invocar '{}'. Comprueba la consola del servidor.|r",
                petData.name);
            return;
        }

        pet->GetMotionMaster()->MoveFollow(player, 2.0f, (float)M_PI);

        if (petData.speed != 1.0f)
        {
            pet->SetSpeed(MOVE_RUN,  petData.speed, true);
            pet->SetSpeed(MOVE_WALK, petData.speed, true);
        }

        sCustomPetsTracker->Set(playerKey, pet->GetGUID(), petData.type);

        ChatHandler(player->GetSession()).PSendSysMessage(
            "|cff00ff00{} invocado/a.|r "
            "|cffaaaaaa(Lanza el hechizo de nuevo o usa .custompet dismiss para despedirlo)|r",
            petData.name);
    }

    void Register() override
    {
        OnEffectHit += SpellEffectFn(
            spell_custom_pet_companion_summon::HandleSummon,
            EFFECT_0,
            SPELL_EFFECT_SUMMON);
    }
};

// ──────────────────────────────────────────────────────────────────────────────
// Registro: llamado desde AddCustomPetsScripts() en CustomPets.cpp
// El vínculo con spell IDs específicos se hace en spell_script_names (SQL).
// ──────────────────────────────────────────────────────────────────────────────
void AddCustomPetCompanionSpellScripts()
{
    RegisterSpellScript(spell_custom_pet_companion_summon);
}

