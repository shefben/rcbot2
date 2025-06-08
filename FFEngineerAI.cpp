#include "FFEngineerAI.h"
#include "ObjectivePlanner.h"
#include "BotTasks.h"
#include "FFStateStructs.h"
#include "BotKnowledgeBase.h"
// #include "GameDefines_Placeholder.h" // No longer needed, use SDK types
#include "CFFPlayer.h"        // Will be CFFPlayerWrapper.h
#include "BotDefines.h"       // For WEAPON_NAME_*, IMPULSE_*, WRENCH_SWING_RATE_FF etc.
#include "FFBot_SDKDefines.h" // For SDK Class/Team IDs, weapon classnames
#include "game/server/cbase.h" // For CBaseEntity
#include "game/shared/usercmd.h" // For CUserCmd
#include "game/shared/in_buttons.h" // For IN_ATTACK

#include <iostream>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Conceptual time access is now CFFBaseAI::GetWorldTime()


CEngineerAI_FF::CEngineerAI_FF(CFFPlayerWrapper* pBotPlayer, CObjectivePlanner* pPlanner,
                             BotKnowledgeBase* pKnowledgeBase, const ClassConfigInfo* pClassConfig) // Updated types
    : CFFBaseAI(pBotPlayer, pPlanner, pKnowledgeBase, pClassConfig),
      m_eNextBuildingToPlace(BuildingType_FF::DISPENSER),
      m_pBuildingToHaul_SDK(nullptr), // Changed type to CBaseEntity* for SDK compatibility
      m_fNextWrenchHitTime(0.0f),
      m_fNextBuildCmdTime(0.0f)
{
    m_SentryInfo.type = BuildingType_FF::SENTRY_GUN;
    m_DispenserInfo.type = BuildingType_FF::DISPENSER;
    m_TeleporterEntranceInfo.type = BuildingType_FF::TELEPORTER_ENTRANCE;
    m_TeleporterExitInfo.type = BuildingType_FF::TELEPORTER_EXIT;
    // std::cout << "CEngineerAI_FF created." << std::endl;
}

CEngineerAI_FF::~CEngineerAI_FF() {}

void CEngineerAI_FF::UpdateBuildingStates() { // Removed const BotKnowledgeBase* kb param, use member m_pKnowledgeBase
    // if (!m_pKnowledgeBase || !m_pBotPlayer || !m_pBotPlayer->GetEdict()) return;
    // This method needs to get buildings associated with this bot from BotKnowledgeBase
    // std::vector<const BuildingInfo*> myBuildings = m_pKnowledgeBase->GetOwnBuildings(m_pBotPlayer->GetEdict()); // Conceptual: GetEdict gives edict_t*
    // For each building in myBuildings, update m_SentryInfo, m_DispenserInfo, etc.
    // Example for Sentry:
    // auto it_sentry = std::find_if(myBuildings.begin(), myBuildings.end(),
    //     [](const BuildingInfo* b){ return b->type == BuildingType_FF::SENTRY_GUN; });
    // if (it_sentry != myBuildings.end()) {
    //     m_SentryInfo.pEntity_SDK = (*it_sentry)->pEdict ? CBaseEntity::Instance((*it_sentry)->pEdict) : nullptr;
    //     m_SentryInfo.health = (*it_sentry)->health;
    //     m_SentryInfo.maxHealth = (*it_sentry)->maxHealth;
    //     m_SentryInfo.level = (*it_sentry)->level;
    //     m_SentryInfo.isSapped = (*it_sentry)->isSapped;
    //     m_SentryInfo.isBuilding = (*it_sentry)->isBuildingInProgress;
    //     m_SentryInfo.position = (*it_sentry)->position;
    // } else {
    //     m_SentryInfo = EngineerBuildingInfo(); // Reset if not found
    //     m_SentryInfo.type = BuildingType_FF::SENTRY_GUN;
    // }
    // Repeat for Dispenser, TeleporterEntrance, TeleporterExit
}

