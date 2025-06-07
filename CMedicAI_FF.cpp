#include "CMedicAI_FF.h"
#include "ObjectivePlanner.h"
#include "BotTasks.h"
#include "FFStateStructs.h"
// Conceptual includes (ensure these are defined or properly included from FFBaseAI.cpp's similar section)
#include "FFBaseAI.h" // For BotKnowledgeBase, and conceptual CFFPlayer/CBaseEntity/UserCmd if defined there

#include <iostream>
#include <vector>
#include <algorithm>          // For std::sort, std::min_element etc.

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// --- Conceptual constants for Medic AI ---
const float MEDIC_HEAL_RANGE = 480.0f;
const float MEDIC_HEAL_RANGE_SQ = MEDIC_HEAL_RANGE * MEDIC_HEAL_RANGE;
const float MEDIC_MAX_HEAL_TARGET_SEARCH_RANGE_SQ = 1000.0f * 1000.0f;
const float MEDIC_FOLLOW_HEAL_TARGET_DISTANCE = 200.0f;
const float MEDIC_FOLLOW_HEAL_TARGET_DISTANCE_SQ = MEDIC_FOLLOW_HEAL_TARGET_DISTANCE * MEDIC_FOLLOW_HEAL_TARGET_DISTANCE;

const float MEDIC_ALLY_EVAL_INTERVAL_SECONDS = 0.5f;
const float MEDIC_SELF_DEFENSE_SCAN_INTERVAL_SECONDS = 0.3f;
const float MEDIC_UBER_DURATION_SECONDS = 8.0f;
const float MEDIC_UBER_CHARGE_PER_SEC_HEALING_SIM = 1.0f / 32.0f; // Sim value
const float MEDIC_UBER_CHARGE_PER_SEC_NOT_HEALING_SIM = 1.0f / 64.0f; // Sim value

// Conceptual button defines
#ifndef IN_ATTACK
#define IN_ATTACK (1 << 0)
#endif
#ifndef IN_ATTACK2
#define IN_ATTACK2 (1 << 11)
#endif
#ifndef IN_RELOAD
#define IN_RELOAD (1 << 3)
#endif

// Conceptual global time for placeholders
// In a real scenario, this would come from CBot::GetTime() or gpGlobals->curtime
static float g_fMedicConceptualWorldTime = 0.0f;
void AdvanceMedicConceptualTime() { g_fMedicConceptualWorldTime += 0.015f; } // Call per frame for simulation
float GetMedicConceptualWorldTime() { return g_fMedicConceptualWorldTime; }


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
    // std::cout << "CMedicAI_FF created for bot " << (m_pBotPlayer ? m_pBotPlayer->GetNamePlaceholder() : "Unknown") << std::endl;
}

CMedicAI_FF::~CMedicAI_FF() {}

void CMedicAI_FF::UpdateUberChargeLevel() {
    float gameTime = GetMedicConceptualWorldTime();
    // In real scenario: m_fUberChargePercentage = m_pBotPlayer->GetUberChargePercent();
    // Simulation:
    if (m_bIsUberDeployed) {
        if (gameTime >= m_flUberExpireTime) {
            m_bIsUberDeployed = false;
            // Uber is expended, game would set charge to 0. Bot will read it next frame.
            // std::cout << "Medic Uber expired naturally." << std::endl;
        }
    } else { // Not deployed, try to build charge
        if (m_fUberChargePercentage < 1.0f) {
            float chargeRate = MEDIC_UBER_CHARGE_PER_SEC_NOT_HEALING_SIM;
            // Conceptual: if (m_pBotPlayer && m_pBotPlayer->IsActivelyHealingWithMedigun()) {
            //    chargeRate = MEDIC_UBER_CHARGE_PER_SEC_HEALING_SIM;
            // }
            // Use a fixed delta time for simulation if gameTime is not advancing properly
            float deltaTime = 0.015f; // Assuming ~66 ticks per second for simulation rate
            m_fUberChargePercentage += chargeRate * deltaTime;
            if (m_fUberChargePercentage > 1.0f) {
                m_fUberChargePercentage = 1.0f;
            }
        }
    }
}

