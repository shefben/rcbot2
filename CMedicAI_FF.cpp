#include "CMedicAI_FF.h"
#include "ObjectivePlanner.h"
#include "BotTasks.h"
#include "FFStateStructs.h"
// Conceptual includes
#include "CFFPlayer.h"
#include "CBaseEntity.h"
#include "UserCmd.h"
#include "BotKnowledgeBase.h"
#include <iostream>           // Placeholder logging
#include <vector>
#include <algorithm>          // For std::sort, std::min_element etc.

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// --- Conceptual constants for Medic AI ---
const float MEDIC_HEAL_RANGE = 480.0f; // Approx units for Medigun beam
const float MEDIC_HEAL_RANGE_SQ = MEDIC_HEAL_RANGE * MEDIC_HEAL_RANGE;
const float MEDIC_MAX_HEAL_TARGET_SEARCH_RANGE = 1000.0f; // Look for targets in this radius
const float MEDIC_MAX_HEAL_TARGET_SEARCH_RANGE_SQ = MEDIC_MAX_HEAL_TARGET_SEARCH_RANGE * MEDIC_MAX_HEAL_TARGET_SEARCH_RANGE;
const float MEDIC_ALLY_EVAL_INTERVAL = 0.5f;    // How often to re-evaluate best heal target
const float MEDIC_SELF_DEFENSE_SCAN_INTERVAL = 0.3f; // How often to check for direct threats
const float MEDIC_UBER_DURATION = 8.0f;        // Seconds
const float UBER_CHARGE_PER_SECOND_HEALING = 0.03125f; // Approx 3.125% per sec (32s for full charge)
                                                    // This is TF2 specific, FF might differ.
const float UBER_CHARGE_PER_SECOND_NOT_HEALING = 0.015625f; // Slower passive build

// Conceptual button defines (if not from SDK)
#ifndef IN_ATTACK
#define IN_ATTACK (1 << 0)
#endif
#ifndef IN_ATTACK2
#define IN_ATTACK2 (1 << 11)
#endif


CMedicAI_FF::CMedicAI_FF(CFFPlayer* pBotPlayer, CObjectivePlanner* pPlanner,
                       const BotKnowledgeBase* pKnowledgeBase, const ClassConfigInfo* pClassConfig)
    : CFFBaseAI(pBotPlayer, pPlanner, pKnowledgeBase, pClassConfig),
      m_pHealTarget(nullptr),
      m_fUberChargePercentage(0.0f),
      m_bIsUberDeployed(false),
      m_flUberExpireTime(0.0f),
      m_flLastHealTargetCheckTime(0.0f),
      m_flLastEnemyScanTime(0.0f)
{
    std::cout << "CMedicAI_FF created." << std::endl;
}

CMedicAI_FF::~CMedicAI_FF() {}

void CMedicAI_FF::UpdateUberChargeLevel() {
    // Conceptual: This would query m_pBotPlayer->GetUberChargeLevel();
    // Simulating for now:
    float gameTime = 0.0f; // Conceptual: GetCurrentWorldTime();
    static float lastTimeUpdate = 0.0f;
    if (lastTimeUpdate == 0.0f) lastTimeUpdate = gameTime;
    float deltaTime = gameTime - lastTimeUpdate;
    lastTimeUpdate = gameTime;

    if (m_bIsUberDeployed) {
        if (gameTime > m_flUberExpireTime) {
            m_bIsUberDeployed = false;
            m_fUberChargePercentage = 0.0f; // Uber wears off
        }
    } else if (m_fUberChargePercentage < 1.0f) {
        float chargeRate = UBER_CHARGE_PER_SECOND_NOT_HEALING;
        // if (m_pHealTarget && m_pBotPlayer && m_pBotPlayer->IsActivelyHealing()) { // Conceptual
        //     chargeRate = UBER_CHARGE_PER_SECOND_HEALING;
        // }
        m_fUberChargePercentage += chargeRate * deltaTime;
        if (m_fUberChargePercentage > 1.0f) {
            m_fUberChargePercentage = 1.0f;
        }
    }
}

