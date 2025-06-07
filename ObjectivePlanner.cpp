#include "ObjectivePlanner.h"
#include "FFBaseAI.h"       // For BotKnowledgeBase, CFFPlayer, CBaseEntity, ClassConfigInfo
#include "FFStateStructs.h" // For ControlPointInfo etc.
#include "NavSystem.h"      // For NavMeshGraph
#include "BotDefines.h"     // For constants
#include "CRCBotPlugin.h"   // For m_pPluginOwner->StoreTaskLog
#include <algorithm>
#include <iostream>
#include <cmath>

// --- Conceptual Placeholder functions (same as before) ---
bool IsCpCritical_Conceptual(const ControlPointInfo& cpInfo, const BotKnowledgeBase* kb) { return cpInfo.id == 3; }
float GetAreaThreat_Conceptual(const BotKnowledgeBase* kb, const Vector& pos) { return 0.0f; }
int CountAlliesNearPoint_Conceptual(const BotKnowledgeBase* kb, const Vector& pos, float radius) { return 1; }
std::string GetBotClassName_Conceptual(CFFPlayer* bot, const ClassConfigInfo* classCfg) {
    if (classCfg) return classCfg->className;
    return "Unknown";
}
bool IsControlPointLockedForTeam_Conceptual(const ControlPointInfo& cpInfo, int teamId, const BotKnowledgeBase* kb) {
    return cpInfo.isLocked;
}
// --- End Conceptual Placeholders ---


// --- HighLevelTask Method Implementations ---
const SubTask* HighLevelTask::GetCurrentSubTask() const { /* ... (same as before) ... */
    if (type != HighLevelTaskType::NONE && currentSubTaskIndex >= 0 && currentSubTaskIndex < (int)subTasks.size()) {
        return &subTasks[currentSubTaskIndex];
    }
    return nullptr;
}
SubTask* HighLevelTask::GetCurrentSubTaskMutable() { /* ... (same as before) ... */
    if (type != HighLevelTaskType::NONE && currentSubTaskIndex >= 0 && currentSubTaskIndex < (int)subTasks.size()) {
        return &subTasks[currentSubTaskIndex];
    }
    return nullptr;
}
bool HighLevelTask::AdvanceToNextSubTask() { /* ... (same as before, ensures startTime is set for new subtask) ... */
    if (type == HighLevelTaskType::NONE || subTasks.empty()) return false;
    currentSubTaskIndex++;
    if (currentSubTaskIndex < (int)subTasks.size()) {
        subTasks[currentSubTaskIndex].isCompleted = false;
        subTasks[currentSubTaskIndex].startTime = std::chrono::system_clock::now(); // Set start time for new subtask
        return true;
    }
    return false;
}
void HighLevelTask::StartFirstSubTask() { // Added in previous BotTasks.h update
    if (!subTasks.empty()) {
        currentSubTaskIndex = 0;
        subTasks[0].isCompleted = false;
        subTasks[0].startTime = std::chrono::system_clock::now();
    } else {
        currentSubTaskIndex = -1;
    }
}
void HighLevelTask::Reset() { /* ... (same as before, added targetNameOrId_Logging) ... */
    type = HighLevelTaskType::NONE; priority = 0.0f; targetPosition = Vector();
    pTargetEntity = nullptr; description = ""; targetNameOrId_Logging = "";
    subTasks.clear(); currentSubTaskIndex = -1;
}


// --- CObjectivePlanner Implementation ---
CObjectivePlanner::CObjectivePlanner(CFFPlayer* pBotOwner, const BotKnowledgeBase* pKnowledgeBase, CRCBotPlugin* pPluginOwner)
    : m_pBotOwner(pBotOwner),
      m_pKnowledgeBase(pKnowledgeBase),
      m_pPluginOwner(pPluginOwner) { // Store plugin owner for logging
    m_CurrentHighLevelTask.Reset();
    m_CurrentTaskLogEntry.Reset();
}

CObjectivePlanner::~CObjectivePlanner() {}

GameModeType_KB CObjectivePlanner::GetCurrentGameMode() const {
    if (m_pKnowledgeBase) {
        // return m_pKnowledgeBase->GetCurrentGameMode();
    }
    return GameModeType_KB::CONTROL_POINT;
}

