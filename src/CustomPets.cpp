#include "CustomPets.h"
#include "ScriptMgr.h"
#include "Config.h"
#include "Log.h"
#include "Chat.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ──────────────────────────────────────────────────────────────────────────────
// CustomPetsMgr – implementación
// ──────────────────────────────────────────────────────────────────────────────
void CustomPetsMgr::LoadFromDB()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _pets.clear();

    QueryResult result = WorldDatabase.Query(
            "SELECT id, name, type, creature_entry, description, speed, item_entry, spell_id, enabled "
            "FROM mod_custom_pets");

    if (!result)
    {
        LOG_INFO("module", "CustomPetsMgr: Tabla mod_custom_pets vacía o inexistente.");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        CustomPetData data;
        data.id             = fields[0].Get<uint32>();
        data.name           = fields[1].Get<std::string>();
        data.type           = fields[2].Get<uint8>();
        data.creature_entry = fields[3].Get<uint32>();
        data.description    = fields[4].Get<std::string>();
        data.speed          = fields[5].Get<float>();
        data.item_entry     = fields[6].Get<uint32>();
        data.spell_id       = fields[7].Get<uint32>();
        data.enabled        = fields[8].Get<bool>();

        _pets.push_back(std::move(data));
    } while (result->NextRow());

    LOG_INFO("module", "CustomPetsMgr: {} mascota(s) cargada(s).", (uint32)_pets.size());
}

bool CustomPetsMgr::GetById(uint32 id, CustomPetData& out) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    for (const auto& p : _pets)
    {
        if (p.id == id && p.enabled)
        {
            out = p;
            return true;
        }
    }
    return false;
}

bool CustomPetsMgr::GetPetByItemEntry(uint32 itemEntry, CustomPetData& out) const
{
    if (itemEntry == 0)
        return false;

    std::lock_guard<std::mutex> lock(_mutex);
    for (const auto& p : _pets)
    {
        if (p.item_entry == itemEntry && p.enabled)
        {
            out = p;
            return true;
        }
    }
    return false;
}

bool CustomPetsMgr::GetPetBySpellId(uint32 spellId, CustomPetData& out) const
{
    if (spellId == 0)
        return false;

    std::lock_guard<std::mutex> lock(_mutex);
    for (const auto& p : _pets)
    {
        if (p.spell_id == spellId && p.enabled)
        {
            out = p;
            return true;
        }
    }
    return false;
}

std::vector<CustomPetData> CustomPetsMgr::GetAll() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _pets;
}

// ── Sistema de mascotas aprendidas ────────────────────────────────────────────

void CustomPetsMgr::LoadPlayerPets(uint32 playerGuid)
{
    QueryResult result = CharacterDatabase.Query(
        "SELECT pet_id FROM mod_custom_pets_learned WHERE player_guid = {}",
        playerGuid);

    std::lock_guard<std::mutex> lock(_learnedMutex);
    auto& petSet = _learnedPets[playerGuid];
    petSet.clear();

    if (!result)
        return;

    do
    {
        petSet.insert(result->Fetch()[0].Get<uint32>());
    } while (result->NextRow());
}

void CustomPetsMgr::UnloadPlayerPets(uint32 playerGuid)
{
    std::lock_guard<std::mutex> lock(_learnedMutex);
    _learnedPets.erase(playerGuid);
}

bool CustomPetsMgr::HasLearnedPet(uint32 playerGuid, uint32 petId) const
{
    std::lock_guard<std::mutex> lock(_learnedMutex);
    auto it = _learnedPets.find(playerGuid);
    if (it == _learnedPets.end())
        return false;
    return it->second.count(petId) > 0;
}

void CustomPetsMgr::LearnPet(uint32 playerGuid, uint32 petId)
{
    // Persiste en la base de datos de personajes
    CharacterDatabase.Execute(
        "INSERT IGNORE INTO mod_custom_pets_learned (player_guid, pet_id) VALUES ({}, {})",
        playerGuid, petId);

    // Actualiza la caché en memoria
    std::lock_guard<std::mutex> lock(_learnedMutex);
    _learnedPets[playerGuid].insert(petId);
}

