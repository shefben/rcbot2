#ifndef FF_STATE_STRUCTS_H
#define FF_STATE_STRUCTS_H

#include <string>
#include <vector>
#include <cmath> // For sqrt in Vector::Length if added

// Forward declare CBaseEntity for pFlagEntity, pCarrierEntity
class CBaseEntity; // This is a conceptual class used throughout the AI framework

// 2. Define Vector Struct
struct Vector {
    float x, y, z;
    Vector(float _x = 0.f, float _y = 0.f, float _z = 0.f) : x(_x), y(_y), z(_z) {}

    Vector operator+(const Vector& other) const { return Vector(x + other.x, y + other.y, z + other.z); }
    Vector operator-(const Vector& other) const { return Vector(x - other.x, y - other.y, z - other.z); }
    Vector operator*(float scalar) const { return Vector(x * scalar, y * scalar, z * scalar); }
    float LengthSquared() const { return x*x + y*y + z*z; }
    float Length() const { return std::sqrt(LengthSquared()); }
    void Normalize() {
        float len = Length();
        if (len > 0.00001f) { x /= len; y /= len; z /= len; } else { x=0;y=0;z=0;}
    }
};

// 3. Define ControlPointInfo Struct
struct ControlPointInfo {
    int id;
    std::string luaName;
    std::string gameDisplayName;
    Vector position;
    int ownerTeam;
    float captureProgress;
    bool isLocked;
    CBaseEntity* pEntity_conceptual; // Conceptual pointer to game entity for this CP

    ControlPointInfo() : id(-1), ownerTeam(0), captureProgress(0.0f), isLocked(false), pEntity_conceptual(nullptr) {}
};

// 4. Define PayloadWaypoint Struct
struct PayloadWaypoint { /* ... (as before) ... */
    Vector position;
    float desiredSpeedAtPoint;
    bool isCheckPoint;
    std::string eventOnReachLuaFunc;
    PayloadWaypoint() : desiredSpeedAtPoint(100.0f), isCheckPoint(false) {}
};

// 5. Define PayloadPathInfo Struct
#define MAX_PAYLOAD_WAYPOINTS_FF 64
struct PayloadPathInfo { /* ... (as before) ... */
    int pathId;
    std::string pathName;
    PayloadWaypoint waypoints[MAX_PAYLOAD_WAYPOINTS_FF];
    int numWaypoints;
    float defaultSpeed;
    CBaseEntity* pCartEntity_conceptual;
    PayloadPathInfo() : pathId(0), numWaypoints(0), defaultSpeed(75.0f), pCartEntity_conceptual(nullptr) {}
};

// 6. Define ClassWeaponInfo Struct
struct ClassWeaponInfo { /* ... (as before) ... */
    std::string weaponNameId;
    int slot;
    int maxAmmoClip;
    int maxAmmoReserve;
    ClassWeaponInfo() : slot(-1), maxAmmoClip(0), maxAmmoReserve(0) {}
};

// 7. Define ClassConfigInfo Struct
struct ClassConfigInfo { /* ... (as before) ... */
    int classId;
    std::string className;
    std::string luaBotClassName;
    int maxHealth;
    int maxArmor;
    std::string armorType;
    float speed;
    Vector mins, maxs;
    std::vector<ClassWeaponInfo> availableWeapons;
    ClassConfigInfo() : classId(-1), maxHealth(0), maxArmor(0), speed(0.0f) {}
};

// --- CTF Specific Structures ---
enum class FlagStatus {
    AT_BASE,
    DROPPED_ALLY,       // Our flag dropped
    DROPPED_ENEMY,      // Enemy flag dropped by us
    CARRIED_BY_ALLY,    // Our teammate has enemy flag
    CARRIED_BY_ENEMY    // Enemy has our flag
};

struct FlagInfo {
    int teamId; // Which team this flag belongs to (e.g., TEAM_ID_RED, TEAM_ID_BLUE)
    FlagStatus status;
    Vector currentPosition; // Current position if dropped or carried, otherwise base position
    Vector basePosition;    // Static base position of the flag
    CBaseEntity* pFlagEntity;    // Conceptual pointer to the flag's game entity
    CBaseEntity* pCarrierEntity; // Conceptual pointer to player entity carrying it (if any)
    float lastStatusChangeTime;  // Game time of last status change (e.g., drop time)

    FlagInfo(int team = 0) :
        teamId(team),
        status(FlagStatus::AT_BASE),
        pFlagEntity(nullptr),
        pCarrierEntity(nullptr),
        lastStatusChangeTime(0.0f)
    {}
};


#endif // FF_STATE_STRUCTS_H
