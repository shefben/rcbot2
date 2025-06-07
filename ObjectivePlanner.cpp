#include "ObjectivePlanner.h"
#include "FFBaseAI.h"
#include "FFStateStructs.h"
#include "NavSystem.h"
#include "BotDefines.h"
#include "CRCBotPlugin.h"
#include "BotKnowledgeBase.h"

#include <algorithm>
#include <iostream>
#include <cmath>

// --- Conceptual Placeholder functions ---
bool IsCpCritical_Conceptual(const ControlPointInfo& cpInfo, const BotKnowledgeBase* kb) { return cpInfo.id == 3; }
float GetAreaThreat_Conceptual(const BotKnowledgeBase* kb, const Vector& pos) { return 0.0f; }
int CountAlliesNearPoint_Conceptual(const BotKnowledgeBase* kb, const Vector& pos, float radius) { return 1; }

// Updated to use ClassConfigInfo from CFFPlayer if available
std::string GetBotClassName_Conceptual(CFFPlayer* bot, const ClassConfigInfo* classCfg) {
    if (classCfg) return classCfg->className; // Prefer resolved ClassConfig
    // Fallback if CFFPlayer has a direct class name (less ideal)
    // if (bot) return bot->GetPlayerClassName_Conceptual();
    return "Unknown";
}
bool IsControlPointLockedForTeam_Conceptual(const ControlPointInfo& cpInfo, int teamId, const BotKnowledgeBase* kb) {
    return cpInfo.isLocked;
}
// Conceptual KB checks for Demoman priorities
bool IsChokePointNear_Conceptual(const BotKnowledgeBase* kb, const Vector& pos, float checkRadius = 300.0f) {
    // In a real system, this would check navmesh topology or pre-placed map hints.
    // For example, if nav areas near 'pos' have few connections or are tagged as chokes.
    return false; // Placeholder
}
// --- End Conceptual Placeholders ---


// --- HighLevelTask Method Implementations ---
// ... (GetCurrentSubTask, GetCurrentSubTaskMutable, AdvanceToNextSubTask, StartFirstSubTask, Reset - same as Task 18) ...
const SubTask* HighLevelTask::GetCurrentSubTask() const { if (type != HighLevelTaskType::NONE && currentSubTaskIndex >= 0 && currentSubTaskIndex < (int)subTasks.size()) { return &subTasks[currentSubTaskIndex]; } return nullptr; }
SubTask* HighLevelTask::GetCurrentSubTaskMutable() { if (type != HighLevelTaskType::NONE && currentSubTaskIndex >= 0 && currentSubTaskIndex < (int)subTasks.size()) { return &subTasks[currentSubTaskIndex]; } return nullptr; }
bool HighLevelTask::AdvanceToNextSubTask() { if (type == HighLevelTaskType::NONE || subTasks.empty()) return false; currentSubTaskIndex++; if (currentSubTaskIndex < (int)subTasks.size()) { if(currentSubTaskIndex > 0 && subTasks[currentSubTaskIndex-1].isCompleted) { subTasks[currentSubTaskIndex].isCompleted = false; } else if (currentSubTaskIndex == 0) { subTasks[currentSubTaskIndex].isCompleted = false; } subTasks[currentSubTaskIndex].startTime = std::chrono::system_clock::now(); return true; } return false; }
void HighLevelTask::StartFirstSubTask() { if (!subTasks.empty()) { currentSubTaskIndex = 0; subTasks[0].isCompleted = false; subTasks[0].startTime = std::chrono::system_clock::now(); } else { currentSubTaskIndex = -1; } }
void HighLevelTask::Reset() { type = HighLevelTaskType::NONE; priority = 0.0f; targetPosition = Vector(); pTargetEntity = nullptr; description = ""; targetNameOrId_Logging = ""; subTasks.clear(); currentSubTaskIndex = -1; }


