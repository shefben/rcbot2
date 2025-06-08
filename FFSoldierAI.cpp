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

// SDK Includes (Conceptual - these would come from actual SDK headers)
// #include "game/shared/usercmd.h"
// #include "game/server/cbase.h"      // For CBaseEntity
// #include "CFFPlayer.h" // Now CFFPlayerWrapper.h
#include "CFFPlayer.h" // Will be CFFPlayerWrapper.h
#include "BotKnowledgeBase.h"
#include "FFBot_SDKDefines.h" // For WEAPON_ID_ and AMMO_ID_ mappings, and SDK CLASS/TEAM defines
#include "game/shared/in_buttons.h" // For IN_ flags
#include "public/const.h"           // For FL_ flags


// Conceptual constants for Soldier (some might become part of ClassConfigInfo)
const float SOLDIER_MIN_ROCKET_SPLASH_DIST_SQ = 70.0f * 70.0f;
const float SOLDIER_IDEAL_ROCKET_ENGAGEMENT_RANGE_MIN_SQ = 200.0f * 200.0f;
const float SOLDIER_IDEAL_ROCKET_ENGAGEMENT_RANGE_MAX_SQ = 1200.0f * 1200.0f;
const float SOLDIER_SHOTGUN_EFFECTIVE_RANGE_SQ = 800.0f * 800.0f;
const float SOLDIER_MELEE_RANGE_SQ = 90.0f * 90.0f;
const float SOLDIER_ROCKET_PROJECTILE_SPEED = 1100.0f; // HU/s (Example, get from ClassConfigInfo or BotDefines)
const float SOLDIER_ROCKET_JUMP_HEALTH_THRESHOLD = 50;
const float SOLDIER_ROCKET_JUMP_PITCH_LOOKDOWN = 55.0f;
const float SOLDIER_RJ_STATE_TIMEOUT_SHORT = 0.3f;
const float SOLDIER_RJ_STATE_TIMEOUT_FLIGHT = 3.0f;


CSoldierAI_FF::CSoldierAI_FF(CFFPlayerWrapper* pBotPlayer, CObjectivePlanner* pPlanner,
                           BotKnowledgeBase* pKnowledgeBase, const ClassConfigInfo* pClassConfig) // Updated types
    : CFFBaseAI(pBotPlayer, pPlanner, pKnowledgeBase, pClassConfig),
      m_eRocketJumpState(RJState::NONE),
      m_flRocketJumpStateStartTime(0.0f)
{
    // std::cout << "CSoldierAI_FF created." << std::endl;
}

CSoldierAI_FF::~CSoldierAI_FF() {}

CBaseEntity* CSoldierAI_FF::SelectTarget() {
    // if (!m_pKnowledgeBase || !m_pBotPlayer) return nullptr;
    // SetCurrentTarget(nullptr); // Use CFFBaseAI's SetCurrentTarget

    // CBaseEntity* pBestTarget = nullptr;
    // float highestScore = -1.0f;
    // Vector myPos = m_pBotPlayer->GetOrigin(); // SDK Call

    // const auto& enemies = m_pKnowledgeBase->GetTrackedEnemies();
    // for (const auto& enemyInfo : enemies) {
    //     if (!enemyInfo.pEdict) continue; // Should have an edict if valid
    //     CBaseEntity* pEnemy = CBaseEntity::Instance(enemyInfo.pEdict); // SDK Call
    //     if (!pEnemy || !pEnemy->IsAlive()) continue;

    //     Vector enemyPos = pEnemy->GetAbsOrigin(); // SDK Call
    //     float distSq = myPos.DistToSqr(enemyPos);
    //     float currentScore = 1000.0f / (1.0f + sqrt(distSq));

        // Prioritize based on class, health, etc. (conceptual logic from before)
        // ...

    //     if (currentScore > highestScore) {
    //         highestScore = currentScore;
    //         pBestTarget = pEnemy;
    //     }
    // }
    // SetCurrentTarget(pBestTarget);
    // return GetCurrentTarget();
    return CFFBaseAI::SelectTarget(); // Fallback to base or more generic selection for now
}

