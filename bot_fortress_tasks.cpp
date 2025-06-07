#include "bot_fortress_tasks.h"
#include "bot_fortress.h" // For CBotFortress and its methods like HasEnemy, GetEnemy, etc.
// #include "world.h" // For TheWorld()->CurrentTime() or similar global time access
// #include "tf_gamerules.h" // For game-specific rules like CP capture logic (conceptual)
// #include "nav_mesh.h" // For pathing or area information if needed

// Assuming these are defined in RCBot2's CBot or CBotFortress:
// enum LookAtTaskType { LOOK_AROUND, LOOK_ENEMY, LOOK_AROUND_SCAN, LOOK_AT_POINT };
// pBot->setLookAtTask(type, targetPointIfAny);
// pBot->handleAttack(weapon, enemy);
// pBot->getVisionInterface()->IsVisible(enemy); // Or similar
// pBot->stopMoving();
// pBot->GetTime(); // Current game time

// Conceptual placeholder for global time
float GetCurrentWorldTime() {
    // In Source 1, this is often gpGlobals->curtime
    // For RCBot2, it might be TheWorld()->CurrentTime() or pBot->GetTime()
    return 0.0f; // Needs actual implementation
}

// --- CSecureAreaTask_FF Implementation ---

CSecureAreaTask_FF::CSecureAreaTask_FF(const Vector& vAreaCenter, float fRadius, float fDuration)
    : CBotTask(), // Call base constructor
      m_vAreaCenter(vAreaCenter),
      m_fRadius(fRadius),
      m_fDuration(fDuration),
      m_fTaskStartTime(0.0f),
      m_bFoundEnemy(false)
{
    // setID(TASK_FF_SECURE_AREA);
}

void CSecureAreaTask_FF::init(CBot* pBot) {
    CBotTask::init(pBot); // Call base init
    m_fTaskStartTime = GetCurrentWorldTime(); // Or pBot->GetTime();
    m_bFoundEnemy = false;
    // pBot->GetPathFollower()->clearPath(); // Stop any previous movement
    // pBot->stopMoving();
}

void CSecureAreaTask_FF::execute(CBot* pBot, CBotSchedule* pSchedule) {
    CBotFortress* pBotFF = dynamic_cast<CBotFortress*>(pBot);
    if (!pBotFF) {
        fail(pSchedule, "Bot is not a CBotFortress instance");
        return;
    }

    // Check for enemies in the area
    // CBaseEntity* pEnemy = pBotFF->getVisionInterface()->findNearestEnemy(m_vAreaCenter, m_fRadius); // Conceptual
    CBaseEntity* pEnemy = pBotFF->getEnemy(); // Simpler: use bot's current enemy if any

    if (pEnemy /* && pBotFF->getVisionInterface()->IsVisible(pEnemy) */ ) {
        m_bFoundEnemy = true;
        // pBotFF->setLookAtTask(LOOK_ENEMY, pEnemy->getPosition()); // LOOK_ENEMY often implies looking at pBotFF->getEnemy()
        pBotFF->setLookAtTask(pEnemy); // Assumed API: look at current enemy
        // pBotFF->handleAttack(pBotFF->getBestWeapon(pEnemy), pEnemy); // Engage the enemy

        // Task remains ongoing while fighting. Could complete if enemy dies or bot is no longer fighting.
        // For simplicity, let's assume it completes if no enemy is actively engaged *after* finding one.
        // Or, it could fail if the bot is taking too much damage.
        // if (!pBotFF->isAttacking()) { // if combat logic decided to stop
        //    complete();
        // }
        // For now, let it run for its duration or until no enemies.
        // If the bot kills the enemy, pBotFF->getEnemy() should become null.
    } else {
        // No enemy currently visible/targeted
        if (m_bFoundEnemy) { // We previously fought an enemy, and now it's gone (dead or retreated)
            // Log_Msg("%s: Area secured (enemy dealt with) at (%.1f, %.1f, %.1f)\n", pBotFF->getName(), m_vAreaCenter.x, m_vAreaCenter.y, m_vAreaCenter.z);
            complete(); // Consider area secured for now
            return;
        }
        // No enemy found yet, or never was one.
        // pBotFF->setLookAtTask(LOOK_AROUND_SCAN, m_vAreaCenter); // Scan the area
        // pBotFF->stopMoving(); // Or patrol slightly within radius

        // Potentially make the bot move to different spots within the radius to "secure" it.
        // For now, just looking around from current spot is enough.
    }

    // Check task duration
    if (GetCurrentWorldTime() - m_fTaskStartTime > m_fDuration) {
        if (!m_bFoundEnemy) {
            // Log_Msg("%s: SecureArea timed out (no threats found) at (%.1f, %.1f, %.1f)\n", pBotFF->getName(), m_vAreaCenter.x, m_vAreaCenter.y, m_vAreaCenter.z);
        } else {
            // Log_Msg("%s: SecureArea timed out (threats were present) at (%.1f, %.1f, %.1f)\n", pBotFF->getName(), m_vAreaCenter.x, m_vAreaCenter.y, m_vAreaCenter.z);
        }
        complete(); // Duration expired
    }
}

