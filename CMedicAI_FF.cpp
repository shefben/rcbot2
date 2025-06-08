#include "CMedicAI_FF.h"
#include "ObjectivePlanner.h"
#include "BotTasks.h"
#include "FFStateStructs.h"   // For Vector, QAngle if not using SDK's directly in structs
#include "CFFPlayer.h"        // Renamed to CFFPlayerWrapper.h
#include "BotKnowledgeBase.h"
#include "FFBot_SDKDefines.h"     // For weapon/ammo name constants, SDK TEAM/CLASS defines
#include "game/shared/in_buttons.h" // For IN_ATTACK etc. (SDK header)
#include "game/server/cbase.h"      // For CBaseEntity (SDK header)
#include "TrackedEntityInfo.h"    // For TrackedEntityInfo struct

#include <iostream>
#include <vector>
#include <algorithm>          // For std::sort, std::min_element etc.

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Constants (some might move to ClassConfigInfo or BotDefines.h)
const float MEDIC_HEAL_RANGE_SQ = 480.0f * 480.0f;
const float MEDIC_MAX_HEAL_TARGET_SEARCH_RANGE_SQ = 1000.0f * 1000.0f;
const float MEDIC_FOLLOW_HEAL_TARGET_DISTANCE_SQ = 200.0f * 200.0f;
const float MEDIC_ALLY_EVAL_INTERVAL_SECONDS = 0.5f;
const float MEDIC_SELF_DEFENSE_SCAN_INTERVAL_SECONDS = 0.3f;
const float MEDIC_UBER_DURATION_SECONDS = 8.0f;


CMedicAI_FF::CMedicAI_FF(CFFPlayerWrapper* pBotPlayer, CObjectivePlanner* pPlanner,
                       BotKnowledgeBase* pKnowledgeBase, const ClassConfigInfo* pClassConfig)
    : CFFBaseAI(pBotPlayer, pPlanner, pKnowledgeBase, pClassConfig),
      m_pHealTargetInfo(nullptr), // Changed from m_pHealTarget (CBaseEntity*)
      m_fUberChargePercentage(0.0f),
      m_bIsUberDeployed(false),
      m_flUberExpireTime(0.0f),
      m_flLastHealTargetCheckTime(0.0f),
      m_flLastEnemyScanTime(0.0f)
{
    // Constructor
}

CMedicAI_FF::~CMedicAI_FF() {}

void CMedicAI_FF::UpdateUberChargeLevel() {
    if (!m_pBotPlayer || !m_pBotPlayer->IsValid()) return;
    // m_fUberChargePercentage = m_pBotPlayer->GetUberChargeLevel_Conceptual() / 100.0f; // Conceptual CFFPlayerWrapper method
    // Example direct SDK access:
    // if (m_pBotPlayer->GetSDKPlayer()) {
    //    m_fUberChargePercentage = m_pBotPlayer->GetSDKPlayer()->m_flChargeLevel / 100.0f; // Assuming SDK ::CFFPlayer has m_flChargeLevel
    // }


    float currentTime = GetWorldTime();
    if (m_bIsUberDeployed && currentTime >= m_flUberExpireTime) {
        m_bIsUberDeployed = false;
    }
}

TrackedEntityInfo* CMedicAI_FF::FindBestHealTarget() {
    if (!m_pBotPlayer || !m_pKnowledgeBase) return nullptr;

    TrackedEntityInfo* pBestTargetInfo = nullptr;
    float highestScore = -1000.0f;
    Vector myPos = m_pBotPlayer->GetOrigin();
    int myTeam = m_pBotPlayer->GetTeam();

    for (const auto& allyInfo : m_pKnowledgeBase->GetTrackedAllies()) {
        if (allyInfo.pEdict == m_pBotPlayer->GetEdict()) continue;
        if (allyInfo.health <= 0) continue;

        float allyHealthPercent = (allyInfo.maxHealth > 0) ? (float)allyInfo.health / allyInfo.maxHealth : 1.0f;
        if (allyHealthPercent >= 0.99f && !allyInfo.isOnFire) continue; // Heal burning allies even if full HP

        float distSq = myPos.DistToSqr(allyInfo.lastKnownPosition);
        if (distSq > MEDIC_MAX_HEAL_TARGET_SEARCH_RANGE_SQ) continue;

        float score = 100.0f * (1.0f - allyHealthPercent);
        if (allyInfo.isOnFire) score += 100.0f; // Prioritize burning allies

        // FF_BotPlayerClassID allyClassID = GetBotClassIDFromString_FF(allyInfo.className);
        // if (allyClassID == FF_BOT_CLASS_HVYWEAP || allyClassID == FF_BOT_CLASS_SOLDIER || allyClassID == FF_BOT_CLASS_DEMOMAN) {
        //    score += 30.0f; // Prioritize power classes
        // }
        score -= sqrt(distSq) * 0.05f;

        if (score > highestScore) {
            highestScore = score;
            pBestTargetInfo = const_cast<TrackedEntityInfo*>(&allyInfo);
        }
    }
    return pBestTargetInfo;
}