bool CSoldierAI_FF::AttackTarget(CBaseEntity* pTarget, CUserCmd* pCmd) {
    // if (!pTarget || !pTarget->IsAlive() || !m_pBotPlayer || !m_pBotPlayer->IsValid() || !m_pBotPlayer->IsAlive() || !pCmd) {
    //     SetCurrentTarget(nullptr);
    //     return false;
    // }
    // SetCurrentTarget(pTarget);

    // Vector myPos = m_pBotPlayer->GetOrigin(); // SDK Call
    // Vector targetPos = pTarget->GetAbsOrigin(); // SDK Call
    // float distSq = myPos.DistToSqr(targetPos);

    // bool hasRockets = m_pBotPlayer->GetAmmo(AMMO_ROCKETS_NAME) > 0; // AMMO_ROCKETS_NAME from ff_weapon_base.h
    // bool hasShotgunAmmo = m_pBotPlayer->GetAmmo(AMMO_SHELLS_NAME) > 0; // AMMO_SHELLS_NAME

    // std::string desiredWeaponClassname = "ff_weapon_shovel"; // Default to melee

    // if (hasRockets && distSq > SOLDIER_MIN_ROCKET_SPLASH_DIST_SQ && distSq < SOLDIER_IDEAL_ROCKET_ENGAGEMENT_RANGE_MAX_SQ) {
    //     desiredWeaponClassname = "ff_weapon_rocketlauncher";
    // } else if (hasShotgunAmmo && distSq < SOLDIER_SHOTGUN_EFFECTIVE_RANGE_SQ) {
    //     desiredWeaponClassname = "ff_weapon_shotgun_soldier";
    // } else if (distSq < SOLDIER_MELEE_RANGE_SQ) {
    //     desiredWeaponClassname = "ff_weapon_shovel";
    // } else if (hasShotgunAmmo) {
    //     desiredWeaponClassname = "ff_weapon_shotgun_soldier";
    // } else if (hasRockets) {
    //     desiredWeaponClassname = "ff_weapon_rocketlauncher";
    // }

    // m_pBotPlayer->SelectWeapon(pCmd, desiredWeaponClassname);
    // Conceptual: Add delay for weapon switch if (m_pBotPlayer->IsSwitchingWeapon_Conceptual()) return true;

    // Vector aimPos = targetPos;
    // if (desiredWeaponClassname == "ff_weapon_rocketlauncher") {
    //     float rocketSpeed = m_pClassConfig ? m_pClassConfig->GetWeaponParamFloat(desiredWeaponClassname, "projectile_speed") : SOLDIER_ROCKET_PROJECTILE_SPEED;
    //     aimPos = PredictTargetPosition(pTarget, rocketSpeed);
    // } else {
    //     aimPos = PredictTargetPosition(pTarget, 9999.0f); // Hitscan
    //     if (desiredWeaponClassname == "ff_weapon_shovel" && distSq > SOLDIER_MELEE_RANGE_SQ * 1.5f*1.5f) {
    //          return true;
    //     }
    // }

    // AimAt(aimPos, pCmd); // Uses CFFBaseAI::AimAt -> m_pBotPlayer->SetViewAngles
    // if (CanSeeTarget(pTarget)) { // Conceptual CanSeeTarget
    //    m_pBotPlayer->AddButton(pCmd, IN_ATTACK);
    // }
    return true;
}

bool CSoldierAI_FF::UseAbility(int abilitySlot, CBaseEntity* pTargetEntity, const Vector& targetPosition, CUserCmd* pCmd) {
    // if (!pCmd) return false;
    // if (abilitySlot == 0) { // Assuming abilitySlot 0 is Rocket Jump
    //     Vector jumpTarget = targetPosition;
    //     bool isDirection = false;
        // if (pTargetEntity) {
        //     jumpTarget = pTargetEntity->GetAbsOrigin(); // SDK Call
        // } else if (m_CurrentPath_NavAreaIDs.empty() && m_pObjectivePlanner && m_pObjectivePlanner->GetCurrentHighLevelTask()) {
        //     jumpTarget = m_pObjectivePlanner->GetCurrentHighLevelTask()->targetPosition;
        // }

        // if (jumpTarget.IsZero() || (m_pBotPlayer && m_pBotPlayer->GetOrigin().DistToSqr(jumpTarget) < 1.0f) ) {
        //      if (m_pBotPlayer) {
        //         Vector forward;
        //         AngleVectors(m_pBotPlayer->GetEyeAngles(), &forward, nullptr, nullptr); // SDK Call for forward vector
        //         jumpTarget = forward;
        //      } else {
        //         jumpTarget = Vector(1,0,0);
        //      }
        //      isDirection = true;
        // }
    //     return AttemptRocketJump(jumpTarget, isDirection, pCmd);
    // }
    return false;
}

