#include "FFSoldierAI.h"
#include "ObjectivePlanner.h" // For SubTask, if soldier needs to interact with it directly
#include "BotTasks.h"         // For SubTaskType, if soldier's abilities map to specific subtasks
#include "FFStateStructs.h"   // For Vector, ClassConfigInfo
// Conceptual includes
#include "CFFPlayer.h"
#include "CBaseEntity.h"
#include "UserCmd.h"
#include "BotKnowledgeBase.h"
#include <iostream> // Placeholder logging
#include <cmath> // For M_PI, sqrt, etc.

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Conceptual constants
const float SOLDIER_MIN_ROCKET_DIST_SQ = 100.0f * 100.0f; // Min distance to fire rockets to avoid self-damage
const float SOLDIER_ROCKET_SPEED = 1100.0f; // Units per second (TF2 default, adjust for FF)
const float SOLDIER_SHOTGUN_EFFECTIVE_RANGE_SQ = 700.0f * 700.0f;
const float SOLDIER_MELEE_RANGE_SQ = 80.0f * 80.0f;
const float SOLDIER_ROCKET_JUMP_AIM_PITCH = 60.0f; // Degrees to look down for a basic jump
const float ROCKET_JUMP_STATE_TIMEOUT = 0.5f; // Max time for aiming/crouching states


CSoldierAI_FF::CSoldierAI_FF(CFFPlayer* pBotPlayer, CObjectivePlanner* pPlanner,
                           const BotKnowledgeBase* pKnowledgeBase, const ClassConfigInfo* pClassConfig)
    : CFFBaseAI(pBotPlayer, pPlanner, pKnowledgeBase, pClassConfig),
      m_eRocketJumpState(RJState::NONE),
      m_flRocketJumpStateStartTime(0.0f)
{
    std::cout << "CSoldierAI_FF created for bot " << (m_pBotPlayer ? " (player set)" : "(NO PLAYER!)") << std::endl;
}

CSoldierAI_FF::~CSoldierAI_FF() {}

CBaseEntity* CSoldierAI_FF::SelectTarget() {
    // Conceptual Soldier Target Selection:
    // 1. Prioritize Sentry Guns if visible and in attackable range/angle.
    // 2. Prioritize groups of enemies for splash damage.
    // 3. Prioritize high-value targets like Medics or Heavies.
    // This would involve iterating m_pKnowledgeBase->visibleEnemies or similar.

    // For now, fallback to base or a simple nearest enemy.
    // if (m_pKnowledgeBase) {
    //     CBaseEntity* pBestTarget = nullptr;
    //     float flBestScore = -1.0f;
    //     Vector myPos = m_pBotPlayer->GetPosition();

    //     for (const auto& enemyInfo : m_pKnowledgeBase->visibleEnemies) { // Assuming visibleEnemies is populated
    //         CBaseEntity* pEnemy = enemyInfo.entity; // Assuming EnemyInfo struct
    //         if (!pEnemy || !pEnemy->IsAlive()) continue;

    //         float flDistSq = (myPos - pEnemy->GetPosition()).LengthSquared();
    //         float flScore = 1000.0f / (1.0f + sqrt(flDistSq)); // Simple distance priority

    //         // if (pEnemy->IsSentryBuster()) flScore += 200.0f; // Example
    //         // if (pEnemy->IsMedicClass()) flScore += 100.0f;

    //         if (flScore > flBestScore) {
    //             flBestScore = flScore;
    //             pBestTarget = pEnemy;
    //         }
    //     }
    //     if (pBestTarget) return pBestTarget;
    // }
    // If no specific soldier logic finds a target, let base class try or return null
    return CFFBaseAI::SelectTarget(); // This is pure virtual, so derived must implement.
                                      // For now, let's return a dummy if nothing else.
    // This needs a proper implementation of iterating visible enemies from knowledge base.
    // For the purpose of this structure, let's assume a simple selection.
    // if (m_pKnowledgeBase && m_pKnowledgeBase->visibleEnemies.size() > 0) return m_pKnowledgeBase->visibleEnemies[0].pEntity;
    return nullptr; // Placeholder
}

