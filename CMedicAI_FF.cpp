#include "CMedicAI_FF.h"
#include "ObjectivePlanner.h" // For accessing planner/tasks if needed by Medic logic directly
#include "CFFPlayer.h"        // Conceptual
#include "CBaseEntity.h"      // Conceptual
#include "UserCmd.h"          // Conceptual
#include "BotKnowledgeBase.h" // Conceptual
#include "ClassConfigInfo.h"  // Conceptual
#include <iostream>           // Placeholder logging
#include <vector>             // For FindBestHealTarget
#include <algorithm>          // For std::sort

// --- CMedicAI_FF Implementation ---

// Define conceptual constants if not available from engine includes
const float MEDIC_HEAL_RANGE_SQ = 450.0f * 450.0f; // Squared for efficiency
const float MEDIC_MAX_HEAL_TARGET_DIST_SQ = 750.0f * 750.0f; // Max distance to consider healing
const float MEDIC_ALLY_CHECK_INTERVAL = 0.5f; // Seconds
const float MEDIC_ENEMY_SCAN_INTERVAL = 0.3f; // Seconds
const float UBER_MIN_ALLY_HEALTH_FOR_OFFENSIVE_UBER = 0.7f; // Ally needs to be somewhat healthy for offensive uber
const int BUTTON_ATTACK = 1; // Conceptual IN_ATTACK
const int BUTTON_ATTACK2 = 1 << 11; // Conceptual IN_ATTACK2 (often this bit)


CMedicAI_FF::CMedicAI_FF(CFFPlayer* pBotPlayer, CObjectivePlanner* pPlanner, const BotKnowledgeBase* pKnowledgeBase, const ClassConfigInfo* pClassConfig)
    : CFFBaseAI(pBotPlayer, pPlanner, pKnowledgeBase, pClassConfig),
      m_pHealTarget(nullptr),
      m_fUberChargePercentage(0.0f),
      m_bIsUberDeployed(false),
      m_flLastHealTargetCheckTime(0.0f),
      m_flLastEnemyScanTime(0.0f)
{
    std::cout << "CMedicAI_FF created." << std::endl;
}

CMedicAI_FF::~CMedicAI_FF() {}

void CMedicAI_FF::UpdateUberChargeLevel() {
    // Conceptual: This would query the CFFPlayer object or game state
    // m_fUberChargePercentage = m_pBotPlayer->GetUberChargeLevel();
    // For testing, simulate Uber build-up when healing
    if (m_pHealTarget && !m_bIsUberDeployed && m_fUberChargePercentage < 1.0f) {
         // m_fUberChargePercentage += 0.01f; // Simulate slow charge gain
         // if(m_fUberChargePercentage > 1.0f) m_fUberChargePercentage = 1.0f;
    }
}

CBaseEntity* CMedicAI_FF::FindBestHealTarget() {
    // Conceptual:
    // CBaseEntity* bestTarget = nullptr;
    // float bestScore = -1.0f;
    // Vector myPos = m_pBotPlayer->GetPosition();

    // Access visible teammates (e.g., from m_pKnowledgeBase or a CFFPlayer method)
    // const std::vector<AllyInfo>& visibleTeammates = m_pKnowledgeBase->GetVisibleTeammates();
    std::vector<CBaseEntity*> conceptualTeammates; // Placeholder
    // conceptualTeammates.push_back(new CBaseEntity()); // Add some dummy teammates for logic flow

    // for (CBaseEntity* ally : conceptualTeammates) {
    //     if (!ally || !ally->IsAlive() || ally == m_pBotPlayer) continue;
    //     if (ally->GetHealthPercentage() >= 1.0f && !ally->IsBuffed()) continue; // Already full health and not buffed

    //     float distSq = (myPos - ally->GetPosition()).LengthSqr();
    //     if (distSq > MEDIC_MAX_HEAL_TARGET_DIST_SQ) continue;

    //     float score = 0.0f;
    //     score += (1.0f - ally->GetHealthPercentage()) * 100.0f; // Prioritize lower health
    //     if (ally->IsInCombat()) score += 50.0f; // Prioritize those in combat
    //     if (ally->IsHeavyClass() || ally->IsSoldierClass()) score += 30.0f; // Prioritize combat classes
    //     score -= distSq * 0.01f; // Penalize distance slightly

    //     if (score > bestScore) {
    //         bestScore = score;
    //         bestTarget = ally;
    //     }
    // }
    // delete conceptualTeammates[0]; // Clean up placeholder
    // return bestTarget;
    return nullptr; // Placeholder
}