void CSecureAreaTask_FF::reset(CBot* pBot) {
    CBotTask::reset(pBot);
    m_fTaskStartTime = 0.0f;
    m_bFoundEnemy = false;
}


// --- CStandOnPointTask_FF Implementation ---

CStandOnPointTask_FF::CStandOnPointTask_FF(CBaseEntity* pCPEntity, const Vector& vCPPosition, float fMaxTime)
    : CBotTask(), // Call base constructor
      m_pTargetCPEntity(pCPEntity),
      m_vCPPosition(vCPPosition),
      m_fMaxTime(fMaxTime),
      m_fTaskStartTime(0.0f)
{
    // setID(TASK_FF_STAND_ON_POINT);
}

void CStandOnPointTask_FF::init(CBot* pBot) {
    CBotTask::init(pBot);
    m_fTaskStartTime = GetCurrentWorldTime(); // Or pBot->GetTime();

    CBotFortress* pBotFF = dynamic_cast<CBotFortress*>(pBot);
    if (pBotFF) {
        // pBotFF->GetPathFollower()->clearPath(); // Ensure bot stops if it was moving
        pBotFF->stopMoving();
    }
}

void CStandOnPointTask_FF::execute(CBot* pBot, CBotSchedule* pSchedule) {
    CBotFortress* pBotFF = dynamic_cast<CBotFortress*>(pBot);
    if (!pBotFF) {
        fail(pSchedule, "Bot is not a CBotFortress instance");
        return;
    }
    if (!m_pTargetCPEntity) {
        fail(pSchedule, "Target CP Entity is null");
        return;
    }

    // Check distance to point
    // float distToPointSq = (pBotFF->getPosition() - m_vCPPosition).LengthSqr(); // Assuming getPosition() and Vector math
    // if (distToPointSq > 100.0f * 100.0f) { // Example: if >100 units away from where it should be
    //     fail(pSchedule, "Too far from CP capture point");
    //     return;
    // }

    // Conceptual: Check if CP is now owned by bot's team
    // FFTeamID myTeam = (FFTeamID)pBotFF->GetTeamNumber();
    // FFTeamID cpOwner = FortressForever::GetObjectiveData()->GetControlPointStateByID(m_pTargetCPEntity->GetID())->ownerTeam; // Conceptual
    // if (cpOwner == myTeam) {
    //     Log_Msg("%s: CP %d successfully captured/confirmed owned by team %d!\n", pBotFF->getName(), m_pTargetCPEntity->GetID(), myTeam);
    //     complete();
    //     return;
    // }

    // Timeout check
    if (GetCurrentWorldTime() - m_fTaskStartTime > m_fMaxTime) {
        // Log_Msg("%s: Timed out trying to stand on CP %d\n", pBotFF->getName(), m_pTargetCPEntity->GetID());
        fail(pSchedule, "Timed out standing on point");
        return;
    }

    // If an enemy is nearby and engaging, the bot's combat routines should take over.
    // This task might fail if the bot is forced to move significantly or if it dies.
    // Or, it could allow fighting while trying to stay on point.
    if (pBotFF->getEnemy() /* && pBotFF->getVisionInterface()->IsVisible(pBotFF->getEnemy()) */) {
        // pBotFF->setLookAtTask(LOOK_ENEMY);
        // pBotFF->handleAttack(pBotFF->getBestWeapon(pBotFF->getEnemy()), pBotFF->getEnemy());
        // If fighting, we are still "standing on point" (conceptually).
        // If bot is forced off the point due to combat, the distance check above might fail it later,
        // or a CONDITION_HEAVY_DAMAGE interrupt could trigger.
    } else {
        // No immediate enemy, just hold the position and look around.
        pBotFF->stopMoving();
        // pBotFF->setLookAtTask(LOOK_AROUND_SCAN, m_vCPPosition); // Scan around the point
    }
    // Task is ongoing. Completion/failure is based on CP capture, timeout, or external interruption.
}

void CStandOnPointTask_FF::reset(CBot* pBot) {
    CBotTask::reset(pBot);
    m_fTaskStartTime = 0.0f;
}


// --- CRocketJumpTask_FF Implementation ---

CRocketJumpTask_FF::CRocketJumpTask_FF(const Vector& vTargetApexOrDirection, bool isDirection)
    : CBotTask(),
      m_vTarget(vTargetApexOrDirection),
      m_bIsDirection(isDirection),
      m_eCurrentState(RJState::IDLE),
      m_fStateEntryTime(0.0f)
{
    // setID(TASK_FF_ROCKET_JUMP);
}