CBaseEntity* CMedicAI_FF::FindBestHealTarget() {
    if (!m_pBotPlayer || !m_pKnowledgeBase) return nullptr;

    CBaseEntity* pBestTarget = nullptr;
    float highestScore = -1000.0f;
    Vector myPos = m_pBotPlayer->GetPosition();
    int myTeam = m_pBotPlayer->GetTeamNumber();

    for (const auto& allyInfo : m_pKnowledgeBase->GetTrackedAllies()) {
        if (!allyInfo.pEdict) continue;
        // CBaseEntity* pAlly = reinterpret_cast<CBaseEntity*>(allyInfo.pEdict); // Highly conceptual and unsafe without proper casting/entity system
        // For now, create a temporary conceptual CBaseEntity to use its methods for scoring, using allyInfo data
        CBaseEntity conceptualAlly; // This is problematic, as it's not the real entity.
                                    // We should primarily use allyInfo's direct fields.
        // if (allyInfo.pEdict == m_pBotPlayer->GetEdict()) continue; // Don't heal self with this logic
        if (allyInfo.health <= 0) continue;

        float allyHealthPercent = (allyInfo.maxHealth > 0) ? (float)allyInfo.health / allyInfo.maxHealth : 1.0f;
        if (allyHealthPercent >= 0.99f /* && !allyInfo.isOverhealed() */) continue;

        float distSq = (myPos.x - allyInfo.lastKnownPosition.x)*(myPos.x - allyInfo.lastKnownPosition.x) +
                       (myPos.y - allyInfo.lastKnownPosition.y)*(myPos.y - allyInfo.lastKnownPosition.y);
        if (distSq > MEDIC_MAX_HEAL_TARGET_SEARCH_RANGE_SQ) continue;

        float score = 100.0f * (1.0f - allyHealthPercent);
        // if (allyInfo.isInCombat_conceptual_flag) score += 50.0f;
        // if (allyInfo.className == "Heavy" || allyInfo.className == "Soldier") score += 30.0f;
        score -= sqrt(distSq) * 0.05f;

        if (score > highestScore) {
            highestScore = score;
            // pBestTarget = pAlly; // This requires pAlly to be a valid, persistent CBaseEntity*
            // For now, we can't return a CBaseEntity* reliably from TrackedEntityInfo without an entity manager.
            // This function's purpose is more to identify *which* TrackedEntityInfo is best.
            // The actual CBaseEntity* would be obtained via its edict_t* when needed.
            // For this example, let's assume TrackedEntityInfo could store a CBaseEntity* if available from perception.
            // If not, this function might return the edict_t* or ID.
        }
    }
    // This is where we'd convert the best TrackedEntityInfo's edict to CBaseEntity*. For now, returning null.
    return nullptr;
}

