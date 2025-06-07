#include "ObjectivePlanner.h"
#include "FFBaseAI.h"       // For BotKnowledgeBase (if not in its own header)
#include "CFFPlayer.h"      // Conceptual CFFPlayer
#include "CBaseEntity.h"    // Conceptual CBaseEntity
#include <algorithm>        // For std::sort (in PrioritizeTasks if used)
#include <iostream>         // For placeholder logging

// --- CObjectivePlanner Implementation ---

CObjectivePlanner::CObjectivePlanner(CFFPlayer* pBotOwner, const BotKnowledgeBase* pKnowledgeBase)
    : m_pBotOwner(pBotOwner),
      m_pKnowledgeBase(pKnowledgeBase) {
    m_CurrentHighLevelTask.type = HighLevelTaskType::NONE; // Start with no task
}

CObjectivePlanner::~CObjectivePlanner() {}

void CObjectivePlanner::UpdateGameState(const BotKnowledgeBase* pKnowledgeBase) {
    m_pKnowledgeBase = pKnowledgeBase; // Update pointer if KB changes externally
}

GameMode CObjectivePlanner::GetCurrentGameMode() const {
    // Conceptual: This would query the game rules or a global state manager.
    // For example: if (FortressForever::GetGameRules()->IsControlPointGame()) return GameMode::CONTROL_POINT;
    // For now, hardcode to Control Point for testing.
    return GameMode::CONTROL_POINT;
}

void CObjectivePlanner::EvaluateAndSelectTask() {
    // If current HLT is valid, has subtasks, and current subtask is not completed, continue it.
    const SubTask* currentSub = GetCurrentSubTask();
    if (m_CurrentHighLevelTask.IsValid() && currentSub && !currentSub->isCompleted) {
        // std::cout << "Planner: Continuing current HLT & SubTask" << std::endl;
        return;
    }
    // If HLT is valid but current subtask IS completed, OnSubTaskCompleted should have advanced it.
    // If all subtasks of current HLT are done, IsValid() might still be true but GetCurrentSubTask() would be null.

    // Clear completed/failed HLT to allow new selection.
    // OnSubTaskCompleted/Failed should handle clearing m_CurrentHighLevelTask when the HLT is fully done/failed.
    // This check ensures we re-evaluate if the HLT finished.
    if (m_CurrentHighLevelTask.IsValid() && !GetCurrentSubTask()) { // HLT is done (all subtasks finished)
        // Log completion of the HLT here for learning data if needed
        std::cout << "Planner: HighLevelTask '" << m_CurrentHighLevelTask.description << "' completed all subtasks." << std::endl;
        m_CurrentHighLevelTask.Reset(); // Mark HLT as invalid/done
    }


    // If no valid HLT, or current one just finished, try to get a new one.
    if (!m_CurrentHighLevelTask.IsValid()) {
        // std::cout << "Planner: No valid HLT or current HLT finished. Evaluating new tasks." << std::endl;
        GenerateAvailableTasks();
        PrioritizeTasks(); // Simple version will just use order from Generate.
        if (SelectTaskFromList()) { // This sets m_CurrentHighLevelTask
            // std::cout << "Planner: New HLT selected: " << m_CurrentHighLevelTask.description << std::endl;
            if (!DecomposeTask(m_CurrentHighLevelTask)) {
                std::cerr << "Planner Error: Failed to decompose task: " << m_CurrentHighLevelTask.description << std::endl;
                m_CurrentHighLevelTask.Reset(); // Failed decomposition, clear it.
            } else {
                // std::cout << "Planner: Task decomposed. First subtask: " << (int)GetCurrentSubTask()->type << std::endl;
            }
        } else {
            // std::cout << "Planner: No suitable new HLT found." << std::endl;
        }
    }
}

const HighLevelTask* CObjectivePlanner::GetCurrentHighLevelTask() const {
    return m_CurrentHighLevelTask.IsValid() ? &m_CurrentHighLevelTask : nullptr;
}

const SubTask* CObjectivePlanner::GetCurrentSubTask() const {
    if (m_CurrentHighLevelTask.IsValid()) {
        return m_CurrentHighLevelTask.GetCurrentSubTask();
    }
    return nullptr;
}