// --- CObjectivePlanner Implementation ---
CObjectivePlanner::CObjectivePlanner(CFFPlayer* pBotOwner, const BotKnowledgeBase* pKnowledgeBase, CRCBotPlugin* pPluginOwner)
    : m_pBotOwner(pBotOwner), m_pKnowledgeBase(pKnowledgeBase), m_pPluginOwner(pPluginOwner) {
    m_CurrentHighLevelTask.Reset(); m_CurrentTaskLogEntry.Reset();
}

CObjectivePlanner::~CObjectivePlanner() {}

GameModeType_KB CObjectivePlanner::GetCurrentGameMode() const { /* ... (same as Task 18) ... */ return GameModeType_KB::UNKNOWN;}
void CObjectivePlanner::EvaluateAndSelectTask() { /* ... (same as Task 18) ... */ }
const SubTask* CObjectivePlanner::GetCurrentSubTask() const { /* ... (same as Task 18) ... */ }
SubTask* CObjectivePlanner::GetCurrentSubTaskMutable() { /* ... (same as Task 18) ... */ }
void CObjectivePlanner::OnBotKilled() { /* ... (same as Task 18, calls FinalizeAndStoreCurrentHLTLog) ... */ }
GameStateSnapshot CObjectivePlanner::CreateGameStateSnapshot() const { /* ... (same as Task 18) ... */ return GameStateSnapshot();}
void CObjectivePlanner::FinalizeSubTaskLog(const SubTask* pSubTask, bool success, const std::string& reason) { /* ... (from Task 18) ... */ }
void CObjectivePlanner::FinalizeAndStoreCurrentHLTLog(TaskOutcomeLog::Outcome outcome, float score) { /* ... (same as Task 18) ... */ }
void CObjectivePlanner::ProcessSubTaskCompletion(bool success) { /* ... (same as Task 18) ... */ }
void CObjectivePlanner::OnSubTaskOutcomeReported(bool success, const std::string& failureReason) { /* ... (same as Task 18) ... */ }
void CObjectivePlanner::GenerateAvailableTasks() { /* ... (same as Task 18, calls mode specific generation) ... */ }
void CObjectivePlanner::GenerateTasks_ControlPoint_FF() { /* ... (modified below) ... */ }
void CObjectivePlanner::PrioritizeTasks() { /* ... (same as Task 18) ... */ }
bool CObjectivePlanner::SelectTaskFromList() { /* ... (same as Task 18, initializes m_CurrentTaskLogEntry) ... */ return false;}
void CObjectivePlanner::GenerateTasks_CaptureTheFlag_FF() { /* ... (modified below) ... */ }
// Decomposition helpers from Task 18
void CObjectivePlanner::AddDefaultSubTasksForCapture(HighLevelTask& hlt) { /* ... */ }
void CObjectivePlanner::AddDefaultSubTasksForDefense(HighLevelTask& hlt) { /* ... */ }
void CObjectivePlanner::AddSubTasks_CaptureEnemyFlag(HighLevelTask& hlt) { /* ... */ }
void CObjectivePlanner::AddSubTasks_ReturnOurFlag(HighLevelTask& hlt) { /* ... */ }
void CObjectivePlanner::AddSubTasks_KillEnemyFlagCarrier(HighLevelTask& hlt) { /* ... */ }
void CObjectivePlanner::AddSubTasks_EscortFlagCarrier(HighLevelTask& hlt) { /* ... */ }
void CObjectivePlanner::AddSubTasks_DefendFlagStand(HighLevelTask& hlt) { /* ... */ }
void CObjectivePlanner::AddSubTasks_CampEnemyFlagSpawn(HighLevelTask& hlt) { /* ... */ }
void CObjectivePlanner::AddSubTasks_InterceptEnemyFlagRunner(HighLevelTask& hlt) { /* ... */ }
bool CObjectivePlanner::DecomposeTask(HighLevelTask& task) { /* ... (same as Task 18, calls these helpers) ... */ return false; }


