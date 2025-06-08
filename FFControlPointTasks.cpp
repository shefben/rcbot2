#include "FFControlPointTasks.h"
#include "FFBaseAI.h"       // For CFFBaseAI (now SDK-aware)
#include "CFFPlayer.h"      // For CFFPlayerWrapper (header name might still be CFFPlayer.h)
#include "BotKnowledgeBase.h"
#include "BotDefines.h"     // For constants like DEFAULT_ARRIVAL_TOLERANCE_SQR
// #include "GameDefines_Placeholder.h" // No longer needed, SDK types are used
#include "game/shared/usercmd.h" // For CUserCmd (if tasks were to ever directly use it, though AI does now)

#include <string>
#include <iostream>         // For placeholder debug prints


// --- CMoveToPositionTask Implementation ---
CMoveToPositionTask::CMoveToPositionTask(const Vector& targetPos, float arrivalTolerance)
    : CBotTask(),
      m_vTargetPosition(targetPos),
      m_fArrivalToleranceSqr(arrivalTolerance * arrivalTolerance),
      m_fStuckTimer(0.0f) {
}

void CMoveToPositionTask::init(CFFBaseAI* botAI) {
    CBotTask::init(botAI);
    setState(TASK_STATE_RUNNING);
    m_fStuckTimer = 0.0f;
    // Path planning is now implicitly handled by CFFBaseAI::ExecuteSubTask when it first
    // processes a MOVE_TO_POSITION subtask with this task's target.
}

void CMoveToPositionTask::execute(CFFBaseAI* botAI, CBotSchedule* pSchedule) {
    if (!botAI || !botAI->GetBotPlayer()) { fail(pSchedule, "MoveToPositionTask: BotAI or BotPlayerWrapper is null"); return; }
    CFFPlayerWrapper* ffPlayer = botAI->GetBotPlayer(); // Now CFFPlayerWrapper

    if (ffPlayer->GetOrigin().DistToSqr2D(m_vTargetPosition) < m_fArrivalToleranceSqr) { // Using DistToSqr2D for nav
        botAI->ClearCurrentPath();
        // No direct StopMoving call; AI's FollowPath will simply not set movement if path is clear.
        complete();
        return;
    }

    // CFFBaseAI::ExecuteSubTask will call FollowPath. If FollowPath returns false (path ended/failed)
    // and we are not at the target, this task should fail or re-evaluate.
    // The logic for "IsCurrentlyWithPath" and "IsAtMoveToTarget" should be part of CFFBaseAI.
    // if (!botAI->IsCurrentlyWithPath() && !botAI->IsAtMoveToTarget(m_vTargetPosition, m_fArrivalToleranceSqr)) {
    //     if (!botAI->MoveTo(m_vTargetPosition, nullptr)) { // MoveTo now only plans
    //        fail(pSchedule, "Failed to re-plan path in MoveToPositionTask");
    //        return;
    //    }
    // }

    // Stuck logic would be more complex, relying on CFFBaseAI's movement state.
    // For now, this task assumes CFFBaseAI handles movement and pathing.
    // If CFFBaseAI's ExecuteSubTask determines it cannot proceed with this subtask's goal,
    // it should mark the subtask as not completed, and this task might fail due to timeout or planner logic.
}

void CMoveToPositionTask::reset(CFFBaseAI* botAI) {
    CBotTask::reset(botAI);
    m_fStuckTimer = 0.0f;
    if (botAI) botAI->ClearCurrentPath();
}

std::string CMoveToPositionTask::debugString(CFFBaseAI* botAI) const {
    return "CMoveToPositionTask to (" + std::to_string(m_vTargetPosition.x) + "," + std::to_string(m_vTargetPosition.y) + "," + std::to_string(m_vTargetPosition.z) + ")";
}


// --- CSecureAreaTask_FF Implementation ---
CSecureAreaTask_FF::CSecureAreaTask_FF(const Vector& vAreaCenter, float fRadius, float fDuration)
    : CBotTask(), m_vAreaCenter(vAreaCenter), m_fRadius(fRadius), m_fDuration(fDuration),
      m_fTaskStartTime(0.0f), m_bFoundEnemyDuringTask(false) {}

void CSecureAreaTask_FF::init(CFFBaseAI* botAI) {
    CBotTask::init(botAI);
    setState(TASK_STATE_RUNNING);
    m_fTaskStartTime = botAI ? botAI->GetWorldTime() : 0.0f; // Use SDK-aware time
    m_bFoundEnemyDuringTask = false;
    // No direct StopMoving call to botAI here. AI manages its movement based on current subtask.
}

