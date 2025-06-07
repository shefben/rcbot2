#include "FFControlPointTasks.h"
#include "FFBaseAI.h"       // For CFFBaseAI, and its members like CFFPlayer*, BotKnowledgeBase*
#include "FFStateStructs.h" // For ControlPointInfo, Vector
#include "BotDefines.h"     // For constants like CP_CAPTURE_RADIUS_FF, etc.
#include "BotTasks.h"       // For SubTask, though not directly used by these task's execute, CFFBaseAI uses it
// #include "bot_schedule.h" // Assumed for pSchedule parameter if used by fail/complete (RCBot2 context)

#include <string>
#include <iostream>         // For placeholder debug prints

// --- Conceptual Time & Bot Access ---
// This would come from CFFBaseAI or a global engine interface.
// Using placeholders for now.
// CFFPlayer, CBaseEntity, UserCmd are conceptually defined/included via FFBaseAI.h's chain for this context.

static float s_fConceptualTaskExecutionTime = 0.0f; // Separate timer for task execution simulation
void AdvanceTaskExecutionTime() { s_fConceptualTaskExecutionTime += 0.1f; } // Approx 10hz task thinking
float GetTaskConceptualWorldTime() { return s_fConceptualTaskExecutionTime; }
// --- End Conceptual ---


// --- CMoveToPositionTask Implementation ---
// This task is now primarily a data holder. CFFBaseAI::ExecuteSubTask with SubTaskType::MOVE_TO_POSITION
// will call CFFBaseAI::MoveTo() and CFFBaseAI::FollowPath().
// This task's execute() is called by CBotSchedule (RCBot2 style). If we are fully in the new system,
// this task class might not even be "executed" in this way if CFFBaseAI::ExecuteSubTask handles it all.
// For this exercise, we'll assume it IS called, and its job is to check if CFFBaseAI has completed its movement.
CMoveToPositionTask::CMoveToPositionTask(const Vector& targetPos, float arrivalTolerance)
    : CBotTask(), m_vTargetPosition(targetPos), m_fArrivalTolerance(arrivalTolerance), m_fStuckTimer(0.0f) {
    // setID(TASK_MOVE_TO_POSITION_FF); // Conceptual RCBot2 Task ID
}

void CMoveToPositionTask::init(CFFBaseAI* botAI) { // Changed param to CFFBaseAI*
    CBotTask::init(botAI); // Call base init if it takes CBot* or void*
    setState(TASK_STATE_RUNNING);
    m_fStuckTimer = 0.0f;
    if (botAI) {
        // Path planning is initiated by CFFBaseAI::ExecuteSubTask when it first sees this subtask type
        // So, this task's init doesn't need to call botAI->MoveTo() directly.
        // It just ensures botAI knows the target (which it does via the SubTask passed to ExecuteSubTask).
        // std::cout << botAI->GetBotPlayer()->GetNamePlaceholder() << ": Init MoveToTask to (" << m_vTargetPosition.x << ", " << m_vTargetPosition.y << ")" << std::endl;
    }
}

void CMoveToPositionTask::execute(CFFBaseAI* botAI, CBotSchedule* pSchedule) {
    if (!botAI || !botAI->GetBotPlayer()) { fail(pSchedule, "MoveTo: BotAI or BotPlayer is null"); return; }
    // CFFPlayer* ffPlayer = botAI->GetBotPlayer(); // Already available via botAI->m_pBotPlayer

    // CFFBaseAI::ExecuteSubTask is now responsible for calling botAI->FollowPath().
    // This task's execute() simply checks if the AI considers the movement complete.
    // The CFFBaseAI::ExecuteSubTask for MOVE_TO_POSITION will have called FollowPath.
    // If FollowPath returned false (path ended or failed), ExecuteSubTask should have marked the SubTask as completed or failed.
    // This task instance (CMoveToPositionTask) needs to reflect that.

    // This logic is now somewhat redundant if CFFBaseAI's ExecuteSubTask directly manages completion.
    // However, if this task is directly on an RCBot2 schedule stack, it needs to complete itself.
    float distSq = (botAI->GetBotPlayer()->GetOrigin().x - m_vTargetPosition.x)*(botAI->GetBotPlayer()->GetOrigin().x - m_vTargetPosition.x) +
                   (botAI->GetBotPlayer()->GetOrigin().y - m_vTargetPosition.y)*(botAI->GetBotPlayer()->GetOrigin().y - m_vTargetPosition.y);

    if (distSq < (m_fArrivalTolerance * m_fArrivalTolerance)) {
        // std::cout << botAI->GetBotPlayer()->GetNamePlaceholder() << ": Reached target for MoveToPositionTask." << std::endl;
        complete();
        return;
    }

    // If CFFBaseAI::FollowPath indicated failure (e.g., path exhausted but not at target, or stuck),
    // this task should also fail. This requires CFFBaseAI to expose that state.
    // if (botAI->HasPathfindingFailed_Conceptual()) {
    //     fail(pSchedule, "MoveToPositionTask: Underlying pathing failed");
    //     return;
    // }
    // If this task is still running, it means CFFBaseAI::ExecuteSubTask's call to FollowPath is still returning true.
}