CBaseEntity* CMedicAI_FF::SelectTarget() {
    float currentTime = GetMedicConceptualWorldTime();

    // High priority: Self-defense if directly attacked
    // Conceptual: if (m_pBotPlayer && m_pBotPlayer->IsTakingSignificantDamage()) {
    //    CBaseEntity* pLastAttacker = m_pBotPlayer->GetLastAttacker();
    //    if (pLastAttacker && pLastAttacker->IsAlive() && pLastAttacker->GetTeamNumber() != m_pBotPlayer->GetTeamNumber()) {
    //        m_pHealTarget = nullptr; // Stop healing
    //        m_pCurrentTarget = pLastAttacker; // This is an enemy
    //        return m_pCurrentTarget;
    //    }
    // }

    bool needsNewHealTarget = false;
    if (!m_pHealTarget || !m_pHealTarget->IsAlive()) {
        needsNewHealTarget = true;
    } else if (m_pBotPlayer) {
        float distToHealTargetSq = (m_pBotPlayer->GetPosition().x - m_pHealTarget->GetPosition().x)*(m_pBotPlayer->GetPosition().x - m_pHealTarget->GetPosition().x) + (m_pBotPlayer->GetPosition().y - m_pHealTarget->GetPosition().y)*(m_pBotPlayer->GetPosition().y - m_pHealTarget->GetPosition().y);
        if (distToHealTargetSq > MEDIC_HEAL_BEAM_RANGE_SQ * 1.8f * 1.8f) { // If target is way too far (beam definitely broken)
            needsNewHealTarget = true;
        }
        // Conceptual: float healTargetHealthPercent = m_pHealTarget->GetHealth() / (float)m_pHealTarget->GetMaxHealth();
        // if (healTargetHealthPercent >= 0.99f /* && !m_pHealTarget->IsOverhealed() */) {
        //    needsNewHealTarget = true; // Current target is fully healed
        // }
    }
    if (currentTime - m_flLastHealTargetCheckTime > MEDIC_ALLY_EVAL_INTERVAL_SECONDS) {
        needsNewHealTarget = true;
    }

    if (needsNewHealTarget) {
        m_pHealTarget = FindBestHealTarget();
        m_flLastHealTargetCheckTime = currentTime;
    }

    if (m_pHealTarget && m_pHealTarget->IsAlive()) {
        m_pCurrentTarget = nullptr;
        return m_pHealTarget; // This will be interpreted as "heal this target" by AttackTarget
    }

    // Fallback: No heal target, scan for enemies for self-defense if appropriate.
    // This would use a generic enemy scanning logic from CFFBaseAI or here.
    // m_pCurrentTarget = CFFBaseAI::SelectTarget(); // This is pure virtual.
    // For now, Medic only fights if forced by self-defense (handled above) or if AttackTarget gets an enemy.
    m_pCurrentTarget = nullptr;
    return nullptr;
}

void CMedicAI_FF::Update(CUserCmd* pCmd) {
    if (!m_pBotPlayer || !m_pBotPlayer->IsValid() || !m_pBotPlayer->IsAlive() || !pCmd || !m_pObjectivePlanner) {
        if(pCmd) {pCmd->buttons = 0; pCmd->forwardmove = 0; pCmd->sidemove = 0;}
        return;
    }
    AdvanceMedicConceptualTime(); // For internal timers if GetMedicConceptualWorldTime is used
    UpdateUberChargeLevel();

    // SelectTarget will set m_pHealTarget if a suitable ally is found,
    // or m_pCurrentTarget (enemy) if self-defending.
    // The target passed to AttackTarget by ExecuteSubTask will be this.
    // For a Medic, CFFBaseAI::Update will call ExecuteSubTask. If the subtask is ATTACK_TARGET,
    // it will use the result of this class's SelectTarget().

    // --- Follow Heal Target Logic ---
    // This logic needs to influence the m_vCurrentMoveToTarget used by CFFBaseAI::MoveTo/FollowPath
    // when a movement subtask is active.
    const SubTask* currentSubTask = m_pObjectivePlanner->GetCurrentSubTask();
    if (m_pHealTarget && m_pHealTarget->IsAlive()) {
        Vector myPos = m_pBotPlayer->GetPosition();
        Vector healTargetPos = m_pHealTarget->GetPosition();
        float distToHealTargetSq = (myPos.x-healTargetPos.x)*(myPos.x-healTargetPos.x) + (myPos.y-healTargetPos.y)*(myPos.y-healTargetPos.y);

        bool shouldFollowCloser = distToHealTargetSq > (MEDIC_HEAL_BEAM_RANGE_SQ * 0.49f); // (range * 0.7)^2, try to stay well within range

        if (currentSubTask &&
            (currentSubTask->type == SubTaskType::MOVE_TO_POSITION || currentSubTask->type == SubTaskType::MOVE_TO_ENTITY)) {

            if (shouldFollowCloser || currentSubTask->pTargetEntity == m_pHealTarget) { // If task is to follow heal target, or we need to get closer for healing
                Vector desiredFollowPos = healTargetPos;
                // Conceptual offset to stay behind the heal target
                // Vector healTargetForward = m_pHealTarget->GetForwardVector();
                // desiredFollowPos.x -= healTargetForward.x * MEDIC_FOLLOW_HEAL_TARGET_DISTANCE * 0.5f;
                // desiredFollowPos.y -= healTargetForward.y * MEDIC_FOLLOW_HEAL_TARGET_DISTANCE * 0.5f;

                // Check if current high-level move target needs to be updated
                if ((m_vCurrentMoveToTarget.x - desiredFollowPos.x)*(m_vCurrentMoveToTarget.x - desiredFollowPos.x) +
                    (m_vCurrentMoveToTarget.y - desiredFollowPos.y)*(m_vCurrentMoveToTarget.y - desiredFollowPos.y) > 32.0f*32.0f ) { // If desired follow pos changed significantly
                     ClearCurrentPath(); // Force re-plan in CFFBaseAI::ExecuteSubTask
                     m_vCurrentMoveToTarget = desiredFollowPos; // CFFBaseAI::ExecuteSubTask will use this for MoveTo
                }
            }
        }
    }
    // --- End Follow Heal Target Logic ---

    CFFBaseAI::Update(pCmd); // Base class handles subtask execution using current targets & m_vCurrentMoveToTarget

    // After base Update (which might have set IN_ATTACK for combat against an enemy),
    // ensure Medigun is still active and healing if m_pHealTarget is set and no direct enemy threat.
    if (m_pHealTarget && m_pHealTarget->IsAlive() && m_pCurrentTarget == nullptr) { // No current *enemy* target
        bool isActivelyHealingBeam = (pCmd->buttons & IN_ATTACK) &&
                                   (m_pBotPlayer && /* m_pBotPlayer->GetActiveWeaponId() == WEAPON_ID_MEDIC_MEDIGUN */ true);

        if (!isActivelyHealingBeam) { // If base::Update didn't result in healing (e.g. subtask was not ATTACK_TARGET)
             Vector myPos = m_pBotPlayer->GetPosition();
             Vector healTargetPos = m_pHealTarget->GetPosition();
             if ((myPos.x-healTargetPos.x)*(myPos.x-healTargetPos.x) + (myPos.y-healTargetPos.y)*(myPos.y-healTargetPos.y) < MEDIC_HEAL_BEAM_RANGE_SQ) {
                HealAlly(m_pHealTarget, pCmd); // This will set IN_ATTACK and aim
            }
        }
    }
}

