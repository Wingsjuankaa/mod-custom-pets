#include "CustomPets.h"
#include "ScriptMgr.h"
#include "Config.h"
#include "Chat.h"
#include "Item.h"
#include "SpellMgr.h"

// ──────────────────────────────────────────────────────────────────────────────
// item_custom_pet_tome
//
// ItemScript genérico para todos los "Tomos de Mascota" del módulo.
// Al usarlo (clic derecho), identifica a qué mascota corresponde el ítem
// (via mod_custom_pets.item_entry), verifica si el jugador ya la conoce,
// la aprende, destruye el ítem y notifica al jugador.
//
// La entrada en item_template debe tener:
//   spellid_1     = 5001  (spell DUMMY, activo en el DBC del cliente 3.3.5a)
//   spelltrigger_1= 0     (ON_USE)
//   spellcharges_1= 0     (no auto-consume; lo hace el script manualmente)
//   ScriptName    = 'item_custom_pet_tome'
// ──────────────────────────────────────────────────────────────────────────────
class item_custom_pet_tome : public ItemScript
{
public:
    item_custom_pet_tome() : ItemScript("item_custom_pet_tome") {}

    bool OnUse(Player* player, Item* item, SpellCastTargets const& /*targets*/) override
    {
        if (!sConfigMgr->GetOption<bool>("CustomPets.Enable", true))
            return true; // módulo desactivado, consumir acción pero no el ítem

        // Buscar la mascota que corresponde a este entry de ítem
        CustomPetData petData;
        if (!sCustomPetsMgr->GetPetByItemEntry(item->GetEntry(), petData))
        {
            // No hay ninguna mascota asignada a este ítem; dejar que el servidor
            // procese el spell normal (no debería ocurrir en uso normal)
            return false;
        }

        uint32 playerGuid = player->GetGUID().GetCounter();

        // ── ¿Ya la conoce? ────────────────────────────────────────────────
        if (sCustomPetsMgr->HasLearnedPet(playerGuid, petData.id))
        {
            ChatHandler(player->GetSession()).PSendSysMessage(
                "|cffffff00Ya conoces la mascota '{}'. "
                "Usa .custompet summon {} para invocarla.|r",
                petData.name, petData.id);

            // Devolver true para "anular" el uso del ítem SIN consumirlo.
            return true;
        }

        // ── Aprender la mascota ───────────────────────────────────────────
        sCustomPetsMgr->LearnPet(playerGuid, petData.id);

        // ── Enseñar el hechizo de compañero (para la pestaña de Compañeros) ──
        if (petData.spell_id > 0 && !player->HasSpell(petData.spell_id))
            player->learnSpell(petData.spell_id);

        // ── Destruir el ítem (equivale al consumo del tomo) ───────────────
        player->DestroyItemCount(item->GetEntry(), 1, true, false);

        // ── Notificar al jugador ──────────────────────────────────────────
        ChatHandler(player->GetSession()).PSendSysMessage(
            "|cff00ff00Has aprendido la mascota: {}!|r",
            petData.name);

        ChatHandler(player->GetSession()).PSendSysMessage(
            "|cffaaaaaa(Usa .custompet summon {} para invocarla · "
            ".custompet mylist para ver tus mascotas)|r",
            petData.id);

        return true; // acción manejada; no castear el spell dummy
    }
};

// ──────────────────────────────────────────────────────────────────────────────
// Registro: llamado desde AddCustomPetsScripts() en CustomPets.cpp
// ──────────────────────────────────────────────────────────────────────────────
void AddCustomPetLearnSystemScripts()
{
    new item_custom_pet_tome();
}

