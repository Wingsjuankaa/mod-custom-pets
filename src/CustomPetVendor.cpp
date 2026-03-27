#include "CustomPets.h"
#include "ScriptMgr.h"
#include "GossipDef.h"
#include "ScriptedGossip.h"
#include "WorldSession.h"

// ──────────────────────────────────────────────────────────────────────────────
// npc_custom_pet_vendor
//
// Script asociado al creature_template con ScriptName='npc_custom_pet_vendor'.
// Al hacer clic derecho sobre la mascota muestra un menú gossip con la opción
// de abrir la tienda. Los artículos se configuran en la tabla npc_vendor para
// el entry correspondiente.
// ──────────────────────────────────────────────────────────────────────────────
class npc_custom_pet_vendor : public CreatureScript
{
public:
    npc_custom_pet_vendor() : CreatureScript("npc_custom_pet_vendor") {}

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (creature->IsVendor())
            AddGossipItemFor(player, GOSSIP_ICON_VENDOR,
                "Ver tienda", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_TRADE);

        AddGossipItemFor(player, GOSSIP_ICON_CHAT,
            "¡Hasta luego!", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);

        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature,
                        uint32 /*sender*/, uint32 action) override
    {
        ClearGossipMenuFor(player);

        if (action == GOSSIP_ACTION_TRADE)
        {
            // Abre la ventana de tienda del NPC (requiere npcflag=128 en creature_template
            // y filas en npc_vendor para el entry de esta criatura)
            player->GetSession()->SendListInventory(creature->GetGUID());
        }
        else
        {
            // "¡Hasta luego!" – cierra el menú de conversación
            CloseGossipMenuFor(player);
        }

        return true;
    }
};

// ──────────────────────────────────────────────────────────────────────────────
// Registro: llamado desde AddCustomPetsScripts() en CustomPets.cpp
// ──────────────────────────────────────────────────────────────────────────────
void AddCustomPetVendorScripts()
{
    new npc_custom_pet_vendor();
}