bool CMedicAI_FF::HealAlly(CBaseEntity* pAlly, CUserCmd* pCmd) {
    if (!pAlly || !pAlly->IsAlive() || !m_pBotPlayer || !pCmd) {
        if (m_pHealTarget == pAlly) m_pHealTarget = nullptr;
        return false;
    }
    m_pHealTarget = pAlly;

    SwitchToWeapon(WEAPON_ID_MEDIC_MEDIGUN, pCmd);
    // if (m_pBotPlayer->GetActiveWeaponId_Placeholder() != WEAPON_ID_MEDIC_MEDIGUN) {
    //     return true;
    // }

    AimAt(pAlly->GetWorldSpaceCenter(), pCmd);

    Vector myPos = m_pBotPlayer->GetPosition();
    Vector allyPos = pAlly->GetPosition();
    float distSq = (myPos.x - allyPos.x)*(myPos.x - allyPos.x) + (myPos.y - allyPos.y)*(myPos.y - allyPos.y);

    if (distSq > MEDIC_HEAL_BEAM_RANGE_SQ * 1.05f*1.05f) { // Small tolerance before beam breaks
        // Movement should handle getting closer. Don't clear m_pHealTarget here immediately.
        // Stop pressing attack if out of range to prevent "dry firing" beam.
        // But the "intent" to heal is ongoing.
        return true;
    }

    pCmd->buttons |= IN_ATTACK;
    AttemptUberCharge(pCmd);
    return true;
}

