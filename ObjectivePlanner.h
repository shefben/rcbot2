#ifndef OBJECTIVE_PLANNER_H
#define OBJECTIVE_PLANNER_H

#include <vector>
#include <string>
#include <memory>
#include <chrono> // For m_CurrentSubTaskStartTime

#include "BotTasks.h"
#include "BotLearningData.h" // For TaskOutcomeLog, GameStateSnapshot, SubTaskOutcomeLog

// Forward declarations
class CFFPlayer;
struct BotKnowledgeBase;
class CBaseEntity;
class CRCBotPlugin; // For storing logs, if planner calls back to plugin

// Conceptual Game Mode enum
// Assuming GameModeType_KB from BotKnowledgeBase.h is preferred if defined there.
// If not, this can be used:
// enum class GameModeType { UNKNOWN, CONTROL_POINT, CAPTURE_THE_FLAG, PAYLOAD_ATTACK, PAYLOAD_DEFEND };


class CObjectivePlanner {
public:
    // Constructor now takes CRCBotPlugin* for log storage callback
    CObjectivePlanner(CFFPlayer* pBotOwner, const BotKnowledgeBase* pKnowledgeBase, CRCBotPlugin* pPluginOwner);
    ~CObjectivePlanner();

    void EvaluateAndSelectTask();

    const HighLevelTask* GetCurrentHighLevelTask() const {
        return m_CurrentHighLevelTask.type != HighLevelTaskType::NONE ? &m_CurrentHighLevelTask : nullptr;
    }
    const SubTask* GetCurrentSubTask() const;
    SubTask* GetCurrentSubTaskMutable();

    // Callbacks from AI module about subtask status
    // These will now also take actual outcome details
    void OnSubTaskOutcomeReported(bool success, const std::string& failureReason = "");
    // OnSubTaskCompleted and OnSubTaskFailed will be internal after outcome reported

    void OnBotKilled();

private:
    CFFPlayer* m_pBotOwner;
    const BotKnowledgeBase* m_pKnowledgeBase;
    CRCBotPlugin* m_pPluginOwner; // For storing logs

    HighLevelTask m_CurrentHighLevelTask;
    TaskOutcomeLog m_CurrentTaskLogEntry; // Log entry for the m_CurrentHighLevelTask
    // std::chrono::system_clock::time_point m_CurrentSubTaskStartTime; // Moved to SubTask struct in BotTasks.h

    std::vector<HighLevelTask> m_AvailableTasks;

    // Task Selection Pipeline
    void GenerateAvailableTasks();
    void PrioritizeTasks();
    bool SelectTaskFromList(); // This will now also initialize m_CurrentTaskLogEntry

    bool DecomposeTask(HighLevelTask& taskToDecompose);

    // Game Mode Specific Task Generation
    GameModeType_KB GetCurrentGameMode() const; // Assuming GameModeType_KB from BotKnowledgeBase.h
    void GenerateTasks_ControlPoint_FF();

    // Priority Calculation Helpers
    float CalculateCapturePriority(const ControlPointInfo& cpInfo, CFFPlayer* bot, const BotKnowledgeBase* kb);
    float CalculateDefensePriority(const ControlPointInfo& cpInfo, CFFPlayer* bot, const BotKnowledgeBase* kb);

    // Decomposition Helpers (kept from previous version)
    void AddDefaultSubTasksForMovement(HighLevelTask& hlt, const Vector& targetPos, CBaseEntity* targetEnt = nullptr);
    void AddDefaultSubTasksForCapture(HighLevelTask& hlt, const Vector& targetPos, CBaseEntity* targetEnt = nullptr);
    void AddDefaultSubTasksForDefense(HighLevelTask& hlt, const Vector& targetPos, CBaseEntity* targetEnt = nullptr);

    // Internal Logging Helper Methods
    GameStateSnapshot CreateGameStateSnapshot() const;
    void StartNewSubTaskLogging(); // Called when a subtask becomes current
    void FinalizeSubTaskLog(bool success, const std::string& reason = ""); // Called before advancing/resetting HLT due to subtask end
    void FinalizeAndStoreCurrentHLTLog(TaskOutcomeLog::Outcome outcome, float score);

    // Internal task advancement after logging subtask outcome
    void ProcessSubTaskCompletion(bool success);
};

#endif // OBJECTIVE_PLANNER_H