void CObjectivePlanner::EvaluateAndSelectTask() {
    const SubTask* currentSub = GetCurrentSubTask();
    // If current HLT is valid AND its current subtask is valid AND that subtask is not completed, continue it.
    if (m_CurrentHighLevelTask.IsValid() && currentSub && !currentSub->isCompleted) {
        return;
    }
    // If HLT is valid but GetCurrentSubTask() is null (all subtasks done) or current subtask is completed.
    if (m_CurrentHighLevelTask.IsValid() && (!currentSub || currentSub->isCompleted)) {
        // If the subtask just completed, OnSubTaskOutcomeReported would have handled advancing or finalizing HLT.
        // This path is more for when an HLT has genuinely finished all its subtasks.
        if (!currentSub) { // All subtasks were processed and AdvanceToNextSubTask returned false
             FinalizeAndStoreCurrentHLTLog(TaskOutcomeLog::Outcome::SUCCESS, 100.0f); // Example score
             m_CurrentHighLevelTask.Reset(); // Resets the HLT itself
        }
    }

    if (m_CurrentHighLevelTask.type == HighLevelTaskType::NONE) {
        GenerateAvailableTasks();
        PrioritizeTasks();
        if (SelectTaskFromList()) { // This sets m_CurrentHighLevelTask and starts m_CurrentTaskLogEntry
            if (!DecomposeTask(m_CurrentHighLevelTask) || m_CurrentHighLevelTask.subTasks.empty()) {
                std::cerr << "Planner Error: Failed to decompose or got no subtasks for: " << m_CurrentHighLevelTask.description << std::endl;
                FinalizeAndStoreCurrentHLTLog(TaskOutcomeLog::Outcome::FAILURE, -10.0f); // Log failure
                m_CurrentHighLevelTask.Reset();
            } else {
                m_CurrentHighLevelTask.StartFirstSubTask(); // Sets up first subtask and its startTime
                // StartNewSubTaskLogging(); // Not needed if SubTask itself stores startTime and LogSubTaskOutcome uses it
            }
        }
    }
}

const SubTask* CObjectivePlanner::GetCurrentSubTask() const {
    return m_CurrentHighLevelTask.GetCurrentSubTask();
}

SubTask* CObjectivePlanner::GetCurrentSubTaskMutable() {
    return m_CurrentHighLevelTask.GetCurrentSubTaskMutable();
}

// New method called by CFFBaseAI::Update
void CObjectivePlanner::OnSubTaskOutcomeReported(bool success, const std::string& failureReason) {
    const SubTask* pFinishedSubTask = GetCurrentSubTask();
    if (pFinishedSubTask) { // Ensure there was a subtask to report on
        FinalizeSubTaskLog(success, failureReason); // Log its outcome

        if (success) {
            if (!m_CurrentHighLevelTask.AdvanceToNextSubTask()) { // No more subtasks
                FinalizeAndStoreCurrentHLTLog(TaskOutcomeLog::Outcome::SUCCESS, 100.0f); // Example score
                m_CurrentHighLevelTask.Reset();
            } else {
                // StartNewSubTaskLogging(); // Set startTime for the new current subtask (handled by AdvanceToNextSubTask)
            }
        } else { // Subtask failed
            FinalizeAndStoreCurrentHLTLog(TaskOutcomeLog::Outcome::FAILURE, -50.0f); // HLT fails if a subtask fails
            m_CurrentHighLevelTask.Reset();
        }
    }
}


void CObjectivePlanner::OnBotKilled() {
    if (m_CurrentHighLevelTask.IsValid()) {
        FinalizeAndStoreCurrentHLTLog(TaskOutcomeLog::Outcome::ABORTED, -20.0f); // Score for being aborted
    }
    m_CurrentHighLevelTask.Reset();
}

