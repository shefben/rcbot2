#include "FFEngineerAI.h"
#include "ObjectivePlanner.h"
#include "BotTasks.h"
#include "FFStateStructs.h"
#include "BotKnowledgeBase.h"
#include "GameDefines_Placeholder.h" // For CUserCmd, IN_ATTACK, etc.
#include "CFFPlayer.h"
#include "CBaseEntity.h"
#include "BotDefines.h"       // For WEAPON_NAME_*, IMPULSE_*, WRENCH_SWING_RATE_FF etc.

#include <iostream>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Conceptual time access, assuming CFFBaseAI provides it
// float CFFBaseAI::GetCurrentWorldTime_conceptual() const { /* ... */ }


CEngineerAI_FF::CEngineerAI_FF(CFFPlayer* pBotPlayer, CObjectivePlanner* pPlanner,
                             const BotKnowledgeBase* pKnowledgeBase, const ClassConfigInfo* pClassConfig)
    : CFFBaseAI(pBotPlayer, pPlanner, pKnowledgeBase, pClassConfig),
      m_eNextBuildingToPlace(BuildingType_FF::DISPENSER),
      m_pBuildingToHaul(nullptr),
      m_fNextWrenchHitTime(0.0f),
      m_fNextBuildCmdTime(0.0f)
{
    m_SentryInfo.type = BuildingType_FF::SENTRY_GUN;
    m_DispenserInfo.type = BuildingType_FF::DISPENSER;
    m_TeleporterEntranceInfo.type = BuildingType_FF::TELEPORTER_ENTRANCE;
    m_TeleporterExitInfo.type = BuildingType_FF::TELEPORTER_EXIT;
    // std::cout << "CEngineerAI_FF created for bot " << (m_pBotPlayer ? m_pBotPlayer->GetNamePlaceholder() : "Unknown") << std::endl;
}

CEngineerAI_FF::~CEngineerAI_FF() {}

void CEngineerAI_FF::UpdateBuildingStates(const BotKnowledgeBase* kb) { /* ... (Placeholder from Task 20) ... */ }
CBaseEntity* CEngineerAI_FF::SelectTarget() { /* ... (Placeholder from Task 20) ... */ return nullptr; }

bool CEngineerAI_FF::AttackTarget(CBaseEntity* pTarget, CUserCmd* pCmd) {
    if (!pTarget || !pTarget->IsAlive() || !m_pBotPlayer || !m_pBotPlayer->IsValid() || !m_pBotPlayer->IsAlive() || !pCmd) {
        m_pCurrentTarget = nullptr;
        return false;
    }
    m_pCurrentTarget = pTarget;

    // If target is a friendly building (conceptual check, e.g. pTarget->GetTeamNumber() == m_pBotPlayer->GetTeam())
    // and it's one of our own buildings that needs repair/upgrade.
    // if (pTarget->IsBuilding_Conceptual() && pTarget->GetTeamNumber() == m_pBotPlayer->GetTeam()) {
    //    if (pTarget->GetHealth() < pTarget->GetMaxHealth() || CanUpgradeBuilding_Conceptual(pTarget)) {
    //        return ActionWrenchHit(pTarget, pCmd); // Wrench it (repair/upgrade)
    //    }
    // }

    // Standard enemy combat
    Vector myPos = m_pBotPlayer->GetPosition();
    Vector targetPos = pTarget->GetPosition();
    float distSq = (myPos - targetPos).LengthSquared();
    std::string desiredWeaponName = WEAPON_NAME_WRENCH_FF; // Default to melee for self-defense if very close

    // Conceptual: Check ammo and weapon availability from CFFPlayer / ClassConfig
    bool hasShotgun = true; // m_pBotPlayer->HasWeaponAndAmmo_Conceptual(WEAPON_NAME_SHOTGUN_ENG_FF);
    bool hasPistol = false; // m_pBotPlayer->HasWeaponAndAmmo_Conceptual(WEAPON_NAME_PISTOL_ENG_FF);

    if (hasShotgun && distSq < SHOTGUN_EFFECTIVE_RANGE_FF * SHOTGUN_EFFECTIVE_RANGE_FF) { // Conceptual range constant
        desiredWeaponName = WEAPON_NAME_SHOTGUN_ENG_FF;
    } else if (hasPistol /* && distSq < PISTOL_EFFECTIVE_RANGE_FF */) {
        desiredWeaponName = WEAPON_NAME_PISTOL_ENG_FF;
    } else if (distSq > MELEE_RANGE_FF * MELEE_RANGE_FF && hasShotgun) { // Too far for melee, prefer shotgun if available
        desiredWeaponName = WEAPON_NAME_SHOTGUN_ENG_FF;
    } // else stick with Wrench if very close or no other options

    m_pBotPlayer->SelectWeaponByName_Conceptual(desiredWeaponName, pCmd);
    // if (!m_pBotPlayer->IsWeaponActive_Conceptual(desiredWeaponName)) {
    //     return true; // Weapon is switching
    // }

    AimAt(pTarget->GetWorldSpaceCenter(), pCmd);
    m_pBotPlayer->AddButton(pCmd, IN_ATTACK);
    return true;
}