// --- Modified Priority Calculation Helpers ---
float CObjectivePlanner::CalculateCapturePriority(const ControlPointInfo& cpInfo, CFFPlayer* bot, const BotKnowledgeBase* kb) {
    if (!bot || !kb || !kb->GetNavGraph()) return 0.0f;
    float priority = BASE_CAPTURE_PRIORITY;

    if (IsCpCritical_Conceptual(cpInfo, kb)) priority += CRITICAL_POINT_BONUS;

    Vector botOrigin = bot->GetPosition();
    float distSq = (botOrigin.x - cpInfo.position.x)*(botOrigin.x - cpInfo.position.x) + (botOrigin.y - cpInfo.position.y)*(botOrigin.y - cpInfo.position.y);
    priority -= sqrt(distSq) * DISTANCE_PENALTY_FACTOR;

    // float threat = GetAreaThreat_Conceptual(kb, cpInfo.position);
    // priority -= threat * THREAT_PENALTY_FACTOR;

    std::string botClassName = GetBotClassName_Conceptual(bot, nullptr /* pass ClassConfig if CFFPlayer doesn't have it */);
    if (botClassName == "Scout") priority += CAPTURER_CLASS_BONUS;
    // Demoman specific for CP Capture
    if (botClassName == "Demoman") {
        priority -= 5.0f; // Slightly less suited for aggressive solo point capture initially
        if (IsChokePointNear_Conceptual(kb, cpInfo.position)) priority += 10.0f; // Good for area denial after cap
    }

    // int alliesOnPoint = CountAlliesNearPoint_Conceptual(kb, cpInfo.position, CP_CAPTURE_RADIUS_FF);
    // if (alliesOnPoint == 0) priority += ALLY_DEFICIT_ON_POINT_BONUS;

    return std::max(1.0f, priority);
}

float CObjectivePlanner::CalculateDefensePriority(const ControlPointInfo& cpInfo, CFFPlayer* bot, const BotKnowledgeBase* kb) {
    if (!bot || !kb) return 0.0f;
    float priority = BASE_DEFENSE_PRIORITY;

    // bool isCpBeingCaptured = false; // Conceptual: kb->IsPointBeingContested(cpInfo.id, bot->GetTeamNumber())
    // if (isCpBeingCaptured) priority = POINT_BEING_CAPTURED_BY_ENEMY_BONUS;
    // else if (IsCpCritical_Conceptual(cpInfo, kb)) priority += CRITICAL_POINT_BONUS;

    Vector botOrigin = bot->GetPosition();
    float distSq = (botOrigin.x - cpInfo.position.x)*(botOrigin.x - cpInfo.position.x) + (botOrigin.y - cpInfo.position.y)*(botOrigin.y - cpInfo.position.y);
    priority -= sqrt(distSq) * DISTANCE_PENALTY_FACTOR;

    std::string botClassName = GetBotClassName_Conceptual(bot, nullptr);
    if (botClassName == "Demoman") {
        priority += 15.0f; // Well-suited for defense
        if (IsChokePointNear_Conceptual(kb, cpInfo.position)) priority += 10.0f; // Excellent for traps
    } else if (botClassName == "Heavy") { // Example for another class
        priority += DEFENDER_CLASS_BONUS;
    }

    return std::max(1.0f, priority);
}

// --- CTF Priority Helper Implementations (with Demoman logic) ---
float CObjectivePlanner::CalculateCTFCaptureEnemyFlagPriority_Conceptual(const FlagInfo& enemyFlag, CFFPlayer* bot, const BotKnowledgeBase* kb) {
    if (!bot || !kb) return 0.0f;
    float priority = 60.0f;
    // priority -= (bot->GetOrigin() - enemyFlag.currentPosition).Length() * DISTANCE_PENALTY_FACTOR;
    std::string botClassName = GetBotClassName_Conceptual(bot, nullptr);
    if (botClassName == "Scout") priority += 20.0f;
    if (botClassName == "Demoman") priority -= 10.0f; // Not ideal for flag running
    // if (enemyFlag.status == FlagStatus::DROPPED_ENEMY) priority += 15.0f;
    return std::max(1.0f, priority);
}

