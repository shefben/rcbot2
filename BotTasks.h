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
    CAPTURE_OBJECTIVE,
    STAND_ON_POINT,
    HOLD_POSITION,
    PLANT_STICKY_TRAP_FF,
    ATTEMPT_BACKSTAB_FF,
    HEAL_ALLY_FF,
    DEPLOY_UBERCHARGE_FF,
    ROCKET_JUMP_FF
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

    // Constructor for simple position-based tasks (e.g. MOVE_TO_POSITION)
    SubTask(SubTaskType t, const Vector& pos)
        : type(t), targetPosition(pos), pTargetEntity(nullptr), abilitySlot(-1),
          desiredDuration(0.0f), radiusParam(0.0f),
          isCompleted(false), canBeInterrupted(true) {}

    // Constructor for entity-based tasks (e.g. MOVE_TO_ENTITY, ATTACK_TARGET)
    SubTask(SubTaskType t, CBaseEntity* entity)
        : type(t), pTargetEntity(entity), abilitySlot(-1),
          desiredDuration(0.0f), radiusParam(0.0f),
          isCompleted(false), canBeInterrupted(true) {}

    // Constructor for tasks with position, entity, duration, and radius (SECURE_AREA, HOLD_POSITION, STAND_ON_POINT)
    SubTask(SubTaskType t, const Vector& pos, CBaseEntity* entity, float duration, float radius)
        : type(t), targetPosition(pos), pTargetEntity(entity), abilitySlot(-1),
          desiredDuration(duration), radiusParam(radius),
          isCompleted(false), canBeInterrupted(true) {}

    // Constructor for ability on entity
    SubTask(SubTaskType t, CBaseEntity* entity, int slot)
        : type(t), pTargetEntity(entity), abilitySlot(slot),
          desiredDuration(0.0f), radiusParam(0.0f),
          isCompleted(false), canBeInterrupted(true) {}

    // Constructor for ability at position
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
    CAPTURE_FLAG_FF,
    RETRIEVE_FLAG_FF,
    DEFEND_FLAG_FF,
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
    std::string targetNameOrId_Logging; // For logging convenience if pTargetEntity is not enough

    std::vector<SubTask> subTasks;
    int currentSubTaskIndex;

    HighLevelTask(HighLevelTaskType t = HighLevelTaskType::NONE, float prio = 0.0f)
        : type(t), priority(prio), pTargetEntity(nullptr), currentSubTaskIndex(-1) {}

    bool IsValid() const { return type != HighLevelTaskType::NONE; } // Simplified: valid if it has a type
    bool HasPendingSubTasks() const {
        if (type == HighLevelTaskType::NONE || subTasks.empty() || currentSubTaskIndex < 0) return false;
        if (currentSubTaskIndex >= (int)subTasks.size()) return false; // Index out of bounds (all done)
        // If current subtask is not completed, then it's pending.
        // If currentSubTaskIndex points to a valid task, it's considered pending until completed/advanced.
        return currentSubTaskIndex < (int)subTasks.size();
    }


    void AddSubTask(const SubTask& sub) {
        subTasks.push_back(sub);
    }

    const SubTask* GetCurrentSubTask() const {
        if (type != HighLevelTaskType::NONE && currentSubTaskIndex >= 0 && currentSubTaskIndex < (int)subTasks.size()) {
            return &subTasks[currentSubTaskIndex];
        }
        return nullptr;
    }

    SubTask* GetCurrentSubTaskMutable() {
        if (type != HighLevelTaskType::NONE && currentSubTaskIndex >= 0 && currentSubTaskIndex < (int)subTasks.size()) {
            return &subTasks[currentSubTaskIndex];
        }
        return nullptr;
    }

    bool AdvanceToNextSubTask() {
        if (type == HighLevelTaskType::NONE || subTasks.empty()) return false;

        currentSubTaskIndex++;
        if (currentSubTaskIndex < (int)subTasks.size()) {
            // When advancing, the new current subtask's startTime should be set.
            subTasks[currentSubTaskIndex].isCompleted = false;
            subTasks[currentSubTaskIndex].startTime = std::chrono::system_clock::now();
            return true;
        }
        return false;
    }

    void StartFirstSubTask() { // Call this after DecomposeTask
        if (!subTasks.empty()) {
            currentSubTaskIndex = 0;
            subTasks[0].isCompleted = false;
            subTasks[0].startTime = std::chrono::system_clock::now();
        } else {
            currentSubTaskIndex = -1;
        }
    }

    void Reset() {
        type = HighLevelTaskType::NONE; priority = 0.0f; targetPosition = Vector();
        pTargetEntity = nullptr; description = ""; targetNameOrId_Logging = "";
        subTasks.clear(); currentSubTaskIndex = -1;
    }
};

#endif // BOT_TASKS_H
