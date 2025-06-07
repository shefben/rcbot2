#ifndef OBJECTIVE_PLANNER_H
#define OBJECTIVE_PLANNER_H

#include "BotTasks.h"         // For HighLevelTask, SubTask
#include "ff_state_structs.h" // For game state knowledge structs
#include <vector>
#include <string>
#include <memory> // For std::unique_ptr if needed for tasks

// Forward declarations
class CFFPlayer;             // Represents the bot this planner is for
struct BotKnowledgeBase;     // Shared game state, navmesh, etc.
class CBaseEntity;           // Game entity

// Conceptual Game Mode
enum class GameMode { UNKNOWN, CONTROL_POINT, CAPTURE_THE_FLAG, PAYLOAD_ATTACK, PAYLOAD_DEFEND };


class CObjectivePlanner {
public:
    CObjectivePlanner(CFFPlayer* pBotOwner, const BotKnowledgeBase* pKnowledgeBase);
    ~CObjectivePlanner();

    void UpdateGameState(const BotKnowledgeBase* pKnowledgeBase); // Optionally refresh KB pointer

    // Main planning call for the bot AI to invoke
    void EvaluateAndSelectTask();

    const HighLevelTask* GetCurrentHighLevelTask() const;
    const SubTask* GetCurrentSubTask() const;

    void OnSubTaskCompleted();
    void OnSubTaskFailed();
    void OnBotKilled();

private:
    CFFPlayer* m_pBotOwner;
    const BotKnowledgeBase* m_pKnowledgeBase;

    HighLevelTask m_CurrentHighLevelTask;
    std::vector<HighLevelTask> m_AvailableTasks;

    // Task Selection
    void GenerateAvailableTasks();
    void PrioritizeTasks();        // Simple version for now
    bool SelectTaskFromList();

    // Subtask Decomposition
    bool DecomposeTask(HighLevelTask& task);

    // Game Mode Specific Logic
    GameMode GetCurrentGameMode() const; // Conceptual
    void GenerateTasks_ControlPoint();
    // void GenerateTasks_CaptureTheFlag();
    // void GenerateTasks_PayloadAttack();
    // void GenerateTasks_PayloadDefend();

    // Helper for decomposition
    void AddMoveAndCaptureSubTasks(HighLevelTask& hlt, const Vector& targetPos, CBaseEntity* targetEnt = nullptr);
    void AddMoveAndDefendSubTasks(HighLevelTask& hlt, const Vector& targetPos, CBaseEntity* targetEnt = nullptr);
};

#endif // OBJECTIVE_PLANNER_H
