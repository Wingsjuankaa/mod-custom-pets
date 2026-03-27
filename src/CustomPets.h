#ifndef MOD_CUSTOM_PETS_H
#define MOD_CUSTOM_PETS_H

#include "Player.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "ObjectGuid.h"
#include <unordered_map>
#include <mutex>
#include <vector>
#include <string>

// ──────────────────────────────────────────────────────────────────────────────
// Tipos de mascota custom.
// Cada valor corresponde a una columna `type` en mod_custom_pets y a un
// archivo de script independiente (CustomPetVendor.cpp, CustomPetLooter.cpp…).
// ──────────────────────────────────────────────────────────────────────────────
enum CustomPetType : uint8
{
    CUSTOM_PET_VENDOR = 1,   // Abre ventana de tienda al interactuar
    CUSTOM_PET_LOOTER = 2,   // Saquea automáticamente los cadáveres cercanos
    // CUSTOM_PET_BANKER  = 3,   // Acceso al banco (futuro)
    // CUSTOM_PET_REPAIR  = 4,   // Reparación de equipo (futuro)
};

// Devuelve el nombre legible de un tipo (para comandos y logs)
inline const char* GetCustomPetTypeName(uint8 type)
{
    switch (type)
    {
        case CUSTOM_PET_VENDOR: return "Vendedor";
        case CUSTOM_PET_LOOTER: return "Saqueador";
        default:                return "Desconocido";
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Datos de una mascota cargados desde mod_custom_pets
// ──────────────────────────────────────────────────────────────────────────────
struct CustomPetData
{
    uint32      id;
    std::string name;
    uint8       type;
    uint32      creature_entry;
    std::string description;
    bool        enabled;
};

// ──────────────────────────────────────────────────────────────────────────────
// Manager singleton: carga y consulta definiciones de la BD
// ──────────────────────────────────────────────────────────────────────────────
class CustomPetsMgr
{
public:
    static CustomPetsMgr* instance()
    {
        static CustomPetsMgr inst;
        return &inst;
    }

    void LoadFromDB();
    bool GetById(uint32 id, CustomPetData& out) const;
    std::vector<CustomPetData> GetAll() const;

private:
    std::vector<CustomPetData> _pets;
    mutable std::mutex         _mutex;
};

#define sCustomPetsMgr CustomPetsMgr::instance()

// ──────────────────────────────────────────────────────────────────────────────
// Tracker: registra la mascota activa de cada jugador junto a su tipo.
// Clave = GUID bajo del jugador (uint32).
// Thread-safe: los mapas pueden ejecutar OnPlayerUpdate en paralelo.
// ──────────────────────────────────────────────────────────────────────────────
struct ActivePetInfo
{
    ObjectGuid guid;
    uint8      type{0};
};

class CustomPetsTracker
{
public:
    static CustomPetsTracker* instance()
    {
        static CustomPetsTracker inst;
        return &inst;
    }

    void Set(uint32 playerKey, ObjectGuid creatureGuid, uint8 petType);
    void Remove(uint32 playerKey);

    // Devuelve solo el GUID (ObjectGuid::Empty si no hay mascota activa)
    ObjectGuid GetGuid(uint32 playerKey) const;

    // Devuelve el tipo de la mascota activa (0 si no hay ninguna)
    uint8 GetType(uint32 playerKey) const;

    bool Has(uint32 playerKey) const;

private:
    std::unordered_map<uint32, ActivePetInfo> _active;
    mutable std::mutex                        _mutex;
};

#define sCustomPetsTracker CustomPetsTracker::instance()

// ──────────────────────────────────────────────────────────────────────────────
// Helper global: despide la mascota activa de un jugador.
// Si la criatura ya no existe (murió, teleport, etc.) solo limpia el tracker.
// ──────────────────────────────────────────────────────────────────────────────
void DismissActivePet(Player* player);

// ──────────────────────────────────────────────────────────────────────────────
// Declaraciones de registro de scripts por tipo.
// Añade una línea por cada nuevo CustomPetXxx.cpp que crees.
// ──────────────────────────────────────────────────────────────────────────────
void AddCustomPetVendorScripts();
void AddCustomPetLooterScripts();
// void AddCustomPetBankerScripts();   // futuro
// void AddCustomPetRepairScripts();   // futuro

#endif // MOD_CUSTOM_PETS_H

