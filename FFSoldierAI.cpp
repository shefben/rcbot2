#include "FFSoldierAI.h"
#include "ObjectivePlanner.h"
#include "BotTasks.h"
#include "FFStateStructs.h"
#include "NavSystem.h"        // For NavAreaNode for example if used by SelectTarget

// Conceptual includes
#include <iostream>
#include <cmath>
#include <algorithm> // For std::min/max if needed

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// --- Conceptual Placeholder Types (already defined in FFBaseAI.cpp, ensure consistency or centralize) ---
#ifndef CUSERCMD_CONCEPTUAL_DEF_FFSOLDIERAI_CPP
#define CUSERCMD_CONCEPTUAL_DEF_FFSOLDIERAI_CPP
struct CUserCmd { int buttons = 0; float forwardmove = 0.0f; float sidemove = 0.0f; Vector viewangles; int weaponselect = 0; };
#endif
#ifndef CBASEENTITY_CONCEPTUAL_DEF_FFSOLDIERAI_CPP
#define CBASEENTITY_CONCEPTUAL_DEF_FFSOLDIERAI_CPP
class CBaseEntity { public: virtual ~CBaseEntity() {} Vector GetPosition() const { return Vector(0,0,0); } Vector GetWorldSpaceCenter() const { return Vector(0,0,32); } Vector GetAbsVelocity() const { return Vector(0,0,0); } bool IsAlive() const { return true; } int GetTeamNumber() const { return 0; } virtual bool IsSentryGun() const { return false; } virtual bool IsPlayer() const { return true; } virtual int GetHealth() const { return 100;} /* ... */ };
#endif
#ifndef CFFPLAYER_CONCEPTUAL_DEF_FFSOLDIERAI_CPP
#define CFFPLAYER_CONCEPTUAL_DEF_FFSOLDIERAI_CPP
class CFFPlayer { public: CFFPlayer(edict_t* ed=nullptr) : m_pEdict(ed) {} edict_t* GetEdict() const { return m_pEdict; } bool IsValid() const {return m_pEdict != nullptr;} bool IsAlive() const { return true; } Vector GetPosition() const { return m_CurrentPosition_placeholder; } Vector GetEyePosition() const { return Vector(m_CurrentPosition_placeholder.x, m_CurrentPosition_placeholder.y, m_CurrentPosition_placeholder.z + 64); } Vector GetViewAngles() const { return m_CurrentViewAngles_placeholder; } void SetViewAngles(const Vector& ang) { m_CurrentViewAngles_placeholder = ang; } int GetTeamNumber() const { return 1; } int GetFlags() const { return (1 << 0); } int GetActiveWeaponId_Placeholder() const { return WEAPON_ID_SOLDIER_ROCKETLAUNCHER; } int GetAmmoCount_Placeholder(int type) const {return 10;} Vector m_CurrentPosition_placeholder; Vector m_CurrentViewAngles_placeholder; private: edict_t* m_pEdict; };
#endif
// --- End Conceptual Placeholders ---


// Conceptual constants for Soldier
const float SOLDIER_MIN_ROCKET_SPLASH_DIST_SQ = 70.0f * 70.0f;  // Min distance to fire rockets to avoid self-damage by splash
const float SOLDIER_IDEAL_ROCKET_ENGAGEMENT_RANGE_MIN_SQ = 200.0f * 200.0f;
const float SOLDIER_IDEAL_ROCKET_ENGAGEMENT_RANGE_MAX_SQ = 1200.0f * 1200.0f;
const float SOLDIER_SHOTGUN_EFFECTIVE_RANGE_SQ = 800.0f * 800.0f;
const float SOLDIER_MELEE_RANGE_SQ = 90.0f * 90.0f; // Generous melee
const float SOLDIER_ROCKET_PROJECTILE_SPEED = 1100.0f; // HU/s (TF2 default)
const float SOLDIER_ROCKET_JUMP_HEALTH_THRESHOLD = 50;
const float SOLDIER_ROCKET_JUMP_PITCH_LOOKDOWN = 55.0f; // Degrees to look down for a basic jump.
const float SOLDIER_RJ_STATE_TIMEOUT_SHORT = 0.3f; // For quick states like aiming/crouching
const float SOLDIER_RJ_STATE_TIMEOUT_FLIGHT = 3.0f; // Max time in IN_AIR state