CBaseEntity* CMedicAI_FF::FindBestHealTarget() {
    if (!m_pBotPlayer || !m_pKnowledgeBase /* || !m_pKnowledgeBase->visibleTeammates */) return nullptr;

    CBaseEntity* bestTarget = nullptr;
    float highestScore = -1.0f;
    Vector myPos = m_pBotPlayer->GetPosition();

    // Conceptual: Iterate through m_pKnowledgeBase->visibleTeammates
    // std::vector<CBaseEntity*> visibleTeammates = m_pKnowledgeBase->GetTeammatesNear(myPos, MEDIC_MAX_HEAL_TARGET_SEARCH_RANGE);

    // --- Placeholder Teammate List for demonstration ---
    static CBaseEntity dummyAlly1, dummyAlly2; // Static to have stable addresses for this example
    // In a real game, these would come from the engine/perception system.
    // dummyAlly1.SetHealth(50); dummyAlly1.SetMaxHealth(150); dummyAlly1.SetPosition(myPos + Vector(100,0,0)); dummyAlly1.SetClassId(CLASS_SOLDIER);
    // dummyAlly2.SetHealth(120); dummyAlly2.SetMaxHealth(125); dummyAlly2.SetPosition(myPos + Vector(0,100,0)); dummyAlly2.SetClassId(CLASS_SCOUT);
    std::vector<CBaseEntity*> visibleTeammates = {&dummyAlly1, &dummyAlly2}; // Example
    // --- End Placeholder ---


    for (CBaseEntity* pAlly : visibleTeammates) {
        if (!pAlly || !pAlly->IsAlive() || pAlly == reinterpret_cast<CBaseEntity*>(m_pBotPlayer)) continue;

        // float allyHealthPercent = pAlly->GetHealth() / (float)pAlly->GetMaxHealth(); // Conceptual
        float allyHealthPercent = 0.5f; // Placeholder
        if (allyHealthPercent >= 0.99f /* && !pAlly->NeedsBuff() */) continue; // Full health, no buff needed

        float distSq = (myPos.x - pAlly->GetPosition().x)*(myPos.x - pAlly->GetPosition().x) + (myPos.y - pAlly->GetPosition().y)*(myPos.y - pAlly->GetPosition().y);
        if (distSq > MEDIC_MAX_HEAL_TARGET_SEARCH_RANGE_SQ) continue;

        float score = 100.0f * (1.0f - allyHealthPercent); // Prioritize lower health
        // if (pAlly->IsInCombat()) score += 50.0f; // Conceptual
        // if (pAlly->IsClass(CLASS_HEAVY) || pAlly->IsClass(CLASS_SOLDIER) || pAlly->IsClass(CLASS_DEMOMAN)) score += 30.0f;
        score -= sqrt(distSq) * 0.1f; // Penalize distance

        if (score > highestScore) {
            highestScore = score;
            bestTarget = pAlly;
        }
    }
    return bestTarget;
}

