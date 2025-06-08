#include "FFSpyAI.h"
#include "ObjectivePlanner.h"
#include "BotTasks.h"
#include "FFStateStructs.h"
#include "FFSpyAI.h"
#include "ObjectivePlanner.h"
#include "BotTasks.h"
#include "FFStateStructs.h"   // For Vector, QAngle if not using SDK's directly
#include "BotKnowledgeBase.h"
#include "CFFPlayer.h"        // Renamed to CFFPlayerWrapper.h
#include "FFBot_SDKDefines.h" // For WEAPON_NAME_*, SPY_* constants, SDK TEAM/CLASS
#include "game/server/cbase.h" // For CBaseEntity
#include "game/shared/usercmd.h" // For CUserCmd
#include "game/shared/in_buttons.h" // For IN_ATTACK etc
#include "game/shared/shareddefs.h" // For CLASS_SPY, FF_TEAM_RED etc.

#include <iostream>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Conceptual GetCurrentWorldTime is now CFFBaseAI::GetWorldTime()

CSpyAI_FF::CSpyAI_FF(CFFPlayerWrapper* pBotPlayer, CObjectivePlanner* pPlanner,
                   BotKnowledgeBase* pKnowledgeBase, const ClassConfigInfo* pClassConfig) // Updated types
    : CFFBaseAI(pBotPlayer, pPlanner, pKnowledgeBase, pClassConfig),
      m_bIsCurrentlyCloaked_internal(false),
      m_eCurrentDisguiseTeam_internal(DisguiseTeamType_FF::ENEMY_TEAM),
      m_eCurrentDisguiseClass_internal(DisguiseClassType_FF::NONE),
      m_fCloakEnergy_internal(1.0f),
      m_flNextCloakToggleTime(0.0f),
      m_flNextDisguiseTime(0.0f),
      m_flNextSapTime(0.0f),
      m_flNextStabTime(0.0f),
      m_flTimeSinceLastSeenByEnemy(999.0f)
{
    // Constructor
}

CSpyAI_FF::~CSpyAI_FF() {}

void CSpyAI_FF::SyncSpyState() {
    // if (!m_pBotPlayer) return;
    // m_bIsCurrentlyCloaked_internal = m_pBotPlayer->IsCloaked(); // SDK Call
    // m_fCloakEnergy_internal = m_pBotPlayer->GetCloakEnergy();   // SDK Call
    // m_eCurrentDisguiseTeam_internal = static_cast<DisguiseTeamType_FF>(m_pBotPlayer->GetDisguiseTeam()); // SDK Call + mapping if needed
    // FF_BotPlayerClassID sdkDisguiseClass = m_pBotPlayer->GetDisguiseClass(); // SDK Call (returns our enum)
    // m_eCurrentDisguiseClass_internal = static_cast<DisguiseClassType_FF>(sdkDisguiseClass); // Map if our internal enum differs structurally

    // Conceptual:
    // float frameInterval = GetWorldTime() - m_flLastSpyStateSyncTime; // Need m_flLastSpyStateSyncTime
    // m_flTimeSinceLastSeenByEnemy += frameInterval;
    // if (m_pKnowledgeBase->IsEntityVisibleToEnemy(m_pBotPlayer->GetEdict())) m_flTimeSinceLastSeenByEnemy = 0.0f;
    // m_flLastSpyStateSyncTime = GetWorldTime();
}


CBaseEntity* CSpyAI_FF::SelectTarget() {
    // SyncSpyState();
    // SetCurrentTarget(nullptr);
    // if (!m_pKnowledgeBase || !m_pBotPlayer) return nullptr;

    // CBaseEntity* pBestBackstabTarget = FindBestBackstabTarget();
    // if (pBestBackstabTarget) {
    //     SetCurrentTarget(pBestBackstabTarget);
    //     return GetCurrentTarget();
    // }

    // CBaseEntity* pBestSappingTarget = FindBestSappingTarget();
    // if (pBestSappingTarget) {
    //     SetCurrentTarget(pBestSappingTarget);
    //     return GetCurrentTarget();
    // }
    return CFFBaseAI::SelectTarget(); // Fallback
}