CBaseEntity* CEngineerAI_FF::SelectTarget() {
    // UpdateBuildingStates(); // Ensure our building info is fresh
    // Prioritize self-defense, then building defense/repair, then opportunistic attacks.
    // CBaseEntity* enemy = CFFBaseAI::SelectTarget(); // Base class finds general enemy
    // if (enemy) { SetCurrentTarget(enemy); return GetCurrentTarget(); }

    // CBaseEntity* damagedBuilding = FindClosestDamagedBuilding();
    // if (damagedBuilding) { SetCurrentTarget(damagedBuilding); return GetCurrentTarget(); } // Target for repair

    SetCurrentTarget(nullptr);
    return nullptr;
}

bool CEngineerAI_FF::AttackTarget(CBaseEntity* pTarget, CUserCmd* pCmd) {
    // if (!pTarget || !pTarget->IsAlive() || !m_pBotPlayer || !m_pBotPlayer->IsValid() || !m_pBotPlayer->IsAlive() || !pCmd) {
    //     SetCurrentTarget(nullptr);
    //     return false;
    // }
    // SetCurrentTarget(pTarget);

    // if (pTarget->GetTeamNumber() == m_pBotPlayer->GetTeam() && /* pTarget->IsBuilding_SDK_Check() */ true) {
    //    // Target is a friendly building (hopefully one of ours)
    //    const BuildingInfo* buildingState = m_pKnowledgeBase->GetBuildingInfo(pTarget->edict());
    //    if (buildingState && (buildingState->health < buildingState->maxHealth || CanUpgradeBuilding(pTarget))) {
    //        return ActionWrenchHit(pTarget, pCmd); // Wrench it (repair/upgrade)
    //    }
    //    return true; // No action needed on this friendly building
    // }

    // Standard enemy combat
    // Vector myPos = m_pBotPlayer->GetOrigin();
    // Vector targetPos = pTarget->GetAbsOrigin();
    // float distSq = myPos.DistToSqr(targetPos);
    // std::string desiredWeaponName = WEAPON_CLASSNAME_WRENCH_FF; // Default

    // bool hasShotgun = m_pBotPlayer->GetAmmo(AMMO_SHELLS_NAME) > 0; // Use SDK ammo name
    // bool hasPistol = m_pBotPlayer->GetAmmo(AMMO_BULLETS_NAME) > 0; // Example SDK ammo name for pistol

    // if (hasShotgun && distSq < SHOTGUN_EFFECTIVE_RANGE_SQR_FF) {
    //     desiredWeaponName = WEAPON_CLASSNAME_SHOTGUN_ENG_FF;
    // } else if (hasPistol /* && distSq < PISTOL_EFFECTIVE_RANGE_SQR_FF */) {
    //     desiredWeaponName = WEAPON_CLASSNAME_PISTOL_ENG_FF; // Use actual pistol classname
    // } else if (distSq > MELEE_RANGE_SQR_FF && hasShotgun) {
    //     desiredWeaponName = WEAPON_CLASSNAME_SHOTGUN_ENG_FF;
    // }

    // m_pBotPlayer->SelectWeapon(pCmd, desiredWeaponName);
    // // if (m_pBotPlayer->IsSwitchingWeapon_Conceptual()) return true;

    // AimAt(pTarget->GetAbsOrigin(), pCmd); // CFFBaseAI::AimAt
    // m_pBotPlayer->AddButton(pCmd, IN_ATTACK);
    return true;
}

bool CEngineerAI_FF::UseAbility(int abilitySlot, CBaseEntity* pTargetEntity, const Vector& targetPosition, CUserCmd* pCmd) {
    BuildingType_FF buildingType = BuildingType_FF::UNKNOWN;
    // switch (abilitySlot) { // Assuming abilitySlot maps to an action or building type directly
    //     case BUILD_SENTRY_ABILITY_SLOT: buildingType = BuildingType_FF::SENTRY_GUN; break;
    //     case BUILD_DISPENSER_ABILITY_SLOT: buildingType = BuildingType_FF::DISPENSER; break;
    //     // ... other mappings ...
    //     case DETONATE_ALL_ABILITY_SLOT: return ActionDetonateAllBuildings(pCmd);
    //     case REPAIR_TARGET_ABILITY_SLOT: return ActionRepair(pTargetEntity, pCmd);
    //     case UPGRADE_TARGET_ABILITY_SLOT: return ActionUpgrade(pTargetEntity, pCmd);
    //     default: return false;
    // }

    // if (buildingType != BuildingType_FF::UNKNOWN) {
    //     Vector buildPos = targetPosition; // Use targetPosition from SubTask
    //     if (pTargetEntity) buildPos = pTargetEntity->GetAbsOrigin();
    //     return ActionBuild(buildingType, buildPos, pCmd);
    // }
    return false;
}