CBaseEntity* CMedicAI_FF::SelectTarget() {
    float currentTime = 0.0f; // Conceptual: GetCurrentWorldTime();

    // Self-preservation: if directly attacked, fight back or flee.
    // if (m_pBotPlayer && m_pBotPlayer->IsTakingHeavyDamage()) { // Conceptual
    //     CBaseEntity* attacker = m_pBotPlayer->GetLastAttacker();
    //     if (attacker && attacker->IsAlive()) {
    //         m_pHealTarget = nullptr; // Stop healing
    //         m_pCurrentTarget = attacker; // Set combat target
    //         return m_pCurrentTarget;
    //     }
    // }

    // Periodically re-evaluate the best heal target
    if (currentTime - m_flLastHealTargetCheckTime > MEDIC_ALLY_EVAL_INTERVAL || !m_pHealTarget || !m_pHealTarget->IsAlive()) {
        m_pHealTarget = FindBestHealTarget();
        m_flLastHealTargetCheckTime = currentTime;
    }

    if (m_pHealTarget && m_pHealTarget->IsAlive()) {
        // Check if heal target is still in range
        // if (m_pBotPlayer && (m_pBotPlayer->GetPosition() - m_pHealTarget->GetPosition()).LengthSqr() > MEDIC_HEAL_RANGE_SQ * 1.2f*1.2f) { // Lost beam
        //     m_pHealTarget = nullptr; // Find new one next cycle
        // } else {
            m_pCurrentTarget = nullptr; // Clear combat target if we have a heal target
            return m_pHealTarget; // This target will be passed to AttackTarget, which will call HealAlly
        // }
    }

    // If no heal target, or current one is invalid, maybe look for an enemy for self-defense
    // This uses the base class CFFBaseAI::SelectTarget() which should find generic enemies
    // m_pCurrentTarget = CFFBaseAI::SelectTarget(); // This is pure virtual, should not be called directly.
    // Instead, implement generic enemy selection here or in a common utility if needed.
    // For now, if no heal target, Medic won't select an enemy unless attacked.
    m_pCurrentTarget = nullptr; // No combat target unless self-defense overrides
    return nullptr;
}

void CMedicAI_FF::Update(CUserCmd* pCmd) {
    if (!m_pBotPlayer || !m_pBotPlayer->IsAlive() || !pCmd || !m_pObjectivePlanner) {
        if(pCmd) { pCmd->buttons = 0; pCmd->forwardmove = 0; pCmd->sidemove = 0;}
        return;
    }

    UpdateUberChargeLevel();
    m_pCurrentTarget = SelectTarget(); // Sets m_pHealTarget or m_pCurrentTarget (enemy)

    // --- Movement logic for following heal target ---
    if (m_pHealTarget && m_pHealTarget->IsAlive()) {
        const SubTask* currentSubTask = m_pObjectivePlanner->GetCurrentSubTask();
        bool needsToMoveCloserToHealTarget = (m_pBotPlayer->GetPosition().x - m_pHealTarget->GetPosition().x) * (m_pBotPlayer->GetPosition().x - m_pHealTarget->GetPosition().x) +
                                             (m_pBotPlayer->GetPosition().y - m_pHealTarget->GetPosition().y) * (m_pBotPlayer->GetPosition().y - m_pHealTarget->GetPosition().y)
                                             > (MEDIC_HEAL_RANGE_SQ * 0.64f); // (heal range * 0.8)^2, try to stay closer

        if (currentSubTask && (currentSubTask->type == SubTaskType::MOVE_TO_POSITION || currentSubTask->type == SubTaskType::MOVE_TO_ENTITY) &&
            !needsToMoveCloserToHealTarget) {
            // If current subtask is movement towards an objective, but we are close enough to heal target,
            // let the base AI handle moving towards objective, but we'll still heal.
            // The medic will prioritize healing over exact objective positioning if heal target is valid.
        } else if (needsToMoveCloserToHealTarget) {
            // Override current path/movement to follow heal target
            Vector followPos = m_pHealTarget->GetPosition();
            // Potentially add offset logic: followPos -= m_pHealTarget->GetForwardVector() * 50.0f;
            ClearCurrentPath(); // Clear any objective-based path
            MoveTo(followPos, pCmd); // Path to heal target's current position
        }
    }
    // --- End Movement Logic ---

    // Call base class Update to handle current subtask from planner
    // This will call ExecuteSubTask, which for Medic's AttackTarget, will call HealAlly or fight.
    CFFBaseAI::Update(pCmd);

    // If no subtask is actively causing button presses (e.g. if subtask was just MOVE and it finished, or IDLE)
    // and we have a heal target, ensure we are healing.
    if (m_pHealTarget && m_pHealTarget->IsAlive() && !(pCmd->buttons & IN_ATTACK)) {
        if ((m_pBotPlayer->GetPosition().x - m_pHealTarget->GetPosition().x) * (m_pBotPlayer->GetPosition().x - m_pHealTarget->GetPosition().x) +
            (m_pBotPlayer->GetPosition().y - m_pHealTarget->GetPosition().y) * (m_pBotPlayer->GetPosition().y - m_pHealTarget->GetPosition().y)
            < MEDIC_HEAL_RANGE_SQ) {
            HealAlly(m_pHealTarget, pCmd); // Ensure heal beam is on if in range and no other action is pressing buttons
        }
    }
}

