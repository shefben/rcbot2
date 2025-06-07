#include "ObjectivePlanner.h"
#include "FFBaseAI.h"       // For BotKnowledgeBase (which includes ff_state_structs.h for Vector, CPInfo etc.)
                            // Also for CFFPlayer, CBaseEntity conceptual placeholders
#include <algorithm>        // For std::sort, std::find_if
#include <iostream>         // For placeholder logging

// --- Conceptual Placeholder Implementations (would rely on actual game/bot SDK) ---
// These allow the ObjectivePlanner to compile and have basic logic flow.

#ifndef CBASEENTITY_CONCEPTUAL_DEF_OBJECTIVEPLANNER // Prevent redefinition
#define CBASEENTITY_CONCEPTUAL_DEF_OBJECTIVEPLANNER
// class CBaseEntity { public: Vector GetPosition() const; bool IsAlive() const; int GetTeamNumber() const; /* ... */ };
#endif

#ifndef CFFPLAYER_CONCEPTUAL_DEF_OBJECTIVEPLANNER
#define CFFPLAYER_CONCEPTUAL_DEF_OBJECTIVEPLANNER
// class CFFPlayer { public: Vector GetPosition() const; bool IsAlive() const; int GetTeamNumber() const; /* ... */ };
#endif

// Assume BotKnowledgeBase provides access to ControlPointInfo vector
// struct BotKnowledgeBase {
//    const std::vector<ControlPointInfo>* controlPoints;
//    GameModeType GetGameMode() const { return GameModeType::CONTROL_POINT; } // Example
//    // ... other game state accessors
//    BotKnowledgeBase() : controlPoints(nullptr) {}
// };
// --- End Conceptual Placeholders ---


// --- HighLevelTask Method Implementations (from BotTasks.h) ---
// (Moved here as they are primarily used by ObjectivePlanner and this was specified in plan)

const SubTask* HighLevelTask::GetCurrentSubTask() const {
    if (type != HighLevelTaskType::NONE && currentSubTaskIndex >= 0 && currentSubTaskIndex < (int)subTasks.size()) {
        return &subTasks[currentSubTaskIndex];
    }
    return nullptr;
}

SubTask* HighLevelTask::GetCurrentSubTaskMutable() {
    if (type != HighLevelTaskType::NONE && currentSubTaskIndex >= 0 && currentSubTaskIndex < (int)subTasks.size()) {
        return &subTasks[currentSubTaskIndex];
    }
    return nullptr;
}

bool HighLevelTask::AdvanceToNextSubTask() {
    if (type == HighLevelTaskType::NONE || subTasks.empty()) return false;

    // Check if current task was actually completed successfully before advancing and resetting next.
    // If it failed, the HLT might need to fail entirely or re-strategize.
    // For now, we assume OnSubTaskCompleted is only called on success by AI.
    if (currentSubTaskIndex >=0 && currentSubTaskIndex < (int)subTasks.size()){
        // if (!subTasks[currentSubTaskIndex].isCompleted) {
            // Current subtask wasn't marked completed, this might be an issue or forced advance.
            // std::cout << "Warning: Advancing HLT when current subtask not marked completed." << std::endl;
        // }
    }

    currentSubTaskIndex++;
    if (currentSubTaskIndex < (int)subTasks.size()) {
        subTasks[currentSubTaskIndex].isCompleted = false; // Ensure new subtask is not stale
        return true; // Advanced to a new valid subtask
    }
    // Reached end of subtask list
    // std::cout << "HLT: All subtasks finished for " << description << std::endl;
    return false;
}

void HighLevelTask::Reset() {
    type = HighLevelTaskType::NONE;
    priority = 0.0f;
    targetPosition = Vector();
    pTargetEntity = nullptr;
    description = "";
    subTasks.clear();
    currentSubTaskIndex = -1;
}


// --- CObjectivePlanner Implementation ---

CObjectivePlanner::CObjectivePlanner(CFFPlayer* pBotOwner, const BotKnowledgeBase* pKnowledgeBase)
    : m_pBotOwner(pBotOwner),
      m_pKnowledgeBase(pKnowledgeBase) {
    m_CurrentHighLevelTask.Reset(); // Ensure it starts as NONE
    // std::cout << "CObjectivePlanner: Initialized." << std::endl;
}