// --- Logging Helper Methods ---
GameStateSnapshot CObjectivePlanner::CreateGameStateSnapshot() const {
    GameStateSnapshot snapshot;
    if (!m_pKnowledgeBase || !m_pBotOwner) return snapshot;

    // Conceptual: Populate from kb
    // snapshot.teamScore_Ally = m_pKnowledgeBase->GetTeamScore(m_pBotOwner->GetTeam());
    // snapshot.teamScore_Enemy = m_pKnowledgeBase->GetTeamScore(OtherTeam(m_pBotOwner->GetTeam()));
    // snapshot.playersAlive_Ally = m_pKnowledgeBase->GetAlivePlayerCount(m_pBotOwner->GetTeam());
    // snapshot.playersAlive_Enemy = m_pKnowledgeBase->GetAlivePlayerCount(OtherTeam(m_pBotOwner->GetTeam()));
    // snapshot.simpleObjectiveStatus = m_pKnowledgeBase->GetCurrentObjectiveStatusString();
    snapshot.simpleObjectiveStatus = "CP_Middle: Neutral, CP_Red_Last: Red"; // Placeholder
    return snapshot;
}

void CObjectivePlanner::StartNewSubTaskLogging() {
    // This is now handled by HighLevelTask::AdvanceToNextSubTask or StartFirstSubTask
    // which sets the SubTask::startTime.
}

void CObjectivePlanner::FinalizeSubTaskLog(bool success, const std::string& reason) {
    const SubTask* pSubTask = GetCurrentSubTask(); // Get the subtask that just finished
    if (pSubTask && m_CurrentTaskLogEntry.taskType != HighLevelTaskType::NONE) { // Ensure HLT log is active
        float duration = 0.0f;
        if (pSubTask->startTime.time_since_epoch().count() != 0) { // Check if startTime was set
             duration = std::chrono::duration<float>(std::chrono::system_clock::now() - pSubTask->startTime).count();
        }
        m_CurrentTaskLogEntry.executedSubTasks.emplace_back(pSubTask->type, success, duration, reason);
    }
}

void CObjectivePlanner::FinalizeAndStoreCurrentHLTLog(TaskOutcomeLog::Outcome outcome, float score) {
    if (m_CurrentTaskLogEntry.taskType != HighLevelTaskType::NONE) { // If a task was active
        m_CurrentTaskLogEntry.endTime = std::chrono::system_clock::now();
        if (m_CurrentTaskLogEntry.startTime.time_since_epoch().count() != 0) {
            m_CurrentTaskLogEntry.durationSeconds = std::chrono::duration<float>(
                m_CurrentTaskLogEntry.endTime - m_CurrentTaskLogEntry.startTime).count();
        }
        m_CurrentTaskLogEntry.outcome = outcome;
        m_CurrentTaskLogEntry.outcomeScore = score;
        m_CurrentTaskLogEntry.stateAtEnd = CreateGameStateSnapshot();

        if (m_pPluginOwner) {
            m_pPluginOwner->StoreTaskLog(m_CurrentTaskLogEntry);
        } else {
            // std::cout << "Log (no plugin): " << m_CurrentTaskLogEntry.taskDescription << " Outcome: " << (int)outcome << std::endl;
        }
        m_CurrentTaskLogEntry.Reset(); // Clear for the next HLT
    }
}

// --- Task Selection & Decomposition (largely same as before, with logging hooks) ---
bool CObjectivePlanner::SelectTaskFromList() {
    if (m_AvailableTasks.empty()) {
        m_CurrentHighLevelTask.Reset();
        m_CurrentTaskLogEntry.Reset(); // Ensure log is also reset
        return false;
    }
    m_CurrentHighLevelTask = m_AvailableTasks[0];

    // Start logging for this new HLT
    m_CurrentTaskLogEntry.Reset();
    m_CurrentTaskLogEntry.taskType = m_CurrentHighLevelTask.type;
    m_CurrentTaskLogEntry.taskDescription = m_CurrentHighLevelTask.description;
    // Conceptual: if (m_CurrentHighLevelTask.pTargetEntity) m_CurrentTaskLogEntry.targetNameOrId = m_CurrentHighLevelTask.pTargetEntity->GetName();
    // else m_CurrentTaskLogEntry.targetNameOrId = "Position";
    m_CurrentTaskLogEntry.targetNameOrId = m_CurrentHighLevelTask.targetNameOrId_Logging; // Use new field
    m_CurrentTaskLogEntry.targetPosition = m_CurrentHighLevelTask.targetPosition;
    m_CurrentTaskLogEntry.startTime = std::chrono::system_clock::now();
    if (m_pBotOwner) {
        // m_CurrentTaskLogEntry.botId = m_pBotOwner->GetId_conceptual();
        // m_CurrentTaskLogEntry.botName = m_pBotOwner->GetName_conceptual();
        // if (m_pBotOwner->GetClassConfig()) m_CurrentTaskLogEntry.botClassUsed = m_pBotOwner->GetClassConfig()->className;
    }
    m_CurrentTaskLogEntry.stateAtStart = CreateGameStateSnapshot();
    // std::cout << "Planner: Selected HLT: " << m_CurrentHighLevelTask.description << ". Logging started." << std::endl;
    return true;
}