bool CSoldierAI_FF::AttemptRocketJump(const Vector& targetOrDirection, bool isDirection, CUserCmd* pCmd) {
    // if (!m_pBotPlayer || !m_pBotPlayer->IsValid() || !pCmd) {
    //     m_eRocketJumpState = RJState::FAILED; return false;
    // }
    // if (m_pBotPlayer->GetHealth() < SOLDIER_ROCKET_JUMP_HEALTH_THRESHOLD) { m_eRocketJumpState = RJState::FAILED; return false; }

    // std::string rl_classname = "ff_weapon_rocketlauncher"; // Get from defines or ClassConfig
    // if (!m_pBotPlayer->IsWeaponActive_Conceptual(rl_classname) || m_pBotPlayer->GetAmmo(AMMO_ROCKETS_NAME) == 0) {
    //     m_pBotPlayer->SelectWeapon(pCmd, rl_classname);
    //     m_eRocketJumpState = RJState::NONE;
    //     return true;
    // }
    // if (!m_pBotPlayer->IsOnGround() && m_eRocketJumpState != RJState::IN_AIR) {
    //     m_eRocketJumpState = RJState::FAILED; return false;
    // }

    // float currentTime = GetWorldTime(); // From CFFBaseAI

    // switch (m_eRocketJumpState) {
    //     case RJState::NONE:
    //     case RJState::FAILED:
    //     case RJState::COMPLETED:
    //         m_eRocketJumpState = RJState::AIMING_DOWN;
    //         m_flRocketJumpStateStartTime = currentTime;
            // Fallthrough
    //     case RJState::AIMING_DOWN: {
    //         Vector botPos = m_pBotPlayer->GetOrigin();
    //         QAngle viewAngles = m_pBotPlayer->GetEyeAngles(); // For yaw
    //         Vector aimTargetPos = botPos;

            // Simplified: aim down with current yaw
    //         QAngle desiredAngles = viewAngles;
    //         desiredAngles.x = SOLDIER_ROCKET_JUMP_PITCH_LOOKDOWN; // Pitch down
    //         desiredAngles.z = 0; // No roll

    //         m_pBotPlayer->SetViewAngles(pCmd, desiredAngles); // Uses CFFPlayerWrapper

    //         if (currentTime - m_flRocketJumpStateStartTime > SOLDIER_RJ_STATE_TIMEOUT_SHORT) {
    //             m_eRocketJumpState = RJState::CROUCHING;
    //             m_flRocketJumpStateStartTime = currentTime;
    //         }
    //         break;
    //     }
    //     case RJState::CROUCHING:
    //         m_pBotPlayer->AddButton(pCmd, IN_DUCK);
    //         // Keep aiming
    //         QAngle currentAngles = m_pBotPlayer->GetEyeAngles();
    //         currentAngles.x = SOLDIER_ROCKET_JUMP_PITCH_LOOKDOWN;
    //         m_pBotPlayer->SetViewAngles(pCmd, currentAngles);

    //         if (currentTime - m_flRocketJumpStateStartTime > SOLDIER_RJ_STATE_TIMEOUT_SHORT || m_pBotPlayer->IsDucking()) {
    //             m_eRocketJumpState = RJState::JUMP_FIRE;
    //             m_flRocketJumpStateStartTime = currentTime;
    //         }
    //         break;
    //     case RJState::JUMP_FIRE:
    //         m_pBotPlayer->AddButton(pCmd, IN_DUCK | IN_JUMP | IN_ATTACK);
    //         // Keep aiming
    //         QAngle fireAngles = m_pBotPlayer->GetEyeAngles();
    //         fireAngles.x = SOLDIER_ROCKET_JUMP_PITCH_LOOKDOWN;
    //         m_pBotPlayer->SetViewAngles(pCmd, fireAngles);

    //         m_eRocketJumpState = RJState::IN_AIR;
    //         m_flRocketJumpStateStartTime = currentTime;
    //         return false;
    //     case RJState::IN_AIR:
    //         if (m_pBotPlayer->IsOnGround() || currentTime - m_flRocketJumpStateStartTime > SOLDIER_RJ_STATE_TIMEOUT_FLIGHT) {
    //             m_eRocketJumpState = RJState::COMPLETED;
    //             return false;
    //         }
    //         break;
    //     default:
    //         m_eRocketJumpState = RJState::NONE;
    //         return false;
    // }
    return true;
}

void CSoldierAI_FF::SwitchToWeapon(int weaponId, CUserCmd* pCmd) { // weaponId here is conceptual, should be classname string
    // if (!pCmd || !m_pBotPlayer) return;
    // std::string weaponClassName; // = GetWeaponClassNameFromBotDefinesOrConfig(weaponId);
    // if (!weaponClassName.empty()) {
    //    m_pBotPlayer->SelectWeapon(pCmd, weaponClassName);
    // }
}

Vector CSoldierAI_FF::PredictTargetPosition(CBaseEntity* pTarget, float projectileSpeed) const {
    // if (!pTarget || !m_pBotPlayer) return pTarget ? pTarget->GetAbsOrigin() : Vector(); // SDK Call

    // Vector targetPos = pTarget->GetAbsOrigin(); // SDK Call
    // if (projectileSpeed <= 0.001f) return targetPos;

    // Vector targetVel = pTarget->GetAbsVelocity(); // SDK Call
    // Vector myPos = m_pBotPlayer->GetEyeAngles(); // Incorrect, should be GetOrigin() or EyePosition()
    // Vector myEyePos = m_pBotPlayer->GetOrigin(); // SDK Call, simplistic, use EyePosition if available
    // QAngle eyeAngles = m_pBotPlayer->GetEyeAngles(); // SDK Call
    // myEyePos.z += 64; // Approximate eye height if GetEyePosition not available

    // float dist = myEyePos.DistTo(targetPos);
    // float timeToTarget = dist / projectileSpeed;

    // return targetPos + targetVel * timeToTarget;
    return Vector(); // Placeholder
}
