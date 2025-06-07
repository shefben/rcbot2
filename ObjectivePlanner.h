#ifndef OBJECTIVE_PLANNER_H
#define OBJECTIVE_PLANNER_H

#include <vector>
#include <string>
#include <memory>
#include <chrono>

#include "BotTasks.h"
#include "BotLearningData.h"
#include "FFStateStructs.h" // For FlagInfo, ControlPointInfo etc.


// Forward declarations
class CFFPlayer;
struct BotKnowledgeBase;
class CBaseEntity;
class CRCBotPlugin;

// Assuming GameModeType_KB from BotKnowledgeBase.h is the one to use
// enum class GameModeType { UNKNOWN, CONTROL_POINT, CAPTURE_THE_FLAG, PAYLOAD_ATTACK, PAYLOAD_DEFEND };


class CObjectivePlanner {
public:
    CObjectivePlanner(CFFPlayer* pBotOwner, const BotKnowledgeBase* pKnowledgeBase, CRCBotPlugin* pPluginOwner);
    ~CObjectivePlanner();

    void EvaluateAndSelectTask();

    const HighLevelTask* GetCurrentHighLevelTask() const {
        return m_CurrentHighLevelTask.type != HighLevelTaskType::NONE ? &m_CurrentHighLevelTask : nullptr;
    }
    const SubTask* GetCurrentSubTask() const;
    SubTask* GetCurrentSubTaskMutable();

    void OnSubTaskOutcomeReported(bool success, const std::string& failureReason = "");
    void OnBotKilled();

private:
    CFFPlayer* m_pBotOwner;
    const BotKnowledgeBase* m_pKnowledgeBase;
    CRCBotPlugin* m_pPluginOwner;

    HighLevelTask m_CurrentHighLevelTask;
    TaskOutcomeLog m_CurrentTaskLogEntry;

    std::vector<HighLevelTask> m_AvailableTasks;

    // Task Selection Pipeline
    void GenerateAvailableTasks();
    void PrioritizeTasks();
    bool SelectTaskFromList();
    bool DecomposeTask(HighLevelTask& taskToDecompose);

    // Game Mode Specific Task Generation
    GameModeType_KB GetCurrentGameMode() const;
    void GenerateTasks_ControlPoint_FF();
    void GenerateTasks_CaptureTheFlag_FF();
    // void GenerateTasks_PayloadAttack_FF();
    // void GenerateTasks_PayloadDefend_FF();

    // Priority Calculation Helpers
    float CalculateCapturePriority(const ControlPointInfo& cpInfo, CFFPlayer* bot, const BotKnowledgeBase* kb);
    float CalculateDefensePriority(const ControlPointInfo& cpInfo, CFFPlayer* bot, const BotKnowledgeBase* kb);
    float CalculateCTFCaptureEnemyFlagPriority_Conceptual(const FlagInfo& enemyFlag, CFFPlayer* bot, const BotKnowledgeBase* kb);
    float CalculateCTFReturnOurFlagPriority_Conceptual(const FlagInfo& ourFlag, CFFPlayer* bot, const BotKnowledgeBase* kb);
    float CalculateCTFDefendFlagStandPriority_Conceptual(const FlagInfo& ourFlag, CFFPlayer* bot, const BotKnowledgeBase* kb);
    float CalculateCTFKillEnemyFlagCarrierPriority_Conceptual(const FlagInfo& ourFlag, const CBaseEntity* carrier, CFFPlayer* bot, const BotKnowledgeBase* kb);
    float CalculateCTFEscortFlagCarrierPriority_Conceptual(const FlagInfo& enemyFlag, const CBaseEntity* allyCarrier, CFFPlayer* bot, const BotKnowledgeBase* kb);


    // Decomposition Helpers
    // CP Helpers
    void AddDefaultSubTasksForCapture(HighLevelTask& hlt); // Removed position/entity params, uses HLT's
    void AddDefaultSubTasksForDefense(HighLevelTask& hlt); // Removed position/entity params, uses HLT's
    // CTF Helpers
    void AddSubTasks_CaptureEnemyFlag(HighLevelTask& hlt);
    void AddSubTasks_ReturnOurFlag(HighLevelTask& hlt);
    void AddSubTasks_KillEnemyFlagCarrier(HighLevelTask& hlt);
    void AddSubTasks_EscortFlagCarrier(HighLevelTask& hlt);
    void AddSubTasks_DefendFlagStand(HighLevelTask& hlt);
    void AddSubTasks_CampEnemyFlagSpawn(HighLevelTask& hlt);
    void AddSubTasks_InterceptEnemyFlagRunner(HighLevelTask& hlt);


    // Internal Logging Helper Methods
    GameStateSnapshot CreateGameStateSnapshot() const;
    void FinalizeSubTaskLog(const SubTask* pSubTask, bool success, const std::string& reason = "");
    void FinalizeAndStoreCurrentHLTLog(TaskOutcomeLog::Outcome outcome, float score);

    void ProcessSubTaskCompletion(bool success);
};

#endif // OBJECTIVE_PLANNER_H
