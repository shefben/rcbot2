#include "FFControlPointTasks.h"
#include "bot.h"            // Assumed: CBot definition, CBot::GetTime(), CBot::GetOrigin(), SetMoveTo(), etc.
#include "bot_schedule.h"   // Assumed: CBotSchedule definition
// #include "tf_gamerules.h" // Conceptual: For FortressForever::GetObjectiveData()->GetPointOwner()
// #include "world.h"          // Conceptual: For TheWorld()->CurrentTime() if CBot::GetTime() not available
#include <string>           // For std::to_string in debugString
#include <iostream>         // For placeholder debug prints

// --- Conceptual RCBot2 / Engine methods often used by tasks ---
// These would be part of CBot or accessible globally/via interfaces.
// float CBot::GetTime() const; // Returns current game time.
// const Vector& CBot::GetOrigin() const; // Returns bot's current position.
// void CBot::SetMoveTo(const Vector& pos, bool bIsUrgent = false); // Tells bot to move towards pos.
// bool CBot::MoveFailed() const; // If last SetMoveTo resulted in a path failure.
// void CBot::StopMoving(); // Clears current movement.
// CBaseEntity* CBot::SelectTarget(float range = -1.0f, bool onlyVisible = true); // Selects best enemy.
// bool CBot::AttackTarget(CBaseEntity* target); // Engages target.
// void CBot::SetLookAtTask(LookAtTaskType type, const Vector* targetPoint = nullptr, CBaseEntity* targetEntity = nullptr);
// bool CBot::IsOnGround() const;
// int CBot::GetTeamNumber() const;
// PathFollower* CBot::GetPathFollower(); // For more complex path interactions.

// For placeholder time:
static float s_fConceptualGameTime = 0.0f;
void AdvanceConceptualGameTime() { s_fConceptualGameTime += 0.1f; } // Call this in a conceptual game loop
float GetConceptualWorldTime() { return s_fConceptualGameTime; }


// --- CMoveToPositionTask Implementation ---
CMoveToPositionTask::CMoveToPositionTask(const Vector& targetPos, float arrivalTolerance)
    : CBotTask(), m_vTargetPosition(targetPos), m_fArrivalTolerance(arrivalTolerance), m_fStuckTimer(0.0f) {
    // setID(TASK_MOVE_TO_POSITION); // Conceptual: Set task ID if using RCBot2's enum system
}

void CMoveToPositionTask::init(CBot* pBot) {
    CBotTask::init(pBot); // Base class init
    setState(TASK_STATE_RUNNING);
    m_fStuckTimer = 0.0f;
    if (pBot) {
        // pBot->SetMoveTo(m_vTargetPosition); // Initial command to move
        // std::cout << pBot->getName() << ": Moving to (" << m_vTargetPosition.x << ", " << m_vTargetPosition.y << ")" << std::endl;
    }
}

void CMoveToPositionTask::execute(CBot* pBot, CBotSchedule* pSchedule) {
    if (!pBot) {
        fail(pSchedule, "CMoveToPositionTask: Bot is null");
        return;
    }
    AdvanceConceptualGameTime(); // Simulate time passing for stuck timer

    // Vector botOrigin = pBot->GetOrigin(); // Conceptual
    // float distSq = (botOrigin - m_vTargetPosition).LengthSqr();
    // if (distSq < (m_fArrivalTolerance * m_fArrivalTolerance)) {
    //     std::cout << pBot->getName() << ": Reached target (" << m_vTargetPosition.x << ", " << m_vTargetPosition.y << ")" << std::endl;
    //     pBot->StopMoving(); // Stop once arrived
    //     complete();
    //     return;
    // }

    // // Conceptual: Re-issue move command periodically or if path follower is idle
    // if (!pBot->GetPathFollower()->IsMoving()) { // Or some other way to check if actively pathing
    //     pBot->SetMoveTo(m_vTargetPosition);
    // }

    // Conceptual stuck logic
    // if (pBot->GetVelocity().LengthSqr() < 1.0f && pBot->IsAttemptingToMove()) { // Conceptual
    //     m_fStuckTimer += pBot->GetFrameInterval(); // Conceptual frame interval
    //     if (m_fStuckTimer > 3.0f) { // Stuck for 3 seconds
    //         std::cout << pBot->getName() << ": Movement failed (stuck) to (" << m_vTargetPosition.x << ", " << m_vTargetPosition.y << ")" << std::endl;
    //         fail(pSchedule, "Stuck trying to move");
    //         return;
    //     }
    // } else {
    //     m_fStuckTimer = 0.0f;
    // }

    // if (pBot->MoveFailed()) { // Conceptual: if pathing system reports failure
    //     std::cout << pBot->getName() << ": Movement failed (path error) to (" << m_vTargetPosition.x << ", " << m_vTargetPosition.y << ")" << std::endl;
    //     fail(pSchedule, "Pathing error");
    //     return;
    // }

    // For this placeholder, assume it completes after a few "frames"
    static int frame_count = 0;
    if (++frame_count > 20) { // Simulate taking 2 seconds (20 * 0.1s)
        frame_count = 0;
        pBot->StopMoving(); // Conceptual
        complete();
    } else {
         pBot->SetMoveTo(m_vTargetPosition); // Conceptual
    }
}