bool CEngineerAI_FF::ActionBuild(BuildingType_FF type, const Vector& designatedBuildPos, CUserCmd* pCmd) {
    // if (!m_pBotPlayer || !pCmd || GetWorldTime() < m_fNextBuildCmdTime) return false;

    // if (m_pBotPlayer->GetMetalCount() < GetBuildingCost(type)) {
    //     return false;
    // }

    // std::string pdaWeaponClass = WEAPON_CLASSNAME_PDA_ENG_BUILD_FF; // From BotDefines
    // m_pBotPlayer->SelectWeapon(pCmd, pdaWeaponClass);
    // // if (m_pBotPlayer->IsSwitchingWeapon_Conceptual()) {
    // //     m_fNextBuildCmdTime = GetWorldTime() + WEAPON_SWITCH_DELAY_FF;
    // //     return true;
    // // }

    // This part is tricky with SDK. CFFPlayer::BuildObject from SDK is what actually does it.
    // The PDA selection via impulse and then IN_ATTACK is more for human players.
    // Bots might call a direct server command or use a simplified interface if available.
    // For now, assume CFFPlayerWrapper's BuildBuilding_Command handles it.
    // m_pBotPlayer->BuildBuilding_Command(type); // This issues a server command like "buildsentry"

    // The following AimAt and IN_ATTACK would be for placing a blueprint if that's how FF works
    // *after* selecting the item on PDA via impulse, which is not done by BuildBuilding_Command.
    // This needs to be a two-step process for bots if mimicking human exactly, or one step if direct build command.
    // AimAt(designatedBuildPos, pCmd);
    // m_pBotPlayer->AddButton(pCmd, IN_ATTACK);
    // m_fNextBuildCmdTime = GetWorldTime() + 1.0f;
    return true;
}

bool CEngineerAI_FF::ActionRepair(CBaseEntity* pBuildingEntity, CUserCmd* pCmd) {
    // if (!pBuildingEntity || !m_pBotPlayer || !pCmd) return false;
    // const BuildingInfo* buildingState = m_pKnowledgeBase ? m_pKnowledgeBase->GetBuildingInfo(pBuildingEntity->edict()) : nullptr;
    // if (!buildingState || buildingState->health >= buildingState->maxHealth) return false;
    // if (m_pBotPlayer->GetMetalCount() < 1 && buildingState->health < buildingState->maxHealth) return false;

    return ActionWrenchHit(pBuildingEntity, pCmd);
}

bool CEngineerAI_FF::ActionUpgrade(CBaseEntity* pBuildingEntity, CUserCmd* pCmd) {
    // if (!pBuildingEntity || !m_pBotPlayer || !pCmd) return false;
    // const BuildingInfo* buildingState = m_pKnowledgeBase ? m_pKnowledgeBase->GetBuildingInfo(pBuildingEntity->edict()) : nullptr;
    // if (!buildingState || buildingState->level >= 3) return false;
    // if (m_pBotPlayer->GetMetalCount() < WRENCH_HIT_METAL_COST_FF ) return false;

    return ActionWrenchHit(pBuildingEntity, pCmd);
}

bool CEngineerAI_FF::ActionWrenchHit(CBaseEntity* pTargetEntity, CUserCmd* pCmd) {
    // if (!pTargetEntity || !m_pBotPlayer || !pCmd ) return false;

    // float currentTime = GetWorldTime();
    // if (currentTime < m_fNextWrenchHitTime) {
    //     return true;
    // }

    // m_pBotPlayer->SelectWeapon(pCmd, WEAPON_CLASSNAME_WRENCH_FF); // Use SDK weapon classname
    // // if (m_pBotPlayer->IsSwitchingWeapon_Conceptual()) {
    // //     m_fNextWrenchHitTime = currentTime + WEAPON_SWITCH_DELAY_FF;
    // //     return true;
    // // }

    // if (m_pBotPlayer->GetOrigin().DistToSqr(pTargetEntity->GetAbsOrigin()) > WRENCH_RANGE_SQR_FF) {
    //     return true; // Out of range, movement should handle.
    // }

    // AimAt(pTargetEntity->GetAbsOrigin(), pCmd); // AimAt center of building
    // m_pBotPlayer->AddButton(pCmd, IN_ATTACK);
    // m_fNextWrenchHitTime = currentTime + WRENCH_SWING_RATE_FF;
    return true;
}

