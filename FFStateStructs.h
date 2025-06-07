#ifndef FF_STATE_STRUCTS_H
#define FF_STATE_STRUCTS_H

#include <string>
#include <vector>
#include <cmath> // For sqrt in Vector::Length if added

// 2. Define Vector Struct
struct Vector {
    float x, y, z;
    Vector(float _x = 0.f, float _y = 0.f, float _z = 0.f) : x(_x), y(_y), z(_z) {}

    // Basic operations (can be expanded)
    Vector operator+(const Vector& other) const {
        return Vector(x + other.x, y + other.y, z + other.z);
    }
    Vector operator-(const Vector& other) const {
        return Vector(x - other.x, y - other.y, z - other.z);
    }
    Vector operator*(float scalar) const {
        return Vector(x * scalar, y * scalar, z * scalar);
    }
    float LengthSquared() const {
        return x*x + y*y + z*z;
    }
    float Length() const {
        return std::sqrt(LengthSquared());
    }
    void Normalize() { // Basic normalization
        float len = Length();
        if (len > 0) {
            x /= len;
            y /= len;
            z /= len;
        }
    }
};

// 3. Define ControlPointInfo Struct
struct ControlPointInfo {
    int id;                       // Unique map ID for the CP
    std::string luaName;          // Name used in Lua tables (e.g., "cp1", "middle")
    std::string gameDisplayName;  // Name displayed in HUD, e.g. "Middle Point"
    Vector position;              // Center of the CP
    int ownerTeam;                // Current owning team (e.g., 0=neutral, 1=RED_TEAM_FF, 2=BLUE_TEAM_FF - FF specific enums needed)
    float captureProgress;        // 0.0 to 1.0 for capture status, or -1.0 to 1.0 if it shows enemy progress too
    bool isLocked;                // Is the CP currently locked (e.g., previous point not capped)
    // void* pEntity;             // Placeholder for game entity reference (e.g., edict_t* or CBaseEntity*)

    ControlPointInfo() : id(-1), ownerTeam(0), captureProgress(0.0f), isLocked(false) /*, pEntity(nullptr)*/ {}
};

// 4. Define PayloadWaypoint Struct
struct PayloadWaypoint {
    Vector position;
    float desiredSpeedAtPoint;    // Speed payload should ideally have here
    bool isCheckPoint;            // Does this waypoint trigger timer extension/spawn changes?
    std::string eventOnReachLuaFunc; // Lua function to call when this waypoint is reached by cart

    PayloadWaypoint() : desiredSpeedAtPoint(100.0f), isCheckPoint(false) {}
};

// 5. Define PayloadPathInfo Struct
#define MAX_PAYLOAD_WAYPOINTS_FF 64

struct PayloadPathInfo {
    int pathId;                   // If multiple paths exist on a map (e.g. pl_badwater last has multiple tracks)
    std::string pathName;         // e.g., "main_track", "alternative_route"
    PayloadWaypoint waypoints[MAX_PAYLOAD_WAYPOINTS_FF];
    int numWaypoints;
    float defaultSpeed;           // Default speed of the payload cart (units/sec)
    // void* pCartEntity;         // Placeholder for cart entity
    // int  owningTeam;           // Which team needs to push this cart (e.g. RED_TEAM_FF on attack)

    PayloadPathInfo() : pathId(0), numWaypoints(0), defaultSpeed(75.0f) /*, pCartEntity(nullptr), owningTeam(0) */ {}
};

// 6. Define ClassWeaponInfo Struct
struct ClassWeaponInfo {
    std::string weaponNameId;     // Internal name/ID, e.g., "tf_weapon_rocketlauncher", "ff_weapon_nailgun"
    int slot;                     // e.g., 0=primary, 1=secondary, 2=melee, 3=special1, 4=special2
    int maxAmmoClip;
    int maxAmmoReserve;
    // float damagePerShot;       // Base damage, could be more complex (falloff, splash)
    // float fireRate;            // Shots per second
    // bool  isHitscan;

    ClassWeaponInfo() : slot(-1), maxAmmoClip(0), maxAmmoReserve(0) {}
};

// 7. Define ClassConfigInfo Struct
struct ClassConfigInfo {
    int classId;                  // Internal enum/ID for the class (e.g., FF_CLASS_SCOUT, FF_CLASS_SOLDIER)
    std::string className;        // e.g., "Soldier", "Medic_FF"
    std::string luaBotClassName;  // Name for specific bot AI type, e.g., "SoldierBotAggressive"
    int maxHealth;
    int maxArmor;
    std::string armorType;        // e.g. "light", "medium", "heavy", or numeric value like 50, 100, 200
    float speed;                  // Base movement speed (units/sec)
    Vector mins, maxs;            // Bounding box for the class
    std::vector<ClassWeaponInfo> availableWeapons;
    // std::vector<std::string> primaryGrenades;   // Type and count
    // std::vector<std::string> secondaryGrenades; // Type and count
    // float abilityCooldown1; // For special abilities

    ClassConfigInfo() : classId(-1), maxHealth(0), maxArmor(0), speed(0.0f) {}
};

#endif // FF_STATE_STRUCTS_H
