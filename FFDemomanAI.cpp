#include "FFDemomanAI.h"
#include "ObjectivePlanner.h" // For SubTask, if demoman needs to interact with it directly
#include "BotTasks.h"         // For SubTaskType, if demoman's abilities map to specific subtasks
#include "FFStateStructs.h"   // For Vector, ClassConfigInfo
#include "BotKnowledgeBase.h" // For accessing game state (tracked enemies, navgraph)
#include "GameDefines_Placeholder.h" // For CUserCmd, IN_ATTACK, IN_ATTACK2, etc.
#include "CFFPlayer.h"        // For bot actions & state queries
#include "CBaseEntity.h"      // For target representation

#include <iostream> // Placeholder logging
#include <cmath>    // For M_PI, sqrt, etc.
#include <algorithm> // For std::min/max

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Conceptual constants for Demoman
const float DEMOMAN_PIPE_SPEED_FF = 1200.0f;         // Units per second (example)
const float DEMOMAN_STICKY_MIN_CHARGE_SPEED_FF = 900.0f; // Speed with no charge
const float DEMOMAN_STICKY_MAX_CHARGE_SPEED_FF = 2400.0f; // Speed with full charge
const float DEMOMAN_STICKY_CHARGE_TIME_FF = 1.0f;    // Time to full charge sticky (example)
const float DEMOMAN_PIPE_FIRE_INTERVAL_FF = 0.6f;    // Seconds between pipe shots
const float DEMOMAN_STICKY_FIRE_INTERVAL_FF = 0.6f;  // Seconds between sticky shots (uncharged)
const float DEMOMAN_STICKY_ARM_TIME_FF = 0.7f;       // Time until a sticky can be detonated
const float DEMOMAN_MELEE_RANGE_SQ_FF = 80.0f * 80.0f;
const float DEMOMAN_STICKY_DETONATE_RADIUS_SQ_FF = 150.0f * 150.0f; // Radius for detonating near enemies

CDemomanAI_FF::CDemomanAI_FF(CFFPlayer* pBotPlayer, CObjectivePlanner* pPlanner,
                           const BotKnowledgeBase* pKnowledgeBase, const ClassConfigInfo* pClassConfig)
    : CFFBaseAI(pBotPlayer, pPlanner, pKnowledgeBase, pClassConfig),
      m_iActiveStickies(0),
      m_flNextStickyFireTime(0.0f),
      m_flNextPipeFireTime(0.0f)
{
    // std::cout << "CDemomanAI_FF created." << std::endl;
}

CDemomanAI_FF::~CDemomanAI_FF() {}

CBaseEntity* CDemomanAI_FF::SelectTarget() {
    if (!m_pKnowledgeBase || !m_pBotPlayer) return nullptr;
    m_pCurrentTarget = nullptr; // Clear previous
    CBaseEntity* pBestTarget = nullptr;
    float highestScore = -1.0f;
    Vector myPos = m_pBotPlayer->GetPosition();

    // Conceptual: Check if any existing stickies can be detonated effectively
    if (m_iActiveStickies > 0 && ShouldDetonateNow(nullptr /* pass potential target or scan all enemies */)) {
        // If a good detonation opportunity exists, this might influence target selection
        // (e.g. focus on enemies near the trap even if they aren't highest priority otherwise)
        // For now, this logic is in UseAbility. SelectTarget focuses on direct engagement or new traps.
    }

    // Iterate m_pKnowledgeBase->GetTrackedEnemies()
    // For this placeholder, assume a single conceptual enemy for logic flow
    // static CBaseEntity conceptualEnemy; // Problematic for multiple bots
    // if (/* conceptualEnemy is valid and visible */) {
    //     std::vector<CBaseEntity*> potentialTargets = {&conceptualEnemy};
    //     for (CBaseEntity* pEnemy : potentialTargets) {
    //         if (!pEnemy || !pEnemy->IsAlive()) continue;
    //         Vector enemyPos = pEnemy->GetPosition();
    //         float distSq = (myPos - enemyPos).LengthSquared();
    //         float currentScore = 1000.0f / (1.0f + sqrt(distSq)); // Base score

    //         if (pEnemy->IsSentryGun()) currentScore += 300.0f; // Sentry is good for demo
    //         // int enemiesInSplash = CountEnemiesNearPoint_Conceptual(m_pKnowledgeBase, enemyPos, 150.0f);
    //         // currentScore += enemiesInSplash * 50.0f; // Bonus for groups

    //         if (currentScore > highestScore) {
    //             highestScore = currentScore;
    //             pBestTarget = pEnemy;
    //         }
    //     }
    // }
    // If no specific logic finds a target, this will return nullptr.
    // CFFBaseAI::Update will then potentially lead to idle or objective-based actions.
    m_pCurrentTarget = pBestTarget;
    return pBestTarget;
}