void CMoveToPositionTask::reset(CFFBaseAI* botAI) { // Changed param
    CBotTask::reset(botAI);
    m_fStuckTimer = 0.0f;
    if (botAI) botAI->ClearCurrentPath();
}

std::string CMoveToPositionTask::debugString(CFFBaseAI* botAI) const { // Changed param
    return "CMoveToPositionTask to (" + std::to_string(m_vTargetPosition.x) + "," + std::to_string(m_vTargetPosition.y) + ")";
}


// --- CSecureAreaTask_FF Implementation ---
CSecureAreaTask_FF::CSecureAreaTask_FF(const Vector& vAreaCenter, float fRadius, float fDuration)
    : CBotTask(), m_vAreaCenter(vAreaCenter), m_fRadius(fRadius), m_fDuration(fDuration),
      m_fTaskStartTime(0.0f), m_bFoundEnemyDuringTask(false) {}

void CSecureAreaTask_FF::init(CFFBaseAI* botAI) {
    CBotTask::init(botAI);
    setState(TASK_STATE_RUNNING);
    m_fTaskStartTime = botAI ? botAI->GetCurrentWorldTime_conceptual() : 0.0f;
    m_bFoundEnemyDuringTask = false;
    // if(botAI) std::cout << botAI->GetBotPlayer()->GetNamePlaceholder() << ": Init SecureAreaTask." << std::endl;
}

void CSecureAreaTask_FF::execute(CFFBaseAI* botAI, CBotSchedule* pSchedule) {
    if (!botAI || !botAI->GetBotPlayer()) { fail(pSchedule, "SecureArea: BotAI or BotPlayer is null"); return; }
    // CFFPlayer* ffPlayer = botAI->GetBotPlayer();
    // BotKnowledgeBase* kb = botAI->GetKnowledgeBase();
    // CUserCmd* pCmd = botAI->GetUserCmd_conceptual(); // AI needs to provide UserCmd for AttackTarget

    if (botAI->GetCurrentWorldTime_conceptual() - m_fTaskStartTime > m_fDuration) {
        // std::cout << botAI->GetBotPlayer()->GetNamePlaceholder() << ": SecureArea duration ended." << std::endl;
        complete(); return;
    }

    // The actual enemy selection and attack logic is now primarily driven by CFFBaseAI::ExecuteSubTask
    // when it handles a SubTaskType::SECURE_AREA. That method would call botAI->SelectTarget() and botAI->AttackTarget().
    // This task's execute() is more about checking conditions for completion or specific state changes.
    // If CFFBaseAI::ExecuteSubTask for SECURE_AREA decides there are no more enemies OR an enemy was dealt with,
    // it should mark the SubTask as completed. This CBotTask derivative then needs to reflect that.

    // For this exercise, let's assume this task is mostly a data holder for CFFBaseAI's ExecuteSubTask logic.
    // If this task is still running, it means CFFBaseAI's ExecuteSubTask hasn't marked the SubTask as complete.
    // Example of how CFFBaseAI::ExecuteSubTask might interact with this task's state:
    // if (pSubTask->type == SubTaskType::SECURE_AREA) {
    //    bool enemyFoundAndEngaged = /* ... AI logic ... */;
    //    if (GetCurrentWorldTime() - pSubTask->startTimeParam > pSubTask->desiredDuration) {
    //        ((SubTask*)pSubTask)->isCompleted = true; return false; // Task done
    //    }
    //    if (!enemyFoundAndEngaged && GetCurrentWorldTime() - pSubTask->startTimeParam > SHORT_SECURE_TIMEOUT_IF_CLEAR) {
    //         ((SubTask*)pSubTask)->isCompleted = true; return false; // Area clear, finish early
    //    }
    //    return true; // Ongoing
    // }
}

void CSecureAreaTask_FF::reset(CFFBaseAI* botAI) { CBotTask::reset(botAI); m_fTaskStartTime = 0.0f; m_bFoundEnemyDuringTask = false; }
std::string CSecureAreaTask_FF::debugString(CFFBaseAI* botAI) const { return "CSecureAreaTask_FF"; }


// --- CStandOnPointTask_FF Implementation ---
CStandOnPointTask_FF::CStandOnPointTask_FF(CBaseEntity* pCPEntity, const Vector& vCPPosition, float fMaxTime, float captureRadius)
    : CBotTask(), m_pTargetCPEntity_conceptual_id(0), m_vCPPosition(vCPPosition), m_fMaxTime(fMaxTime),
      m_fCaptureRadiusSqr(captureRadius*captureRadius), m_fTaskStartTime(0.0f) {
    // if (pCPEntity) m_pTargetCPEntity_conceptual_id = pCPEntity->GetId_conceptual();
}