CBaseEntity* CMedicAI_FF::SelectTarget() {
    // float currentTime = GetCurrentWorldTime(); // Assume available

    // // Periodically re-evaluate heal target
    // if (currentTime - m_flLastHealTargetCheckTime > MEDIC_ALLY_CHECK_INTERVAL) {
    //     m_pHealTarget = FindBestHealTarget();
    //     m_flLastHealTargetCheckTime = currentTime;
    // }

    // // If directly attacked, prioritize attacker
    // if (m_pBotPlayer->IsUnderAttackByEnemy()) { // Conceptual
    //     m_pCurrentTarget = m_pBotPlayer->GetAttacker(); // Conceptual
    //     m_pHealTarget = nullptr; // Stop healing to fight back
    //     return m_pCurrentTarget;
    // }

    // // If has a heal target, stick to them unless critical threat
    // if (m_pHealTarget && m_pHealTarget->IsAlive()) {
    //     m_pCurrentTarget = nullptr; // No combat target if healing
    //     return m_pHealTarget; // This will be used by AttackTarget to mean "heal this"
    // }

    // // If no heal target, or heal target is dead/out of range, look for enemies
    // if (currentTime - m_flLastEnemyScanTime > MEDIC_ENEMY_SCAN_INTERVAL) {
    //    m_pCurrentTarget = CFFBaseAI::SelectTarget(); // Call base enemy selection (e.g. nearest visible enemy)
    //    m_flLastEnemyScanTime = currentTime;
    // }
    // return m_pCurrentTarget;

    // Simplified for this step:
    if (m_pHealTarget && m_pHealTarget->IsAlive()) return m_pHealTarget;
    m_pHealTarget = FindBestHealTarget(); // Try to find one
    if (m_pHealTarget) return m_pHealTarget;

    return CFFBaseAI::SelectTarget(); // Fallback to base enemy selection
}

void CMedicAI_FF::Update(UserCmd* pCmd) {
    if (!m_pBotPlayer || !m_pBotPlayer->IsAlive() || !pCmd) return;

    UpdateUberChargeLevel();
    m_pCurrentTarget = SelectTarget(); // This will set m_pHealTarget or m_pCurrentTarget (enemy)

    // If current HLT/SubTask is movement, CFFBaseAI::Update will handle it.
    // Medic specific actions (Heal/Attack/Uber) are typically driven by AttackTarget/UseAbility
    // which are called by CFFBaseAI::ExecuteSubTask if the subtask is ATTACK_TARGET etc.

    // If main task is to attack (which for medic could mean heal or fight)
    const SubTask* currentSubTask = m_pObjectivePlanner ? m_pObjectivePlanner->GetCurrentSubTask() : nullptr;
    if (currentSubTask && currentSubTask->type == SubTaskType::ATTACK_TARGET) {
        // AttackTarget will handle if it's healing or fighting
        if (!AttackTarget(m_pCurrentTarget, pCmd)) { // Returns true if action ongoing
            // If AttackTarget returns false, it means action completed or failed.
            // The ExecuteSubTask in CFFBaseAI will notify planner.
        }
    } else if (currentSubTask && currentSubTask->type == SubTaskType::USE_ABILITY_ON_TARGET) { // Assume Uber is this
         if (!UseAbility(0, m_pHealTarget, pCmd)) { // Slot 0 for Uber for now
            // ...
         }
    }
    else {
        // If not attacking/using ability via subtask, let base class handle movement or other subtask types
        CFFBaseAI::Update(pCmd);
    }

    // If no specific subtask is causing action, but we have a heal target, ensure we are healing.
    // This can also be integrated into an "IDLE" or "SUPPORT" subtask behavior.
    if (m_pHealTarget && (!currentSubTask ||
        (currentSubTask->type != SubTaskType::ATTACK_TARGET && currentSubTask->type != SubTaskType::USE_ABILITY_ON_TARGET))) {
        HealTarget(m_pHealTarget, pCmd);
    }
}