CSoldierAI_FF::CSoldierAI_FF(CFFPlayer* pBotPlayer, CObjectivePlanner* pPlanner,
                           const BotKnowledgeBase* pKnowledgeBase, const ClassConfigInfo* pClassConfig)
    : CFFBaseAI(pBotPlayer, pPlanner, pKnowledgeBase, pClassConfig),
      m_eRocketJumpState(RJState::NONE),
      m_flRocketJumpStateStartTime(0.0f)
{
    // std::cout << "CSoldierAI_FF created." << std::endl;
}

CSoldierAI_FF::~CSoldierAI_FF() {}

CBaseEntity* CSoldierAI_FF::SelectTarget() {
    if (!m_pKnowledgeBase || !m_pBotPlayer) return nullptr;

    CBaseEntity* pBestTarget = nullptr;
    float highestScore = -1.0f;
    Vector myPos = m_pBotPlayer->GetPosition();

    // Conceptual: Iterate m_pKnowledgeBase->GetTrackedEnemies()
    // For now, let's assume we have a list of potential CBaseEntity* targets
    // std::vector<CBaseEntity*> potentialTargets = GetVisibleEnemiesFromKB();

    // --- Placeholder: Create a dummy enemy for testing logic flow ---
    static CBaseEntity dummyEnemy;
    // dummyEnemy.SetPosition(myPos + Vector(300, 50, 0)); // Conceptual SetPosition
    // dummyEnemy.SetHealth(100); dummyEnemy.SetTeam(m_pBotPlayer->GetTeamNumber() == 2 ? 3 : 2);
    std::vector<CBaseEntity*> potentialTargets = {&dummyEnemy};
    if (m_pBotPlayer->GetTeamNumber() == 0) potentialTargets.clear(); // Don't attack if bot team unknown
    // --- End Placeholder ---


    for (CBaseEntity* pEnemy : potentialTargets) {
        if (!pEnemy || !pEnemy->IsAlive() /*|| pEnemy->IsInvulnerable() */ ) continue;

        Vector enemyPos = pEnemy->GetPosition();
        float distSq = (myPos - enemyPos).LengthSquared();
        float currentScore = 1000.0f / (1.0f + sqrt(distSq)); // Base score on distance

        // if (pEnemy->IsSentryGun()) { // Conceptual
        //     currentScore += 500.0f;
        // } else if (pEnemy->IsPlayer()) {
        //     if (pEnemy->IsMedicClass()) currentScore += 200.0f; // Conceptual IsMedicClass
        //     if (pEnemy->IsHeavyClass()) currentScore += 150.0f;
        //     // Conceptual: Check for clustered enemies around pEnemy for splash bonus
        //     // int nearbyEnemies = CountEnemiesNearPoint(enemyPos, ROCKET_SPLASH_RADIUS);
        //     // currentScore += nearbyEnemies * 50.0f;
        // }
        // if (pEnemy == m_pBotPlayer->GetLastAttacker()) currentScore += 100.0f; // Conceptual

        if (currentScore > highestScore) {
            highestScore = currentScore;
            pBestTarget = pEnemy;
        }
    }
    m_pCurrentTarget = pBestTarget; // Update the base class's current target
    return pBestTarget;
}

