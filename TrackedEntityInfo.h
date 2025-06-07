#ifndef TRACKED_ENTITY_INFO_H
#define TRACKED_ENTITY_INFO_H

#include "FFStateStructs.h" // For Vector
#include <string>

// Forward declare edict_t (conceptual representation of a game entity)
struct edict_t;
// Or, if you have a CBaseEntity class:
// class CBaseEntity;

// Conceptual representation of a tracked game entity
struct TrackedEntityInfo {
    edict_t* pEdict;        // Pointer to the engine's entity structure (could be CBaseEntity*)
    int entityId;           // Unique ID if available (e.g., edict index, or a persistent game ID)

    bool isVisible;         // Is the entity currently visible to any bot or perception system?
    float lastSeenTime;     // Game time when this entity was last confirmed visible
    Vector lastKnownPosition; // Last position where it was seen or known to be
    Vector velocity;        // Current velocity, if known

    int health;
    int maxHealth;
    int team;               // Game-specific team ID

    std::string className;  // If it's a player/bot, their class (e.g., "Soldier", "Medic_FF")
    // For non-player entities like Sentry Guns, this might be "obj_sentrygun"

    // Example type identifiers (could use an enum or more detailed type system)
    // bool isSentryGun;
    // bool isDispenser;
    // bool isTeleporter;
    // bool isProjectile;
    // bool isPlayer; // More robust check than just className

    // Conceptual: CBaseEntity* pActiveWeapon; // If it's a player and we know their weapon

    TrackedEntityInfo(edict_t* ed = nullptr, int id = -1) :
        pEdict(ed), entityId(id),
        isVisible(false), lastSeenTime(0.0f),
        health(0), maxHealth(0), team(0)
        // isSentryGun(false), isDispenser(false), isTeleporter(false), isProjectile(false), isPlayer(false)
        {}

    bool IsPlayer() const {
        // Simplified check; a more robust way would be based on entity type flags or game API
        return !className.empty() &&
               className != "obj_sentrygun" &&
               className != "obj_dispenser" &&
               className != "obj_teleporter_entrance" &&
               className != "obj_teleporter_exit";
    }

    bool IsHostileTo(int myTeam) const {
        return team != 0 && team != myTeam; // Basic check: not on my team and not neutral/world
    }

    bool IsFriendlyTo(int myTeam) const {
        return team != 0 && team == myTeam;
    }
};

#endif // TRACKED_ENTITY_INFO_H