bool CEngineerAI_FF::UseAbility(int abilitySlot, CBaseEntity* pTargetEntity, CUserCmd* pCmd) {
    // This method is now a better fit for dispatching Engineer actions based on SubTask parameters
    // if the SubTask itself has an abilitySlot that maps to BuildingType or action.
    // For example, if a SubTask(USE_ABILITY, targetPos, abilitySlot=BUILD_SENTRY_AS_INT) is given.

    // Conceptual mapping:
    // abilitySlot 0-3 could be Build Sentry, Disp, TP_In, TP_Out respectively
    // abilitySlot 4 could be Demolish All
    // abilitySlot 5 could be Repair pTargetEntity
    // abilitySlot 6 could be Upgrade pTargetEntity

    BuildingType_FF buildingType = BuildingType_FF::UNKNOWN;
    switch (abilitySlot) {
        case 0: buildingType = BuildingType_FF::SENTRY_GUN; break;
        case 1: buildingType = BuildingType_FF::DISPENSER; break;
        case 2: buildingType = BuildingType_FF::TELEPORTER_ENTRANCE; break;
        case 3: buildingType = BuildingType_FF::TELEPORTER_EXIT; break;
        case 4: return ActionDetonateAllBuildings(pCmd);
        case 5: return ActionRepair(pTargetEntity, pCmd);
        case 6: return ActionUpgrade(pTargetEntity, pCmd);
        default: return false;
    }

    if (buildingType != BuildingType_FF::UNKNOWN) {
        Vector buildPos = m_vCurrentMoveToTarget; // Use HLT's target position or subtask's
        if (pTargetEntity) buildPos = pTargetEntity->GetPosition(); // If target entity is a hint for position
        return ActionBuild(buildingType, buildPos, pCmd);
    }
    return false;
}

bool CEngineerAI_FF::UseAbilityAtPosition(int abilitySlot, const Vector& targetPos, CUserCmd* pCmd) {
    BuildingType_FF buildingType = static_cast<BuildingType_FF>(abilitySlot); // Assumes direct mapping for build
    return ActionBuild(buildingType, targetPos, pCmd);
}


bool CEngineerAI_FF::ActionBuild(BuildingType_FF type, const Vector& designatedBuildPos, CUserCmd* pCmd) {
    if (!m_pBotPlayer || !pCmd || GetCurrentWorldTime_conceptual() < m_fNextBuildCmdTime) return false;

    if (m_pBotPlayer->GetMetalCount_Conceptual() < GetBuildingCost(type)) {
        // std::cout << "Engineer: Not enough metal to build " << static_cast<int>(type) << std::endl;
        return false; // Not enough metal
    }

    // State 1: Ensure PDA is active and correct building type is selected via impulse
    // if (!m_pBotPlayer->IsWeaponActive_Conceptual(WEAPON_NAME_PDA_FF)) {
    //     m_pBotPlayer->SelectWeaponByName_Conceptual(WEAPON_NAME_PDA_FF, pCmd);
    //     m_fNextBuildCmdTime = GetCurrentWorldTime_conceptual() + WEAPON_SWITCH_DELAY_FF;
    //     return true; // Action ongoing (switching weapon)
    // }

    // Issue PDA command to select the building type (e.g., via impulse)
    m_pBotPlayer->IssuePDABuildCommand_Conceptual(type, SUBCOMMAND_PLACE_BLUEPRINT_FF, pCmd); // This sets pCmd->impulse

    // State 2: After PDA selection via impulse, next UserCmd should place it with IN_ATTACK
    // This requires the AI to manage a state machine for building.
    // For simplicity in this Action method, we assume the impulse selects, and if the AI aims and clicks
    // in a subsequent frame (or same if impulse is immediate for bots), it works.
    // This action should signal that it has initiated the PDA selection.
    // The actual placement (IN_ATTACK) and wrenching would be subsequent actions/states.

    // std::cout << "Engineer: Issued PDA command for " << static_cast<int>(type) << ". Next step: Aim & Place blueprint." << std::endl;
    // For now, let's assume this action is "done" once the command to select building is issued.
    // The next sub-task or AI state would be to aim at designatedBuildPos and press IN_ATTACK.
    // Or, if this task is about placing the blueprint directly:
    AimAt(designatedBuildPos, pCmd);
    m_pBotPlayer->AddButton(pCmd, IN_ATTACK); // Add IN_ATTACK to attempt placement
    m_fNextBuildCmdTime = GetCurrentWorldTime_conceptual() + 1.0f; // Cooldown before another build action

    // Conceptual: m_pBuildingInProgress_conceptual = FindNewlyPlacedBlueprintNear(designatedBuildPos);
    // m_eCurrentBuildingGoal_conceptual = type; m_vTargetBuildPos_conceptual = designatedBuildPos;
    return true; // Action "place blueprint" attempted
}