bool CDemomanAI_FF::AttackTarget(CBaseEntity* pTarget, CUserCmd* pCmd) {
    if (!pTarget || !pTarget->IsAlive() || !m_pBotPlayer || !m_pBotPlayer->IsValid() || !m_pBotPlayer->IsAlive() || !pCmd) {
        m_pCurrentTarget = nullptr;
        return false;
    }
    m_pCurrentTarget = pTarget;

    Vector myPos = m_pBotPlayer->GetPosition();
    Vector targetPos = pTarget->GetPosition();
    float distSq = (myPos - targetPos).LengthSquared();
    float currentTime = 0.0f; // Conceptual: m_pBotPlayer->GetCurrentWorldTime();

    // Weapon stats from ClassConfigInfo (conceptual)
    bool hasGL = false, hasSticky = false, hasMelee = false;
    float glFireRate = DEMOMAN_PIPE_FIRE_INTERVAL_FF;
    float stickyFireRate = DEMOMAN_STICKY_FIRE_INTERVAL_FF;
    // if (m_pClassConfig) { /* Iterate availableWeapons to find GL, Sticky, Melee and their stats */ }
    hasGL = true; hasSticky = true; hasMelee = true; // Assume has all for now

    // --- Strategy: Trap vs. Direct ---
    // Conceptual: If target is moving predictably into a chokepoint we want to trap
    // bool isGoodTrapTarget = false;
    // Vector trapLocation;
    // if (m_iActiveStickies < MAX_STICKIES_FF && IsTargetGoodForNewTrap(pTarget, trapLocation)) {
    //    return ActionPlantSticky(trapLocation, pCmd, true);
    // }


    // --- Direct Attack Logic ---
    int desiredWeaponId = WEAPON_ID_DEMOMAN_MELEE; // Default

    // Prefer Grenade Launcher for direct combat usually
    if (hasGL && currentTime >= m_flNextPipeFireTime /* && HasAmmo(GL) */ ) {
        desiredWeaponId = WEAPON_ID_DEMOMAN_GRENADELAUNCHER;
    }
    // Sticky for spam/area denial if GL on cooldown or less suitable
    else if (hasSticky && currentTime >= m_flNextStickyFireTime /* && HasAmmo(Sticky) && m_iActiveStickies < MAX_STICKIES_FF */) {
        desiredWeaponId = WEAPON_ID_DEMOMAN_STICKYLAUNCHER;
    }
    // Melee as last resort or if very close
    else if (hasMelee && distSq < DEMOMAN_MELEE_RANGE_SQ_FF) {
        desiredWeaponId = WEAPON_ID_DEMOMAN_MELEE;
    } else if (hasGL /* && HasAmmo(GL) */) { // Fallback if others on cooldown
         desiredWeaponId = WEAPON_ID_DEMOMAN_GRENADELAUNCHER;
    } else if (hasSticky /* && HasAmmo(Sticky) */) {
         desiredWeaponId = WEAPON_ID_DEMOMAN_STICKYLAUNCHER;
    }


    SwitchToWeapon(desiredWeaponId, pCmd);
    // if (m_pBotPlayer->GetActiveWeaponId_Placeholder() != desiredWeaponId) {
    //     return true; // Weapon switching
    // }

    Vector aimPos;
    if (desiredWeaponId == WEAPON_ID_DEMOMAN_GRENADELAUNCHER) {
        if (currentTime < m_flNextPipeFireTime) return true; // Waiting for cooldown
        aimPos = PredictPipeAim(targetPos, pTarget->GetAbsVelocity(), DEMOMAN_PIPE_SPEED_FF);
        AimAt(aimPos, pCmd);
        pCmd->buttons |= IN_ATTACK;
        m_flNextPipeFireTime = currentTime + glFireRate;
    } else if (desiredWeaponId == WEAPON_ID_DEMOMAN_STICKYLAUNCHER) {
        if (currentTime < m_flNextStickyFireTime) return true; // Waiting for cooldown
        if (m_iActiveStickies >= MAX_STICKIES_FF) { // Max stickies out, maybe detonate?
            if (ShouldDetonateNow(pTarget)) return ActionDetonateStickies(pCmd);
            // If not detonating, can't fire more stickies unless one is destroyed.
            // This logic might be flawed if stickies auto-detonate. Assume they don't for now.
            return true; // Can't attack with stickies right now
        }
        aimPos = PredictStickyAim(targetPos, pTarget->GetAbsVelocity(), 0.0f /* no charge */); // Aim for direct hit or area
        AimAt(aimPos, pCmd);
        pCmd->buttons |= IN_ATTACK; // Tap fire for quick sticky
        m_iActiveStickies++;
        // Add to m_PlantedStickies list conceptually
        m_flNextStickyFireTime = currentTime + stickyFireRate;
    } else if (desiredWeaponId == WEAPON_ID_DEMOMAN_MELEE) {
        if (distSq < DEMOMAN_MELEE_RANGE_SQ_FF) {
            AimAt(pTarget->GetWorldSpaceCenter(), pCmd);
            pCmd->buttons |= IN_ATTACK;
        } else {
            // Too far for melee, even if selected. Consider this an "unable to attack" state.
            // The AI's MoveTo/FollowPath should handle getting closer if this is the only option.
            return true; // Ongoing, but not actively hitting.
        }
    } else {
        return false; // No weapon available/selected
    }
    return true; // Attack action performed or ongoing
}