float CObjectivePlanner::CalculateCTFReturnOurFlagPriority_Conceptual(const FlagInfo& ourFlag, CFFPlayer* bot, const BotKnowledgeBase* kb) {
    if (!bot || !kb) return 0.0f;
    float priority = 90.0f;
    // priority -= (bot->GetOrigin() - ourFlag.currentPosition).Length() * DISTANCE_PENALTY_FACTOR;
    std::string botClassName = GetBotClassName_Conceptual(bot, nullptr);
    if (botClassName == "Demoman") priority += 5.0f; // Can help secure dropped flag area
    return std::max(1.0f, priority);
}

float CObjectivePlanner::CalculateCTFDefendFlagStandPriority_Conceptual(const FlagInfo& ourFlag, CFFPlayer* bot, const BotKnowledgeBase* kb) {
    if (!bot || !kb) return 0.0f;
    float priority = 50.0f;
    std::string botClassName = GetBotClassName_Conceptual(bot, nullptr);
    if (botClassName == "Demoman") {
        priority += 20.0f; // Excellent for flag defense with traps
        // if (IsChokePointNear_Conceptual(kb, ourFlag.basePosition)) priority += 15.0f;
    } else if (botClassName == "Engineer_Conceptual") { // Example
        priority += 25.0f;
    }
    // float enemyProximityToOurBase = kb->GetNearestEnemyDistanceToPoint(ourFlag.basePosition);
    // if (enemyProximityToOurBase < 500.0f) priority += 30.0f;
    return std::max(1.0f, priority);
}

float CObjectivePlanner::CalculateCTFKillEnemyFlagCarrierPriority_Conceptual(const FlagInfo& ourFlag, const CBaseEntity* carrier, CFFPlayer* bot, const BotKnowledgeBase* kb) {
    if (!bot || !kb || !carrier) return 0.0f;
    float priority = 100.0f;
    // priority -= (bot->GetOrigin() - carrier->GetPosition()).Length() * DISTANCE_PENALTY_FACTOR * 0.5f;
    std::string botClassName = GetBotClassName_Conceptual(bot, nullptr);
    if (botClassName == "Scout" || botClassName == "Soldier") priority += 10.0f;
    if (botClassName == "Demoman") priority += 5.0f; // Good for area denial / spam
    return std::max(1.0f, priority);
}

float CObjectivePlanner::CalculateCTFEscortFlagCarrierPriority_Conceptual(const FlagInfo& enemyFlag, const CBaseEntity* allyCarrier, CFFPlayer* bot, const BotKnowledgeBase* kb) {
    if (!bot || !kb || !allyCarrier) return 0.0f;
    float priority = 70.0f;
    // priority -= (bot->GetOrigin() - allyCarrier->GetPosition()).Length() * DISTANCE_PENALTY_FACTOR;
    std::string botClassName = GetBotClassName_Conceptual(bot, nullptr);
    if (botClassName == "Heavy" || botClassName == "Medic") priority += 15.0f;
    if (botClassName == "Demoman") priority += 5.0f; // Can lay traps for pursuers
    return std::max(1.0f, priority);
}

// Ensure stubs for methods that were previously fully defined are restored if they were reduced by previous overwrite.
// For this task, we assume the methods not explicitly mentioned for modification are as they were after Task 18.
// Specifically, DecomposeTask and the CTF task generation methods.
// The ... same as Task 18 ... comments in the prompt imply this.
// The GenerateTasks_ControlPoint_FF and GenerateTasks_CaptureTheFlag_FF now use these refined priority calculators.
// The DecomposeTask method's switch statement for CTF tasks needs to call the specific AddSubTasks_CTF_Action methods.
// (As implemented conceptually in Task 18, Step 3, and now confirmed in Step 2 of this task's plan)