CRocketJumpTask_FF::~CRocketJumpTask_FF() {}

void CRocketJumpTask_FF::init(CBot* pBot) {
    CBotTask::init(pBot);
    m_eCurrentState = RJState::AIMING; // Start by aiming
    m_fStateEntryTime = GetCurrentWorldTime(); // Or pBot->GetTime();

    CBotFortress* pBotFF = dynamic_cast<CBotFortress*>(pBot);
    if (pBotFF) {
        // Ensure rocket launcher is equipped. This might take a frame or two.
        // if (pBotFF->GetActiveWeaponID() != TF2_WEAPON_ROCKETLAUNCHER) {
        //     pBotFF->selectWeaponName("weapon_rocketlauncher"); // Or by ID
        // }
    }
}

void CRocketJumpTask_FF::execute(CBot* pBot, CBotSchedule* pSchedule) {
    CBotFortress* pBotFF = dynamic_cast<CBotFortress*>(pBot);
    UserCmd* pCmd = pBotFF ? pBotFF->GetUserCmd() : nullptr; // GetUserCmd() is conceptual

    if (!pBotFF || !pCmd) {
        fail(pSchedule, "Invalid bot or UserCmd for RocketJump");
        return;
    }

    // Basic state machine for rocket jump
    switch (m_eCurrentState) {
        case RJState::AIMING:
            // Ensure RL is out and ready
            // if (pBotFF->GetActiveWeaponID() != TF2_WEAPON_ROCKETLAUNCHER || !pBotFF->CanAttackWithWeapon(TF2_WEAPON_ROCKETLAUNCHER)) {
            //     if (GetCurrentWorldTime() - m_fStateEntryTime > 1.0f) { // Timeout for weapon switch
            //         fail(pSchedule, "Failed to equip Rocket Launcher for RJ");
            //     }
            //     return; // Wait for weapon switch or ammo
            // }

            // Aim down at a specific angle (e.g., 45-60 degrees from horizontal)
            // Or, if m_bIsDirection, aim slightly behind the bot if m_vTarget is forward.
            // For a simple downward jump:
            // QAngle currentAngles = pBotFF->getViewAngles();
            // pBotFF->setIdealViewAngles(QAngle(60.0f, currentAngles.y, 0)); // Look 60 deg down
            // pBotFF->setLookAtTask(LOOK_AT_POINT, pBotFF->getPosition() + Vector(0,0,-100)); // Look down

            // For now, assume aiming is quick or handled by bot's aiming system.
            // Transition to next state after a short delay or if aim is correct.
            // if (pBotFF->IsAimAtAngle(QAngle(60.0f, currentAngles.y, 0))) {
                m_eCurrentState = RJState::CROUCHING; // Or JUMPING_AND_FIRING directly
                m_fStateEntryTime = GetCurrentWorldTime();
            // }
            break;

        case RJState::CROUCHING: // Optional for crouch-jumps
            // pCmd->buttons |= IN_DUCK;
            // if (pBotFF->IsCrouching() || GetCurrentWorldTime() - m_fStateEntryTime > 0.2f) { // Wait for crouch or timeout
                m_eCurrentState = RJState::JUMPING_AND_FIRING;
                m_fStateEntryTime = GetCurrentWorldTime();
            // }
            break;

        case RJState::JUMPING_AND_FIRING:
            // pCmd->buttons |= IN_DUCK; // Hold duck if crouch jumping
            // pCmd->buttons |= IN_JUMP;
            // pCmd->buttons |= IN_ATTACK;

            // Log_Msg("%s: Executing RJ: JUMP & FIRE!\n", pBotFF->getName());

            // Transition to in-air or completed after this frame
            // The actual jump happens due to physics based on these inputs.
            // This task might complete here, letting physics take over, or move to an IN_AIR state.
            // For simplicity, complete after firing. More advanced would monitor flight.
            m_eCurrentState = RJState::IN_AIR_CONTROL;
            m_fStateEntryTime = GetCurrentWorldTime();
            break;

        case RJState::IN_AIR_CONTROL:
            // Bot is airborne. Optionally, apply air strafing logic towards m_vTarget if it's an apex.
            // Or simply wait for landing or a timeout.
            // if (pBotFF->IsOnGround()) {
            //    Log_Msg("%s: RJ: Landed.\n", pBotFF->getName());
            //    complete();
            // } else if (GetCurrentWorldTime() - m_fStateEntryTime > 3.0f) { // Max flight time
            //    Log_Msg("%s: RJ: In-air timeout.\n", pBotFF->getName());
            //    complete(); // Or fail depending on if target was reached
            // }
            // For this example, we assume it's a short action.
            complete(); // Simplified: task is done after the jump action itself.
            break;

        case RJState::COMPLETED:
            complete();
            break;
        case RJState::FAILED:
            fail(pSchedule, "Rocket jump failed internally");
            break;
        default:
            fail(pSchedule, "Unknown rocket jump state");
            break;
    }
}

