#ifndef FF_STATE_STRUCTS_H
#define FF_STATE_STRUCTS_H

#include <string>
#include <vector>

// Simple Vector struct for position data
struct Vector {
    float x, y, z;

    Vector(float _x = 0.f, float _y = 0.f, float _z = 0.f) : x(_x), y(_y), z(_z) {}
};

struct ControlPointInfo {
    int id;
    std::string luaName;
    Vector position;
    int teamOwner;
    float captureProgress;
    bool isLocked;
    // std::vector<int> playersOnPoint; // Example of further detail

    ControlPointInfo() : id(-1), teamOwner(0), captureProgress(0.0f), isLocked(false) {}
};

#define MAX_PAYLOAD_WAYPOINTS 64

struct PayloadWaypoint {
    Vector position;
    float desiredSpeedAtPoint;
    // bool isCheckPoint;
    // std::string eventOnReachLuaFunc;

    PayloadWaypoint() : desiredSpeedAtPoint(0.0f) {}
};

struct PayloadPathInfo {
    int pathId;
    std::string pathName;
    PayloadWaypoint waypoints[MAX_PAYLOAD_WAYPOINTS]; // Fixed-size array for simplicity with MAX macro
    int numWaypoints;
    float defaultSpeed;
    // bool isReversing;
    // float currentProgress; // 0.0 to 1.0

    PayloadPathInfo() : pathId(0), numWaypoints(0), defaultSpeed(100.0f) {}
};

struct ClassWeaponInfo {
    std::string weaponName;
    int slot; // e.g., 0 for primary, 1 for secondary, 2 for melee
    // int clipSize;
    // float damage;

    ClassWeaponInfo() : slot(-1) {}
};

struct ClassConfigInfo {
    int classId; // TF2 class ID (e.g., 1 for Scout, 2 for Soldier)
    std::string className; // "Scout", "Soldier"
    std::string luaBotClassName;
    int maxHealth;
    float speed; // Base movement speed units/sec
    std::vector<ClassWeaponInfo> availableWeapons;
    // std::string primaryRole; // "Offense", "Defense", "Support"
    // float jumpHeight;

    ClassConfigInfo() : classId(-1), maxHealth(0), speed(0.0f) {}
};

#endif // FF_STATE_STRUCTS_H