void CSecureAreaTask_FF::execute(CFFBaseAI* botAI, CBotSchedule* pSchedule) {
    if (!botAI || !botAI->GetBotPlayer()) { fail(pSchedule, "SecureArea: Invalid bot state"); return; }

    if (botAI->GetWorldTime() - m_fTaskStartTime > m_fDuration) {
        complete(); return;
    }

    // CBaseEntity* pEnemy = botAI->SelectTarget(); // General target selection by AI module
    // if (pEnemy && pEnemy->IsAlive() && pEnemy->GetAbsOrigin().DistToSqr(m_vAreaCenter) < (m_fRadius * m_fRadius) ) {
    //     m_bFoundEnemyDuringTask = true;
    //     // AttackTarget is called by CFFBaseAI::ExecuteSubTask if subtask is ATTACK_TARGET.
    //     // This task should ensure the AI's current subtask is to attack this enemy if appropriate.
    //     // For now, assume the AI's main update loop handles attacking if SelectTarget returns an enemy.
    // } else {
    //     if (m_bFoundEnemyDuringTask) {
    //         complete(); return;
    //     }
    //     // AI should look around or patrol based on its ExecuteSubTask for SECURE_AREA.
    //     // This task doesn't directly set look tasks or stop movement.
    // }
}
void CSecureAreaTask_FF::reset(CFFBaseAI* botAI) { CBotTask::reset(botAI); m_fTaskStartTime = 0.0f; m_bFoundEnemyDuringTask = false; }
std::string CSecureAreaTask_FF::debugString(CFFBaseAI* botAI) const { return "CSecureAreaTask_FF"; }


// --- CStandOnPointTask_FF Implementation ---
CStandOnPointTask_FF::CStandOnPointTask_FF(CBaseEntity* pCPEntity, const Vector& vCPPosition, float fMaxTime, float captureRadius) // pCPEntity is CBaseEntity*
    : CBotTask(), m_pTargetCPEntity_SDK(pCPEntity), m_vCPPosition(vCPPosition), m_fMaxTime(fMaxTime),
      m_fCaptureRadiusSqr(captureRadius*captureRadius), m_fTaskStartTime(0.0f) {
    // if (pCPEntity) m_iTargetCPEntityId_conceptual = pCPEntity->entindex(); // Example SDK way to get an ID
}

void CStandOnPointTask_FF::init(CFFBaseAI* botAI) {
    CBotTask::init(botAI);
    setState(TASK_STATE_RUNNING);
    m_fTaskStartTime = botAI ? botAI->GetWorldTime() : 0.0f;
}

void CStandOnPointTask_FF::execute(CFFBaseAI* botAI, CBotSchedule* pSchedule) {
    if (!botAI || !botAI->GetBotPlayer() || !botAI->GetKnowledgeBase()) { fail(pSchedule, "StandOnPoint: Invalid bot state"); return; }
    CFFPlayerWrapper* ffPlayer = botAI->GetBotPlayer();
    BotKnowledgeBase* kb = botAI->GetKnowledgeBase(); // Non-const

    if (ffPlayer->GetOrigin().DistToSqr2D(m_vCPPosition) > m_fCaptureRadiusSqr * 2.25f) {
        fail(pSchedule, "Moved too far off CP"); return;
    }

    // int cpIndex = -1; // Get CP index from m_pTargetCPEntity_SDK or m_vCPPosition via KB
    // const ControlPointInfo* cpState = kb->GetControlPoint(cpIndex);
    // if (cpState && cpState->ownerTeam == ffPlayer->GetTeam()) {
    //     complete(); return;
    // }

    if (botAI->GetWorldTime() - m_fTaskStartTime > m_fMaxTime) {
        fail(pSchedule, "Timeout standing on CP"); return;
    }
    // Logic for attacking enemies or looking around is handled by CFFBaseAI::ExecuteSubTask
    // when subtask is STAND_ON_POINT.
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
    m_fTaskStartTime = botAI ? botAI->GetWorldTime() : 0.0f;
}

void CHoldPositionTask_FF::execute(CFFBaseAI* botAI, CBotSchedule* pSchedule) {
    if (!botAI || !botAI->GetBotPlayer()) { fail(pSchedule, "HoldPosition: Invalid bot state"); return; }

    if (botAI->GetWorldTime() - m_fTaskStartTime > m_fDuration) {
        complete(); return;
    }

    // CFFPlayerWrapper* ffPlayer = botAI->GetBotPlayer();
    // if (ffPlayer->GetOrigin().DistToSqr2D(m_vPositionToHold) > m_fHoldRadiusSqr) {
    //     // Bot strayed. CFFBaseAI::ExecuteSubTask for HOLD_POSITION should initiate MoveTo.
    // }
    // Attacking/looking around is handled by CFFBaseAI::ExecuteSubTask for HOLD_POSITION.
}
void CHoldPositionTask_FF::reset(CFFBaseAI* botAI) { CBotTask::reset(botAI); m_fTaskStartTime = 0.0f; }
std::string CHoldPositionTask_FF::debugString(CFFBaseAI* botAI) const { return "CHoldPositionTask_FF"; }