bool CMedicAI_FF::HealTarget(CBaseEntity* pTargetToHeal, UserCmd* pCmd) {
    if (!pTargetToHeal || !pTargetToHeal->IsAlive() || !m_pBotPlayer) {
        m_pHealTarget = nullptr;
        return false; // Action not ongoing
    }
    m_pHealTarget = pTargetToHeal; // Ensure it's set

    // Conceptual: SwitchToWeapon(WEAPON_ID_MEDIGUN, pCmd);
    // if (m_pBotPlayer->GetActiveWeaponId() != WEAPON_ID_MEDIGUN) {
    //     return true; // Waiting for weapon switch
    // }

    // Aim at heal target
    AimAt(m_pHealTarget->GetWorldSpaceCenter(), pCmd);

    // Check range
    // if ((m_pBotPlayer->GetPosition() - m_pHealTarget->GetPosition()).LengthSqr() > MEDIC_HEAL_RANGE_SQ) {
    //     // Heal target out of range, MoveTo logic should handle getting closer.
    //     // Stop healing explicitly if they get too far.
    //     m_pHealTarget = nullptr; // Stop active healing if target runs too far
    //     return false;
    // }

    pCmd->buttons |= BUTTON_ATTACK; // Hold primary fire to heal
    // std::cout << "Medic healing target" << std::endl;

    // Uber logic might be triggered here or in Update
    AttemptUberCharge(pCmd);

    return true; // Healing is ongoing
}

bool CMedicAI_FF::AttackTarget(CBaseEntity* pTarget, UserCmd* pCmd) {
    // If pTarget is the heal target, then heal.
    if (pTarget && m_pHealTarget && pTarget == m_pHealTarget) {
        return HealTarget(pTarget, pCmd);
    }

    // If pTarget is an enemy:
    if (pTarget && m_pBotPlayer /* && m_pBotPlayer->IsEnemy(pTarget) */ ) {
        m_pHealTarget = nullptr; // Stop healing if fighting an enemy
        // Conceptual: Weapon choice (Syringe or Bonesaw)
        // float distSq = (m_pBotPlayer->GetPosition() - pTarget->GetPosition()).LengthSqr();
        // if (distSq > MELEE_RANGE_SQ_PLUS_BUFFER) {
        //    SwitchToWeapon(WEAPON_ID_SYRINGEGUN, pCmd);
        // } else {
        //    SwitchToWeapon(WEAPON_ID_BONESAW, pCmd);
        // }
        // if (m_pBotPlayer->GetActiveWeaponId() == WEAPON_ID_SYRINGEGUN || m_pBotPlayer->GetActiveWeaponId() == WEAPON_ID_BONESAW) {
             AimAt(pTarget->GetWorldSpaceCenter(), pCmd);
             pCmd->buttons |= BUTTON_ATTACK;
        //     std::cout << "Medic fighting enemy" << std::endl;
             return true; // Combat ongoing
        // }
        // return true; // Waiting for weapon switch
    }
    m_pHealTarget = nullptr; // No valid heal or enemy target
    return false; // Action not ongoing
}

bool CMedicAI_FF::ShouldDeployUber(CBaseEntity* currentEnemy) const {
    if (m_fUberChargePercentage < 1.0f || !m_pHealTarget || !m_pHealTarget->IsAlive()) {
        return false;
    }

    // Offensive Uber: Heal target is healthy, pushing an objective, or key enemy Sentry sighted.
    // if (m_pHealTarget->GetHealthPercentage() > UBER_MIN_ALLY_HEALTH_FOR_OFFENSIVE_UBER) {
    //    if (m_pObjectivePlanner->IsCurrentTaskOffensivePush() ||
    //        (currentEnemy && currentEnemy->IsSentrygun())) { // Conceptual
    //        return true;
    //    }
    // }
    // Defensive Uber: Heal target or self is critical, multiple allies critical, or under heavy fire.
    // if (m_pHealTarget->GetHealthPercentage() < 0.4f || m_pBotPlayer->GetHealthPercentage() < 0.4f) {
    //    return true;
    // }
    // if (m_pKnowledgeBase->CountNearbyCriticalAllies(m_pBotPlayer->GetPosition()) >= 2) return true; // Conceptual

    return false; // Placeholder
}