bool CSpyAI_FF::AttackTarget(CBaseEntity* pTarget, CUserCmd* pCmd) {
    // if (!pTarget || !pTarget->IsAliveSDK() || !m_pBotPlayer || !m_pBotPlayer->IsValid() || !m_pBotPlayer->IsAlive() || !pCmd) { // Use IsAliveSDK if pTarget is CBaseEntity
    //     SetCurrentTarget(nullptr);
    //     return false;
    // }
    // SetCurrentTarget(pTarget);

    // if (pTarget->IsNPC() && pTarget->GetTeamNumber() != m_pBotPlayer->GetTeam() /* Conceptual IsBuilding */ && IsInRangeForSapping(pTarget)) {
    //    return ActionSapBuilding(pTarget, pCmd);
    // }
    // else if (pTarget->IsPlayer() && IsBehindTarget(pTarget) && IsInRangeForBackstab(pTarget)) {
    //    return ActionAttemptBackstab(pTarget, pCmd);
    // }
    // else {
    //     m_pBotPlayer->SelectWeapon(pCmd, WEAPON_CLASSNAME_REVOLVER_SPY_FF); // Use actual classname
    //     // if (m_pBotPlayer->IsSwitchingWeapon_Conceptual()) return true;
    //     AimAt(pTarget->GetAbsOrigin(), pCmd);
    //     m_pBotPlayer->AddButton(pCmd, IN_ATTACK);
    //     return true;
    // }
    return false;
}

bool CSpyAI_FF::UseAbility(int abilitySlot, CBaseEntity* pTargetEntity, const Vector& targetPosition, CUserCmd* pCmd) {
    // if (!pCmd || !m_pBotPlayer) return false;
    // SyncSpyState();

    // switch (abilitySlot) { // Assuming abilitySlot maps to specific Spy actions
    //     case SPY_ABILITY_CLOAK_TOGGLE:
    //         if (IsCurrentlyCloaked()) return ActionDecloak(pCmd);
    //         else return ActionCloak(pCmd);
    //     case SPY_ABILITY_DISGUISE: {
    //         int teamToDisguiseAs = FF_TEAM_RED; // Default, use SDK define
    //         int classToDisguiseAs = CLASS_SOLDIER; // Default, use SDK define
    //         // Determine actual disguise target team/class based on situation or pTargetEntity
    //         // if (pTargetEntity && pTargetEntity->IsPlayer() && pTargetEntity->GetTeamNumber() != m_pBotPlayer->GetTeam()) {
    //         //    teamToDisguiseAs = pTargetEntity->GetTeamNumber();
    //         //    classToDisguiseAs = static_cast<::CFFPlayer*>(pTargetEntity)->m_PlayerClass.GetClassNum();
    //         // } else { // Pick a common enemy class if no specific target
    //         //    teamToDisguiseAs = (m_pBotPlayer->GetTeam() == FF_TEAM_RED) ? FF_TEAM_BLUE : FF_TEAM_RED;
    //         //    classToDisguiseAs = CLASS_SOLDIER; // A common choice
    //         // }
    //         return ActionDisguise(teamToDisguiseAs, classToDisguiseAs, pCmd);
    //     }
    //     case SPY_ABILITY_SAP:
    //         if (pTargetEntity /* && pTargetEntity->IsBuilding_SDK() && pTargetEntity->GetTeamNumber() != m_pBotPlayer->GetTeam() */) {
    //             return ActionSapBuilding(pTargetEntity, pCmd);
    //         }
    //         return false;
    //     default:
    //         return CFFBaseAI::UseAbility(abilitySlot, pTargetEntity, targetPosition, pCmd);
    // }
    return false;
}