void CObjectivePlanner::GenerateAvailableTasks() { /* ... same as Task 11 ... */
    m_AvailableTasks.clear();
    if (!m_pKnowledgeBase || !m_pBotOwner) {
        std::cerr << "Planner: KnowledgeBase or BotOwner not set, cannot generate tasks." << std::endl;
        return;
    }
    GameModeType_KB currentMode = GetCurrentGameMode(); // Now GameModeType_KB
    switch (currentMode) {
        case GameModeType_KB::CONTROL_POINT:
            GenerateTasks_ControlPoint_FF();
            break;
        default:
            break;
    }
}

void CObjectivePlanner::GenerateTasks_ControlPoint_FF() { /* ... same as Task 11, ensure pTargetEntity and targetNameOrId_Logging are set ... */
    if (!m_pKnowledgeBase || !m_pBotOwner) return;
    const std::vector<ControlPointInfo>& controlPoints = m_pKnowledgeBase->GetControlPoints();
    if (controlPoints.empty()) {
        HighLevelTask cTask(HighLevelTaskType::CAPTURE_POINT_FF);
        cTask.targetPosition = Vector(100,100,0);
        cTask.description = "Capture Alpha (Dummy - No CP Data)";
        cTask.targetNameOrId_Logging = "DummyAlpha";
        cTask.priority = CalculateCapturePriority(ControlPointInfo{0, "DummyAlpha", "Dummy Alpha", Vector(100,100,0), TEAM_ID_NEUTRAL, 0.f, false}, m_pBotOwner, m_pKnowledgeBase);
        m_AvailableTasks.push_back(cTask);
        return;
    }
    int myTeam = m_pBotOwner->GetTeamNumber();
    for (const auto& cpInfo : controlPoints) {
        bool isCpLockedForMyTeam = IsControlPointLockedForTeam_Conceptual(cpInfo, myTeam, m_pKnowledgeBase);
        if (cpInfo.ownerTeam != myTeam && !isCpLockedForMyTeam) {
            HighLevelTask capTask(HighLevelTaskType::CAPTURE_POINT_FF);
            // capTask.pTargetEntity = cpInfo.pEntity_conceptual;
            capTask.targetPosition = cpInfo.position;
            capTask.description = "Capture " + (cpInfo.luaName.empty() ? cpInfo.gameDisplayName : cpInfo.luaName);
            capTask.targetNameOrId_Logging = cpInfo.luaName.empty() ? cpInfo.gameDisplayName : cpInfo.luaName;
            capTask.priority = CalculateCapturePriority(cpInfo, m_pBotOwner, m_pKnowledgeBase);
            m_AvailableTasks.push_back(capTask);
        } else if (cpInfo.ownerTeam == myTeam) {
            HighLevelTask defTask(HighLevelTaskType::DEFEND_POINT_FF);
            // defTask.pTargetEntity = cpInfo.pEntity_conceptual;
            defTask.targetPosition = cpInfo.position;
            defTask.description = "Defend " + (cpInfo.luaName.empty() ? cpInfo.gameDisplayName : cpInfo.luaName);
            defTask.targetNameOrId_Logging = cpInfo.luaName.empty() ? cpInfo.gameDisplayName : cpInfo.luaName;
            defTask.priority = CalculateDefensePriority(cpInfo, m_pBotOwner, m_pKnowledgeBase);
            m_AvailableTasks.push_back(defTask);
        }
    }
}
float CObjectivePlanner::CalculateCapturePriority(const ControlPointInfo& cpInfo, CFFPlayer* bot, const BotKnowledgeBase* kb) { /* ... same as Task 11 ... */
    if (!bot || !kb || !kb->GetNavGraph()) return 0.0f;
    float priority = BASE_CAPTURE_PRIORITY;
    if (IsCpCritical_Conceptual(cpInfo, kb)) priority += CRITICAL_POINT_BONUS;
    Vector botOrigin = bot->GetOrigin();
    float distSq = (botOrigin.x - cpInfo.position.x)*(botOrigin.x - cpInfo.position.x) + (botOrigin.y - cpInfo.position.y)*(botOrigin.y - cpInfo.position.y);
    priority -= sqrt(distSq) * DISTANCE_PENALTY_FACTOR;
    return std::max(1.0f, priority);
}
float CObjectivePlanner::CalculateDefensePriority(const ControlPointInfo& cpInfo, CFFPlayer* bot, const BotKnowledgeBase* kb) { /* ... same as Task 11 ... */
    if (!bot || !kb) return 0.0f;
    float priority = BASE_DEFENSE_PRIORITY;
    Vector botOrigin = bot->GetOrigin();
    float distSq = (botOrigin.x - cpInfo.position.x)*(botOrigin.x - cpInfo.position.x) + (botOrigin.y - cpInfo.position.y)*(botOrigin.y - cpInfo.position.y);
    priority -= sqrt(distSq) * DISTANCE_PENALTY_FACTOR;
    return std::max(1.0f, priority);
}
void CObjectivePlanner::PrioritizeTasks() { /* ... same as Task 11 ... */
    if (m_AvailableTasks.empty()) return;
    std::sort(m_AvailableTasks.begin(), m_AvailableTasks.end(),
              [](const HighLevelTask& a, const HighLevelTask& b) { return a.priority > b.priority; });
}