void CMoveToPositionTask::reset(CBot* pBot) {
    CBotTask::reset(pBot);
    m_fStuckTimer = 0.0f;
    if (pBot) pBot->StopMoving(); // Conceptual
}

std::string CMoveToPositionTask::debugString(CBot* pBot) const {
    return getName() + " to (" + std::to_string(m_vTargetPosition.x) + "," + std::to_string(m_vTargetPosition.y) + ")";
}


// --- CSecureAreaTask_FF Implementation ---
CSecureAreaTask_FF::CSecureAreaTask_FF(const Vector& vAreaCenter, float fRadius, float fDuration)
    : CBotTask(), m_vAreaCenter(vAreaCenter), m_fRadius(fRadius), m_fDuration(fDuration),
      m_fTaskStartTime(0.0f), m_bFoundEnemyDuringTask(false) {
    // setID(TASK_FF_SECURE_AREA);
}

void CSecureAreaTask_FF::init(CBot* pBot) {
    CBotTask::init(pBot);
    setState(TASK_STATE_RUNNING);
    m_fTaskStartTime = GetConceptualWorldTime(); // pBot->GetTime();
    m_bFoundEnemyDuringTask = false;
    // std::cout << pBot->getName() << ": Securing area around (" << m_vAreaCenter.x << ", " << m_vAreaCenter.y << ")" << std::endl;
}

void CSecureAreaTask_FF::execute(CBot* pBot, CBotSchedule* pSchedule) {
    if (!pBot) { fail(pSchedule, "CSecureAreaTask_FF: Bot is null"); return; }
    AdvanceConceptualGameTime();

    if (GetConceptualWorldTime() - m_fTaskStartTime > m_fDuration) {
        // std::cout << pBot->getName() << ": SecureArea duration ended." << std::endl;
        complete();
        return;
    }

    // CBaseEntity* pEnemy = pBot->SelectTarget(m_fRadius, true); // Conceptual: find enemy within radius
    CBaseEntity* pEnemy = pBot->getEnemy(); // Simpler: use current enemy if any

    // if (pEnemy && (pEnemy->GetOrigin() - m_vAreaCenter).LengthSqr() < m_fRadius * m_fRadius) { // Conceptual
    if (pEnemy) { // Simplified: if bot has any current enemy
        m_bFoundEnemyDuringTask = true;
        // pBot->AttackTarget(pEnemy); // Conceptual
        // pBot->SetLookAtTask(LOOK_AT_ENEMY_FF, pEnemy); // Conceptual
        // std::cout << pBot->getName() << ": Engaging enemy while securing area." << std::endl;
        // Task continues while fighting or until duration up
    } else {
        // No enemy, or enemy outside radius/not viable
        // pBot->SetLookAtTask(LOOK_AROUND_SCAN_IN_AREA_FF, &m_vAreaCenter, m_fRadius); // Conceptual
        // pBot->StopMoving(); // Or patrol slightly
        // std::cout << pBot->getName() << ": Scanning area, no enemy." << std::endl;
    }
    // For placeholder, complete after a few frames if no enemy, or keep running if enemy was found (until duration)
    static int secure_frames = 0; secure_frames++;
    if (!pEnemy && secure_frames > 15) { complete(); secure_frames = 0; }
    if (pEnemy && secure_frames > 30) { complete(); secure_frames = 0; } // Simulate longer if fighting
}

void CSecureAreaTask_FF::reset(CBot* pBot) {
    CBotTask::reset(pBot);
    m_fTaskStartTime = 0.0f;
    m_bFoundEnemyDuringTask = false;
}

std::string CSecureAreaTask_FF::debugString(CBot* pBot) const {
    return getName() + " at (" + std::to_string(m_vAreaCenter.x) + "," + std::to_string(m_vAreaCenter.y) + ") for " + std::to_string(m_fDuration) + "s";
}


// --- CStandOnPointTask_FF Implementation ---
CStandOnPointTask_FF::CStandOnPointTask_FF(CBaseEntity* pCPEntity, const Vector& vCPPosition, float fMaxTime, float captureRadius)
    : CBotTask(), m_pTargetCPEntity(pCPEntity), m_vCPPosition(vCPPosition), m_fMaxTime(fMaxTime),
      m_fCaptureRadius(captureRadius), m_fTaskStartTime(0.0f) {
    // setID(TASK_FF_STAND_ON_POINT);
}

void CStandOnPointTask_FF::init(CBot* pBot) {
    CBotTask::init(pBot);
    setState(TASK_STATE_RUNNING);
    m_fTaskStartTime = GetConceptualWorldTime(); // pBot->GetTime();
    if (pBot) {
        pBot->StopMoving(); // Conceptual
        // std::cout << pBot->getName() << ": Standing on point (" << m_vCPPosition.x << ", " << m_vCPPosition.y << ")" << std::endl;
    }
}