bool CMedicAI_FF::AttemptUberCharge(UserCmd* pCmd) {
    if (ShouldDeployUber(m_pCurrentTarget /* can be null if not fighting */)) {
        // if (m_pBotPlayer->GetActiveWeaponId() == WEAPON_ID_MEDIGUN) {
            pCmd->buttons |= BUTTON_ATTACK2; // Secondary fire for Uber
            m_fUberChargePercentage = 0.0f;
            m_bIsUberDeployed = true; // Track deployed state
            // Set timer for m_bIsUberDeployed = false after ~8 seconds
            // std::cout << "MEDIC DEPLOYED UBERCHARGE!" << std::endl;
            return true;
        // }
    }
    if (m_bIsUberDeployed && GetCurrentWorldTime() - m_fStateEntryTime > 8.0f) { // Conceptual Uber duration
        m_bIsUberDeployed = false;
    }
    return false;
}

// This is a conceptual helper that would be part of CFFPlayer or CBotFortress
void CMedicAI_FF::SwitchToWeapon(int weaponId, UserCmd* pCmd) {
    // pCmd->weaponselect = weaponId; // Engine specific
    // std::cout << "Medic switching to weapon ID: " << weaponId << std::endl;
}

bool CMedicAI_FF::UseAbility(int abilitySlot, CBaseEntity* pTarget, UserCmd* pCmd) {
    // Assuming abilitySlot 0 for Medic is Ubercharge
    if (abilitySlot == 0) {
        return AttemptUberCharge(pCmd);
    }
    return false;
}

```

**Conceptual `CBotFortress` / `CFFPlayer` additions needed for `CMedicAI_FF`:**
*   `GetUberChargeLevel()`: Returns float 0.0-1.0.
*   `IsUnderAttackByEnemy()`: Returns bool.
*   `GetAttacker()`: Returns `CBaseEntity*`.
*   `GetHealthPercentage()`: Returns float 0.0-1.0 for self or any `CBaseEntity`.
*   `IsInCombat()`: For self or any `CBaseEntity`.
*   `IsHeavyClass()`, `IsSoldierClass()`, `IsBuffed()` for `CBaseEntity`.
*   `GetActiveWeaponID()` and `selectWeaponName()` or `selectWeaponId()`.
*   `CanAttackWithWeapon(weaponId)`: Checks ammo, etc.
*   `GetTime()`: Current game time. (Used `GetCurrentWorldTime()` as placeholder).
*   `GetUserCmd()`: Returns `UserCmd*`.

**`AIFactory` update in `FFBaseAI.h` (Conceptual):**
The `AIFactory::CreateAIModule` would need to be updated to create `CMedicAI_FF` if `className == "medic"`.
```cpp
// Conceptual update to AIFactory in FFBaseAI.h
// namespace AIFactory {
//    inline std::unique_ptr<CFFBaseAI> CreateAIModule(...) {
//        if (className == "medic" || classCfg->classId == TF_CLASS_MEDIC) { // TF_CLASS_MEDIC is conceptual
//            return std::make_unique<CMedicAI_FF>(pPlayer, planner, kb, classCfg);
//        }
//        // ... other classes ...
//        return std::make_unique<CBotFortress>(pPlayer, planner, kb, classCfg); // Default
//    }
// }
```

---
**Step 4 & 5: Integrate Medic Logic into `modThink`/Schedules & Following Behavior**

*   **Task Selection (`CObjectivePlanner`):**
    *   When `GenerateTasks_ControlPoint` (or other game mode task generators) is called for a Medic bot, it might slightly adjust priorities or task types.
    *   For example, instead of a raw `CAPTURE_POINT` task, a Medic might get a `SUPPORT_ATTACK_ON_POINT` task. This task would decompose similarly (move to point, secure, stand on point) but the Medic's AI (`CMedicAI_FF::Update`) would inherently prioritize healing teammates involved in that HLT.
    *   Alternatively, the Medic gets the same HLT (e.g. `CAPTURE_POINT`), but its `SelectTarget` and `AttackTarget` overrides ensure it primarily heals.
*   **Behavior within Schedules:**
    *   **`CMoveToTask_FF` for Medics:**
        *   If `m_pHealTarget` is set and valid, the `CMoveToTask_FF` (or the main movement logic in `CMedicAI_FF::Update` if not using a formal move task from planner) should adjust its destination.
        *   Instead of moving directly to `subTask->targetPosition`, it moves to `m_pHealTarget->GetPosition() + offsetVector` (e.g., slightly behind the heal target).
        *   This needs to be dynamic. If `m_pHealTarget` moves, the Medic's path/destination needs to update.
        *   This means `CFFBaseAI::MoveTo()` might need to be called more frequently by `CMedicAI_FF::Update()` if following a player, or the `FollowPath()` needs to be smarter about dynamic targets.
    *   **`CSecureAreaTask_FF` / `CHoldPositionTask_FF` for Medics:**
        *   When `execute()` is called, the Medic's overridden `SelectTarget()` will prioritize finding heal targets within that area.
        *   Its `AttackTarget()` will then become `HealTarget()`.
        *   It will only engage enemies if directly threatened or no allies need healing.

**Implementation of Following Behavior (Conceptual in `CMedicAI_FF::Update` or a dedicated `FollowHealTargetTask`):**

```cpp
// Inside CMedicAI_FF::Update, if a heal target is active and current subtask allows following:
// if (m_pHealTarget && m_pHealTarget->IsAlive()) {
//     Vector targetFollowPos = m_pHealTarget->GetPosition() - m_pHealTarget->GetForwardVector() * 50.0f + m_pHealTarget->GetRightVector() * 20.0f; // Stay behind and slightly to the side
//     float distToFollowPosSq = (m_pBotPlayer->GetPosition() - targetFollowPos).LengthSqr();
//     float distToHealTargetSq = (m_pBotPlayer->GetPosition() - m_pHealTarget->GetPosition()).LengthSqr();