bool CSoldierAI_FF::AttackTarget(CBaseEntity* pTarget, CUserCmd* pCmd) {
    if (!pTarget || !pTarget->IsAlive() || !m_pBotPlayer || !m_pBotPlayer->IsValid() || !m_pBotPlayer->IsAlive() || !pCmd) {
        m_pCurrentTarget = nullptr;
        return false;
    }
    m_pCurrentTarget = pTarget; // Ensure current target is set

    Vector myPos = m_pBotPlayer->GetPosition();
    Vector targetPos = pTarget->GetPosition(); // Use GetPosition for simpler prediction base
    float distSq = (myPos - targetPos).LengthSquared();

    // Conceptual: Check available weapons and ammo from CFFPlayer or ClassConfigInfo
    bool hasRockets = true; // m_pBotPlayer->GetAmmoCount_Placeholder(AMMO_TYPE_ROCKET) > 0;
    bool hasShotgunAmmo = true; // m_pBotPlayer->GetAmmoCount_Placeholder(AMMO_TYPE_SHOTGUN_PRIMARY) > 0;
    int activeWeaponId = m_pBotPlayer->GetActiveWeaponId_Placeholder();

    int desiredWeaponId = WEAPON_ID_SOLDIER_MELEE; // Default

    if (hasRockets && distSq > SOLDIER_MIN_ROCKET_SPLASH_DIST_SQ && distSq < SOLDIER_IDEAL_ROCKET_ENGAGEMENT_RANGE_MAX_SQ) {
        desiredWeaponId = WEAPON_ID_SOLDIER_ROCKETLAUNCHER;
    } else if (hasShotgunAmmo && distSq < SOLDIER_SHOTGUN_EFFECTIVE_RANGE_SQ) {
        desiredWeaponId = WEAPON_ID_SOLDIER_SHOTGUN;
    } else if (distSq < SOLDIER_MELEE_RANGE_SQ) {
        desiredWeaponId = WEAPON_ID_SOLDIER_MELEE;
    } else if (hasShotgunAmmo) { // Fallback to shotgun if out of ideal RL range but too far for melee
        desiredWeaponId = WEAPON_ID_SOLDIER_SHOTGUN;
    } else if (hasRockets) { // Fallback to RL even if too close, if it's the only option left
        desiredWeaponId = WEAPON_ID_SOLDIER_ROCKETLAUNCHER;
    }

    SwitchToWeapon(desiredWeaponId, pCmd);
    // if (activeWeaponId != desiredWeaponId && pCmd->weaponselect == desiredWeaponId) {
    //     return true; // Weapon is switching, task is ongoing
    // }
    // if (activeWeaponId != desiredWeaponId) return true; // Still waiting for switch if not immediate

    Vector aimPos = targetPos; // Default aim position
    if (desiredWeaponId == WEAPON_ID_SOLDIER_ROCKETLAUNCHER) {
        aimPos = PredictTargetPosition(pTarget, SOLDIER_ROCKET_PROJECTILE_SPEED);
        // Conceptual: Aim for feet/splash
        // if (pTarget->IsOnGround() && !pTarget->IsSentryGun()) { // Assuming IsOnGround can be checked
        //     Vector enemyOrigin = pTarget->GetPosition(); // Get bottom origin
        //     Vector predictedOrigin = PredictTargetPosition(pTarget, SOLDIER_ROCKET_PROJECTILE_SPEED);
        //     predictedOrigin.z = enemyOrigin.z; // Aim at feet level of predicted X/Y
        //     aimPos = predictedOrigin;
        // }
    } else { // Shotgun or Melee (hitscan or near-hitscan)
        aimPos = PredictTargetPosition(pTarget, 9999.0f); // Effectively instant for hitscan prediction
        if (desiredWeaponId == WEAPON_ID_SOLDIER_MELEE && distSq > SOLDIER_MELEE_RANGE_SQ * 1.5f*1.5f) {
             // Too far to melee, even if selected as last resort. Don't attack.
             // Might want to move closer if this is the only weapon.
             return true; // Ongoing, but not attacking this frame.
        }
    }

    AimAt(aimPos, pCmd);
    // Conceptual: if (m_pBotPlayer->IsAimReady() && m_pBotPlayer->HasLOS(pTarget))
    pCmd->buttons |= IN_ATTACK;
    // std::cout << "Soldier attacking with weapon " << desiredWeaponId << std::endl;
    return true;
}

bool CSoldierAI_FF::UseAbility(int abilitySlot, CBaseEntity* pTarget, CUserCmd* pCmd) {
    if (!pCmd) return false;
    // Assuming abilitySlot 0 for Soldier is a context-based Rocket Jump
    if (abilitySlot == 0) {
        Vector jumpTarget = m_vCurrentMoveToTarget; // Use current movement goal as general direction/target
        bool isDirection = false; // Assume it's a target point unless specified
        if (pTarget) { // If a specific target entity is provided for the ability
            jumpTarget = pTarget->GetPosition();
        } else if (m_CurrentPath_NavAreaIDs.empty() && m_pObjectivePlanner && m_pObjectivePlanner->GetCurrentHighLevelTask()) {
            // If no path, but HLT exists, jump towards HLT target
            jumpTarget = m_pObjectivePlanner->GetCurrentHighLevelTask()->targetPosition;
        }
        // If jumpTarget is still zero or bot's current pos, pick a forward direction
        if ((jumpTarget.x ==0 && jumpTarget.y==0 && jumpTarget.z==0) || (m_pBotPlayer && (jumpTarget - m_pBotPlayer->GetPosition()).LengthSquared() < 1.0f) ) {
             if (m_pBotPlayer) {
                Vector forward;
                float pitch = m_pBotPlayer->GetViewAngles().x * M_PI / 180.0f;
                float yaw = m_pBotPlayer->GetViewAngles().y * M_PI / 180.0f;
                forward.x = cos(yaw) * cos(pitch);
                forward.y = sin(yaw) * cos(pitch);
                forward.z = -sin(pitch); // Should be GetForwardVector from angles
                jumpTarget = forward;
             } else {
                jumpTarget = Vector(1,0,0); // Absolute forward if no player data
             }
             isDirection = true;
        }
        return AttemptRocketJump(jumpTarget, isDirection, pCmd);
    }
    return false;
}

