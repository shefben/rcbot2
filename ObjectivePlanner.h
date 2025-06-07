#ifndef OBJECTIVE_PLANNER_H
#define OBJECTIVE_PLANNER_H

#include <vector>
#include <string>
#include <memory> // For std::unique_ptr if used for tasks, though HLT is direct member

#include "BotTasks.h" // Includes HighLevelTask, SubTask, Enums

// Forward declarations
class CFFPlayer;             // Represents the bot this planner is for
struct BotKnowledgeBase;     // Shared game state, navmesh, etc. (ensure FFStateStructs.h is included by this eventually)
class CBaseEntity;           // Conceptual game entity

// Conceptual Game Mode enum, if not defined globally
enum class GameModeType { UNKNOWN, CONTROL_POINT, CAPTURE_THE_FLAG, PAYLOAD_ATTACK, PAYLOAD_DEFEND };


class CObjectivePlanner {
public:
    CObjectivePlanner(CFFPlayer* pBotOwner, const BotKnowledgeBase* pKnowledgeBase);
    ~CObjectivePlanner();

    // Main method called by AI to get decisions
    void EvaluateAndSelectTask();

    const HighLevelTask* GetCurrentHighLevelTask() const {
        return m_CurrentHighLevelTask.type != HighLevelTaskType::NONE ? &m_CurrentHighLevelTask : nullptr;
    }
    const SubTask* GetCurrentSubTask() const;      // Gets subtask from current HLT
    SubTask* GetCurrentSubTaskMutable(); // Gets modifiable subtask from current HLT

    // Callbacks from AI module about subtask status
    void OnSubTaskCompleted();
    void OnSubTaskFailed();
    void OnBotKilled();         // Resets current HLT

    // Optional: To update KB if it's not always the same instance or needs refreshing signal
    // void UpdateKnowledgeBase(const BotKnowledgeBase* pKnowledgeBase);

private:
    CFFPlayer* m_pBotOwner;
    const BotKnowledgeBase* m_pKnowledgeBase;   // Pointer to shared game state info

    HighLevelTask m_CurrentHighLevelTask;       // The currently active high-level task
    std::vector<HighLevelTask> m_AvailableTasks; // Pool of potential tasks, priorities recalculated

    // Task Selection Pipeline
    void GenerateAvailableTasks(); // Populates m_AvailableTasks based on game mode & state
    void PrioritizeTasks();        // Scores and sorts m_AvailableTasks
    bool SelectTaskFromList();     // Chooses the highest priority valid task and sets it to m_CurrentHighLevelTask

    // Subtask Decomposition
    // Fills m_CurrentHighLevelTask.subTasks based on its type
    bool DecomposeTask(HighLevelTask& taskToDecompose);

    // Game Mode Specific Task Generation (called by GenerateAvailableTasks)
    GameModeType GetCurrentGameMode() const; // Conceptual: queries KB or game state
    void GenerateTasks_ControlPoint_FF();
    // void GenerateTasks_CaptureTheFlag_FF();
    // void GenerateTasks_PayloadAttack_FF();
    // void GenerateTasks_PayloadDefend_FF();

    // Example Helper for decomposition
    void AddDefaultSubTasksForMovement(HighLevelTask& hlt, const Vector& targetPos, CBaseEntity* targetEnt = nullptr);
    void AddDefaultSubTasksForCapture(HighLevelTask& hlt, const Vector& targetPos, CBaseEntity* targetEnt = nullptr);
    void AddDefaultSubTasksForDefense(HighLevelTask& hlt, const Vector& targetPos, CBaseEntity* targetEnt = nullptr);
};

#endif // OBJECTIVE_PLANNER_H