void CObjectivePlanner::OnSubTaskCompleted() {
    // std::cout << "Planner: SubTask completed." << std::endl;
    if (m_CurrentHighLevelTask.IsValid()) {
        if (!m_CurrentHighLevelTask.AdvanceToNextSubTask()) { // No more subtasks
            // Log HLT completion here for learning
            std::cout << "Planner: All subtasks for HLT '" << m_CurrentHighLevelTask.description << "' completed." << std::endl;
            // For learning: LogTaskOutcome(m_CurrentHighLevelTask, Outcome::SUCCESS);
            m_CurrentHighLevelTask.Reset(); // Mark HLT as done
        } else {
            // std::cout << "Planner: Advanced to next SubTask." << std::endl;
        }
    }
}

void CObjectivePlanner::OnSubTaskFailed() {
    // std::cout << "Planner: SubTask failed." << std::endl;
    if (m_CurrentHighLevelTask.IsValid()) {
        // Log HLT failure here for learning
         std::cout << "Planner: HLT '" << m_CurrentHighLevelTask.description << "' failed due to subtask failure." << std::endl;
        // For learning: LogTaskOutcome(m_CurrentHighLevelTask, Outcome::FAILURE);
        m_CurrentHighLevelTask.Reset(); // Mark HLT as failed, force re-evaluation
    }
}

void CObjectivePlanner::OnBotKilled() {
    // std::cout << "Planner: Bot killed. Resetting HLT." << std::endl;
    if (m_CurrentHighLevelTask.IsValid()) {
         // For learning: LogTaskOutcome(m_CurrentHighLevelTask, Outcome::ABORTED); // Or FAILED
    }
    m_CurrentHighLevelTask.Reset();
}

void CObjectivePlanner::GenerateAvailableTasks() {
    m_AvailableTasks.clear();
    if (!m_pKnowledgeBase /* || !m_pBotOwner */) return; // Need KB and bot owner info

    GameMode currentMode = GetCurrentGameMode();
    switch (currentMode) {
        case GameMode::CONTROL_POINT:
            GenerateTasks_ControlPoint();
            break;
        // case GameMode::CAPTURE_THE_FLAG: GenerateTasks_CaptureTheFlag(); break;
        // ... other game modes
        default:
            std::cout << "Planner: No task generation logic for current game mode." << std::endl;
            break;
    }
}

void CObjectivePlanner::GenerateTasks_ControlPoint() {
    // Conceptual: Access CP data from m_pKnowledgeBase, which points to CRCBotPlugin's m_ControlPoints vector
    // For now, create dummy tasks as the actual CP data structures might not be fully populated or accessible yet.
    // int myTeam = m_pBotOwner ? m_pBotOwner->GetTeamNumber() : 0; // Conceptual

    // Dummy Task 1: Capture a point (e.g., CP1 at 1000,0,0)
    HighLevelTask captureTask(HighLevelTaskType::CAPTURE_POINT, 80.0f); // Default priority 80
    captureTask.targetPosition = Vector(1000, 0, 0);
    // captureTask.pTargetEntity = FindEntityForCP("CP1"); // Conceptual
    captureTask.description = "Capture CP at (1000,0,0)";
    m_AvailableTasks.push_back(captureTask);

    // Dummy Task 2: Defend a point (e.g., CP2 at -1000,0,0)
    HighLevelTask defendTask(HighLevelTaskType::DEFEND_POINT, 70.0f); // Default priority 70
    defendTask.targetPosition = Vector(-1000, 0, 0);
    // defendTask.pTargetEntity = FindEntityForCP("CP2"); // Conceptual
    defendTask.description = "Defend CP at (-1000,0,0)";
    m_AvailableTasks.push_back(defendTask);

    std::cout << "Planner: Generated " << m_AvailableTasks.size() << " dummy CP tasks." << std::endl;
}

void CObjectivePlanner::PrioritizeTasks() {
    if (m_AvailableTasks.empty()) return;
    // Simple prioritization: For now, just use the order they were added or sort by default priority.
    // More advanced: iterate m_AvailableTasks, calculate dynamic priority based on features
    // (distance, threat, bot class suitability, game state) and then sort.
    std::sort(m_AvailableTasks.begin(), m_AvailableTasks.end(),
              [](const HighLevelTask& a, const HighLevelTask& b) {
                  return a.priority > b.priority; // Higher priority first
              });
    // std::cout << "Planner: Prioritized available tasks." << std::endl;
}