void CRocketJumpTask_FF::reset(CBot* pBot) {
    CBotTask::reset(pBot);
    m_eCurrentState = RJState::IDLE;
    m_fStateEntryTime = 0.0f;
}

// bool CRocketJumpTask_FF::checkConditions(CBot* pBot) {
//     CBotFortress* pBotFF = dynamic_cast<CBotFortress*>(pBot);
//     if (!pBotFF || pBotFF->GetClass() != TF_CLASS_SOLDIER) return false;
//     if (!pBotFF->IsOnGround() && m_eCurrentState != RJState::IN_AIR_CONTROL) return false; // Must be on ground to start
//     // Check health (don't jump with 5 HP)
//     // Check ammo for rocket launcher
//     return true;
// }


// --- CHoldPositionTask_FF Implementation ---

CHoldPositionTask_FF::CHoldPositionTask_FF(const Vector& vPositionToHold, float fDuration)
    : CBotTask(), // Call base constructor
      m_vPositionToHold(vPositionToHold),
      m_fDuration(fDuration),
      m_fTaskStartTime(0.0f)
{
    // setID(TASK_FF_HOLD_POSITION); // Assuming a new task ID
}

void CHoldPositionTask_FF::init(CBot* pBot) {
    CBotTask::init(pBot);
    m_fTaskStartTime = GetCurrentWorldTime(); // Or pBot->GetTime();

    CBotFortress* pBotFF = dynamic_cast<CBotFortress*>(pBot);
    if (pBotFF) {
        // Stop significant movement, but allow slight adjustments for combat
        // pBotFF->GetPathFollower()->clearPath();
        // pBotFF->stopMoving(); // Might be too restrictive, allow combat movement

        // Check if already near the hold position
        // float distToHoldPosSq = (pBotFF->getPosition() - m_vPositionToHold).LengthSqr();
        // if (distToHoldPosSq > 32.0f * 32.0f) { // If > 32 units away, probably should have moved closer first
             // This task assumes the bot is already at or very near the hold position.
             // A CMoveToTask should precede this if starting from far away.
        // }
    }
}

void CHoldPositionTask_FF::execute(CBot* pBot, CBotSchedule* pSchedule) {
    CBotFortress* pBotFF = dynamic_cast<CBotFortress*>(pBot);
    if (!pBotFF) {
        fail(pSchedule, "Bot is not a CBotFortress instance for HoldPosition");
        return;
    }

    // Check task duration
    if (GetCurrentWorldTime() - m_fTaskStartTime > m_fDuration) {
        // Log_Msg("%s: HoldPosition duration expired at (%.1f, %.1f, %.1f)\n", pBotFF->getName(), m_vPositionToHold.x, m_vPositionToHold.y, m_vPositionToHold.z);
        complete();
        return;
    }

    // Primary behavior: Engage any visible enemies
    CBaseEntity* pEnemy = pBotFF->getEnemy(); // Assumes getEnemy() finds visible/viable enemies
    if (pEnemy /* && pBotFF->getVisionInterface()->IsVisible(pEnemy) */) {
        // pBotFF->setLookAtTask(LOOK_ENEMY); // Or LOOK_AT_POINT(pEnemy->getPosition())
        // pBotFF->handleAttack(pBotFF->getBestWeapon(pEnemy), pEnemy);
        // Bot might need to move slightly for combat (strafing, taking cover near hold point)
        // This would be handled by the bot's internal combat movement logic, not explicitly here.
        // This task doesn't fail just because of combat; it's *about* combat at this position.
    } else {
        // No enemy in sight, maintain situational awareness.
        pBotFF->stopMoving(); // Be more stationary when not actively fighting
        // pBotFF->setLookAtTask(LOOK_AROUND_SCAN_DEFENSIVE, m_vPositionToHold); // Conceptual: scan likely enemy approaches
        // A "LOOK_AROUND_SCAN_DEFENSIVE" might make the bot prioritize directions from which enemies typically appear for this hold spot.
    }

    // Task is ongoing until duration expires or an interrupt condition is met.
    // Interrupt conditions (handled by CBotSchedule or CBotTask base):
    // - CONDITION_HEAVY_DAMAGE: Might cause bot to retreat, failing this task.
    // - CONDITION_NEW_ENEMY_PRIORITY: A more dangerous threat appears elsewhere.
    // - Objective change: e.g., point we are defending is lost or becomes non-critical.
}

void CHoldPositionTask_FF::reset(CBot* pBot) {
    CBotTask::reset(pBot);
    m_fTaskStartTime = 0.0f;
}