std::vector<CustomPetData> CustomPetsMgr::GetLearnedPets(uint32 playerGuid) const
{
    std::vector<CustomPetData> result;

    std::unordered_set<uint32> learnedIds;
    {
        std::lock_guard<std::mutex> lock(_learnedMutex);
        auto it = _learnedPets.find(playerGuid);
        if (it != _learnedPets.end())
            learnedIds = it->second;
    }

    std::lock_guard<std::mutex> lock(_mutex);
    for (const auto& p : _pets)
    {
        if (p.enabled && learnedIds.count(p.id))
            result.push_back(p);
    }
    return result;
}

// ──────────────────────────────────────────────────────────────────────────────
// CustomPetsTracker – implementación
// ──────────────────────────────────────────────────────────────────────────────
void CustomPetsTracker::Set(uint32 playerKey, ObjectGuid creatureGuid, uint8 petType)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _active[playerKey] = { creatureGuid, petType };
}

void CustomPetsTracker::Remove(uint32 playerKey)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _active.erase(playerKey);
}

ObjectGuid CustomPetsTracker::GetGuid(uint32 playerKey) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _active.find(playerKey);
    return (it != _active.end()) ? it->second.guid : ObjectGuid::Empty;
}

uint8 CustomPetsTracker::GetType(uint32 playerKey) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _active.find(playerKey);
    return (it != _active.end()) ? it->second.type : 0;
}

bool CustomPetsTracker::Has(uint32 playerKey) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _active.count(playerKey) > 0;
}