CObjectivePlanner::~CObjectivePlanner() {
    // std::cout << "CObjectivePlanner: Destroyed." << std::endl;
}

// void CObjectivePlanner::UpdateKnowledgeBase(const BotKnowledgeBase* pKnowledgeBase) {
//     m_pKnowledgeBase = pKnowledgeBase;
// }

GameModeType CObjectivePlanner::GetCurrentGameMode() const {
    if (m_pKnowledgeBase) {
        // return m_pKnowledgeBase->GetGameMode(); // Conceptual
    }
    return GameModeType::CONTROL_POINT; // Default for now
}

void CObjectivePlanner::EvaluateAndSelectTask() {
    const SubTask* currentSub = GetCurrentSubTask();
    if (m_CurrentHighLevelTask.IsValid() && currentSub && !currentSub->isCompleted) {
        // std::cout << "Planner: Continuing current HLT '" << m_CurrentHighLevelTask.description
        //           << "', SubTask: " << static_cast<int>(currentSub->type) << std::endl;
        return; // Current HLT has an active, incomplete subtask
    }

    // If HLT is valid but GetCurrentSubTask() is null, it means all subtasks were completed or it failed to advance.
    if (m_CurrentHighLevelTask.IsValid() && !currentSub) {
        // std::cout << "Planner: Current HLT '" << m_CurrentHighLevelTask.description << "' has no more subtasks or finished." << std::endl;
        // For learning: LogTaskOutcome(m_CurrentHighLevelTask, Outcome::SUCCESS); // Assuming success if all subtasks done
        m_CurrentHighLevelTask.Reset();
    }

    // If no valid HLT, try to select a new one
    if (m_CurrentHighLevelTask.type == HighLevelTaskType::NONE) {
        // std::cout << "Planner: No current HLT. Generating and selecting new task." << std::endl;
        GenerateAvailableTasks();
        PrioritizeTasks();
        if (SelectTaskFromList()) { // Sets m_CurrentHighLevelTask
            // std::cout << "Planner: New HLT selected: " << m_CurrentHighLevelTask.description << std::endl;
            if (!DecomposeTask(m_CurrentHighLevelTask)) {
                std::cerr << "Planner Error: Failed to decompose task: " << m_CurrentHighLevelTask.description << std::endl;
                // For learning: LogTaskOutcome(m_CurrentHighLevelTask, Outcome::FAILURE, "DecompositionFailed");
                m_CurrentHighLevelTask.Reset();
            } else if (m_CurrentHighLevelTask.subTasks.empty()){
                 std::cerr << "Planner Error: Task decomposition resulted in no subtasks for " << m_CurrentHighLevelTask.description << std::endl;
                 m_CurrentHighLevelTask.Reset();
            } else {
                // std::cout << "Planner: Task decomposed. First subtask type: "
                //           << static_cast<int>(m_CurrentHighLevelTask.subTasks[0].type) << std::endl;
            }
        } else {
            // std::cout << "Planner: No suitable new HLT found. Bot will idle or do default actions." << std::endl;
        }
    }
}

const SubTask* CObjectivePlanner::GetCurrentSubTask() const {
    return m_CurrentHighLevelTask.GetCurrentSubTask();
}

SubTask* CObjectivePlanner::GetCurrentSubTaskMutable() {
    return m_CurrentHighLevelTask.GetCurrentSubTaskMutable();
}

void CObjectivePlanner::OnSubTaskCompleted() {
    // std::cout << "Planner: Received OnSubTaskCompleted." << std::endl;
    if (m_CurrentHighLevelTask.IsValid()) {
        if (!m_CurrentHighLevelTask.AdvanceToNextSubTask()) { // No more subtasks
            // std::cout << "Planner: All subtasks for HLT '" << m_CurrentHighLevelTask.description << "' are now complete." << std::endl;
            // For learning: LogTaskOutcome(m_CurrentHighLevelTask, Outcome::SUCCESS);
            m_CurrentHighLevelTask.Reset(); // HLT is fully done
        } else {
            // const SubTask* nextSub = GetCurrentSubTask();
            // std::cout << "Planner: Advanced to next SubTask, type: " << (nextSub ? static_cast<int>(nextSub->type) : -1) << std::endl;
        }
    }
}

