# mod-custom-pets

Módulo de mascotas invocables con comportamiento personalizado para AzerothCore 3.3.5a.

## Concepto

El módulo permite invocar **criaturas temporales** junto al jugador, cada una con un tipo
de comportamiento distinto. El jugador interactúa con ellas (o no) igual que con cualquier
NPC del juego.

## Tipos de mascota

| Tipo | Valor | Descripción |
|------|-------|-------------|
| **Vendedor** | 1 | Abre una ventana de tienda con artículos configurables vía `npc_vendor`. |
| **Saqueador** | 2 | Saquea automáticamente cadáveres looteables en un radio de 40 unidades. |
| *(más tipos próximamente)* | | Banco, Reparación, Correo… |

## Comandos in-game

| Comando | Acceso | Descripción |
|---------|--------|-------------|
| `.custompet summon <id>` | Jugador | Invoca la mascota con el ID dado. |
| `.custompet dismiss` | Jugador | Despide la mascota activa. |
| `.custompet list` | Jugador | Muestra todas las mascotas disponibles. |
| `.custompet reload` | Admin | Recarga definiciones desde la base de datos. |

**Ejemplo:**
```
.custompet list
.custompet summon 1    ← invoca al Mercader Ambulante (vendedor)
.custompet summon 2    ← invoca al Recolector Fiel (saqueador)
.custompet dismiss
```

## Comportamiento del Saqueador (tipo 2)

- Aparece junto al jugador y lo sigue.
- No se puede seleccionar con clic (es invisible para la interacción).
- Cada **1.5 segundos** escanea un radio de **40 unidades** buscando cadáveres con loot disponible.
- Se mueve visualmente hacia el cadáver más cercano.
- Saquea automáticamente: oro, objetos normales y objetos de quest (si el jugador tiene la quest activa).
- Respeta los permisos de grupo (no roba loot de otro jugador).
- Si el inventario está lleno, los objetos que no caben se omiten silenciosamente.

## Instalación

1. Copia la carpeta en `modules/`.
2. Ejecuta `sql/mod_custom_pets.sql` en la base de datos `acore_world`.
3. Compila el core (el sistema de módulos detecta el módulo automáticamente).
4. Copia `conf/mod_custom_pets.conf.dist` a tu carpeta de configuración y
   renómbralo a `mod_custom_pets.conf`.

## Estructura de archivos (`src/`)

| Archivo | Contenido |
|---------|-----------|
| `CustomPets.h` | Tipos, structs, managers, tracker, declaraciones compartidas |
| `CustomPets.cpp` | Manager, tracker, `DismissActivePet`, comandos, PlayerScript global, WorldScript |
| `CustomPetVendor.cpp` | `npc_custom_pet_vendor`: gossip + `SendListInventory` |
| `CustomPetLooter.cpp` | `npc_custom_pet_looter` + `CustomPetLooterPlayerScript` (lógica de saqueo) |
| `CustomPets_loader.cpp` | Entry point del módulo |

## Añadir un nuevo tipo de mascota

1. Añade `CUSTOM_PET_NEW = N` al enum en `CustomPets.h`.
2. Crea `CustomPetNew.cpp` con su clase de script y `void AddCustomPetNewScripts()`.
3. Declara `void AddCustomPetNewScripts()` en `CustomPets.h` y llámala en `CustomPets.cpp`.
4. Inserta su `creature_template` y fila en `mod_custom_pets` en el SQL.

## Estructura de la tabla `mod_custom_pets`

| Columna | Tipo | Descripción |
|---------|------|-------------|
| `id` | INT AUTO_INCREMENT | Identificador de la mascota. |
| `name` | VARCHAR(64) | Nombre visible en comandos y mensajes. |
| `type` | TINYINT | Tipo de comportamiento (1=Vendedor, 2=Saqueador). |
| `creature_entry` | INT UNSIGNED | Entry en `creature_template`. |
| `description` | VARCHAR(255) | Texto corto para `.custompet list`. |
| `enabled` | TINYINT(1) | 0=desactivada, 1=invocable. |

## Creatures incluidas en el SQL

| Entry | Nombre | Tipo | ScriptName |
|-------|--------|------|------------|
| 601000 | Mercader Ambulante | Vendedor | `npc_custom_pet_vendor` |
| 601001 | Recolector Fiel | Saqueador | `npc_custom_pet_looter` |

## Añadir artículos al Mercader Ambulante

```sql
INSERT INTO `npc_vendor` (`entry`, `slot`, `item`, `maxcount`, `incrtime`, `ExtendedCost`)
VALUES (601000, 21, <item_entry>, 0, 0, 0);
```

## Cambiar la apariencia de las mascotas

Los modelos se almacenan en `creature_template_model` (no en `creature_template`).

```sql
UPDATE `creature_template_model` SET `CreatureDisplayID` = <id> WHERE `CreatureID` = 601000 AND `Idx` = 0;
UPDATE `creature_template_model` SET `CreatureDisplayID` = <id> WHERE `CreatureID` = 601001 AND `Idx` = 0;
```

| DisplayID | Apariencia |
|-----------|-----------|
| 49 | Human Male |
| 52 | Dwarf Male |
| 55 | Gnome Male (default Saqueador) |
| 1563 | Goblin Male |