bool CSpyAI_FF::ActionAttemptBackstab(CBaseEntity* pTarget, CUserCmd* pCmd) {
    // if (!pTarget || !pTarget->IsAlive() || !m_pBotPlayer || !pCmd) return false;
    // float currentTime = GetWorldTime();
    // if (currentTime < m_flNextStabTime) return true;

    // m_pBotPlayer->SelectWeapon(pCmd, WEAPON_CLASSNAME_KNIFE_SPY_FF); // Use actual classname
    // // if (m_pBotPlayer->IsSwitchingWeapon_Conceptual()) {
    // //     m_flNextStabTime = currentTime + WEAPON_SWITCH_DELAY_FF;
    // //     return true;
    // // }
    // AimAt(pTarget->GetAbsOrigin(), pCmd); // Or specific hitbox
    // m_pBotPlayer->AddButton(pCmd, IN_ATTACK);
    // m_flNextStabTime = currentTime + KNIFE_SWING_RATE_FF; // From BotDefines.h
    return true;
}

bool CSpyAI_FF::ActionCloak(CUserCmd* pCmd) {
    // if (!m_pBotPlayer || !pCmd) return false;
    // float currentTime = GetWorldTime();

    // if (IsCurrentlyCloaked() || currentTime < m_flNextCloakToggleTime /* || !m_pBotPlayer->CanCloak() */) {
    //     return false;
    // }

    // m_pBotPlayer->SelectWeapon(pCmd, WEAPON_CLASSNAME_INVIS_WATCH_FF); // Use actual classname
    // // if (m_pBotPlayer->IsSwitchingWeapon_Conceptual()) {
    // //     m_flNextCloakToggleTime = currentTime + WEAPON_SWITCH_DELAY_FF;
    // //     return true;
    // // }

    // m_pBotPlayer->CloakToggle_Command(); // Uses CFFPlayerWrapper method
    // m_flNextCloakToggleTime = currentTime + SPY_CLOAK_ABILITY_COOLDOWN_FF;
    return true;
}

bool CSpyAI_FF::ActionDecloak(CUserCmd* pCmd) {
    // if (!m_pBotPlayer || !pCmd) return false;
    // float currentTime = GetWorldTime();

    // if (!IsCurrentlyCloaked() || currentTime < m_flNextCloakToggleTime) {
    //     return false;
    // }

    // m_pBotPlayer->SelectWeapon(pCmd, WEAPON_CLASSNAME_INVIS_WATCH_FF);
    // // if (m_pBotPlayer->IsSwitchingWeapon_Conceptual()) {
    // //      m_flNextCloakToggleTime = currentTime + WEAPON_SWITCH_DELAY_FF;
    // //      return true;
    // // }
    // m_pBotPlayer->CloakToggle_Command();
    // m_flNextCloakToggleTime = currentTime + SPY_DECLOAK_TIME_FF;
    return true;
}

bool CSpyAI_FF::ActionDisguise(int ff_sdk_team_id, int ff_sdk_class_id, CUserCmd* pCmd) { // Changed params
    // if (!m_pBotPlayer || !pCmd) return false; // pCmd might not be used if it's a direct ClientCommand
    // float currentTime = GetWorldTime();
    // if (currentTime < m_flNextDisguiseTime /* || !m_pBotPlayer->CanDisguise() */) {
    //     return false;
    // }

    // m_pBotPlayer->Disguise_Command(ff_sdk_team_id, ff_sdk_class_id); // Uses CFFPlayerWrapper method
    // m_flNextDisguiseTime = currentTime + SPY_DISGUISE_APPLY_TIME_FF;
    return true;
}

bool CSpyAI_FF::ActionSapBuilding(CBaseEntity* pBuildingEntity, CUserCmd* pCmd) {
    // if (!pBuildingEntity /* || !pBuildingEntity->IsSappable_SDK() */ || !m_pBotPlayer || !pCmd) return false;
    // float currentTime = GetWorldTime();
    // if (currentTime < m_flNextSapTime) return true;

    // const BuildingInfo* buildingState = m_pKnowledgeBase ? m_pKnowledgeBase->GetBuildingInfo(pBuildingEntity->edict()) : nullptr;
    // if (buildingState && (buildingState->isSapped || buildingState->teamId == m_pBotPlayer->GetTeam())) return false;

    // if (m_pBotPlayer->GetOrigin().DistToSqr(pBuildingEntity->GetAbsOrigin()) > SAPPER_RANGE_SQR_FF) {
    //     return false;
    // }

    // m_pBotPlayer->SelectWeapon(pCmd, WEAPON_CLASSNAME_SAPPER_FF); // Use actual classname
    // // if (m_pBotPlayer->IsSwitchingWeapon_Conceptual()) {
    // //     m_flNextSapTime = currentTime + WEAPON_SWITCH_DELAY_FF;
    // //     return true;
    // // }

    // AimAt(pBuildingEntity->GetAbsOrigin(), pCmd); // AimAt building's origin
    // m_pBotPlayer->AddButton(pCmd, IN_ATTACK); // Sapper uses primary attack
    // m_flNextSapTime = currentTime + SPY_SAPPER_DEPLOY_TIME_FF;
    return true;
}