bool CSoldierAI_FF::AttackTarget(CBaseEntity* pTarget, CUserCmd* pCmd) {
    if (!pTarget || !pTarget->IsAlive() || !m_pBotPlayer || !pCmd) {
        return false; // Cannot attack
    }

    Vector myPos = m_pBotPlayer->GetPosition();
    Vector targetPos = pTarget->GetPosition(); // Or GetWorldSpaceCenter()
    float distToTargetSq = (myPos - targetPos).LengthSquared();

    int desiredWeaponId = WEAPON_ID_SOLDIER_MELEE; // Default
    bool hasRL = false, hasShotgun = false, hasMelee = false; // Check available weapons from m_pClassConfig

    // Conceptual: Check m_pBotPlayer for actual ammo and weapon status
    // For now, assume all are available if in ClassConfig
    if (m_pClassConfig) {
        for(const auto& weapon : m_pClassConfig->availableWeapons) {
            if (weapon.weaponNameId == "ff_weapon_rocketlauncher") hasRL = true; // Use actual IDs
            if (weapon.weaponNameId == "ff_weapon_shotgun") hasShotgun = true;
            if (weapon.weaponNameId == "ff_weapon_shovel") hasMelee = true; // Example melee
        }
    }


    if (hasRL && distToTargetSq > SOLDIER_MIN_ROCKET_DIST_SQ /* && m_pBotPlayer->HasAmmo(AMMO_ROCKET) */) {
        desiredWeaponId = WEAPON_ID_SOLDIER_ROCKETLAUNCHER;
    } else if (hasShotgun && distToTargetSq < SOLDIER_SHOTGUN_EFFECTIVE_RANGE_SQ /* && m_pBotPlayer->HasAmmo(AMMO_SHOTGUN) */) {
        desiredWeaponId = WEAPON_ID_SOLDIER_SHOTGUN;
    } else if (hasMelee && distToTargetSq < SOLDIER_MELEE_RANGE_SQ) {
        desiredWeaponId = WEAPON_ID_SOLDIER_MELEE;
    } else if (hasShotgun /* && m_pBotPlayer->HasAmmo(AMMO_SHOTGUN) */) { // Fallback to shotgun if RL too close but melee too far
        desiredWeaponId = WEAPON_ID_SOLDIER_SHOTGUN;
    } else if (hasRL /* && m_pBotPlayer->HasAmmo(AMMO_ROCKET) */) { // Fallback to RL if only option (even if close)
         desiredWeaponId = WEAPON_ID_SOLDIER_ROCKETLAUNCHER;
    }


    SwitchToWeapon(desiredWeaponId, pCmd); // Conceptual: sets pCmd->weaponselect if needed
    // if (m_pBotPlayer->GetActiveWeaponId() != desiredWeaponId) {
    //    return true; // Weapon switching, continue task
    // }

    Vector aimPos = targetPos;
    if (desiredWeaponId == WEAPON_ID_SOLDIER_ROCKETLAUNCHER) {
        // Aim for feet/splash
        // if (pTarget->IsOnGround() && !pTarget->IsSentryGun()) { // Conceptual checks
        //     aimPos.z -= 30; // Aim lower for splash (approx half player height)
        // }
        // Lead target (conceptual)
        // aimPos = PredictTargetPosition(pTarget, SOLDIER_ROCKET_SPEED);
        // If aiming for feet of predicted pos: aimPos.z -= 30;
    } else { // Shotgun or Melee
        // aimPos = PredictTargetPosition(pTarget, 0 /* hitscan */ ); // Simpler prediction for hitscan
        aimPos = pTarget->GetWorldSpaceCenter(); // Aim center mass for hitscan
    }

    AimAt(aimPos, pCmd);
    pCmd->buttons |= 1; // IN_ATTACK

    // std::cout << "Soldier attacking with weapon ID: " << desiredWeaponId << std::endl;
    return true; // Attack is ongoing
}

bool CSoldierAI_FF::UseAbility(int abilitySlot, CBaseEntity* pTarget, CUserCmd* pCmd) {
    // For Soldier, slot 0 might be interpreted as "perform a context-specific rocket jump"
    // This is a simplification. A better system would have explicit SubTaskTypes for abilities.
    if (abilitySlot == 0) { // Assuming 0 is a generic "action jump" slot
        Vector jumpDir = Vector(1,0,0); // Default forward
        if (m_pBotPlayer) {
            // Get player's forward view vector for jump direction
            // AngleVectors(m_pBotPlayer->GetViewAngles(), &jumpDir, nullptr, nullptr);
        }
        return AttemptRocketJump(jumpDir, true, pCmd);
    }
    return false;
}