bool CDemomanAI_FF::UseAbility(int abilitySlot, CBaseEntity* pTarget, CUserCmd* pCmd) {
    if (!pCmd) return false;
    // Assuming abilitySlot 0 for Demoman is "Detonate Stickies"
    if (abilitySlot == 0) {
        return ActionDetonateStickies(pCmd);
    }
    return false;
}

bool CDemomanAI_FF::ActionPlantSticky(const Vector& position, CUserCmd* pCmd, bool isTrap, float charge) {
    if (!m_pBotPlayer || !pCmd || m_iActiveStickies >= MAX_STICKIES_FF) {
        return false; // Max stickies out or invalid state
    }
    float currentTime = 0.0f; // Conceptual: m_pBotPlayer->GetCurrentWorldTime();
    if (currentTime < m_flNextStickyFireTime) {
        return true; // Waiting for sticky cooldown
    }

    SwitchToWeapon(WEAPON_ID_DEMOMAN_STICKYLAUNCHER, pCmd);
    // if (m_pBotPlayer->GetActiveWeaponId_Placeholder() != WEAPON_ID_DEMOMAN_STICKYLAUNCHER) {
    //     return true; // Switching weapon
    // }
    // if (!m_pBotPlayer->HasAmmo(AMMO_STICKY)) return false;


    // Aiming for sticky traps is complex (arc, surfaces).
    // For direct fire stickies, aimPos might be different.
    // This simplified version just aims at the target position on the ground.
    Vector aimPosition = position;
    // Add logic here if 'charge' affects aim significantly or if arcing is calculated.
    AimAt(aimPosition, pCmd);

    // Conceptual: Hold IN_ATTACK for 'charge' duration, then release.
    // For simplicity, assume tap-fire (IN_ATTACK for one frame) or that engine handles charge on hold.
    pCmd->buttons |= IN_ATTACK;

    m_iActiveStickies++;
    // Add to m_PlantedStickies list: m_PlantedStickies.push_back({nullptr, position, currentTime, isTrap});
    m_flNextStickyFireTime = currentTime + DEMOMAN_STICKY_FIRE_INTERVAL_FF; // Cooldown

    // std::cout << "Demoman: Planted sticky, active: " << m_iActiveStickies << std::endl;
    return true; // Action initiated
}

bool CDemomanAI_FF::ActionDetonateStickies(CUserCmd* pCmd) {
    if (!m_pBotPlayer || !pCmd || m_iActiveStickies == 0) {
        return false; // No stickies to detonate or invalid state
    }

    SwitchToWeapon(WEAPON_ID_DEMOMAN_STICKYLAUNCHER, pCmd);
    // if (m_pBotPlayer->GetActiveWeaponId_Placeholder() != WEAPON_ID_DEMOMAN_STICKYLAUNCHER) {
    //     return true; // Switching weapon
    // }

    pCmd->buttons |= IN_ATTACK2; // Secondary fire detonates for sticky launcher
    // std::cout << "Demoman: Detonating " << m_iActiveStickies << " stickies!" << std::endl;

    m_iActiveStickies = 0; // Assume all active stickies are detonated by one IN_ATTACK2 press
    // m_PlantedStickies.clear(); // Clear conceptual list

    return true; // Detonation action performed
}