bool CMedicAI_FF::HealAlly(CBaseEntity* pAlly, CUserCmd* pCmd) {
    if (!pAlly || !pAlly->IsAlive() || !m_pBotPlayer || !pCmd) {
        m_pHealTarget = nullptr; // Clear if target invalid
        return false;
    }
    m_pHealTarget = pAlly; // Set/confirm current heal target

    // Conceptual: SwitchToWeapon(WEAPON_ID_MEDIC_MEDIGUN, pCmd);
    // if (m_pBotPlayer->GetActiveWeaponId() != WEAPON_ID_MEDIC_MEDIGUN) {
    //     return true; // Waiting for weapon switch, action ongoing
    // }

    AimAt(pAlly->GetWorldSpaceCenter(), pCmd);

    float distSq = (m_pBotPlayer->GetPosition().x - pAlly->GetPosition().x)*(m_pBotPlayer->GetPosition().x - pAlly->GetPosition().x) +
                   (m_pBotPlayer->GetPosition().y - pAlly->GetPosition().y)*(m_pBotPlayer->GetPosition().y - pAlly->GetPosition().y);

    if (distSq > MEDIC_HEAL_RANGE_SQ) {
        // std::cout << "Medic: Heal target out of range." << std::endl;
        // Movement to get in range should be handled by Update()/MoveTo()/FollowPath()
        // If we are here, it means we *should* be healing, so if out of range, something else should move us.
        // Don't clear m_pHealTarget here, let SelectTarget handle losing target over time.
        return true; // Still "trying" to heal, but movement needs to occur.
    }

    pCmd->buttons |= IN_ATTACK; // Hold primary fire to heal
    // std::cout << "Medic: Healing ally." << std::endl;

    AttemptUberCharge(pCmd); // Check if Uber should be deployed while healing
    return true; // Healing is ongoing (or attempting to)
}

bool CMedicAI_FF::AttackTarget(CBaseEntity* pTarget, CUserCmd* pCmd) {
    if (!pTarget || !pTarget->IsAlive() || !m_pBotPlayer || !pCmd) {
        m_pCurrentTarget = nullptr; // Clear combat target
        m_pHealTarget = FindBestHealTarget(); // Maybe find a heal target instead
        return false;
    }

    // If the designated pTarget (from planner or self-defense) is an ally, heal them.
    // Conceptual: if (pTarget->GetTeamNumber() == m_pBotPlayer->GetTeamNumber()) {
    if (pTarget == m_pHealTarget || (pTarget->GetTeamNumber() != 0 && pTarget->GetTeamNumber() == m_pBotPlayer->GetTeamNumber()) ) { // Second check is very basic team check
        return HealAlly(pTarget, pCmd);
    }

    // Target is an enemy
    m_pCurrentTarget = pTarget; // Ensure combat target is set
    m_pHealTarget = nullptr;  // Stop healing if engaging an enemy

    // Conceptual: Weapon choice (Syringe or Bonesaw)
    int desiredWeapon = WEAPON_ID_MEDIC_SYRINGEGUN;
    // float distSq = (m_pBotPlayer->GetPosition() - pTarget->GetPosition()).LengthSqr();
    // if (distSq < MELEE_RANGE_SQ_MEDIC) { // Assuming a MELEE_RANGE_SQ_MEDIC
    //    desiredWeapon = WEAPON_ID_MEDIC_MELEE;
    // }

    // SwitchToWeapon(desiredWeapon, pCmd);
    // if (m_pBotPlayer->GetActiveWeaponId() != desiredWeapon) {
    //    return true; // Waiting for weapon switch
    // }

    AimAt(pTarget->GetWorldSpaceCenter(), pCmd);
    pCmd->buttons |= IN_ATTACK;
    // std::cout << "Medic: Attacking enemy with " << desiredWeapon << std::endl;
    return true; // Combat ongoing
}