void CStandOnPointTask_FF::execute(CBot* pBot, CBotSchedule* pSchedule) {
    if (!pBot) { fail(pSchedule, "CStandOnPointTask_FF: Bot is null"); return; }
    AdvanceConceptualGameTime();

    // float distToPointSq = (pBot->GetOrigin() - m_vCPPosition).LengthSqr();
    // if (distToPointSq > (m_fCaptureRadius * 1.5f * m_fCaptureRadius * 1.5f) ) { // Moved too far
    //     std::cout << pBot->getName() << ": Moved too far from point, failing StandOnPoint." << std::endl;
    //     fail(pSchedule, "Moved off point");
    //     return;
    // }

    // Conceptual: Check actual CP ownership from game state
    // if (m_pTargetCPEntity && FortressForever::GetObjectiveData()->GetPointOwner(m_pTargetCPEntity) == pBot->GetTeamNumber()) {
    //     std::cout << pBot->getName() << ": Point captured!" << std::endl;
    //     complete();
    //     return;
    // }

    if (GetConceptualWorldTime() - m_fTaskStartTime > m_fMaxTime) {
        // std::cout << pBot->getName() << ": Max time on point reached, failing StandOnPoint." << std::endl;
        fail(pSchedule, "Max time on point reached");
        return;
    }

    // CBaseEntity* pEnemy = pBot->SelectTarget(m_fCaptureRadius * 2.0f, true); // Engage nearby threats
    // if (pEnemy) {
    //     pBot->AttackTarget(pEnemy);
    // } else {
    //     pBot->SetLookAtTask(LOOK_AROUND_ON_POINT_FF, &m_vCPPosition); // Conceptual
    // }
    // Placeholder: complete after a few frames
    static int stand_frames = 0;
    if (++stand_frames > 10) { complete(); stand_frames = 0; }
}

void CStandOnPointTask_FF::reset(CBot* pBot) {
    CBotTask::reset(pBot);
    m_fTaskStartTime = 0.0f;
}

std::string CStandOnPointTask_FF::debugString(CBot* pBot) const {
    return getName() + " at (" + std::to_string(m_vCPPosition.x) + "," + std::to_string(m_vCPPosition.y) + ") for max " + std::to_string(m_fMaxTime) + "s";
}


// --- CHoldPositionTask_FF Implementation ---
CHoldPositionTask_FF::CHoldPositionTask_FF(const Vector& vPositionToHold, float fDuration, float fHoldRadius)
    : CBotTask(), m_vPositionToHold(vPositionToHold), m_fDuration(fDuration),
      m_fHoldRadius(fHoldRadius), m_fTaskStartTime(0.0f) {
    // setID(TASK_FF_HOLD_POSITION);
}

void CHoldPositionTask_FF::init(CBot* pBot) {
    CBotTask::init(pBot);
    setState(TASK_STATE_RUNNING);
    m_fTaskStartTime = GetConceptualWorldTime(); // pBot->GetTime();
    // std::cout << pBot->getName() << ": Holding position near (" << m_vPositionToHold.x << ", " << m_vPositionToHold.y << ")" << std::endl;
}

void CHoldPositionTask_FF::execute(CBot* pBot, CBotSchedule* pSchedule) {
    if (!pBot) { fail(pSchedule, "CHoldPositionTask_FF: Bot is null"); return; }
    AdvanceConceptualGameTime();

    if (GetConceptualWorldTime() - m_fTaskStartTime > m_fDuration) {
        // std::cout << pBot->getName() << ": HoldPosition duration ended." << std::endl;
        complete();
        return;
    }

    // CBaseEntity* pEnemy = pBot->SelectTarget();
    // if (pEnemy) {
    //     pBot->AttackTarget(pEnemy);
    // } else {
    //     pBot->SetLookAtTask(LOOK_AROUND_SCAN_DEFENSIVE_FF, &m_vPositionToHold); // Conceptual
    // }

    // float distToHoldPosSq = (pBot->GetOrigin() - m_vPositionToHold).LengthSqr();
    // if (distToHoldPosSq > m_fHoldRadius * m_fHoldRadius) {
    //     pBot->SetMoveTo(m_vPositionToHold); // Return to hold area
    // } else {
    //     pBot->StopMoving(); // Stay within hold radius if not engaging
    // }
    // Placeholder: complete after a few frames
    static int hold_frames = 0;
    if (++hold_frames > 25) { complete(); hold_frames = 0; }
}

void CHoldPositionTask_FF::reset(CBot* pBot) {
    CBotTask::reset(pBot);
    m_fTaskStartTime = 0.0f;
}

std::string CHoldPositionTask_FF::debugString(CBot* pBot) const {
    return getName() + " near (" + std::to_string(m_vPositionToHold.x) + "," + std::to_string(m_vPositionToHold.y) + ") for " + std::to_string(m_fDuration) + "s";
}