bool CEngineerAI_FF::ActionRepair(CBaseEntity* pBuildingEntity, CUserCmd* pCmd) {
    if (!pBuildingEntity || !m_pBotPlayer || !pCmd) return false;
    // Conceptual: const BuildingInfo* buildingState = m_pKnowledgeBase->GetBuildingInfo(pBuildingEntity->GetEdict_conceptual());
    // if (!buildingState || buildingState->health >= buildingState->maxHealth) return false; // Already full or not tracked
    // if (m_pBotPlayer->GetMetalCount_Conceptual() < 1 && buildingState->health < buildingState->maxHealth) { /* No metal for repair */ }

    return ActionWrenchHit(pBuildingEntity, pCmd);
}

bool CEngineerAI_FF::ActionUpgrade(CBaseEntity* pBuildingEntity, CUserCmd* pCmd) {
    if (!pBuildingEntity || !m_pBotPlayer || !pCmd) return false;
    // Conceptual: const BuildingInfo* buildingState = m_pKnowledgeBase->GetBuildingInfo(pBuildingEntity->GetEdict_conceptual());
    // if (!buildingState || buildingState->level >= 3 /* MAX_LEVEL_CONCEPTUAL */) return false;
    // if (m_pBotPlayer->GetMetalCount_Conceptual() < WRENCH_HIT_METAL_COST_FF /* for upgrade */) return false;

    return ActionWrenchHit(pBuildingEntity, pCmd);
}

bool CEngineerAI_FF::ActionWrenchHit(CBaseEntity* pTargetEntity, CUserCmd* pCmd) {
    if (!pTargetEntity || !m_pBotPlayer || !pCmd ) return false;

    float currentTime = GetCurrentWorldTime_conceptual();
    if (currentTime < m_fNextWrenchHitTime) {
        return true; // Waiting for wrench cooldown
    }

    m_pBotPlayer->SelectWeaponByName_Conceptual(WEAPON_NAME_WRENCH_FF, pCmd);
    // if (!m_pBotPlayer->IsWeaponActive_Conceptual(WEAPON_NAME_WRENCH_FF)) {
    //     m_fNextWrenchHitTime = currentTime + WEAPON_SWITCH_DELAY_FF; // Wait for switch
    //     return true;
    // }

    // Conceptual: if ((m_pBotPlayer->GetOrigin() - pTargetEntity->GetPosition()).LengthSqr() > WRENCH_RANGE_SQR_FF) {
    //     // Need to move closer. This task should be part of a sequence where bot is already close.
    //     // Or this action itself should initiate a MoveTo if not close.
    //     // For now, assume bot is close enough or MoveTo is handled by ExecuteSubTask.
    //     // std::cout << "Engineer: Target too far for wrench." << std::endl;
    //     return true; // Still trying, but out of range.
    // }

    AimAt(pTargetEntity->GetWorldSpaceCenter(), pCmd);
    m_pBotPlayer->AddButton(pCmd, IN_ATTACK);
    m_fNextWrenchHitTime = currentTime + WRENCH_SWING_RATE_FF;
    // std::cout << "Engineer: Swung wrench." << std::endl;
    return true; // Wrench swing action performed
}