bool CMedicAI_FF::ShouldDeployUber(CBaseEntity* currentEnemy) const {
    if (m_fUberChargePercentage < 0.99f || !m_pHealTarget || !m_pHealTarget->IsAlive() || !m_pBotPlayer) {
        return false;
    }

    // Example conditions (highly conceptual):
    // 1. Offensive: Pushing with a key teammate into an objective or multiple enemies.
    // float healTargetHealthPercent = m_pHealTarget->GetHealth() / (float)m_pHealTarget->GetMaxHealth();
    // if (healTargetHealthPercent > 0.7f && m_pObjectivePlanner->IsPushingObjective()) { // Conceptual planner state
    //    if (m_pKnowledgeBase->CountNearbyEnemies(m_pHealTarget->GetPosition(), 500.0f) >= 1) return true;
    // }
    // 2. Defensive: Save self or heal target if critical and under heavy fire.
    // if (m_pHealTarget->GetHealthPercentage() < 0.3f && m_pHealTarget->IsTakingDamage()) return true;
    // if (m_pBotPlayer->GetHealthPercentage() < 0.3f && m_pBotPlayer->IsTakingDamage()) return true;
    // 3. Counter Sentry
    // if (currentEnemy && currentEnemy->IsSentry()) return true;

    // For testing, let's say deploy if Uber is full and healing someone.
    if (m_pHealTarget) return true;

    return false;
}

bool CMedicAI_FF::AttemptUberCharge(CUserCmd* pCmd) {
    if (m_bIsUberDeployed) return false; // Already deployed

    if (ShouldDeployUber(m_pCurrentTarget /* can be null */)) {
        // Conceptual: if (m_pBotPlayer->GetActiveWeaponId() == WEAPON_ID_MEDIC_MEDIGUN) {
            pCmd->buttons |= IN_ATTACK2; // Secondary fire for Uber
            m_fUberChargePercentage = 0.0f; // Resets after pop
            m_bIsUberDeployed = true;
            m_flUberExpireTime = 0.0f /*GetCurrentWorldTime()*/ + MEDIC_UBER_DURATION;
            std::cout << "CMedicAI_FF: UBERCHARGE DEPLOYED!" << std::endl;
            return true; // Uber popped this frame
        // }
    }
    return false; // Uber not deployed or conditions not met
}

bool CMedicAI_FF::UseAbility(int abilitySlot, CBaseEntity* pTarget, CUserCmd* pCmd) {
    // Assuming abilitySlot 0 for Medic is Ubercharge
    if (abilitySlot == 0) {
        // pTarget might be used to make sure we are healing someone relevant before ubering.
        // For now, AttemptUberCharge uses m_pHealTarget internally.
        return AttemptUberCharge(pCmd);
    }
    return CFFBaseAI::UseAbility(abilitySlot, pTarget, pCmd); // Call base if not handled
}

void CMedicAI_FF::SwitchToWeapon(int weaponId, CUserCmd* pCmd) {
    // Conceptual
    // if (m_pBotPlayer && m_pBotPlayer->GetActiveWeaponId() != weaponId) {
    //    pCmd->weaponselect = weaponId;
    //    std::cout << "Medic: Attempting to switch to weapon ID: " << weaponId << std::endl;
    // }
}