//     if (distToHealTargetSq > MEDIC_HEAL_RANGE_SQ * 0.8f*0.8f || distToFollowPosSq > 64.0f*64.0f) { // If too far from heal target OR ideal follow pos
//         // Need to move closer to heal target or follow position
//         // This would typically mean the current SubTask should be MoveTo(targetFollowPos)
//         // Or, if the current subtask is not movement, this medic logic might override it
//         // by directly calling CFFBaseAI::MoveTo(targetFollowPos, pCmd) and then CFFBaseAI::FollowPath(pCmd).
//         // This requires careful state management to not conflict with planner's subtasks.

//         // For simplicity, assume if there's a heal target, any "move" subtask from planner
//         // gets its target dynamically updated to be near the heal target.
//         if (currentSubTask && (currentSubTask->type == SubTaskType::MOVE_TO_POSITION || currentSubTask->type == SubTaskType::MOVE_TO_ENTITY)) {
//             // Modify the subtask's effective target for this frame for the Medic.
//             // This is a bit of a hack on the planner's subtask.
//             // A better way: Medic's MoveTo() implementation checks for heal target.
//             if (MoveTo(targetFollowPos, pCmd)) { // Re-path to follow
                 FollowPath(pCmd); // Execute one frame of movement
//             }
//         }
//     }
//     HealTarget(m_pHealTarget, pCmd); // Continue healing
// }
```
This implies that `CFFBaseAI::MoveTo()` when called by a Medic for a generic point, might be overridden in `CMedicAI_FF` to check if `m_pHealTarget` exists. If so, it adjusts the `targetPos` for `FindPath` to be near the heal target instead of the original subtask target, as long as the heal target is near the subtask's overall area. This is getting complex and shows the need for careful interaction design between generic tasks and class-specific overrides.

This completes the structural design and conceptual implementation for Medic AI. The key is that the Medic's `Update` and `SelectTarget` will always try to find and maintain a heal target, and its `AttackTarget` is repurposed for healing when `m_pCurrentTarget` is actually `m_pHealTarget`. Combat is a fallback. Uber logic is a triggered ability. Following is a modification of its movement goal.