bool CObjectivePlanner::DecomposeTask(HighLevelTask& task) { /* ... same as Task 11, ensure StartFirstSubTask is called after this ... */
    if (task.type == HighLevelTaskType::NONE) return false;
    task.subTasks.clear();
    task.currentSubTaskIndex = -1; // Will be set by StartFirstSubTask
    switch (task.type) {
        case HighLevelTaskType::CAPTURE_POINT_FF:
            task.AddSubTask(SubTask(SubTaskType::MOVE_TO_POSITION, task.targetPosition));
            task.AddSubTask(SubTask(SubTaskType::SECURE_AREA, task.targetPosition, task.pTargetEntity, SECURE_DURATION_CAPTURE_FF, SECURE_RADIUS_CAPTURE_FF));
            task.AddSubTask(SubTask(SubTaskType::STAND_ON_POINT, task.targetPosition, task.pTargetEntity, STAND_DURATION_CAPTURE_FF, CP_CAPTURE_RADIUS_FF));
            break;
        case HighLevelTaskType::DEFEND_POINT_FF:
            task.AddSubTask(SubTask(SubTaskType::MOVE_TO_POSITION, task.targetPosition));
            task.AddSubTask(SubTask(SubTaskType::SECURE_AREA, task.targetPosition, task.pTargetEntity, SECURE_DURATION_DEFEND_FF, SECURE_RADIUS_DEFEND_FF));
            task.AddSubTask(SubTask(SubTaskType::HOLD_POSITION, task.targetPosition, task.pTargetEntity, HOLD_DURATION_DEFEND_FF, CP_DEFEND_HOLD_RADIUS_FF));
            break;
        default:
            return false;
    }
    if (!task.subTasks.empty()) {
        task.StartFirstSubTask(); // This sets currentSubTaskIndex = 0 and startTime for the first subtask.
        return true;
    }
    return false;
}

// Added ProcessSubTaskCompletion to consolidate logic after logging
void CObjectivePlanner::ProcessSubTaskCompletion(bool success) {
    if (m_CurrentHighLevelTask.IsValid()) {
        if (success) {
            if (!m_CurrentHighLevelTask.AdvanceToNextSubTask()) { // No more subtasks
                FinalizeAndStoreCurrentHLTLog(TaskOutcomeLog::Outcome::SUCCESS, 100.0f); // Example score
                m_CurrentHighLevelTask.Reset();
            }
            // else: new subtask is now current and its startTime is set by AdvanceToNextSubTask
        } else { // Subtask failed
            FinalizeAndStoreCurrentHLTLog(TaskOutcomeLog::Outcome::FAILURE, -50.0f); // HLT fails if a subtask fails
            m_CurrentHighLevelTask.Reset();
        }
    }
}