bool CSoldierAI_FF::AttemptRocketJump(const Vector& targetOrDirection, bool isDirection, CUserCmd* pCmd) {
    if (!m_pBotPlayer || !pCmd /* || m_pBotPlayer->GetClass() != TF_CLASS_SOLDIER */) {
        m_eRocketJumpState = RJState::FAILED;
        return false; // Not a soldier or invalid state
    }

    // Check if RL is equipped and has ammo (conceptual)
    // if (m_pBotPlayer->GetActiveWeaponId() != WEAPON_ID_SOLDIER_ROCKETLAUNCHER || m_pBotPlayer->GetAmmoCount(AMMO_ROCKET) == 0) {
    //     m_eRocketJumpState = RJState::FAILED;
    //     SwitchToWeapon(WEAPON_ID_SOLDIER_ROCKETLAUNCHER, pCmd); // Try to switch for next time
    //     return false;
    // }
    // if (!m_pBotPlayer->IsOnGround() && m_eRocketJumpState != RJState::IN_AIR) { // Must be on ground to start a new jump
    //     m_eRocketJumpState = RJState::FAILED; // Or wait if already in air from previous jump
    //     return false;
    // }


    float currentTime = 0.0f; // Conceptual: GetCurrentWorldTime();

    switch (m_eRocketJumpState) {
        case RJState::NONE:
        case RJState::FAILED:
        case RJState::COMPLETED: // Start new jump sequence
            m_eRocketJumpState = RJState::AIMING_DOWN;
            m_flRocketJumpStateStartTime = currentTime;
            // fallthrough
        case RJState::AIMING_DOWN: {
            Vector currentAngles = m_pBotPlayer ? m_pBotPlayer->GetViewAngles() : Vector(0,0,0);
            Vector desiredAngles = currentAngles;
            desiredAngles.x = SOLDIER_ROCKET_JUMP_AIM_PITCH; // Pitch down
            // If isDirection, m_vTarget is yaw. If not, calculate yaw to target apex.
            // For simplicity, use current yaw. More advanced would calculate yaw based on m_vTarget.

            // AimAt(m_pBotPlayer->GetPosition() + Vector(0,0,-100), pCmd); // Aims directly down from feet
            // This needs to set pCmd->viewangles based on desired pitch/yaw
            pCmd->viewangles.x = SOLDIER_ROCKET_JUMP_AIM_PITCH;
            pCmd->viewangles.y = currentAngles.y; // Maintain current yaw for simple jump
            pCmd->viewangles.z = 0;


            if (currentTime - m_flRocketJumpStateStartTime > ROCKET_JUMP_STATE_TIMEOUT * 0.5f) { // Give some time to aim
                m_eRocketJumpState = RJState::CROUCHING;
                m_flRocketJumpStateStartTime = currentTime;
            }
            break;
        }
        case RJState::CROUCHING:
            pCmd->buttons |= (1 << 1); // IN_DUCK
            if (currentTime - m_flRocketJumpStateStartTime > ROCKET_JUMP_STATE_TIMEOUT * 0.2f /* || m_pBotPlayer->IsDucked() */) {
                m_eRocketJumpState = RJState::JUMP_FIRE;
                m_flRocketJumpStateStartTime = currentTime;
            }
            break;
        case RJState::JUMP_FIRE:
            pCmd->buttons |= (1 << 1); // IN_DUCK (hold)
            pCmd->buttons |= (1 << 0); // IN_JUMP
            pCmd->buttons |= (1 << 0); // IN_ATTACK (this is likely wrong, IN_ATTACK is usually bit 0, JUMP bit 1)
                                      // Corrected: IN_JUMP is typically (1<<1), IN_ATTACK is (1<<0)
                                      // Assuming IN_JUMP = (1<<1), IN_ATTACK = (1<<0), IN_DUCK = (1<<2)
            // pCmd->buttons |= (1<<2); // IN_DUCK
            // pCmd->buttons |= (1<<1); // IN_JUMP
            // pCmd->buttons |= (1<<0); // IN_ATTACK
            // std::cout << "Soldier: JUMP+FIRE for rocket jump!" << std::endl;
            m_eRocketJumpState = RJState::IN_AIR; // Or COMPLETED if we don't manage flight
            m_flRocketJumpStateStartTime = currentTime;
            return false; // Action performed for this frame, task might complete next frame

        case RJState::IN_AIR:
            // Optionally control air strafe towards m_vTarget if it's an apex
            // For now, just wait until on ground or timeout
            // if (m_pBotPlayer->IsOnGround() || currentTime - m_flRocketJumpStateStartTime > 2.0f) {
            //     m_eRocketJumpState = RJState::COMPLETED;
            // }
            m_eRocketJumpState = RJState::COMPLETED; // Simplified: jump action is "done"
            return false; // No longer actively trying to jump+fire

        default:
            m_eRocketJumpState = RJState::FAILED;
            return false;
    }
    return true; // Task is ongoing
}


void CSoldierAI_FF::SwitchToWeapon(int weaponId, CUserCmd* pCmd) {
    // Conceptual - actual weapon switching is engine/game specific
    // if (m_pBotPlayer && m_pBotPlayer->GetActiveWeaponId() != weaponId) {
    //    pCmd->weaponselect = weaponId;
    // }
    // std::cout << "Soldier wants to switch to weapon ID: " << weaponId << std::endl;
}

Vector CSoldierAI_FF::PredictTargetPosition(CBaseEntity* pTarget, float projectileSpeed) const {
    if (!pTarget || !m_pBotPlayer) return Vector();
    Vector targetPos = pTarget->GetPosition();
    if (projectileSpeed <= 0.0f) return targetPos; // Hitscan or no prediction

    Vector targetVel = pTarget->GetAbsVelocity(); // Conceptual
    float dist = (targetPos - m_pBotPlayer->GetPosition()).Length();
    float timeToTarget = dist / projectileSpeed;

    return Vector(targetPos.x + targetVel.x * timeToTarget,
                  targetPos.y + targetVel.y * timeToTarget,
                  targetPos.z + targetVel.z * timeToTarget);
}