bool CDemomanAI_FF::ShouldDetonateNow(const CBaseEntity* pPotentialTarget) const {
    if (m_iActiveStickies == 0 || !m_pKnowledgeBase || !m_pBotPlayer) return false;

    // Iterate through m_PlantedStickies (conceptual list)
    // For each planted sticky:
    //   float timeSincePlanted = GetCurrentWorldTime() - sticky.plantTime;
    //   if (timeSincePlanted < DEMOMAN_STICKY_ARM_TIME_FF) continue; // Not armed yet

    //   Check enemies near this sticky's position from m_pKnowledgeBase->GetTrackedEnemies()
    //   for (const auto& enemyInfo : m_pKnowledgeBase->GetTrackedEnemies()) {
    //       if (enemyInfo.isVisible && (enemyInfo.lastKnownPosition - sticky.position).LengthSqr() < DEMOMAN_STICKY_DETONATE_RADIUS_SQ_FF) {
    //           // Check LOS from bot to sticky detonation area, or if target is more valuable
    //           return true; // Found an enemy in a trap radius
    //       }
    //   }

    // If a specific pPotentialTarget is provided, check if it's near any stickies
    // if (pPotentialTarget && pPotentialTarget->IsAlive()) {
    //    for (const auto& sticky : m_PlantedStickies) { /* check distance and arm time */ }
    // }
    return false; // Placeholder
}

Vector CDemomanAI_FF::FindGoodTrapLocation(const Vector& generalArea, float radius) const {
    // Conceptual:
    // 1. Use m_pKnowledgeBase->GetNavGraph() to find nearby nav areas.
    // 2. Identify chokepoints (areas with few connections but leading to important places).
    // 3. Or, identify areas near objectives (CPs, payload path) within `generalArea`.
    // 4. Consider visibility / typical enemy approaches.
    return generalArea; // Placeholder: just return the general area for now
}

Vector CDemomanAI_FF::PredictPipeAim(const Vector& targetPos, const Vector& targetVel, float pipeSpeed) const {
    if (!m_pBotPlayer) return targetPos;
    Vector myPos = m_pBotPlayer->GetEyePosition(); // Fire from eyes
    // Simple linear prediction (can be improved with iteration for better accuracy)
    float dist = (targetPos.x-myPos.x)*(targetPos.x-myPos.x) + (targetPos.y-myPos.y)*(targetPos.y-myPos.y) + (targetPos.z-myPos.z)*(targetPos.z-myPos.z);
    dist = sqrt(dist);
    float timeToTarget = dist / std::max(0.1f, pipeSpeed);

    Vector predictedPos = Vector(targetPos.x + targetVel.x * timeToTarget,
                                 targetPos.y + targetVel.y * timeToTarget,
                                 targetPos.z + targetVel.z * timeToTarget);

    // Add gravity drop compensation (conceptual, depends on game physics)
    // float gravity = 800.0f; // Conceptual world gravity
    // predictedPos.z += 0.5f * gravity * timeToTarget * timeToTarget;
    return predictedPos;
}

Vector CDemomanAI_FF::PredictStickyAim(const Vector& targetPos, const Vector& targetVel, float chargePercent) const {
    if (!m_pBotPlayer) return targetPos;
    // Sticky speed depends on charge. chargePercent is 0.0 to 1.0.
    float actualSpeed = DEMOMAN_STICKY_MIN_CHARGE_SPEED_FF +
                        (DEMOMAN_STICKY_MAX_CHARGE_SPEED_FF - DEMOMAN_STICKY_MIN_CHARGE_SPEED_FF) * std::min(1.0f, std::max(0.0f, chargePercent));
    return PredictPipeAim(targetPos, targetVel, actualSpeed); // Reuse pipe prediction logic with different speed
}


void CDemomanAI_FF::SwitchToWeapon(int weaponId, CUserCmd* pCmd) {
    if (!pCmd || !m_pBotPlayer) return;
    // Conceptual
    // if (m_pBotPlayer->GetActiveWeaponId_Placeholder() != weaponId) {
    //    pCmd->weaponselect = weaponId;
    // }
}
