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

    // Objective-specific status (examples, can be detailed per game mode)
    // For Control Points:
    // std::vector<std::pair<int, int>> controlPointStates; // <cpId, ownerTeamId>
    // For Payload:
    // float payloadProgressPercent;
    // bool isCartContested;

    std::string simpleObjectiveStatus; // A string summary, e.g., "CP1:Blue, CP2:Red, CartAt:CheckpointA"

    // Could also include bot's own immediate state if relevant for the snapshot context
    // float botHealth;
    // int botAmmoPrimary;

    GameStateSnapshot() :
        teamScore_Ally(0), teamScore_Enemy(0),
        playersAlive_Ally(0), playersAlive_Enemy(0),
        simpleObjectiveStatus("N/A")
        {}
};

// 3. Define SubTaskOutcomeLog Struct
struct SubTaskOutcomeLog {
    SubTaskType type;
    bool success;
    float durationSeconds;
    // std::string failureReason; // Optional

    SubTaskOutcomeLog(SubTaskType t = SubTaskType::NONE, bool s = false, float d = 0.f)
        : type(t), success(s), durationSeconds(d) {}
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
    // int botId; // If logging for multiple bots centrally and this struct is stored outside BotInfo
    std::string botClassUsed;       // e.g., "Soldier", "Medic" (from ClassConfigInfo)

    GameStateSnapshot stateAtStart;
    GameStateSnapshot stateAtEnd;   // Or just significant changes from start state

    std::vector<SubTaskOutcomeLog> executedSubTasks;

    // Resource usage (conceptual - would be populated by the bot tracking its own changes)
    // int healthLost;
    // int ammoUsedPrimary;
    // bool uberDeployed; // For Medic

    TaskOutcomeLog() :
        taskType(HighLevelTaskType::NONE),
        outcome(Outcome::PENDING),
        outcomeScore(0.0f),
        durationSeconds(0.0f),
        botClassUsed("Unknown")
        // healthLost(0), ammoUsedPrimary(0), uberDeployed(false)
        {}
};

#endif // BOT_LEARNING_DATA_H