CBaseEntity* CMedicAI_FF::SelectTarget() {
    float currentTime = GetWorldTime();

    // Conceptual: Prioritize self-defense if under direct attack
    // CBaseEntity* lastAttacker = m_pBotPlayer->GetLastAttacker_SDK();
    // if (lastAttacker && lastAttacker->IsAlive() && lastAttacker->GetTeamNumber() != m_pBotPlayer->GetTeam()) {
    //     m_pHealTargetInfo = nullptr;
    //     SetCurrentTarget(lastAttacker);
    //     return GetCurrentTarget();
    // }

    bool needsNewHealTarget = false;
    if (!m_pHealTargetInfo || m_pHealTargetInfo->health <= 0) {
        needsNewHealTarget = true;
    } else if (m_pBotPlayer) {
        float distToHealTargetSq = m_pBotPlayer->GetOrigin().DistToSqr(m_pHealTargetInfo->lastKnownPosition);
        if (distToHealTargetSq > MEDIC_HEAL_RANGE_SQ * 2.25f) { // Increased break distance (1.5 * range)^2
            needsNewHealTarget = true;
        }
        float healTargetHealthPercent = (m_pHealTargetInfo->maxHealth > 0) ? (float)m_pHealTargetInfo->health / m_pHealTargetInfo->maxHealth : 1.0f;
        if (healTargetHealthPercent >= 0.99f && !m_pHealTargetInfo->isOnFire) { // Stop if full and not burning
           needsNewHealTarget = true;
        }
    }
    if (currentTime - m_flLastHealTargetCheckTime > MEDIC_ALLY_EVAL_INTERVAL_SECONDS) {
        needsNewHealTarget = true;
    }

    if (needsNewHealTarget) {
        m_pHealTargetInfo = FindBestHealTarget();
        m_flLastHealTargetCheckTime = currentTime;
    }

    if (m_pHealTargetInfo && m_pHealTargetInfo->health > 0 && m_pHealTargetInfo->pEdict) {
        SetCurrentTarget(nullptr);
        return CBaseEntity::Instance(m_pHealTargetInfo->pEdict); // Return CBaseEntity* for healing
    }

    SetCurrentTarget(CFFBaseAI::SelectTarget()); // Fallback to base enemy selection
    return GetCurrentTarget();
}

