#ifndef BOT_LEARNING_DATA_H
#define BOT_LEARNING_DATA_H

#include <string>
#include <vector>
#include <chrono> // For timestamps and duration

// Assuming BotTasks.h defines HighLevelTaskType and SubTaskType enums
#include "BotTasks.h"
// Assuming FFStateStructs.h defines Vector struct
#include "FFStateStructs.h"

// 2. Define GameStateSnapshot Struct
struct GameStateSnapshot {
    // Team scores
    int teamScore_Ally;
    int teamScore_Enemy;

    // Player counts
    int playersAlive_Ally;
    int playersAlive_Enemy;

    std::string simpleObjectiveStatus; // A string summary, e.g., "CP1:Blue, CP2:Red, CartAt:CheckpointA"

    GameStateSnapshot() :
        teamScore_Ally(0), teamScore_Enemy(0),
        playersAlive_Ally(0), playersAlive_Enemy(0),
        simpleObjectiveStatus("N/A")
        {}

    void Clear() { // Helper to reset
        teamScore_Ally = 0; teamScore_Enemy = 0;
        playersAlive_Ally = 0; playersAlive_Enemy = 0;
        simpleObjectiveStatus = "N/A";
    }
};

// 3. Define SubTaskOutcomeLog Struct
struct SubTaskOutcomeLog {
    SubTaskType type;
    bool success;
    float durationSeconds;
    std::string failureReason; // Optional

    SubTaskOutcomeLog(SubTaskType t = SubTaskType::NONE, bool s = false, float d = 0.f, std::string reason = "")
        : type(t), success(s), durationSeconds(d), failureReason(std::move(reason)) {}
};

// 4. Define TaskOutcomeLog Struct
struct TaskOutcomeLog {
    HighLevelTaskType taskType;
    std::string taskDescription;    // e.g., "CAPTURE_POINT_FF: cp_dustbowl_point1"
    std::string targetNameOrId;     // Name of CP, flag entity ID, etc.
    Vector targetPosition;          // Primary target position for the HLT

    enum class Outcome {
        PENDING,            // Task is ongoing
        SUCCESS,            // Task fully achieved
        PARTIAL_SUCCESS,    // Task partially achieved (e.g. payload moved but not to end)
        FAILURE,            // Task attempted but failed to achieve objectives
        ABORTED             // Task was interrupted (e.g. bot died, game ended, higher priority task)
    } outcome;
    float outcomeScore;             // Numerical score/reward for this task completion

    std::chrono::system_clock::time_point startTime;
    std::chrono::system_clock::time_point endTime;
    float durationSeconds;          // Calculated from startTime and endTime

    // Bot context
    int botId;                      // ID of the bot that performed this task
    std::string botName;            // Name of the bot
    std::string botClassUsed;       // e.g., "Soldier", "Medic" (from ClassConfigInfo)

    GameStateSnapshot stateAtStart;
    GameStateSnapshot stateAtEnd;   // Or just significant changes from start state

    std::vector<SubTaskOutcomeLog> executedSubTasks;

    TaskOutcomeLog() { // Default constructor calls Reset
        Reset();
    }

    void Reset() {
        taskType = HighLevelTaskType::NONE;
        taskDescription.clear();
        targetNameOrId.clear();
        targetPosition = Vector(); // Assumes Vector default constructor
        outcome = Outcome::PENDING;
        outcomeScore = 0.0f;
        startTime = std::chrono::system_clock::time_point(); // Default construction (often epoch)
        endTime = std::chrono::system_clock::time_point();
        durationSeconds = 0.0f;
        botId = -1;
        botName.clear();
        botClassUsed = "Unknown";
        stateAtStart.Clear();
        stateAtEnd.Clear();
        executedSubTasks.clear();
    }
};

#endif // BOT_LEARNING_DATA_H