// ──────────────────────────────────────────────────────────────────────────────
// DismissActivePet – helper global
// ──────────────────────────────────────────────────────────────────────────────
void DismissActivePet(Player* player)
{
    uint32 key = player->GetGUID().GetCounter();
    ObjectGuid petGuid = sCustomPetsTracker->GetGuid(key);

    if (!petGuid.IsEmpty())
    {
        if (Creature* pet = player->GetMap()->GetCreature(petGuid))
            pet->DespawnOrUnsummon();

        sCustomPetsTracker->Remove(key);
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Comandos: .custompet summon / dismiss / list / mylist / give / reload
// ──────────────────────────────────────────────────────────────────────────────
using namespace Acore::ChatCommands;

class CustomPets_CommandScript : public CommandScript
{
public:
    CustomPets_CommandScript() : CommandScript("CustomPets_CommandScript") {}

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable subTable =
        {
            { "summon",  HandleSummonCommand,  SEC_PLAYER,        Console::No  },
            { "dismiss", HandleDismissCommand, SEC_PLAYER,        Console::No  },
            { "list",    HandleListCommand,    SEC_PLAYER,        Console::No  },
            { "mylist",  HandleMyListCommand,  SEC_PLAYER,        Console::No  },
            { "give",    HandleGiveCommand,    SEC_ADMINISTRATOR, Console::No  },
            { "reload",  HandleReloadCommand,  SEC_ADMINISTRATOR, Console::No  },
        };
        static ChatCommandTable rootTable =
        {
            { "custompet", subTable }
        };
        return rootTable;
    }

    // ── .custompet summon <id> ────────────────────────────────────────────
    // Jugadores normales: requiere tener la mascota aprendida.
    // Administradores: pueden invocar cualquier mascota sin aprenderla.
    static bool HandleSummonCommand(ChatHandler* handler, uint32 petId)
    {
        if (!sConfigMgr->GetOption<bool>("CustomPets.Enable", true))
        {
            handler->SendSysMessage(
                "|cffff4444El módulo de mascotas custom está desactivado.|r");
            return true;
        }

        Player* player = handler->GetSession()->GetPlayer();
        bool isAdmin   = handler->GetSession()->GetSecurity() >= SEC_ADMINISTRATOR;

        CustomPetData petData;
        if (!sCustomPetsMgr->GetById(petId, petData))
        {
            handler->PSendSysMessage(
                "|cffff4444No existe ninguna mascota con ID {} o está desactivada.|r",
                petId);
            return false;
        }

        // Comprobar si la mascota ha sido aprendida (los admins se saltan este control)
        if (!isAdmin && !sCustomPetsMgr->HasLearnedPet(player->GetGUID().GetCounter(), petId))
        {
            std::string itemHint;
            if (petData.item_entry > 0)
                itemHint = " Busca el tomo correspondiente para aprenderla.";

            handler->PSendSysMessage(
                "|cffff4444No has aprendido la mascota '{}' todavía.|r"
                "|cffaaaaaa{}|r",
                petData.name, itemHint);
            return false;
        }

        // Despedir mascota anterior si ya había una activa
        DismissActivePet(player);

        // Invocar a espaldas del jugador
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
            handler->PSendSysMessage(
                "|cffff4444Error al invocar '{}' (entry {})."
                " Asegúrate de haber ejecutado el SQL y que el entry existe.|r",
                petData.name, petData.creature_entry);
            return false;
        }

        pet->GetMotionMaster()->MoveFollow(player, 2.0f, (float)M_PI);

        if (petData.speed != 1.0f)
        {
            pet->SetSpeed(MOVE_RUN,  petData.speed, true);
            pet->SetSpeed(MOVE_WALK, petData.speed, true);
        }

        sCustomPetsTracker->Set(player->GetGUID().GetCounter(), pet->GetGUID(), petData.type);

        handler->PSendSysMessage(
            "|cff00ff00{} ha sido invocado/a.|r "
            "|cffaaaaaa(Clic derecho para interactuar · .custompet dismiss para despedir)|r",
            petData.name);

        return true;
    }

    // ── .custompet dismiss ───────────────────────────────────────────────
    static bool HandleDismissCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();

        if (!sCustomPetsTracker->Has(player->GetGUID().GetCounter()))
        {
            handler->SendSysMessage(
                "|cffffff00No tienes ninguna mascota invocada.|r");
            return true;
        }

        DismissActivePet(player);
        handler->SendSysMessage("|cffaaaaaa¡Mascota despedida!|r");
        return true;
    }

    // ── .custompet list ───────────────────────────────────────────────────
    // Muestra todas las mascotas disponibles (aprenderlas o no).
    static bool HandleListCommand(ChatHandler* handler)
    {
        auto pets = sCustomPetsMgr->GetAll();

        if (pets.empty())
        {
            handler->SendSysMessage(
                "|cffff4444No hay mascotas custom configuradas.|r");
            return true;
        }

        uint32 playerGuid = handler->GetSession()->GetPlayer()->GetGUID().GetCounter();

        handler->PSendSysMessage(
            "|cffffff00=== Mascotas Custom disponibles ({}) ===|r",
            (uint32)pets.size());

        for (const auto& p : pets)
        {
            const char* stateStr = p.enabled
                ? "|cff00ff00[ON] |r"
                : "|cffff4444[OFF]|r";

            bool learned = sCustomPetsMgr->HasLearnedPet(playerGuid, p.id);
            const char* learnedStr = learned ? "|cff00ff00[Aprendida]|r " : "";

            handler->PSendSysMessage(
                " {}{}  |cffffff00[{}]|r |cffaaddff{}|r |cffddaa44({})|r"
                "  |cffaaaaaa{}|r",
                stateStr, learnedStr, p.id, p.name,
                GetCustomPetTypeName(p.type), p.description);
        }

        return true;
    }

    // ── .custompet mylist ─────────────────────────────────────────────────
    // Muestra únicamente las mascotas que el jugador ha aprendido,
    // con el comando para invocarlas.
    static bool HandleMyListCommand(ChatHandler* handler)
    {
        Player* player   = handler->GetSession()->GetPlayer();
        uint32 playerKey = player->GetGUID().GetCounter();

        auto pets = sCustomPetsMgr->GetLearnedPets(playerKey);

        if (pets.empty())
        {
            handler->SendSysMessage(
                "|cffffff00No has aprendido ninguna mascota todavía.|r "
                "|cffaaaaaa(Usa .custompet list para ver las disponibles)|r");
            return true;
        }

        // Comprueba si hay alguna activa ahora mismo
        uint8 activeType = sCustomPetsTracker->GetType(playerKey);

        handler->PSendSysMessage(
            "|cffffff00=== Tus mascotas aprendidas ({}) ===|r",
            (uint32)pets.size());

        for (const auto& p : pets)
        {
            bool isActive = sCustomPetsTracker->Has(playerKey) && (activeType == p.type);

            handler->PSendSysMessage(
                " |cffaaddff{}|r |cffddaa44({})|r{}  "
                "|cffaaaaaa→ .custompet summon {}|r",
                p.name,
                GetCustomPetTypeName(p.type),
                isActive ? " |cff00ff00[Activa]|r" : "",
                p.id);
        }

        return true;
    }

    // ── .custompet give <id> ─────────────────────────────────────────────
    // Comando de administrador: enseña una mascota al jugador seleccionado
    // (o al admin si no hay objetivo seleccionado).
    static bool HandleGiveCommand(ChatHandler* handler, uint32 petId)
    {
        Player* target = handler->getSelectedPlayer();
        if (!target)
            target = handler->GetSession()->GetPlayer();

        CustomPetData petData;
        if (!sCustomPetsMgr->GetById(petId, petData))
        {
            handler->PSendSysMessage(
                "|cffff4444No existe ninguna mascota con ID {} o está desactivada.|r",
                petId);
            return false;
        }

        uint32 targetGuid = target->GetGUID().GetCounter();

        if (sCustomPetsMgr->HasLearnedPet(targetGuid, petId))
        {
            handler->PSendSysMessage(
                "|cffffff00{} ya conoce la mascota '{}'.|r",
                target->GetName(), petData.name);
            return true;
        }

        sCustomPetsMgr->LearnPet(targetGuid, petId);

        // Notificar al jugador objetivo
        ChatHandler(target->GetSession()).PSendSysMessage(
            "|cff00ff00Has aprendido la mascota: {}!|r "
            "|cffaaaaaa(Usa .custompet summon {} para invocarla)|r",
            petData.name, petId);

        // Notificar al admin
        if (target != handler->GetSession()->GetPlayer())
        {
            handler->PSendSysMessage(
                "|cff00ff00Mascota '{}' enseñada a {}.|r",
                petData.name, target->GetName());
        }

        return true;
    }

    // ── .custompet reload ─────────────────────────────────────────────────
    static bool HandleReloadCommand(ChatHandler* handler)
    {
        sCustomPetsMgr->LoadFromDB();
        handler->SendSysMessage(
            "|cff00ff00Mascotas custom recargadas desde la base de datos.|r");
        return true;
    }
};