bool CSoldierAI_FF::AttemptRocketJump(const Vector& targetOrDirection, bool isDirection, CUserCmd* pCmd) {
    if (!m_pBotPlayer || !m_pBotPlayer->IsValid() || !pCmd) {
        m_eRocketJumpState = RJState::FAILED; return false;
    }
    // Conceptual checks:
    // if (m_pBotPlayer->GetHealth() < SOLDIER_ROCKET_JUMP_HEALTH_THRESHOLD) { m_eRocketJumpState = RJState::FAILED; return false; }
    // if (m_pBotPlayer->GetActiveWeaponId_Placeholder() != WEAPON_ID_SOLDIER_ROCKETLAUNCHER || m_pBotPlayer->GetAmmoCount_Placeholder(0) == 0) {
    //     SwitchToWeapon(WEAPON_ID_SOLDIER_ROCKETLAUNCHER, pCmd); // Attempt to switch
    //     m_eRocketJumpState = RJState::NONE; // Reset to try again after switch
    //     return true; // Ongoing (waiting for switch)
    // }
    // if (!m_pBotPlayer->IsOnGround() && m_eRocketJumpState != RJState::IN_AIR) {
    //     m_eRocketJumpState = RJState::FAILED; return false; // Must be on ground to initiate typical jump
    // }

    float currentTime = 0.0f; // Conceptual: GetCurrentWorldTime();

    switch (m_eRocketJumpState) {
        case RJState::NONE: // Start new jump sequence
        case RJState::FAILED:
        case RJState::COMPLETED:
            m_eRocketJumpState = RJState::AIMING_DOWN;
            m_flRocketJumpStateStartTime = currentTime;
            // Fallthrough
        case RJState::AIMING_DOWN: {
            Vector botPos = m_pBotPlayer->GetPosition();
            Vector viewAngles = m_pBotPlayer->GetViewAngles();
            Vector aimTargetPos = botPos; // Base for aiming calculation

            // For a simple jump, just aim down. For directed, aim at feet offset by direction.
            if (isDirection) { // targetOrDirection is a direction vector
                // Aim slightly behind if moving forward, or directly down for vertical.
                // This simplified version just aims down.
                aimTargetPos.x -= targetOrDirection.x * 10.0f; // small offset behind if dir is fwd
                aimTargetPos.y -= targetOrDirection.y * 10.0f;
                aimTargetPos.z -= 100.0f; // look down
            } else { // targetOrDirection is an apex/destination
                 // Aim at feet, generally towards target apex to get initial momentum
                 Vector dirToApex = targetOrDirection;
                 dirToApex.x -= botPos.x; dirToApex.y -= botPos.y; //dirToApex.z -= botPos.z;
                 dirToApex.z = 0; // Make it 2D for now
                 float len = sqrt(dirToApex.x*dirToApex.x + dirToApex.y*dirToApex.y);
                 if(len > 0.1f) { dirToApex.x/=len; dirToApex.y/=len;} else {dirToApex.x=1; dirToApex.y=0;}

                aimTargetPos.x += dirToApex.x * 10.0f; // aim slightly "in front" of feet towards target
                aimTargetPos.y += dirToApex.y * 10.0f;
                aimTargetPos.z -= 100.0f; // look down
            }

            // This sets pCmd->viewangles
            CFFBaseAI::AimAt(aimTargetPos, pCmd); // Use base class AimAt to set viewangles towards the calculated point
            // Override pitch directly for more reliable downward aim for jump
            pCmd->viewangles.x = SOLDIER_ROCKET_JUMP_PITCH_LOOKDOWN;

            if (currentTime - m_flRocketJumpStateStartTime > SOLDIER_RJ_STATE_TIMEOUT_SHORT) {
                m_eRocketJumpState = RJState::CROUCHING;
                m_flRocketJumpStateStartTime = currentTime;
            }
            break;
        }
        case RJState::CROUCHING:
            pCmd->buttons |= IN_DUCK;
            // Also ensure aiming is still correct from previous state
            CFFBaseAI::AimAt(m_pBotPlayer->GetPosition() + Vector(0,0,-100), pCmd); // Keep aiming down
            pCmd->viewangles.x = SOLDIER_ROCKET_JUMP_PITCH_LOOKDOWN;

            if (currentTime - m_flRocketJumpStateStartTime > SOLDIER_RJ_STATE_TIMEOUT_SHORT /* || m_pBotPlayer->IsDucked() */) {
                m_eRocketJumpState = RJState::JUMP_FIRE;
                m_flRocketJumpStateStartTime = currentTime;
            }
            break;
        case RJState::JUMP_FIRE:
            pCmd->buttons |= IN_DUCK;   // Hold duck
            pCmd->buttons |= IN_JUMP;   // Jump
            pCmd->buttons |= IN_ATTACK; // Fire
            // Aiming should have been set in previous states and held by bot's view inertia or AimAt again
            CFFBaseAI::AimAt(m_pBotPlayer->GetPosition() + Vector(0,0,-100), pCmd);
            pCmd->viewangles.x = SOLDIER_ROCKET_JUMP_PITCH_LOOKDOWN;
            // std::cout << "Soldier: RJ - JUMP+FIRE! Buttons: " << pCmd->buttons << std::endl;
            m_eRocketJumpState = RJState::IN_AIR;
            m_flRocketJumpStateStartTime = currentTime;
            return false; // Action performed, let physics take over for next frame. Task isn't "done" yet.

        case RJState::IN_AIR:
            // Optional: Air strafe towards targetOrDirection if it's an apex
            // For now, just wait to land or timeout
            if (/* m_pBotPlayer->IsOnGround() || */ currentTime - m_flRocketJumpStateStartTime > SOLDIER_RJ_STATE_TIMEOUT_FLIGHT) {
                // std::cout << "Soldier: RJ - Landed or flight timeout." << std::endl;
                m_eRocketJumpState = RJState::COMPLETED;
                return false; // RJ sequence finished (successfully or not)
            }
            break; // Still in air, ongoing

        default: // Includes COMPLETED, FAILED
            m_eRocketJumpState = RJState::NONE; // Reset for next time
            return false; // Sequence is done or failed
    }
    return true; // Task sequence is ongoing
}

