#include "FFControlPointTasks.h"
#include "FFBaseAI.h"       // For CFFBaseAI and its methods (like GetBotPlayer, GetKnowledgeBase, MoveTo, AttackTarget etc.)
#include "CFFPlayer.h"      // For CFFPlayer methods (GetOrigin, GetTeam, etc.)
#include "BotKnowledgeBase.h" // For accessing game state like CP ownership
#include "BotDefines.h"     // For constants like ARRIVAL_TOLERANCE_SQR_FF
#include "GameDefines_Placeholder.h" // For CUserCmd, IN_BUTTONS, FL_*, TEAM_*, etc. (conceptual)
// #include "bot_schedule.h" // Assumed for pSchedule parameter in fail/complete (RCBot2 context)

#include <string>
#include <iostream>         // For placeholder debug prints

// --- Conceptual Time & Bot Access ---
// Using placeholders from CFFBaseAI.cpp or assuming CFFBaseAI provides these:
// float CFFBaseAI::GetCurrentWorldTime_conceptual() const { static float t = 0; t+=0.1f; return t; }
// void CFFBaseAI::StopMoving_conceptual(CUserCmd* pCmd) { if(pCmd) pCmd->forwardmove = pCmd->sidemove = 0; }
// void CFFBaseAI::SetLookAtTask_conceptual(LookAtTaskType_Conceptual type, const Vector& target) { /* ... */ }
// bool CFFBaseAI::IsAtMoveToTarget() const { /* ... */ return false; }
// bool CFFBaseAI::HasPathfindingFailed() const { /* ... */ return false; }
// CUserCmd* CFFBaseAI::GetUserCmd_conceptual() { static CUserCmd cmd; return &cmd; } // Risky static for example
// --- End Conceptual ---


// --- CMoveToPositionTask Implementation ---
CMoveToPositionTask::CMoveToPositionTask(const Vector& targetPos, float arrivalTolerance)
    : CBotTask(), // Base constructor
      m_vTargetPosition(targetPos),
      m_fArrivalToleranceSqr(arrivalTolerance * arrivalTolerance), // Store squared
      m_fStuckTimer(0.0f) {
    // setID(TASK_MOVE_TO_POSITION_FF_CONCEPTUAL_ID);
}

void CMoveToPositionTask::init(CFFBaseAI* botAI) {
    CBotTask::init(botAI); // Assuming CBotTask::init takes CFFBaseAI* or can be adapted
    setState(TASK_STATE_RUNNING);
    m_fStuckTimer = 0.0f;
    if (botAI) {
        // Path is planned by CFFBaseAI when ExecuteSubTask for MOVE_TO_POSITION is first called.
        // This task assumes CFFBaseAI's m_vCurrentMoveToTarget is set to m_vTargetPosition
        // and m_CurrentPath_NavAreaIDs is being processed by botAI->FollowPath().
        // Forcing a MoveTo here could override a path already being followed if this task is part of a sequence.
        // It's better if CFFBaseAI::ExecuteSubTask manages the call to botAI->MoveTo() once.
        // std::cout << botAI->GetBotPlayer()->GetNamePlaceholder() << ": Init MoveToPositionTask to (" << m_vTargetPosition.x << ")" << std::endl;
    }
}

void CMoveToPositionTask::execute(CFFBaseAI* botAI, CBotSchedule* pSchedule) {
    if (!botAI || !botAI->GetBotPlayer()) { fail(pSchedule, "MoveToPositionTask: BotAI or BotPlayer is null"); return; }
    CFFPlayer* ffPlayer = botAI->GetBotPlayer();
    // CUserCmd* pCmd = botAI->GetUserCmd_conceptual(); // Not directly used by this task's logic, CFFBaseAI::Update fills it.

    // Check for arrival
    float distSq = (ffPlayer->GetOrigin().x - m_vTargetPosition.x)*(ffPlayer->GetOrigin().x - m_vTargetPosition.x) +
                   (ffPlayer->GetOrigin().y - m_vTargetPosition.y)*(ffPlayer->GetOrigin().y - m_vTargetPosition.y);
    // Ignoring Z for arrival condition for simplicity with nav areas
    if (distSq < m_fArrivalToleranceSqr) {
        // std::cout << ffPlayer->GetNamePlaceholder() << ": Reached target for MoveToPositionTask." << std::endl;
        botAI->ClearCurrentPath(); // Important: Stop current path following
        botAI->StopMoving_conceptual(nullptr /* pCmd is managed by AI::Update */);
        complete();
        return;
    }

    // If botAI->IsCurrentlyWithPath() becomes false before arrival, it means path ended prematurely or failed.
    // This check relies on CFFBaseAI::FollowPath correctly clearing path on completion/failure.
    if (!botAI->IsCurrentlyWithPath_conceptual() && !botAI->IsAtMoveToTarget_conceptual(m_vTargetPosition, m_fArrivalToleranceSqr)) {
        // Path might have ended, but we are not at the target. Try to re-path.
        // std::cout << ffPlayer->GetNamePlaceholder() << ": Path ended for MoveTo, but not at target. Re-planning." << std::endl;
        if (!botAI->MoveTo(m_vTargetPosition, nullptr /* CFFBaseAI::Update gets cmd */)) {
             // std::cout << ffPlayer->GetNamePlaceholder() << ": Re-plan failed for MoveTo." << std::endl;
            fail(pSchedule, "Failed to re-plan path in MoveToPositionTask"); // Pathing failed
            return;
        }
    }

    // Stuck logic (conceptual, CFFBaseAI should ideally handle this in its FollowPath)
    // if (ffPlayer->GetVelocity().LengthSqr() < 1.0f && botAI->IsTryingToMove_Conceptual()) {
    //     m_fStuckTimer += botAI->GetFrameInterval_Conceptual();
    //     if (m_fStuckTimer > 3.0f) { // Stuck for 3 seconds
    //         fail(pSchedule, "MoveToPositionTask: Stuck"); return;
    //     }
    // } else { m_fStuckTimer = 0.0f; }

    // If this task is still running, it means CFFBaseAI::Update is calling FollowPath,
    // and FollowPath hasn't returned false yet (or this task hasn't timed out/failed).
}