void CMedicAI_FF::Update(CUserCmd* pCmd) {
    if (!m_pBotPlayer || !m_pBotPlayer->IsValid() || !m_pBotPlayer->IsAlive() || !pCmd || !m_pObjectivePlanner) {
        if(pCmd) { pCmd->buttons = 0; pCmd->forwardmove = 0.0f; pCmd->sidemove = 0.0f; }
        return;
    }
    UpdateUberChargeLevel();

    CBaseEntity* currentSelectedTargetAsEntity = SelectTarget(); // This will set m_pHealTargetInfo or CFFBaseAI::m_pCurrentTarget

    // --- Follow Heal Target Logic ---
    if (m_pHealTargetInfo && m_pHealTargetInfo->health > 0) {
        Vector myPos = m_pBotPlayer->GetOrigin();
        Vector healTargetPos = m_pHealTargetInfo->lastKnownPosition;
        float distToHealTargetSq = myPos.DistToSqr(healTargetPos);
        bool shouldFollowCloser = distToHealTargetSq > (MEDIC_FOLLOW_HEAL_TARGET_DISTANCE_SQ);

        const SubTask* currentSubTask = m_pObjectivePlanner->GetCurrentSubTask();
        if (currentSubTask &&
            (currentSubTask->type == SubTaskType::MOVE_TO_POSITION || currentSubTask->type == SubTaskType::MOVE_TO_ENTITY_DYNAMIC_FF)) {

            bool taskIsToFollowHealTarget = currentSubTask->pTargetEntity_SDK && m_pHealTargetInfo->pEdict &&
                                            currentSubTask->pTargetEntity_SDK->edict() == m_pHealTargetInfo->pEdict;

            if (shouldFollowCloser || taskIsToFollowHealTarget ) {
                Vector desiredFollowPos = healTargetPos;
                // TODO: Add offset logic (e.g. stay behind target)
                if (m_vCurrentMoveToTarget.DistToSqr(desiredFollowPos) > 32.0f*32.0f ) {
                     ClearCurrentPath();
                     m_vCurrentMoveToTarget = desiredFollowPos;
                }
            }
        }
    }
    // --- End Follow Heal Target Logic ---

    CFFBaseAI::Update(pCmd); // Base handles subtask execution using CFFBaseAI::m_pCurrentTarget or m_vCurrentMoveToTarget

    // If no enemy is targeted by base AI, and we have a heal target, ensure healing actions.
    if (m_pHealTargetInfo && m_pHealTargetInfo->health > 0 && GetCurrentTarget() == nullptr) {
        CBaseEntity* healTargetEntity = m_pHealTargetInfo->pEdict ? CBaseEntity::Instance(m_pHealTargetInfo->pEdict) : nullptr;
        if (healTargetEntity && healTargetEntity->IsAlive()) {
           // bool isCurrentlyHealingThisTarget = (pCmd->buttons & IN_ATTACK) && m_pBotPlayer->IsWeaponActive_Conceptual("ff_weapon_medikit");
           // if (!isCurrentlyHealingThisTarget) {
                 if (m_pBotPlayer->GetOrigin().DistToSqr(healTargetEntity->GetAbsOrigin()) < MEDIC_HEAL_RANGE_SQ) {
                    HealAlly(healTargetEntity, pCmd);
                }
           // }
        }
    }
}

bool CMedicAI_FF::HealAlly(CBaseEntity* pAlly, CUserCmd* pCmd) {
    if (!pAlly || !pAlly->IsAlive() || !m_pBotPlayer || !pCmd) {
        if (m_pHealTargetInfo && m_pHealTargetInfo->pEdict == (pAlly ? pAlly->edict() : nullptr)) m_pHealTargetInfo = nullptr;
        return false;
    }
    // Update m_pHealTargetInfo if pAlly corresponds to a tracked ally
    // const auto& allies = m_pKnowledgeBase->GetTrackedAllies();
    // auto it = std::find_if(allies.begin(), allies.end(), [&](const TrackedEntityInfo& tei){ return tei.pEdict == pAlly->edict(); });
    // if (it != allies.end()) m_pHealTargetInfo = const_cast<TrackedEntityInfo*>(&(*it));

    m_pBotPlayer->SelectWeapon(pCmd, "ff_weapon_medikit"); // Use SDK weapon classname
    // if (m_pBotPlayer->IsSwitchingWeapon()) return true; // Conceptual: wait for switch

    AimAt(pAlly->GetAbsOrigin(), pCmd);

    if (m_pBotPlayer->GetOrigin().DistToSqr(pAlly->GetAbsOrigin()) > MEDIC_HEAL_RANGE_SQ * 1.05f*1.05f) {
        return true;
    }

    m_pBotPlayer->AddButton(pCmd, IN_ATTACK);
    AttemptUberCharge(pCmd);
    return true;
}