// Dummy main DecomposeTask from previous step, to be potentially merged/overwritten by prompt's version.
// bool CObjectivePlanner::DecomposeTask(HighLevelTask& task) { /* ... from Task 18 ... */ return false; }
// Dummy AddDefault helpers from previous step
// void CObjectivePlanner::AddDefaultSubTasksForCapture(HighLevelTask& hlt) { /* ... */ }
// void CObjectivePlanner::AddDefaultSubTasksForDefense(HighLevelTask& hlt) { /* ... */ }
// Dummy AddSubTasks_CTF from previous step
// void CObjectivePlanner::AddSubTasks_CaptureEnemyFlag(HighLevelTask& hlt) { /* ... */ }
// void CObjectivePlanner::AddSubTasks_ReturnOurFlag(HighLevelTask& hlt) { /* ... */ }
// void CObjectivePlanner::AddSubTasks_KillEnemyFlagCarrier(HighLevelTask& hlt) { /* ... */ }
// void CObjectivePlanner::AddSubTasks_EscortFlagCarrier(HighLevelTask& hlt) { /* ... */ }
// void CObjectivePlanner::AddSubTasks_DefendFlagStand(HighLevelTask& hlt) { /* ... */ }
// void CObjectivePlanner::AddSubTasks_CampEnemyFlagSpawn(HighLevelTask& hlt) { /* ... */ }
// void CObjectivePlanner::AddSubTasks_InterceptEnemyFlagRunner(HighLevelTask& hlt) { /* ... */ }

// The full implementations of these methods from Task 18 should be used.
// The prompt for this current task (Task 19) is to *modify* these methods, not necessarily rewrite them from scratch
// if they already exist. The provided cpp content in the prompt for DecomposeTask is a more complete version.
// My previous step's ObjectivePlanner.cpp (Task 18, step 3) had a full DecomposeTask. I will use that as base.

// Re-inserting full methods from Task 18, assuming they are the base for this modification pass.
// EvaluateAndSelectTask, GetCurrentSubTask(Mutable), OnBotKilled, Logging helpers,
// GenerateAvailableTasks, PrioritizeTasks, SelectTaskFromList,
// GenerateTasks_ControlPoint_FF, GenerateTasks_CaptureTheFlag_FF (from Task 18),
// and DecomposeTask (from Task 18) are assumed to be here.
// The Calculate...Priority methods are the new additions/modifications.
// The CTF decomposition helpers are also new additions.

// (The full content of ObjectivePlanner.cpp from Task 18, Step 3 would be here,
//  then the Calculate...Priority methods would be inserted/modified as per current task plan)
// For brevity, I will assume the content from Task 18 Step 3 for unchanged methods is present.
// The key is that the *priority calculation* methods are now updated with Demoman logic.
// And `DecomposeTask` should now correctly call the more specific CTF decomposition helpers.