// --- Helper Methods (Conceptual Implementations) ---
CBaseEntity* CSpyAI_FF::FindBestBackstabTarget() const { /* ... iterate TrackedEnemies from KB, check IsPlayer(), IsBehindTarget(), IsInRangeForBackstab() ... */ return nullptr; }
CBaseEntity* CSpyAI_FF::FindBestSappingTarget() const { /* ... iterate enemy buildings from KB, check type, range, etc. ... */ return nullptr; }
bool CSpyAI_FF::ShouldCloakNow() const { /* ... use m_pBotPlayer->GetCloakEnergy(), m_pBotPlayer->IsCloaked() ... */ return GetCloakEnergy() > SPY_CLOAK_MIN_ENERGY_FF && !IsCurrentlyCloaked(); }
bool CSpyAI_FF::ShouldDecloakNow(CBaseEntity* pPotentialTarget) const { /* ... */ return IsCurrentlyCloaked(); }
DisguiseClassType_FF CSpyAI_FF::SelectBestDisguiseClass(BotKnowledgeBase* kb) const { return DisguiseClassType_FF::SOLDIER; } // Use non-const KB
bool CSpyAI_FF::IsBehindTarget(CBaseEntity* pTarget) const { /* ... use m_pBotPlayer->GetOrigin/EyeAngles, pTarget->GetAbsOrigin/GetAbsAngles ... */ return false; }
bool CSpyAI_FF::IsInRangeForBackstab(CBaseEntity* pTarget) const { /* ... use m_pBotPlayer->GetOrigin, pTarget->GetAbsOrigin, KNIFE_BACKSTAB_RANGE_SQR_FF ... */ return false; }
bool CSpyAI_FF::IsInRangeForSapping(CBaseEntity* pBuilding) const { /* ... use m_pBotPlayer->GetOrigin, pBuilding->GetAbsOrigin, SAPPER_RANGE_SQR_FF ... */ return false; }

// --- State Checks (Rely on CFFPlayerWrapper) ---
bool CSpyAI_FF::IsCurrentlyCloaked() const { return m_pBotPlayer ? m_pBotPlayer->IsCloaked() : m_bIsCurrentlyCloaked_internal; }
bool CSpyAI_FF::IsCurrentlyDisguised() const { return m_pBotPlayer ? m_pBotPlayer->IsDisguised() : m_bIsCurrentlyDisguised_internal; } // Assuming CFFPlayerWrapper has GetDisguiseTeam() and GetDisguiseClass()
DisguiseTeamType_FF CSpyAI_FF::GetCurrentDisguiseTeam() const { return m_pBotPlayer ? static_cast<DisguiseTeamType_FF>(m_pBotPlayer->GetDisguiseTeam()) : m_eCurrentDisguiseTeam_internal; }
DisguiseClassType_FF CSpyAI_FF::GetCurrentDisguiseClass() const {
    // if (m_pBotPlayer) {
    //     FF_BotPlayerClassID sdkClass = m_pBotPlayer->GetDisguiseClass(); // Returns our enum
    //     return static_cast<DisguiseClassType_FF>(sdkClass); // Map if DisguiseClassType_FF is different
    // }
    return m_eCurrentDisguiseClass_internal;
}
float CSpyAI_FF::GetCloakEnergy() const { return m_pBotPlayer ? m_pBotPlayer->GetCloakEnergy() : m_fCloakEnergy_internal; }
