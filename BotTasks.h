#ifndef BOT_TASKS_H
#define BOT_TASKS_H

#include <string>
#include <vector>
// #include <variant> // Decided against for initial simplicity
#include "FFStateStructs.h" // For Vector (ensure this is created and includes Vector)

// Forward declare CBaseEntity (conceptual game entity)
class CBaseEntity;

enum class SubTaskType {
    NONE,
    MOVE_TO_POSITION,       // targetPosition
    MOVE_TO_ENTITY,         // pTargetEntity
    ATTACK_TARGET,          // pTargetEntity
    DEFEND_POSITION,        // targetPosition, desiredDuration (hold general area)
    SECURE_AREA,            // targetPosition, desiredDuration (clear immediate vicinity)
    USE_ABILITY_ON_TARGET,  // pTargetEntity, abilitySlot
    USE_ABILITY_AT_POSITION,// targetPosition, abilitySlot
    CAPTURE_OBJECTIVE,      // pTargetEntity (e.g. CP entity), targetPosition (stand point)
    STAND_ON_POINT,         // pTargetEntity (e.g. CP entity), targetPosition, desiredDuration
    HOLD_POSITION,          // targetPosition, desiredDuration (similar to DEFEND_POSITION but maybe less aggressive scan)

    // FF Specific examples (can be expanded or mapped to generic types with parameters)
    PLANT_STICKY_TRAP_FF,   // targetPosition
    ATTEMPT_BACKSTAB_FF,    // pTargetEntity
    HEAL_ALLY_FF,           // pTargetEntity (could also use ATTACK_TARGET with special Medic logic)
    DEPLOY_UBERCHARGE_FF,   // (No specific target, usually on current heal target or self)
    ROCKET_JUMP_FF          // targetPosition (desired landing/apex or direction vector)
};

struct SubTask {
    SubTaskType type;
    Vector targetPosition;       // For position-based tasks
    CBaseEntity* pTargetEntity;  // For entity-based tasks
    int abilitySlot;             // For USE_ABILITY tasks (e.g., 0 for primary, 1 for secondary ability)
    float desiredDuration;       // For timed tasks like DEFEND_POSITION, HOLD_POSITION, SECURE_AREA

    bool isCompleted;            // Set by the AI module when this specific sub-task is done
    bool canBeInterrupted;       // Can this subtask be easily overridden by a new high-priority event?

    // float targetValue; // Example: for HEAL_ALLY_FF to certain health % - keeping it simple for now

    SubTask(SubTaskType t = SubTaskType::NONE)
        : type(t), pTargetEntity(nullptr), abilitySlot(-1),
          desiredDuration(0.0f), isCompleted(false), canBeInterrupted(true) {}

    // Constructor for position-based tasks
    SubTask(SubTaskType t, const Vector& pos, float duration = 0.0f)
        : type(t), targetPosition(pos), pTargetEntity(nullptr), abilitySlot(-1),
          desiredDuration(duration), isCompleted(false), canBeInterrupted(true) {}

    // Constructor for entity-based tasks
    SubTask(SubTaskType t, CBaseEntity* entity, float duration = 0.0f)
        : type(t), pTargetEntity(entity), abilitySlot(-1),
          desiredDuration(duration), isCompleted(false), canBeInterrupted(true) {}

    // Constructor for ability on entity
    SubTask(SubTaskType t, CBaseEntity* entity, int slot)
        : type(t), pTargetEntity(entity), abilitySlot(slot),
          desiredDuration(0.0f), isCompleted(false), canBeInterrupted(true) {}

    // Constructor for ability at position
    SubTask(SubTaskType t, const Vector& pos, int slot)
        : type(t), targetPosition(pos), pTargetEntity(nullptr), abilitySlot(slot),
          desiredDuration(0.0f), isCompleted(false), canBeInterrupted(true) {}
};

enum class HighLevelTaskType {
    NONE,
    CAPTURE_POINT_FF,    // Target a specific control point
    DEFEND_POINT_FF,     // Defend a specific control point
    ESCORT_PAYLOAD_FF,   // Stay with and push the payload cart
    STOP_PAYLOAD_FF,     // Prevent enemy from pushing payload
    CAPTURE_FLAG_FF,     // Go to enemy base, get flag, bring to own base
    RETRIEVE_FLAG_FF,    // Recover dropped friendly flag or one taken by enemy
    DEFEND_FLAG_FF,      // Defend friendly flag at base
    ATTACK_ENEMY,        // General combat, not tied to a specific major objective point
    SEEK_HEALTH,
    SEEK_AMMO,
    FALLBACK_REGROUP     // Retreat to a safe position and wait for teammates
};

struct HighLevelTask {
    HighLevelTaskType type;
    float priority;             // Base priority, can be dynamically adjusted
    Vector targetPosition;      // General area or specific point for the task
    CBaseEntity* pTargetEntity; // e.g., the CP entity, the flag, the payload cart
    std::string description;    // For debugging/logging

    std::vector<SubTask> subTasks;
    int currentSubTaskIndex;    // Index of the current subtask in the subTasks vector

    HighLevelTask(HighLevelTaskType t = HighLevelTaskType::NONE, float prio = 0.0f)
        : type(t), priority(prio), pTargetEntity(nullptr), currentSubTaskIndex(-1) {}

    bool IsValid() const { return type != HighLevelTaskType::NONE && !subTasks.empty() && currentSubTaskIndex < (int)subTasks.size(); }
    bool AllSubTasksDone() const { return type != HighLevelTaskType::NONE && currentSubTaskIndex >= (int)subTasks.size() -1 && (subTasks.empty() || subTasks.back().isCompleted); }


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

    // Advances to the next subtask. Returns true if there is a next subtask, false if all are done.
    bool AdvanceToNextSubTask() {
        if (type == HighLevelTaskType::NONE || subTasks.empty()) return false;

        currentSubTaskIndex++;
        if (currentSubTaskIndex < (int)subTasks.size()) {
            if(subTasks[currentSubTaskIndex-1].isCompleted) { // Only reset next if previous was completed
                 subTasks[currentSubTaskIndex].isCompleted = false; // Ensure new subtask is not marked completed
            } else {
                // If previous task failed, the HLT might fail entirely.
                // For now, just advance, planner will see previous failed.
            }
            return true;
        }
        // Reached end of subtask list
        return false;
    }

    void Reset() {
        type = HighLevelTaskType::NONE;
        priority = 0.0f;
        targetPosition = Vector();
        pTargetEntity = nullptr;
        description = "";
        subTasks.clear();
        currentSubTaskIndex = -1;
    }
};

#endif // BOT_TASKS_H