void CStandOnPointTask_FF::init(CFFBaseAI* botAI) {
    CBotTask::init(botAI);
    setState(TASK_STATE_RUNNING);
    m_fTaskStartTime = botAI ? botAI->GetCurrentWorldTime_conceptual() : 0.0f;
    if (botAI) botAI->StopMoving_conceptual(); // Request AI to stop its path following.
}

void CStandOnPointTask_FF::execute(CFFBaseAI* botAI, CBotSchedule* pSchedule) {
    if (!botAI || !botAI->GetBotPlayer() || !botAI->GetKnowledgeBase()) { fail(pSchedule, "StandOnPoint: Invalid bot state"); return; }
    CFFPlayer* ffPlayer = botAI->GetBotPlayer();
    const BotKnowledgeBase* kb = botAI->GetKnowledgeBase();
    // CUserCmd* pCmd = botAI->GetUserCmd_conceptual();

    if ((ffPlayer->GetOrigin().x - m_vCPPosition.x)*(ffPlayer->GetOrigin().x - m_vCPPosition.x) +
        (ffPlayer->GetOrigin().y - m_vCPPosition.y)*(ffPlayer->GetOrigin().y - m_vCPPosition.y)
        > m_fCaptureRadiusSqr * 2.25f) { // Moved too far (1.5x radius)
        fail(pSchedule, "Moved too far off point"); return;
    }

    const ControlPointInfo* cpState = kb->GetControlPoint(m_pTargetCPEntity_conceptual_id); // Use ID
    if (cpState && cpState->ownerTeam == ffPlayer->GetTeamNumber()) {
        complete(); return;
    }

    if (botAI->GetCurrentWorldTime_conceptual() - m_fTaskStartTime > m_fMaxTime) {
        fail(pSchedule, "Timeout standing on point"); return;
    }

    // The CFFBaseAI::ExecuteSubTask for STAND_ON_POINT would handle enemy engagement
    // by calling botAI->SelectTarget() and botAI->AttackTarget().
    // This task's execute just checks conditions. If it's still running, AI handles actions.
    // If no enemy, CFFBaseAI::ExecuteSubTask might make the bot look around.
}

void CStandOnPointTask_FF::reset(CFFBaseAI* botAI) { CBotTask::reset(botAI); m_fTaskStartTime = 0.0f; }
std::string CStandOnPointTask_FF::debugString(CFFBaseAI* botAI) const { return "CStandOnPointTask_FF"; }


// --- CHoldPositionTask_FF Implementation ---
CHoldPositionTask_FF::CHoldPositionTask_FF(const Vector& vPositionToHold, float fDuration, float fHoldRadius)
    : CBotTask(), m_vPositionToHold(vPositionToHold), m_fDuration(fDuration),
      m_fHoldRadiusSqr(fHoldRadius*fHoldRadius), m_fTaskStartTime(0.0f) {}

void CHoldPositionTask_FF::init(CFFBaseAI* botAI) {
    CBotTask::init(botAI);
    setState(TASK_STATE_RUNNING);
    m_fTaskStartTime = botAI ? botAI->GetCurrentWorldTime_conceptual() : 0.0f;
}

void CHoldPositionTask_FF::execute(CFFBaseAI* botAI, CBotSchedule* pSchedule) {
    if (!botAI || !botAI->GetBotPlayer()) { fail(pSchedule, "HoldPosition: Invalid bot state"); return; }
    // CFFPlayer* ffPlayer = botAI->GetBotPlayer();
    // CUserCmd* pCmd = botAI->GetUserCmd_conceptual();

    if (botAI->GetCurrentWorldTime_conceptual() - m_fTaskStartTime > m_fDuration) {
        complete(); return;
    }

    // CFFBaseAI::ExecuteSubTask for HOLD_POSITION would:
    // 1. Check if bot needs to reposition (if ffPlayer->GetOrigin().DistToSqr(m_vPositionToHold) > m_fHoldRadiusSqr)
    //    If so, it would call botAI->MoveTo(m_vPositionToHold, pCmd) and then botAI->FollowPath(pCmd).
    // 2. If in position, it would scan for enemies (botAI->SelectTarget()) and attack (botAI->AttackTarget()).
    // 3. If no enemies, it would make bot look around (botAI->SetLookAtTask_conceptual).
    // This task's execute() is mainly for checking the duration condition.
}

void CHoldPositionTask_FF::reset(CFFBaseAI* botAI) { CBotTask::reset(botAI); m_fTaskStartTime = 0.0f; }
std::string CHoldPositionTask_FF::debugString(CFFBaseAI* botAI) const { return "CHoldPositionTask_FF"; }
