#ifndef MOD_CUSTOM_PETS_H
#define MOD_CUSTOM_PETS_H

#include "Player.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "ObjectGuid.h"
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <vector>
#include <string>

// ──────────────────────────────────────────────────────────────────────────────
// Tipos de mascota custom.
// ──────────────────────────────────────────────────────────────────────────────
enum CustomPetType : uint8
{
    CUSTOM_PET_VENDOR = 1,   // Abre ventana de tienda al interactuar
    CUSTOM_PET_LOOTER = 2,   // Saquea automáticamente los cadáveres cercanos
};

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
    float       speed;
    uint32      item_entry;  // Ítem que enseña esta mascota (0 = solo admin)
    uint32      spell_id;    // Hechizo de compañero en spell_dbc (0 = sin hechizo)
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
    bool GetPetByItemEntry(uint32 itemEntry, CustomPetData& out) const;
    bool GetPetBySpellId(uint32 spellId, CustomPetData& out) const;
    std::vector<CustomPetData> GetAll() const;

    // ── Sistema de mascotas aprendidas (por jugador) ───────────────────────
    // LoadPlayerPets  → llamar en OnPlayerLogin para poblar la caché
    // UnloadPlayerPets→ llamar en OnPlayerLogout para liberar memoria
    void LoadPlayerPets(uint32 playerGuid);
    void UnloadPlayerPets(uint32 playerGuid);
    bool HasLearnedPet(uint32 playerGuid, uint32 petId) const;
    void LearnPet(uint32 playerGuid, uint32 petId);  // persiste en BD + caché
    std::vector<CustomPetData> GetLearnedPets(uint32 playerGuid) const;

private:
    std::vector<CustomPetData> _pets;
    mutable std::mutex         _mutex;

    // Caché: guid de jugador → conjunto de pet_ids aprendidos
    std::unordered_map<uint32, std::unordered_set<uint32>> _learnedPets;
    mutable std::mutex                                     _learnedMutex;
};

#define sCustomPetsMgr CustomPetsMgr::instance()

// ──────────────────────────────────────────────────────────────────────────────
// Tracker de mascota activa por jugador
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
    ObjectGuid GetGuid(uint32 playerKey) const;
    uint8 GetType(uint32 playerKey) const;
    bool Has(uint32 playerKey) const;

private:
    std::unordered_map<uint32, ActivePetInfo> _active;
    mutable std::mutex                        _mutex;
};

#define sCustomPetsTracker CustomPetsTracker::instance()

// ──────────────────────────────────────────────────────────────────────────────
// Helper global: despide la mascota activa de un jugador.
// ──────────────────────────────────────────────────────────────────────────────
void DismissActivePet(Player* player);

// ──────────────────────────────────────────────────────────────────────────────
// Declaraciones de registro de scripts por tipo.
// ──────────────────────────────────────────────────────────────────────────────
void AddCustomPetVendorScripts();
void AddCustomPetLooterScripts();
void AddCustomPetLearnSystemScripts();
void AddCustomPetCompanionSpellScripts();  // hechizos de compañero en spell_dbc

#endif // MOD_CUSTOM_PETS_H