void CMoveToPositionTask::reset(CFFBaseAI* botAI) {
    CBotTask::reset(botAI);
    m_fStuckTimer = 0.0f;
    if (botAI) botAI->ClearCurrentPath(); // Ensure path is cleared on reset
}

std::string CMoveToPositionTask::debugString(CFFBaseAI* botAI) const {
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
    if(botAI) botAI->StopMoving_conceptual(nullptr);
}

void CSecureAreaTask_FF::execute(CFFBaseAI* botAI, CBotSchedule* pSchedule) {
    if (!botAI || !botAI->GetBotPlayer()) { fail(pSchedule, "SecureArea: Invalid bot state"); return; }
    // CUserCmd* pCmd = botAI->GetUserCmd_conceptual(); // AI::Update gets UserCmd

    if (botAI->GetCurrentWorldTime_conceptual() - m_fTaskStartTime > m_fDuration) {
        complete(); return;
    }

    // CBaseEntity* pEnemy = botAI->SelectTargetForArea_conceptual(m_vAreaCenter, m_fRadius); // Specific scan
    CBaseEntity* pEnemy = botAI->SelectTarget(); // Use general target selection for now

    if (pEnemy && pEnemy->IsAlive() && (pEnemy->GetPosition().x-m_vAreaCenter.x)*(pEnemy->GetPosition().x-m_vAreaCenter.x) + (pEnemy->GetPosition().y-m_vAreaCenter.y)*(pEnemy->GetPosition().y-m_vAreaCenter.y) < (m_fRadius * m_fRadius) ) {
        m_bFoundEnemyDuringTask = true;
        botAI->AttackTarget(pEnemy, nullptr /* CFFBaseAI::Update gets UserCmd */); // Let derived class handle attack
    } else {
        if (m_bFoundEnemyDuringTask) { // Enemy was present but now gone (killed or retreated)
            complete(); return;
        }
        // botAI->SetLookAtTask_conceptual(LookAtTaskType_Conceptual::LOOK_AROUND_SCAN_IN_AREA, &m_vAreaCenter);
        // botAI->StopMoving_conceptual(pCmd); // Or patrol slightly
    }
}
void CSecureAreaTask_FF::reset(CFFBaseAI* botAI) { CBotTask::reset(botAI); m_fTaskStartTime = 0.0f; m_bFoundEnemyDuringTask = false; }
std::string CSecureAreaTask_FF::debugString(CFFBaseAI* botAI) const { return "CSecureAreaTask_FF"; }


// --- CStandOnPointTask_FF Implementation ---
CStandOnPointTask_FF::CStandOnPointTask_FF(CBaseEntity* pCPEntity_conceptual, const Vector& vCPPosition, float fMaxTime, float captureRadius)
    : CBotTask(), m_pTargetCPEntity_conceptual(pCPEntity_conceptual), m_vCPPosition(vCPPosition), m_fMaxTime(fMaxTime),
      m_fCaptureRadiusSqr(captureRadius*captureRadius), m_fTaskStartTime(0.0f), m_iTargetCPEntityId_conceptual(-1) {
    // if (pCPEntity_conceptual) m_iTargetCPEntityId_conceptual = pCPEntity_conceptual->GetId_conceptual();
}