// ──────────────────────────────────────────────────────────────────────────────
// PlayerScript – eventos globales de ciclo de vida de la mascota
// ──────────────────────────────────────────────────────────────────────────────
class CustomPetsPlayerScript : public PlayerScript
{
public:
    CustomPetsPlayerScript() : PlayerScript("CustomPetsPlayerScript") {}

    // Login: carga las mascotas aprendidas del jugador en la caché
    void OnPlayerLogin(Player* player) override
    {
        sCustomPetsMgr->LoadPlayerPets(player->GetGUID().GetCounter());
    }

    // Cierre de sesión: libera la caché y despide la mascota
    void OnPlayerLogout(Player* player) override
    {
        DismissActivePet(player);
        sCustomPetsMgr->UnloadPlayerPets(player->GetGUID().GetCounter());
    }

    // Muerte del jugador
    void OnPlayerJustDied(Player* player) override
    {
        if (!sCustomPetsTracker->Has(player->GetGUID().GetCounter()))
            return;

        DismissActivePet(player);
    }

    // Detección de montura
    void OnPlayerUpdate(Player* player, uint32 /*diff*/) override
    {
        if (!sCustomPetsTracker->Has(player->GetGUID().GetCounter()))
            return;

        if (player->IsMounted())
            DismissActivePet(player);
    }
};

// ──────────────────────────────────────────────────────────────────────────────
// WorldScript – carga la BD al arrancar el servidor
// ──────────────────────────────────────────────────────────────────────────────
class CustomPetsWorldScript : public WorldScript
{
public:
    CustomPetsWorldScript() : WorldScript("CustomPetsWorldScript") {}

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        sCustomPetsMgr->LoadFromDB();
    }
};

// ──────────────────────────────────────────────────────────────────────────────
// Registro global
// ──────────────────────────────────────────────────────────────────────────────
void AddCustomPetsScripts()
{
    new CustomPets_CommandScript();
    new CustomPetsPlayerScript();
    new CustomPetsWorldScript();

    AddCustomPetVendorScripts();
    AddCustomPetLooterScripts();
    AddCustomPetLearnSystemScripts();
    AddCustomPetCompanionSpellScripts();
}