void CObjectivePlanner::OnSubTaskFailed() {
    // std::cout << "Planner: Received OnSubTaskFailed for HLT '" << m_CurrentHighLevelTask.description << "'." << std::endl;
    if (m_CurrentHighLevelTask.IsValid()) {
        // For learning: LogTaskOutcome(m_CurrentHighLevelTask, Outcome::FAILURE, "SubTaskFailed");
        m_CurrentHighLevelTask.Reset(); // HLT failed, force re-evaluation
    }
}

void CObjectivePlanner::OnBotKilled() {
    // std::cout << "Planner: Received OnBotKilled. Resetting HLT." << std::endl;
    if (m_CurrentHighLevelTask.IsValid()) {
        // For learning: LogTaskOutcome(m_CurrentHighLevelTask, Outcome::ABORTED, "BotKilled");
    }
    m_CurrentHighLevelTask.Reset();
}

void CObjectivePlanner::GenerateAvailableTasks() {
    m_AvailableTasks.clear();
    if (!m_pKnowledgeBase || !m_pBotOwner) {
        std::cerr << "Planner: KnowledgeBase or BotOwner not set, cannot generate tasks." << std::endl;
        return;
    }

    GameModeType currentMode = GetCurrentGameMode(); // Assumes this can be determined
    switch (currentMode) {
        case GameModeType::CONTROL_POINT:
            GenerateTasks_ControlPoint_FF();
            break;
        // case GameModeType::CAPTURE_THE_FLAG: GenerateTasks_CaptureTheFlag_FF(); break;
        default:
            // std::cout << "Planner: Task generation not implemented for game mode " << static_cast<int>(currentMode) << std::endl;
            break;
    }
     // std::cout << "Planner: Generated " << m_AvailableTasks.size() << " available tasks." << std::endl;
}

void CObjectivePlanner::GenerateTasks_ControlPoint_FF() {
    // This is a placeholder. It needs actual game state data from m_pKnowledgeBase.
    // Conceptual: int myTeam = m_pBotOwner->GetTeamNumber();
    // Conceptual: const std::vector<ControlPointInfo>* cpList = m_pKnowledgeBase->controlPoints;
    // if (!cpList) return;

    // for (const auto& cp : *cpList) {
    //     if (cp.ownerTeam != myTeam && !cp.isLocked /* for myTeam */) {
    //         HighLevelTask capTask(HighLevelTaskType::CAPTURE_POINT_FF, 80.0f);
    //         capTask.targetPosition = cp.position;
    //         capTask.pTargetEntity = reinterpret_cast<CBaseEntity*>(cp.pEntity); // Assuming pEntity exists
    //         capTask.description = "Capture " + cp.gameDisplayName;
    //         m_AvailableTasks.push_back(capTask);
    //     } else if (cp.ownerTeam == myTeam && cp.isBeingCapturedByEnemy /* conceptual */) {
    //         HighLevelTask defTask(HighLevelTaskType::DEFEND_POINT_FF, 90.0f);
    //         defTask.targetPosition = cp.position;
    //         defTask.pTargetEntity = reinterpret_cast<CBaseEntity*>(cp.pEntity);
    //         defTask.description = "Defend " + cp.gameDisplayName;
    //         m_AvailableTasks.push_back(defTask);
    //     }
    // }

    // --- Dummy Tasks for Testing ---
    if (m_AvailableTasks.empty()) { // Only add dummies if real logic (commented out) adds nothing
        HighLevelTask captureTask(HighLevelTaskType::CAPTURE_POINT_FF, 80.0f);
        captureTask.targetPosition = Vector(1000, 200, 0); // Example position
        // captureTask.pTargetEntity = GetSomeGameEntityAt(captureTask.targetPosition); // Conceptual
        captureTask.description = "Capture Dummy CP Alpha";
        m_AvailableTasks.push_back(captureTask);

        HighLevelTask defendTask(HighLevelTaskType::DEFEND_POINT_FF, 75.0f);
        defendTask.targetPosition = Vector(-500, 300, 0); // Example position
        defendTask.description = "Defend Dummy CP Bravo";
        m_AvailableTasks.push_back(defendTask);
    }
    // --- End Dummy Tasks ---
}