void CSoldierAI_FF::SwitchToWeapon(int weaponId, CUserCmd* pCmd) {
    if (!pCmd) return;
    // Conceptual
    // if (m_pBotPlayer && m_pBotPlayer->GetActiveWeaponId_Placeholder() != weaponId) {
    //    pCmd->weaponselect = weaponId; // Engine specific way to request weapon switch
    // }
}

Vector CSoldierAI_FF::PredictTargetPosition(CBaseEntity* pTarget, float projectileSpeed) const {
    if (!pTarget || !m_pBotPlayer) return pTarget ? pTarget->GetPosition() : Vector();

    Vector targetPos = pTarget->GetPosition(); // Current position
    if (projectileSpeed <= 0.001f) return targetPos; // Hitscan or invalid speed

    Vector targetVel = pTarget->GetAbsVelocity(); // Conceptual
    Vector myPos = m_pBotPlayer->GetEyePosition(); // Use eye position for aiming origin

    float dist = (targetPos.x-myPos.x)*(targetPos.x-myPos.x) + (targetPos.y-myPos.y)*(targetPos.y-myPos.y) + (targetPos.z-myPos.z)*(targetPos.z-myPos.z);
    dist = sqrt(dist);
    float timeToTarget = dist / projectileSpeed;

    return Vector(targetPos.x + targetVel.x * timeToTarget,
                  targetPos.y + targetVel.y * timeToTarget,
                  targetPos.z + targetVel.z * timeToTarget); // Simple linear prediction
}
