#ifndef BOT_TASKS_H
#define BOT_TASKS_H

#include <string>
#include <vector>
#include <chrono> // For system_clock
#include "FFStateStructs.h" // For Vector

// Forward declare CBaseEntity
class CBaseEntity;

enum class SubTaskType {
    NONE,
    MOVE_TO_POSITION,
    MOVE_TO_ENTITY,
    ATTACK_TARGET,
    DEFEND_POSITION,
    SECURE_AREA,
    USE_ABILITY_ON_TARGET,
    USE_ABILITY_AT_POSITION,
    CAPTURE_OBJECTIVE,      // Generic for standing on CP or interacting with objective
    STAND_ON_POINT,         // More specific for CPs
    HOLD_POSITION,          // Stay in an area and defend

    // FF Class Specific examples
    PLANT_STICKY_TRAP_FF,
    ATTEMPT_BACKSTAB_FF,
    HEAL_ALLY_FF,           // Could also use ATTACK_TARGET with special Medic logic to heal ally
    DEPLOY_UBERCHARGE_FF,
    ROCKET_JUMP_FF,

    // CTF Specific SubTaskTypes (New)
    PICKUP_FLAG_FF,             // pTargetEntity (the flag item), targetPosition (flag's location)
    CAPTURE_FLAG_AT_POINT_FF,   // pTargetEntity (the capture zone entity), targetPosition (capture zone location). Bot must be carrying flag.
    MOVE_TO_ENTITY_DYNAMIC_FF   // pTargetEntity (entity to follow, e.g. flag carrier), targetPosition (is dynamically updated to pTargetEntity's pos)
};

struct SubTask {
    SubTaskType type;
    Vector targetPosition;
    CBaseEntity* pTargetEntity;
    int abilitySlot;
    float desiredDuration;
    float radiusParam;           // For area tasks like SECURE_AREA, HOLD_POSITION, or capture radius for STAND_ON_POINT

    bool isCompleted;
    bool canBeInterrupted;
    std::chrono::system_clock::time_point startTime; // When this subtask became active

    SubTask(SubTaskType t = SubTaskType::NONE)
        : type(t), pTargetEntity(nullptr), abilitySlot(-1),
          desiredDuration(0.0f), radiusParam(0.0f),
          isCompleted(false), canBeInterrupted(true) {}

    SubTask(SubTaskType t, const Vector& pos)
        : type(t), targetPosition(pos), pTargetEntity(nullptr), abilitySlot(-1),
          desiredDuration(0.0f), radiusParam(0.0f),
          isCompleted(false), canBeInterrupted(true) {}

    SubTask(SubTaskType t, CBaseEntity* entity)
        : type(t), pTargetEntity(entity), abilitySlot(-1),
          desiredDuration(0.0f), radiusParam(0.0f),
          isCompleted(false), canBeInterrupted(true) {}

    SubTask(SubTaskType t, const Vector& pos, CBaseEntity* entity, float duration, float radius)
        : type(t), targetPosition(pos), pTargetEntity(entity), abilitySlot(-1),
          desiredDuration(duration), radiusParam(radius),
          isCompleted(false), canBeInterrupted(true) {}

    SubTask(SubTaskType t, CBaseEntity* entity, int slot)
        : type(t), pTargetEntity(entity), abilitySlot(slot),
          desiredDuration(0.0f), radiusParam(0.0f),
          isCompleted(false), canBeInterrupted(true) {}

    SubTask(SubTaskType t, const Vector& pos, int slot)
        : type(t), targetPosition(pos), pTargetEntity(nullptr), abilitySlot(slot),
          desiredDuration(0.0f), radiusParam(0.0f),
          isCompleted(false), canBeInterrupted(true) {}
};

enum class HighLevelTaskType {
    NONE,
    CAPTURE_POINT_FF,
    DEFEND_POINT_FF,
    ESCORT_PAYLOAD_FF,
    STOP_PAYLOAD_FF,

    // CTF HLTs (already added from Task 17)
    CAPTURE_ENEMY_FLAG_FF,          // Go to enemy flag base, pick it up, bring to our base
    RETURN_OUR_FLAG_FF,             // Our flag is dropped, go get it and return to stand
    DEFEND_FLAG_STAND_FF,           // Defend our flag at its base/stand
    ESCORT_FLAG_CARRIER_FF,         // Escort our teammate who has the enemy flag
    KILL_ENEMY_FLAG_CARRIER_FF,     // Specifically target enemy carrying our flag
    CAMP_ENEMY_FLAG_SPAWN_FF,       // (Advanced) Camp near enemy flag spawn
    INTERCEPT_ENEMY_FLAG_RUNNER_FF, // General interception of enemy with our flag

    ATTACK_ENEMY,
    SEEK_HEALTH,
    SEEK_AMMO,
    FALLBACK_REGROUP
};

struct HighLevelTask {
    HighLevelTaskType type;
    float priority;
    Vector targetPosition;
    CBaseEntity* pTargetEntity;
    std::string description;
    std::string targetNameOrId_Logging;

    std::vector<SubTask> subTasks;
    int currentSubTaskIndex;

    HighLevelTask(HighLevelTaskType t = HighLevelTaskType::NONE, float prio = 0.0f)
        : type(t), priority(prio), pTargetEntity(nullptr), currentSubTaskIndex(-1) {}

    bool IsValid() const { return type != HighLevelTaskType::NONE; }
    bool HasPendingSubTasks() const {
        if (type == HighLevelTaskType::NONE || subTasks.empty() || currentSubTaskIndex < 0) return false;
        if (currentSubTaskIndex >= (int)subTasks.size()) return false;
        return currentSubTaskIndex < (int)subTasks.size();
    }

    void AddSubTask(const SubTask& sub) { subTasks.push_back(sub); }

    const SubTask* GetCurrentSubTask() const { /* ... (same as before) ... */
        if (type != HighLevelTaskType::NONE && currentSubTaskIndex >= 0 && currentSubTaskIndex < (int)subTasks.size()) {
            return &subTasks[currentSubTaskIndex];
        }
        return nullptr;
    }
    SubTask* GetCurrentSubTaskMutable() { /* ... (same as before) ... */
        if (type != HighLevelTaskType::NONE && currentSubTaskIndex >= 0 && currentSubTaskIndex < (int)subTasks.size()) {
            return &subTasks[currentSubTaskIndex];
        }
        return nullptr;
    }
    bool AdvanceToNextSubTask() { /* ... (same as before) ... */
        if (type == HighLevelTaskType::NONE || subTasks.empty()) return false;
        currentSubTaskIndex++;
        if (currentSubTaskIndex < (int)subTasks.size()) {
            subTasks[currentSubTaskIndex].isCompleted = false;
            subTasks[currentSubTaskIndex].startTime = std::chrono::system_clock::now();
            return true;
        }
        return false;
    }
    void StartFirstSubTask() { /* ... (same as before) ... */
        if (!subTasks.empty()) {
            currentSubTaskIndex = 0;
            subTasks[0].isCompleted = false;
            subTasks[0].startTime = std::chrono::system_clock::now();
        } else {
            currentSubTaskIndex = -1;
        }
    }
    void Reset() { /* ... (same as before) ... */
        type = HighLevelTaskType::NONE; priority = 0.0f; targetPosition = Vector();
        pTargetEntity = nullptr; description = ""; targetNameOrId_Logging = "";
        subTasks.clear(); currentSubTaskIndex = -1;
    }
};

#endif // BOT_TASKS_H