void CObjectivePlanner::PrioritizeTasks() {
    if (m_AvailableTasks.empty()) return;
    // More complex prioritization would involve features: distance, threat, objective value, bot role etc.
    // For now, just sort by the base priority assigned during generation.
    std::sort(m_AvailableTasks.begin(), m_AvailableTasks.end(),
              [](const HighLevelTask& a, const HighLevelTask& b) {
                  return a.priority > b.priority; // Higher priority first
              });
    // std::cout << "Planner: Prioritized " << m_AvailableTasks.size() << " tasks." << std::endl;
}

bool CObjectivePlanner::SelectTaskFromList() {
    if (m_AvailableTasks.empty()) {
        // std::cout << "Planner: No available tasks to select." << std::endl;
        m_CurrentHighLevelTask.Reset(); // Ensure current task is invalid
        return false;
    }
    // Select the highest priority task (first in the sorted list)
    m_CurrentHighLevelTask = m_AvailableTasks[0]; // Copy assignment
    // std::cout << "Planner: Selected new HLT: " << m_CurrentHighLevelTask.description << " (Prio: " << m_CurrentHighLevelTask.priority << ")" << std::endl;
    return true;
}

bool CObjectivePlanner::DecomposeTask(HighLevelTask& task) {
    if (task.type == HighLevelTaskType::NONE) return false;

    task.subTasks.clear(); // Clear any previous subtasks
    // std::cout << "Planner: Decomposing task: " << task.description << std::endl;

    // Use helper methods for common sequences
    switch (task.type) {
        case HighLevelTaskType::CAPTURE_POINT_FF:
            AddDefaultSubTasksForCapture(task, task.targetPosition, task.pTargetEntity);
            break;
        case HighLevelTaskType::DEFEND_POINT_FF:
            AddDefaultSubTasksForDefense(task, task.targetPosition, task.pTargetEntity);
            break;
        // ... other HLT types like ATTACK_ENEMY, SEEK_HEALTH etc.
        default:
            std::cerr << "Planner Error: Unknown HighLevelTaskType for decomposition: " << static_cast<int>(task.type) << std::endl;
            return false;
    }

    if (!task.subTasks.empty()) {
        task.currentSubTaskIndex = 0; // Start with the first subtask
        // std::cout << "Planner: Task decomposed into " << task.subTasks.size() << " subtasks. First: " << static_cast<int>(task.subTasks[0].type) << std::endl;
        return true;
    }
    // std::cerr << "Planner Warning: Task decomposition resulted in no subtasks for " << task.description << std::endl;
    return false; // No subtasks added might mean an issue or a very simple HLT
}

// --- Example Decomposition Helpers ---
void CObjectivePlanner::AddDefaultSubTasksForMovement(HighLevelTask& hlt, const Vector& targetPos, CBaseEntity* targetEnt) {
    if (targetEnt) {
        hlt.AddSubTask(SubTask(SubTaskType::MOVE_TO_ENTITY, targetEnt));
    } else {
        hlt.AddSubTask(SubTask(SubTaskType::MOVE_TO_POSITION, targetPos));
    }
}

void CObjectivePlanner::AddDefaultSubTasksForCapture(HighLevelTask& hlt, const Vector& targetPos, CBaseEntity* targetEnt) {
    AddDefaultSubTasksForMovement(hlt, targetPos, targetEnt);
    hlt.AddSubTask(SubTask(SubTaskType::SECURE_AREA, targetPos, 8.0f)); // Secure for 8s
    // STAND_ON_POINT is more specific and better if available, CAPTURE_OBJECTIVE is generic
    hlt.AddSubTask(SubTask(SubTaskType::STAND_ON_POINT, targetEnt ? targetEnt : nullptr, 20.0f));
    if(!targetEnt) hlt.subTasks.back().targetPosition = targetPos; // Ensure STAND_ON_POINT also has position if no entity
}

void CObjectivePlanner::AddDefaultSubTasksForDefense(HighLevelTask& hlt, const Vector& targetPos, CBaseEntity* targetEnt) {
    AddDefaultSubTasksForMovement(hlt, targetPos, targetEnt); // Move to general CP area
    hlt.AddSubTask(SubTask(SubTaskType::SECURE_AREA, targetPos, 15.0f)); // Secure for 15s
    hlt.AddSubTask(SubTask(SubTaskType::HOLD_POSITION, targetPos, 60.0f)); // Hold for 60s
}
