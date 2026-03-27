#include "CustomPets.h"
#include "ScriptMgr.h"
#include "Config.h"
#include "Log.h"
#include "Chat.h"
#include "Map.h"
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
        "SELECT id, name, type, creature_entry, description, enabled "
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
        data.enabled        = fields[5].Get<bool>();

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

std::vector<CustomPetData> CustomPetsMgr::GetAll() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _pets;
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
// DismissActivePet – helper global (declarado en CustomPets.h)
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
// Comandos: .custompet summon / dismiss / list / reload
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
            { "reload",  HandleReloadCommand,  SEC_ADMINISTRATOR, Console::No  },
        };
        static ChatCommandTable rootTable =
        {
            { "custompet", subTable }
        };
        return rootTable;
    }

    // .custompet summon <id>
    static bool HandleSummonCommand(ChatHandler* handler, uint32 petId)
    {
        if (!sConfigMgr->GetOption<bool>("CustomPets.Enable", true))
        {
            handler->SendSysMessage(
                "|cffff4444El módulo de mascotas custom está desactivado.|r");
            return true;
        }

        Player* player = handler->GetSession()->GetPlayer();

        // Despedir mascota anterior si ya había una activa
        DismissActivePet(player);

        CustomPetData petData;
        if (!sCustomPetsMgr->GetById(petId, petData))
        {
            handler->PSendSysMessage(
                "|cffff4444No existe ninguna mascota con ID {} o está desactivada.|r",
                petId);
            return false;
        }

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

        // La mascota sigue al jugador
        pet->GetMotionMaster()->MoveFollow(player, 2.0f, (float)M_PI);

        sCustomPetsTracker->Set(player->GetGUID().GetCounter(), pet->GetGUID(), petData.type);

        handler->PSendSysMessage(
            "|cff00ff00{} ha sido invocado/a.|r "
            "|cffaaaaaa(Clic derecho para interactuar · .custompet dismiss para despedir)|r",
            petData.name);

        return true;
    }

    // .custompet dismiss
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

    // .custompet list
    static bool HandleListCommand(ChatHandler* handler)
    {
        auto pets = sCustomPetsMgr->GetAll();

        if (pets.empty())
        {
            handler->SendSysMessage(
                "|cffff4444No hay mascotas custom configuradas.|r");
            return true;
        }

        handler->PSendSysMessage(
            "|cffffff00=== Mascotas Custom disponibles ({}) ===|r",
            (uint32)pets.size());

        for (const auto& p : pets)
        {
            const char* stateStr = p.enabled
                ? "|cff00ff00[ON] |r"
                : "|cffff4444[OFF]|r";

            handler->PSendSysMessage(
                " {}  |cffffff00[{}]|r |cffaaddff{}|r |cffddaa44({})|r"
                "  |cffaaaaaa{}|r"
                "  |cff888888→ .custompet summon {}|r",
                stateStr, p.id, p.name, GetCustomPetTypeName(p.type),
                p.description, p.id);
        }

        return true;
    }

    // .custompet reload
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
//
//  • OnPlayerLogout    → despide al cerrar sesión
//  • OnPlayerJustDied  → despide al morir (hook directo disponible)
//  • OnPlayerUpdate    → detecta transición a montura (no existe hook propio):
//                        si el jugador monta con una pet activa, la despide.
//                        El check es barato: solo actúa si Has() devuelve true.
// ──────────────────────────────────────────────────────────────────────────────
class CustomPetsPlayerScript : public PlayerScript
{
public:
    CustomPetsPlayerScript() : PlayerScript("CustomPetsPlayerScript") {}

    // Cierre de sesión
    void OnPlayerLogout(Player* player) override
    {
        DismissActivePet(player);
    }

    // Muerte del jugador
    void OnPlayerJustDied(Player* player) override
    {
        if (!sCustomPetsTracker->Has(player->GetGUID().GetCounter()))
            return;

        DismissActivePet(player);
    }

    // Detección de montura: se dispara en el tick en que IsMounted() pasa a true
    // OnPlayerUpdate se llama cada tick; el guard de Has() lo hace eficiente.
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
// Registro global: scripts compartidos + scripts de cada tipo de mascota.
// Para añadir un tipo nuevo: crea CustomPetXxx.cpp, declara AddCustomPetXxxScripts()
// en CustomPets.h y llámala aquí.
// ──────────────────────────────────────────────────────────────────────────────
void AddCustomPetsScripts()
{
    // Infraestructura compartida
    new CustomPets_CommandScript();
    new CustomPetsPlayerScript();
    new CustomPetsWorldScript();

    // Scripts por tipo de mascota
    AddCustomPetVendorScripts();
    AddCustomPetLooterScripts();
    // AddCustomPetBankerScripts();   // futuro
    // AddCustomPetRepairScripts();   // futuro
}