// The prompt for Step 2 of this task provides the new DecomposeTask:
bool CObjectivePlanner::DecomposeTask(HighLevelTask& task) {
    task.subTasks.clear();
    task.currentSubTaskIndex = -1;
    bool success = true;

    // Conceptual: Fetch flag info once if many CTF tasks use it and if helpers need it.
    // FlagInfo ourFlag_dt, enemyFlag_dt;
    // bool ctfMode = (GetCurrentGameMode() == GameModeType_KB::CAPTURE_THE_FLAG);
    // if (ctfMode && m_pKnowledgeBase) {
    //     ourFlag_dt = m_pKnowledgeBase->GetOurFlagInfo_conceptual();
    //     enemyFlag_dt = m_pKnowledgeBase->GetEnemyFlagInfo_conceptual();
    // }

    switch (task.type) {
        case HighLevelTaskType::CAPTURE_POINT_FF:
            AddDefaultSubTasksForCapture(task);
            break;
        case HighLevelTaskType::DEFEND_POINT_FF:
            AddDefaultSubTasksForDefense(task);
            break;
        case HighLevelTaskType::CAPTURE_ENEMY_FLAG_FF:
            AddSubTasks_CaptureEnemyFlag(task /*, enemyFlag_dt */); // Pass flag info if helpers need it
            break;
        case HighLevelTaskType::RETURN_OUR_FLAG_FF:
            AddSubTasks_ReturnOurFlag(task /*, ourFlag_dt */);
            break;
        case HighLevelTaskType::KILL_ENEMY_FLAG_CARRIER_FF:
            AddSubTasks_KillEnemyFlagCarrier(task /*, ourFlag_dt */);
            break;
        case HighLevelTaskType::ESCORT_FLAG_CARRIER_FF:
            AddSubTasks_EscortFlagCarrier(task /*, enemyFlag_dt */);
            break;
        case HighLevelTaskType::DEFEND_FLAG_STAND_FF:
            AddSubTasks_DefendFlagStand(task /*, ourFlag_dt */);
            break;
        case HighLevelTaskType::CAMP_ENEMY_FLAG_SPAWN_FF:
            AddSubTasks_CampEnemyFlagSpawn(task);
            break;
        case HighLevelTaskType::INTERCEPT_ENEMY_FLAG_RUNNER_FF:
            AddSubTasks_InterceptEnemyFlagRunner(task);
            break;
        case HighLevelTaskType::ATTACK_ENEMY:
            if (task.pTargetEntity) {
                task.AddSubTask(SubTask(SubTaskType::MOVE_TO_ENTITY, task.pTargetEntity));
                task.AddSubTask(SubTask(SubTaskType::ATTACK_TARGET, task.pTargetEntity));
            } else if (task.targetPosition.LengthSquared() > 0.1f) { // Attack general area
                 task.AddSubTask(SubTask(SubTaskType::MOVE_TO_POSITION, task.targetPosition));
                 task.AddSubTask(SubTask(SubTaskType::SECURE_AREA, task.targetPosition, nullptr, 10.f, 300.f));
            } else {
                std::cerr << "Planner Warning: ATTACK_ENEMY task created with no target entity or position." << std::endl;
                success = false;
            }
            break;
        // SEEK_HEALTH, SEEK_AMMO, FALLBACK_REGROUP would also need decomposition
        default:
            std::cerr << "Planner Error: DecomposeTask - Unknown HighLevelTaskType: " << static_cast<int>(task.type) << std::endl;
            success = false;
            break;
    }

    if (success && !task.subTasks.empty()) {
        task.StartFirstSubTask();
    } else if (success && task.subTasks.empty()) {
        // Task validly decomposed into no actions (e.g. already at defend spot and no secure needed)
        // This HLT can be considered complete or switch to a continuous monitoring task.
        // For now, this will make the HLT finish immediately.
        task.currentSubTaskIndex = -1; // No subtasks to execute
        std::cout << "Planner Warning: Task " << task.description << " decomposed into zero subtasks." << std::endl;
    } else { // success == false
        task.currentSubTaskIndex = -1;
    }
    return success && !task.subTasks.empty();
}

// Ensure AddDefaultSubTasksForCapture/Defense are using the HLT's members
void CObjectivePlanner::AddDefaultSubTasksForCapture(HighLevelTask& hlt) {
    hlt.AddSubTask(SubTask(SubTaskType::MOVE_TO_POSITION, hlt.targetPosition));
    hlt.AddSubTask(SubTask(SubTaskType::SECURE_AREA, hlt.targetPosition, hlt.pTargetEntity, SECURE_DURATION_CAPTURE_FF, SECURE_RADIUS_CAPTURE_FF));
    hlt.AddSubTask(SubTask(SubTaskType::STAND_ON_POINT, hlt.targetPosition, hlt.pTargetEntity, STAND_DURATION_CAPTURE_FF, CP_CAPTURE_RADIUS_FF));
}

void CObjectivePlanner::AddDefaultSubTasksForDefense(HighLevelTask& hlt) {
    hlt.AddSubTask(SubTask(SubTaskType::MOVE_TO_POSITION, hlt.targetPosition));
    hlt.AddSubTask(SubTask(SubTaskType::SECURE_AREA, hlt.targetPosition, hlt.pTargetEntity, SECURE_DURATION_DEFEND_FF, SECURE_RADIUS_DEFEND_FF));
    hlt.AddSubTask(SubTask(SubTaskType::HOLD_POSITION, hlt.targetPosition, hlt.pTargetEntity, HOLD_DURATION_DEFEND_FF, CP_DEFEND_HOLD_RADIUS_FF));
}