bool CObjectivePlanner::SelectTaskFromList() {
    if (m_AvailableTasks.empty()) {
        m_CurrentHighLevelTask.Reset();
        return false;
    }
    // Select the highest priority task (first in the sorted list)
    m_CurrentHighLevelTask = m_AvailableTasks[0];
    // std::cout << "Planner: Selected HLT: " << m_CurrentHighLevelTask.description << " with prio " << m_CurrentHighLevelTask.priority << std::endl;
    return true;
}

bool CObjectivePlanner::DecomposeTask(HighLevelTask& task) {
    task.subTasks.clear();
    task.currentSubTaskIndex = -1;

    switch (task.type) {
        case HighLevelTaskType::CAPTURE_POINT:
            AddMoveAndCaptureSubTasks(task, task.targetPosition, task.pTargetEntity);
            break;
        case HighLevelTaskType::DEFEND_POINT:
            AddMoveAndDefendSubTasks(task, task.targetPosition, task.pTargetEntity);
            break;
        // ... other HLT types
        default:
            std::cerr << "Planner Error: Unknown HighLevelTaskType for decomposition: " << static_cast<int>(task.type) << std::endl;
            return false;
    }

    if (!task.subTasks.empty()) {
        task.currentSubTaskIndex = 0;
        // std::cout << "Planner: Decomposed task. First subtask type: " << (int)task.subTasks[0].type << std::endl;
        return true;
    }
    std::cerr << "Planner Error: Task decomposition resulted in no subtasks for " << task.description << std::endl;
    return false;
}

void CObjectivePlanner::AddMoveAndCaptureSubTasks(HighLevelTask& hlt, const Vector& targetPos, CBaseEntity* targetEnt) {
    SubTask move(SubTaskType::MOVE_TO_POSITION);
    move.targetPosition = targetPos;
    if (targetEnt) { // If entity is known, can target it more specifically
        move.type = SubTaskType::MOVE_TO_ENTITY;
        move.pTargetEntity = targetEnt;
    }
    hlt.AddSubTask(move);

    // Conceptual: Secure area around point before trying to cap
    // SubTask secure(SubTaskType::SECURE_AREA);
    // secure.targetPosition = targetPos;
    // secure.desiredDuration = 8.0f; // Secure for 8 seconds
    // hlt.AddSubTask(secure);

    SubTask capture(SubTaskType::CAPTURE_OBJECTIVE);
    capture.pTargetEntity = targetEnt; // The CP entity
    capture.targetPosition = targetPos; // Stand on point
    // capture.desiredDuration = 20.0f; // Max time to attempt capture on point
    hlt.AddSubTask(capture);
}

void CObjectivePlanner::AddMoveAndDefendSubTasks(HighLevelTask& hlt, const Vector& targetPos, CBaseEntity* targetEnt){
    SubTask move(SubTaskType::MOVE_TO_POSITION);
    move.targetPosition = targetPos; // Move to the general defensive area / CP
    if (targetEnt) {
        move.type = SubTaskType::MOVE_TO_ENTITY;
        move.pTargetEntity = targetEnt;
    }
    hlt.AddSubTask(move);

    // Conceptual: Secure area more thoroughly for defense
    // SubTask secure(SubTaskType::SECURE_AREA);
    // secure.targetPosition = targetPos;
    // secure.desiredDuration = 15.0f;
    // hlt.AddSubTask(secure);

    // Conceptual: Hold position task
    // SubTask hold(SubTaskType::HOLD_POSITION); // Assuming HOLD_POSITION is a defined SubTaskType
    // hold.targetPosition = targetPos;
    // hold.desiredDuration = 60.0f; // Hold for a minute or until interrupted
    // hlt.AddSubTask(hold);
    // For now, using CAPTURE_OBJECTIVE as a placeholder for "stay on point" for defense too.
    // A dedicated DEFEND_AREA or HOLD_POSITION subtask would be better.
    SubTask defend_stand(SubTaskType::CAPTURE_OBJECTIVE); // Placeholder for "stay and defend"
    defend_stand.pTargetEntity = targetEnt;
    defend_stand.targetPosition = targetPos;
    defend_stand.desiredDuration = 60.0f;
    hlt.AddSubTask(defend_stand);
}
