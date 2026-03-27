#include "CustomPets.h"
#include "ScriptMgr.h"
#include "Config.h"
#include "Chat.h"
#include "Map.h"
#include "LootMgr.h"
#include "Item.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include <unordered_map>
#include <mutex>

// ──────────────────────────────────────────────────────────────────────────────
// Constantes de comportamiento
// ──────────────────────────────────────────────────────────────────────────────
static constexpr uint32 LOOTER_SCAN_INTERVAL_MS = 1500;  // cada cuánto escanea (ms)
static constexpr float  LOOTER_RANGE            = 40.0f; // radio de búsqueda de cadáveres

// ──────────────────────────────────────────────────────────────────────────────
// Check para el buscador de cuadrícula: cadáver looteable dentro del rango
// ──────────────────────────────────────────────────────────────────────────────
struct LootableCorpseCheck
{
    Player const* _player;
    float         _range;

    LootableCorpseCheck(Player const* player, float range)
        : _player(player), _range(range) {}

    bool operator()(Creature* c) const
    {
        return !c->IsAlive()
            && c->HasDynamicFlag(UNIT_DYNFLAG_LOOTABLE)
            && !c->loot.empty()
            && _player->IsWithinDistInMap(c, _range, false);
    }
};

// ──────────────────────────────────────────────────────────────────────────────
// Núcleo del looteo automático
//
// Itera directamente los vectores items / quest_items de la Loot del cadáver,
// almacena cada objeto en el inventario del jugador y cierra el cadáver cuando
// ya no queda nada.
// ──────────────────────────────────────────────────────────────────────────────
static void AutoLootCreatureCorpse(Player* player, Creature* creature)
{
    Loot& loot = creature->loot;

    // ── Oro ──────────────────────────────────────────────────────────────────
    if (loot.gold > 0)
    {
        player->ModifyMoney(loot.gold);
        loot.NotifyMoneyRemoved();
        loot.gold = 0;
    }

    // ── Objetos normales ─────────────────────────────────────────────────────
    for (LootItem& item : loot.items)
    {
        if (item.is_looted)
            continue;

        // AllowedForPlayer: comprueba permisos de grupo, quests, condiciones…
        if (!item.AllowedForPlayer(player, loot.sourceWorldObjectGUID))
            continue;

        // No intentar lootear objetos bloqueados (necesitan tirada de grupo)
        if (item.is_blocked)
            continue;

        ItemPosCountVec dest;
        InventoryResult canStore = player->CanStoreNewItem(
            NULL_BAG, NULL_SLOT, dest, item.itemid, item.count);

        if (canStore != EQUIP_ERR_OK)
            continue;

        AllowedLooterSet looters = item.GetAllowedLooters(); // copia no-const requerida por StoreNewItem
        Item* stored = player->StoreNewItem(
            dest, item.itemid, true, item.randomPropertyId, looters);

        if (stored)
        {
            player->SendNewItem(stored, item.count, false, false, true);
            item.is_looted = true;
            --loot.unlootedCount;
        }
    }

    // ── Objetos de quest ─────────────────────────────────────────────────────
    // FillNotNormalLootFor inicializa la lista personal de quest_items del
    // jugador; es necesario llamarla antes de iterar quest_items.
    loot.FillNotNormalLootFor(player);

    QuestItemMap const& questMap = loot.GetPlayerQuestItems();
    auto qIt = questMap.find(player->GetGUID());
    if (qIt != questMap.end())
    {
        for (QuestItem& qi : *qIt->second)
        {
            if (qi.is_looted)
                continue;

            LootItem& item = loot.quest_items[qi.index];
            if (!item.AllowedForPlayer(player, loot.sourceWorldObjectGUID))
                continue;

            ItemPosCountVec dest;
            InventoryResult canStore = player->CanStoreNewItem(
                NULL_BAG, NULL_SLOT, dest, item.itemid, item.count);

            if (canStore != EQUIP_ERR_OK)
                continue;

            AllowedLooterSet looters = item.GetAllowedLooters(); // copia no-const requerida por StoreNewItem
            Item* stored = player->StoreNewItem(
                dest, item.itemid, true, item.randomPropertyId, looters);

            if (stored)
            {
                player->SendNewItem(stored, item.count, false, false, true);
                qi.is_looted   = true;
                item.is_looted = true;
                --loot.unlootedCount;
            }
        }
    }

    // ── Cerrar cadáver si todo fue saqueado ──────────────────────────────────
    if (loot.isLooted())
    {
        creature->RemoveDynamicFlag(UNIT_DYNFLAG_LOOTABLE);
        creature->AllLootRemovedFromCorpse();
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Creature script: NPC del saqueador.
// No necesita gossip ni interacción — toda la lógica está en el PlayerScript.
// El ScriptName en creature_template apunta aquí para poder identificar el tipo
// de mascota desde el tracker.
// ──────────────────────────────────────────────────────────────────────────────
class npc_custom_pet_looter : public CreatureScript
{
public:
    npc_custom_pet_looter() : CreatureScript("npc_custom_pet_looter") {}
};

// ──────────────────────────────────────────────────────────────────────────────
// PlayerScript: motor del saqueador automático
//
// Cada LOOTER_SCAN_INTERVAL_MS milisegundos:
//   1. Busca cadáveres looteables en el radio LOOTER_RANGE.
//   2. Mueve la mascota hacia el cadáver más cercano (visual cosmético).
//   3. Saquea todos los cadáveres encontrados directamente al inventario.
// ──────────────────────────────────────────────────────────────────────────────
class CustomPetLooterPlayerScript : public PlayerScript
{
public:
    CustomPetLooterPlayerScript() : PlayerScript("CustomPetLooterPlayerScript") {}

    void OnPlayerUpdate(Player* player, uint32 diff) override
    {
        if (!sConfigMgr->GetOption<bool>("CustomPets.Enable", true))
            return;

        uint32 key = player->GetGUID().GetCounter();

        // Solo actuar si la mascota activa es de tipo LOOTER
        if (sCustomPetsTracker->GetType(key) != CUSTOM_PET_LOOTER)
            return;

        ObjectGuid petGuid = sCustomPetsTracker->GetGuid(key);
        if (petGuid.IsEmpty())
            return;

        // Si la criatura desapareció (muerte, teleport, etc.) limpiar el tracker
        Creature* pet = player->GetMap()->GetCreature(petGuid);
        if (!pet || !pet->IsAlive())
        {
            sCustomPetsTracker->Remove(key);
            return;
        }

        // ── Timer: limitar frecuencia del escaneo ──────────────────────────
        {
            std::lock_guard<std::mutex> lock(_timerMutex);
            uint32& timer = _scanTimers[key];
            if (timer > diff)
            {
                timer -= diff;
                return;
            }
            timer = LOOTER_SCAN_INTERVAL_MS;
        }

        // ── Buscar cadáveres looteables en el grid ─────────────────────────
        std::list<Creature*> targets;
        LootableCorpseCheck  check(player, LOOTER_RANGE);
        Acore::CreatureListSearcher<LootableCorpseCheck> searcher(player, targets, check);
        Cell::VisitObjects(player, searcher, LOOTER_RANGE);

        if (targets.empty())
        {
            // Sin objetivos: volver a seguir al jugador
            if (!pet->GetMotionMaster()->GetCurrentMovementGeneratorType() ||
                pet->GetMotionMaster()->GetCurrentMovementGeneratorType() == POINT_MOTION_TYPE)
                pet->GetMotionMaster()->MoveFollow(player, 2.0f, (float)M_PI);
            return;
        }

        // ── Mover la mascota hacia el cadáver más cercano (cosmético) ──────
        Creature* nearest = nullptr;
        float     minDist = LOOTER_RANGE + 1.0f;
        for (Creature* c : targets)
        {
            float d = pet->GetDistance(c);
            if (d < minDist) { minDist = d; nearest = c; }
        }
        if (nearest)
            pet->GetMotionMaster()->MovePoint(
                0,
                nearest->GetPositionX(),
                nearest->GetPositionY(),
                nearest->GetPositionZ());

        // ── Saquear todos los cadáveres encontrados ────────────────────────
        for (Creature* corpse : targets)
            AutoLootCreatureCorpse(player, corpse);
    }

    void OnPlayerLogout(Player* player) override
    {
        std::lock_guard<std::mutex> lock(_timerMutex);
        _scanTimers.erase(player->GetGUID().GetCounter());
    }

private:
    mutable std::mutex                 _timerMutex;
    std::unordered_map<uint32, uint32> _scanTimers; // playerKey → ms restantes
};

// ──────────────────────────────────────────────────────────────────────────────
// Registro: llamado desde AddCustomPetsScripts() en CustomPets.cpp
// ──────────────────────────────────────────────────────────────────────────────
void AddCustomPetLooterScripts()
{
    new npc_custom_pet_looter();
    new CustomPetLooterPlayerScript();
}