bool CEngineerAI_FF::ActionDetonateAllBuildings(CUserCmd* pCmd) {
    // if (!m_pBotPlayer || !pCmd || GetWorldTime() < m_fNextBuildCmdTime) return false;

    // m_pBotPlayer->DetonateBuildings_Command(); // Uses CFFPlayerWrapper method
    // m_fNextBuildCmdTime = GetWorldTime() + 1.0f;

    // UpdateBuildingStates(); // Refresh local building knowledge
    return true;
}


// --- Helper Implementations (Conceptual) ---
Vector CEngineerAI_FF::FindBestBuildLocation(BuildingType_FF type, const Vector& nearLocationHint, BotKnowledgeBase* kb) const {
    return nearLocationHint + Vector(100, 50, 0);
}
bool CEngineerAI_FF::NeedsToBuild(BuildingType_FF type) {
    // UpdateBuildingStates();
    // switch(type) {
    //     case BuildingType_FF::SENTRY_GUN: return !m_SentryInfo.pEntity_SDK || m_SentryInfo.health <= 0;
    //     case BuildingType_FF::DISPENSER:  return !m_DispenserInfo.pEntity_SDK || m_DispenserInfo.health <= 0;
    //     // ...
    // }
    return true;
}
CBaseEntity* CEngineerAI_FF::FindClosestDamagedBuilding() {
    // UpdateBuildingStates();
    // CBaseEntity* closestDamaged = nullptr;
    // float minDistanceSq = ENGINEER_MAINTAIN_BUILDING_RANGE_FF * ENGINEER_MAINTAIN_BUILDING_RANGE_FF;
    // std::vector<EngineerBuildingInfo*> buildings = {&m_SentryInfo, &m_DispenserInfo, &m_TeleporterEntranceInfo, &m_TeleporterExitInfo};
    // Vector myPos = m_pBotPlayer ? m_pBotPlayer->GetOrigin() : Vector();

    // for (auto* buildingInfo : buildings) {
    //     if (buildingInfo && buildingInfo->pEntity_SDK && buildingInfo->health > 0 && buildingInfo->health < buildingInfo->maxHealth) {
    //         float distSq = buildingInfo->position.DistToSqr(myPos);
    //         if (distSq < minDistanceSq) {
    //             minDistanceSq = distSq;
    //             closestDamaged = buildingInfo->pEntity_SDK;
    //         }
    //     }
    // }
    return nullptr;
}
int CEngineerAI_FF::GetBuildingCost(BuildingType_FF type) const { return 100;} // Get from ClassConfigInfo or defines

void CEngineerAI_FF::SwitchToWeapon(const std::string& weaponClassName, CUserCmd* pCmd) {
    if (!pCmd || !m_pBotPlayer) return;
    m_pBotPlayer->SelectWeapon(pCmd, weaponClassName);
}

// Getters now use the local EngineerBuildingInfo which should be updated by UpdateBuildingStates
CBaseEntity* CEngineerAI_FF::GetSentry() const { return m_SentryInfo.pEntity_SDK; }
CBaseEntity* CEngineerAI_FF::GetDispenser() const { return m_DispenserInfo.pEntity_SDK; }
CBaseEntity* CEngineerAI_FF::GetTeleporterEntrance() const { return m_TeleporterEntranceInfo.pEntity_SDK; }
CBaseEntity* CEngineerAI_FF::GetTeleporterExit() const { return m_TeleporterExitInfo.pEntity_SDK; }
int CEngineerAI_FF::GetMetalCount() const { return m_pBotPlayer ? m_pBotPlayer->GetMetalCount() : 0; }

bool CEngineerAI_FF::CanUpgradeBuilding(CBaseEntity* pBuilding) const {
    // if(!pBuilding) return false;
    // const BuildingInfo* info = m_pKnowledgeBase ? m_pKnowledgeBase->GetBuildingInfo(pBuilding->edict()) : nullptr;
    // return info && info->level < 3; // Max level 3
    return false;
}