bool CEngineerAI_FF::ActionDetonateAllBuildings(CUserCmd* pCmd) {
    if (!m_pBotPlayer || !pCmd || GetCurrentWorldTime_conceptual() < m_fNextBuildCmdTime) return false;

    m_pBotPlayer->SelectWeaponByName_Conceptual(WEAPON_NAME_PDA_DEMOLISH_FF, pCmd); // Or however FF does it
    // if (!m_pBotPlayer->IsWeaponActive_Conceptual(WEAPON_NAME_PDA_DEMOLISH_FF)) {
    //     m_fNextBuildCmdTime = GetCurrentWorldTime_conceptual() + WEAPON_SWITCH_DELAY_FF;
    //     return true; // Switching
    // }

    // Demolish PDA might use IN_ATTACK for "demolish all" or specific impulses for targeted demolish
    m_pBotPlayer->AddButton(pCmd, IN_ATTACK);
    m_fNextBuildCmdTime = GetCurrentWorldTime_conceptual() + 1.0f; // Cooldown
    // std::cout << "Engineer: ActionDetonateAllBuildings executed." << std::endl;
    // Reset internal tracking of buildings as they will be gone
    m_SentryInfo = EngineerBuildingInfo(); m_DispenserInfo = EngineerBuildingInfo(); /* etc. */
    m_iActiveStickies = 0; // This was from Demoman, remove if not applicable
    return true;
}


// --- Helper Implementations (Conceptual) ---
Vector CEngineerAI_FF::FindBestBuildLocation(BuildingType_FF type, const Vector& nearLocationHint, const BotKnowledgeBase* kb) const {
    // Needs navmesh analysis, LOS checks, proximity to objectives/chokepoints, friendly buildings etc.
    return nearLocationHint + Vector(100, 50, 0); // Dumb placeholder
}
bool CEngineerAI_FF::NeedsToBuild(BuildingType_FF type, const BotKnowledgeBase* kb) const {
    // UpdateBuildingStates(kb); // Ensure local info is fresh
    // switch(type) {
    //     case BuildingType_FF::SENTRY_GUN: return !m_SentryInfo.pEntity || m_SentryInfo.health <= 0;
    //     case BuildingType_FF::DISPENSER:  return !m_DispenserInfo.pEntity || m_DispenserInfo.health <= 0;
    //     // ... etc. for teleporters
    // }
    return true;
}
CBaseEntity* CEngineerAI_FF::FindClosestDamagedBuilding() const {
    // UpdateBuildingStates(m_pKnowledgeBase);
    // CBaseEntity* closestDamaged = nullptr;
    // float minDistanceSq = 999999.0f;
    // std::vector<const EngineerBuildingInfo*> buildings = {&m_SentryInfo, &m_DispenserInfo, &m_TeleporterEntranceInfo, &m_TeleporterExitInfo};
    // Vector myPos = m_pBotPlayer ? m_pBotPlayer->GetPosition() : Vector();
    // for (const auto* buildingInfo : buildings) {
    //     if (buildingInfo && buildingInfo->pEntity && buildingInfo->health > 0 && buildingInfo->health < buildingInfo->maxHealth) {
    //         float distSq = (buildingInfo->position - myPos).LengthSquared();
    //         if (distSq < minDistanceSq && distSq < ENGINEER_MAINTAIN_BUILDING_RANGE_FF * ENGINEER_MAINTAIN_BUILDING_RANGE_FF) {
    //             minDistanceSq = distSq;
    //             closestDamaged = buildingInfo->pEntity;
    //         }
    //     }
    // }
    return nullptr;
}
int CEngineerAI_FF::GetBuildingCost(BuildingType_FF type) const { /* ... (same as header) ... */ return 100;}
void CEngineerAI_FF::SwitchToWeapon(int weaponId, CUserCmd* pCmd) { /* ... (same as Soldier) ... */ }
CBaseEntity* CEngineerAI_FF::GetSentry() const { return m_SentryInfo.pEntity; }
CBaseEntity* CEngineerAI_FF::GetDispenser() const { return m_DispenserInfo.pEntity; }
CBaseEntity* CEngineerAI_FF::GetTeleporterEntrance() const { return m_TeleporterEntranceInfo.pEntity; }
CBaseEntity* CEngineerAI_FF::GetTeleporterExit() const { return m_TeleporterExitInfo.pEntity; }
int CEngineerAI_FF::GetMetalCount() const { return m_pBotPlayer ? m_pBotPlayer->GetMetalCount_Conceptual() : 0; }