void CStandOnPointTask_FF::init(CFFBaseAI* botAI) {
    CBotTask::init(botAI);
    setState(TASK_STATE_RUNNING);
    m_fTaskStartTime = botAI ? botAI->GetCurrentWorldTime_conceptual() : 0.0f;
    if (botAI) botAI->StopMoving_conceptual(nullptr);
}

void CStandOnPointTask_FF::execute(CFFBaseAI* botAI, CBotSchedule* pSchedule) {
    if (!botAI || !botAI->GetBotPlayer() || !botAI->GetKnowledgeBase()) { fail(pSchedule, "StandOnPoint: Invalid bot state"); return; }
    CFFPlayer* ffPlayer = botAI->GetBotPlayer();
    const BotKnowledgeBase* kb = botAI->GetKnowledgeBase();
    // CUserCmd* pCmd = botAI->GetUserCmd_conceptual();

    float distSqToPoint = (ffPlayer->GetOrigin().x - m_vCPPosition.x)*(ffPlayer->GetOrigin().x - m_vCPPosition.x) +
                          (ffPlayer->GetOrigin().y - m_vCPPosition.y)*(ffPlayer->GetOrigin().y - m_vCPPosition.y);
    if (distSqToPoint > m_fCaptureRadiusSqr * 2.25f) { // Moved too far (1.5x radius)
        fail(pSchedule, "Moved too far off CP"); return;
    }

    // const ControlPointInfo* cpState = kb->GetControlPoint(m_iTargetCPEntityId_conceptual);
    // if (cpState && cpState->ownerTeam == ffPlayer->GetTeamNumber()) {
    //     complete(); return;
    // }

    if (botAI->GetCurrentWorldTime_conceptual() - m_fTaskStartTime > m_fMaxTime) {
        fail(pSchedule, "Timeout standing on CP"); return;
    }

    // CBaseEntity* pEnemy = botAI->SelectTargetForArea_conceptual(m_vCPPosition, sqrt(m_fCaptureRadiusSqr) * 2.0f);
    // if (pEnemy && pEnemy->IsAlive()) {
    //     botAI->AttackTarget(pEnemy, nullptr);
    // } else {
    //     botAI->StopMoving_conceptual(nullptr);
    //     botAI->SetLookAtTask_conceptual(LookAtTaskType_Conceptual::LOOK_AROUND_ON_POINT, &m_vCPPosition);
    // }
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
    // Bot should ideally already be at or near m_vPositionToHold due to a preceding MoveTo task
    // if (botAI) botAI->StopMoving_conceptual(nullptr);
}

void CHoldPositionTask_FF::execute(CFFBaseAI* botAI, CBotSchedule* pSchedule) {
    if (!botAI || !botAI->GetBotPlayer()) { fail(pSchedule, "HoldPosition: Invalid bot state"); return; }
    // CFFPlayer* ffPlayer = botAI->GetBotPlayer();
    // CUserCmd* pCmd = botAI->GetUserCmd_conceptual();

    if (botAI->GetCurrentWorldTime_conceptual() - m_fTaskStartTime > m_fDuration) {
        complete(); return;
    }

    // CBaseEntity* pEnemy = botAI->SelectTarget(); // General target selection
    // if (pEnemy && pEnemy->IsAlive()) {
    //     botAI->AttackTarget(pEnemy, nullptr);
    // } else {
    //     botAI->SetLookAtTask_conceptual(LookAtTaskType_Conceptual::LOOK_AROUND_SCAN_DEFENSIVE, &m_vPositionToHold);
    // }

    // float distSqToHold = (ffPlayer->GetOrigin().x - m_vPositionToHold.x)*(ffPlayer->GetOrigin().x - m_vPositionToHold.x) +
    //                      (ffPlayer->GetOrigin().y - m_vPositionToHold.y)*(ffPlayer->GetOrigin().y - m_vPositionToHold.y);
    // if (distSqToHold > m_fHoldRadiusSqr) {
    //     // Bot strayed too far, CFFBaseAI::ExecuteSubTask for HOLD_POSITION should call MoveTo.
    //     // This task itself doesn't call MoveTo directly to avoid pathfinding every frame here.
    //     // It signals its state/need to the CFFBaseAI layer.
    //     // For now, this task is ongoing. CFFBaseAI's ExecuteSubTask needs to handle repositioning.
    // } else {
    //     // botAI->StopMoving_conceptual(nullptr); // If within hold radius and not fighting
    // }
}
void CHoldPositionTask_FF::reset(CFFBaseAI* botAI) { CBotTask::reset(botAI); m_fTaskStartTime = 0.0f; }
std::string CHoldPositionTask_FF::debugString(CFFBaseAI* botAI) const { return "CHoldPositionTask_FF"; }