bool CMedicAI_FF::AttackTarget(CBaseEntity* pTarget, CUserCmd* pCmd) {
    if (!pTarget || !pTarget->IsAlive() || !m_pBotPlayer || !pCmd) {
        if (m_pCurrentTarget == pTarget) m_pCurrentTarget = nullptr;
        if (m_pHealTarget == pTarget) m_pHealTarget = nullptr;
        return false;
    }

    if (pTarget->GetTeamNumber() != 0 && pTarget->GetTeamNumber() == m_pBotPlayer->GetTeamNumber()) {
        // Target is an ally, switch to healing.
        m_pCurrentTarget = nullptr; // Clear combat target
        return HealAlly(pTarget, pCmd);
    }

    // Target is an enemy
    m_pCurrentTarget = pTarget;
    if (m_pHealTarget == pTarget) m_pHealTarget = nullptr; // Cannot heal an enemy target

    int desiredWeapon = WEAPON_ID_MEDIC_SYRINGEGUN;
    // float distSq = (m_pBotPlayer->GetPosition() - pTarget->GetPosition()).LengthSqr();
    // if (distSq < MELEE_RANGE_SQ_MEDIC_CONCEPTUAL) { desiredWeapon = WEAPON_ID_MEDIC_MELEE; }

    SwitchToWeapon(desiredWeapon, pCmd);
    // if (m_pBotPlayer->GetActiveWeaponId_Placeholder() != desiredWeapon) {
    //    return true; // Waiting for weapon switch
    // }

    AimAt(pTarget->GetWorldSpaceCenter(), pCmd);
    pCmd->buttons |= IN_ATTACK;
    return true;
}

bool CMedicAI_FF::ShouldDeployUber(CBaseEntity* currentEnemyContext) const {
    if (m_fUberChargePercentage < 0.99f || !m_pBotPlayer || !m_pBotPlayer->IsAlive()) {
        return false;
    }
    if (!m_pHealTarget || !m_pHealTarget->IsAlive()) return false;

    // Example conditions:
    // 1. Heal target is critical and in combat.
    // if (m_pHealTarget->GetHealthPercent() < 0.5f && m_pHealTarget->IsInCombat()) return true;
    // 2. Pushing into a Sentry Gun.
    // if (currentEnemyContext && currentEnemyContext->IsSentryGun()) return true;
    // 3. Multiple allies nearby are low health / in combat.
    // if (m_pKnowledgeBase->CountNearbyCriticallyInjuredAllies(m_pHealTarget->GetPosition()) >= 1) return true;

    // For testing, if Uber is full and healing an alive target that is not fully overhealed.
    // float healTargetHPPercent = m_pHealTarget->GetHealth() / (float)m_pHealTarget->GetMaxHealth();
    // if (m_pHealTarget && m_pHealTarget->IsAlive() && healTargetHPPercent < 1.4f) return true;
    return true; // Simplified: pop if full and has heal target
}

bool CMedicAI_FF::AttemptUberCharge(CUserCmd* pCmd) {
    if (!pCmd) return false;
    float gameTime = GetMedicConceptualWorldTime();

    if (m_bIsUberDeployed) {
         if (gameTime >= m_flUberExpireTime) {
             m_bIsUberDeployed = false;
         }
        return false;
    }

    if (ShouldDeployUber(m_pCurrentTarget /* enemy context for decision, can be null */)) {
        // if (m_pBotPlayer->GetActiveWeaponId_Placeholder() == WEAPON_ID_MEDIC_MEDIGUN) {
            pCmd->buttons |= IN_ATTACK2;
            m_bIsUberDeployed = true;
            m_flUberExpireTime = gameTime + MEDIC_UBER_DURATION_SECONDS;
            // std::cout << "CMedicAI_FF: UBERCHARGE DEPLOYED! Expires at " << m_flUberExpireTime << std::endl;
            return true;
        // }
    }
    return false;
}

bool CMedicAI_FF::UseAbility(int abilitySlot, CBaseEntity* pTarget, CUserCmd* pCmd) {
    if (!pCmd) return false;
    if (abilitySlot == 0) { // Assuming slot 0 is Uber for Medic
        return AttemptUberCharge(pCmd);
    }
    return CFFBaseAI::UseAbility(abilitySlot, pTarget, pCmd);
}

void CMedicAI_FF::SwitchToWeapon(int weaponId, CUserCmd* pCmd) {
    if (!pCmd) return;
    // Conceptual
    // if (m_pBotPlayer && m_pBotPlayer->GetActiveWeaponId_Placeholder() != weaponId) {
    //    pCmd->weaponselect = weaponId;
    // }
}