bool CMedicAI_FF::AttackTarget(CBaseEntity* pTarget, CUserCmd* pCmd) {
    if (!pTarget || !pTarget->IsAlive() || !m_pBotPlayer || !pCmd) {
        if (GetCurrentTarget() == pTarget) SetCurrentTarget(nullptr);
        if (m_pHealTargetInfo && m_pHealTargetInfo->pEdict == (pTarget ? pTarget->edict() : nullptr)) m_pHealTargetInfo = nullptr;
        return false;
    }

    if (pTarget->GetTeamNumber() != 0 && pTarget->GetTeamNumber() == m_pBotPlayer->GetTeam()) {
        SetCurrentTarget(nullptr);
        return HealAlly(pTarget, pCmd);
    }

    SetCurrentTarget(pTarget);
    if (m_pHealTargetInfo && m_pHealTargetInfo->pEdict == pTarget->edict()) m_pHealTargetInfo = nullptr;

    std::string desiredWeapon = "ff_weapon_syringegun";
    // float MELEE_RANGE_SQ_MEDIC_CONCEPTUAL = 80.0f * 80.0f; // Define this
    // if (m_pBotPlayer->GetOrigin().DistToSqr(pTarget->GetAbsOrigin()) < MELEE_RANGE_SQ_MEDIC_CONCEPTUAL) {
    //    desiredWeapon = "ff_weapon_bonesaw";
    // }

    m_pBotPlayer->SelectWeapon(pCmd, desiredWeapon);
    // if (m_pBotPlayer->IsSwitchingWeapon_Conceptual()) return true;

    AimAt(pTarget->GetAbsOrigin(), pCmd);
    m_pBotPlayer->AddButton(pCmd, IN_ATTACK);
    return true;
}

bool CMedicAI_FF::ShouldDeployUber(CBaseEntity* currentEnemyContext) const {
    // if (m_fUberChargePercentage < 0.99f || !m_pBotPlayer || !m_pBotPlayer->IsAlive()) {
    //     return false;
    // }
    // if (!m_pHealTargetInfo || m_pHealTargetInfo->health <=0 ) return false;

    // CBaseEntity* healTargetEntity = m_pHealTargetInfo->pEdict ? CBaseEntity::Instance(m_pHealTargetInfo->pEdict) : nullptr;
    // if (!healTargetEntity || !healTargetEntity->IsAlive()) return false;

    // float healTargetHPPercent = (float)healTargetEntity->GetHealth() / healTargetEntity->GetMaxHealth(); // SDK calls
    // if (healTargetHPPercent < 0.5f /* && healTargetEntity->IsInCombat_SDK() */) return true;
    // if (currentEnemyContext && /* currentEnemyContext->IsSentryGun_SDK() */ false ) return true;

    return true; // Simplified for now
}

bool CMedicAI_FF::AttemptUberCharge(CUserCmd* pCmd) {
    if (!pCmd || !m_pBotPlayer) return false;
    float currentTime = GetWorldTime();

    if (m_bIsUberDeployed) {
         if (currentTime >= m_flUberExpireTime) {
             m_bIsUberDeployed = false;
         }
        return false;
    }

    // m_fUberChargePercentage = m_pBotPlayer->GetUberChargeLevel_Conceptual(); // Update from CFFPlayerWrapper
    // if (ShouldDeployUber(GetCurrentTarget())) {
    //     if (m_pBotPlayer->IsWeaponActive_Conceptual("ff_weapon_medikit")) {
    //         m_pBotPlayer->AddButton(pCmd, IN_ATTACK2);
    //         m_bIsUberDeployed = true;
    //         m_flUberExpireTime = currentTime + MEDIC_UBER_DURATION_SECONDS;
    //         return true;
    //     }
    // }
    return false;
}

bool CMedicAI_FF::UseAbility(int abilitySlot, CBaseEntity* pTargetEntity, const Vector& targetPosition, CUserCmd* pCmd) {
    if (!pCmd) return false;
    if (abilitySlot == 0) { // Assuming slot 0 is Uber for Medic
        return AttemptUberCharge(pCmd);
    }
    return CFFBaseAI::UseAbility(abilitySlot, pTargetEntity, targetPosition, pCmd);
}

void CMedicAI_FF::SwitchToWeapon(const std::string& weaponClassName, CUserCmd* pCmd) {
    if (!pCmd || !m_pBotPlayer) return;
    m_pBotPlayer->SelectWeapon(pCmd, weaponClassName);
}
