#include "FFSpyAI.h"
#include "ObjectivePlanner.h"
#include "BotTasks.h"
#include "FFStateStructs.h"
#include "BotKnowledgeBase.h"
#include "GameDefines_Placeholder.h" // For CUserCmd, IN_ATTACK, etc. (should be from BotDefines.h or SDK)
#include "CFFPlayer.h"
#include "CBaseEntity.h"
#include "BotDefines.h"       // For WEAPON_NAME_*, SPY_* constants, COND_FF_*

#include <iostream>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Conceptual GetCurrentWorldTime (should be via CFFBaseAI or CFFPlayer from g_pGlobals)
// float GetSpyAIConceptualWorldTime() { static float t = 0; t += 0.1f; return t; }


CSpyAI_FF::CSpyAI_FF(CFFPlayer* pBotPlayer, CObjectivePlanner* pPlanner,
                   const BotKnowledgeBase* pKnowledgeBase, const ClassConfigInfo* pClassConfig)
    : CFFBaseAI(pBotPlayer, pPlanner, pKnowledgeBase, pClassConfig),
      m_bIsCurrentlyCloaked_internal(false), // Should be initialized from player state in init/update
      m_eCurrentDisguiseTeam_internal(DisguiseTeamType_FF::ENEMY_TEAM),
      m_eCurrentDisguiseClass_internal(DisguiseClassType_FF::NONE), // Default to no disguise
      m_fCloakEnergy_internal(1.0f), // Full energy
      m_flNextCloakToggleTime(0.0f),
      m_flNextDisguiseTime(0.0f),
      m_flNextSapTime(0.0f),
      m_flNextStabTime(0.0f),
      m_flTimeSinceLastSeenByEnemy(999.0f)
{
    // std::cout << "CSpyAI_FF created for: " << (m_pBotPlayer ? m_pBotPlayer->GetNamePlaceholder() : "Unknown Bot") << std::endl;
}

CSpyAI_FF::~CSpyAI_FF() {}

// Renamed from UpdateInternalState to avoid conflict if base class gets one.
void CSpyAI_FF::SyncSpyState() {
    if (!m_pBotPlayer) return;
    m_bIsCurrentlyCloaked_internal = m_pBotPlayer->IsCloaked_Conceptual();
    m_fCloakEnergy_internal = m_pBotPlayer->GetCloakEnergy_Conceptual();
    m_eCurrentDisguiseTeam_internal = static_cast<DisguiseTeamType_FF>(m_pBotPlayer->GetDisguiseTeam_Conceptual()); // Needs mapping
    m_eCurrentDisguiseClass_internal = static_cast<DisguiseClassType_FF>(m_pBotPlayer->GetDisguiseClass_Conceptual()); // Needs mapping

    // m_flTimeSinceLastSeenByEnemy += GetFrameInterval_Conceptual();
    // if (m_pBotPlayer->WasRecentlySeenByEnemies_Conceptual()) m_flTimeSinceLastSeenByEnemy = 0.0f;
}


CBaseEntity* CSpyAI_FF::SelectTarget() {
    // SyncSpyState(); // Update current cloak/disguise status first
    m_pCurrentTarget = nullptr;
    if (!m_pKnowledgeBase || !m_pBotPlayer) return nullptr;

    CBaseEntity* pBestBackstabTarget = FindBestBackstabTarget();
    if (pBestBackstabTarget) {
        m_pCurrentTarget = pBestBackstabTarget;
        return m_pCurrentTarget;
    }

    CBaseEntity* pBestSappingTarget = FindBestSappingTarget();
    if (pBestSappingTarget) {
        m_pCurrentTarget = pBestSappingTarget;
        return m_pCurrentTarget;
    }

    // Fallback: if already in combat (e.g. discovered), might use Revolver.
    // This needs more sophisticated logic based on current threat / situation.
    // For now, primarily focus on backstabs and saps via specific actions.
    return nullptr;
}

bool CSpyAI_FF::AttackTarget(CBaseEntity* pTarget, CUserCmd* pCmd) {
    if (!pTarget || !pTarget->IsAlive() || !m_pBotPlayer || !pCmd) {
        m_pCurrentTarget = nullptr; // Clear if target invalid
        return false;
    }
    m_pCurrentTarget = pTarget; // Set current combat target

    // Conceptual: Check if target is a building for sapping
    // if (pTarget->IsBuilding_Conceptual() && IsInRangeForSapping(pTarget)) {
    //    return ActionSapBuilding(pTarget, pCmd);
    // }
    // Conceptual: Check for backstab opportunity
    // else if (pTarget->IsPlayer_Conceptual() && IsBehindTarget(pTarget) && IsInRangeForBackstab(pTarget)) {
    //    return ActionAttemptBackstab(pTarget, pCmd);
    // }
    // Default to Revolver if not backstabbing or sapping (or if those actions failed to start)
    // else {
        m_pBotPlayer->SelectWeaponByName_Conceptual(WEAPON_NAME_REVOLVER_FF, pCmd);
        // if (m_pBotPlayer->IsWeaponActive_Conceptual(WEAPON_NAME_REVOLVER_FF)) {
            AimAt(pTarget->GetWorldSpaceCenter(), pCmd); // Use base class AimAt
            m_pBotPlayer->AddButton(pCmd, IN_ATTACK);
            return true; // Firing revolver
        // }
        // return true; // Waiting for weapon switch
    // }
    return false; // Should have taken an action if target is valid
}

bool CSpyAI_FF::UseAbility(int abilitySlot, CBaseEntity* pTargetEntity, CUserCmd* pCmd) {
    if (!pCmd || !m_pBotPlayer) return false;
    // SyncSpyState(); // Update cloak/disguise status before deciding

    // Conceptual mapping from a generic abilitySlot to Spy actions
    // Slot 0: Cloak/Decloak (toggle)
    // Slot 1: Disguise (e.g. as enemy soldier)
    // Slot 2: Sap (pTargetEntity must be a building)
    switch (abilitySlot) {
        case 0: // Cloak/Decloak toggle
            if (m_pBotPlayer->IsCloaked_Conceptual()) return ActionDecloak(pCmd);
            else return ActionCloak(pCmd);
        case 1: { // Disguise
            // Conceptual: Determine desired team/class for disguise based on situation or pTargetEntity
            DisguiseTeamType_FF teamToDisguiseAs = DisguiseTeamType_FF::ENEMY_TEAM; // Default
            DisguiseClassType_FF classToDisguiseAs = SelectBestDisguiseClass(m_pKnowledgeBase);
            // if (pTargetEntity && pTargetEntity->IsPlayer_Conceptual() && pTargetEntity->GetTeamNumber() != m_pBotPlayer->GetTeam()) {
            //     teamToDisguiseAs = static_cast<DisguiseTeamType_FF>(pTargetEntity->GetTeamNumber()); // Needs mapping
            //     classToDisguiseAs = static_cast<DisguiseClassType_FF>(pTargetEntity->GetClassId_Conceptual()); // Needs mapping
            // }
            return ActionDisguise(teamToDisguiseAs, classToDisguiseAs, pCmd);
        }
        case 2: // Sap building
            if (pTargetEntity /* && pTargetEntity->IsBuilding_Conceptual() && pTargetEntity->GetTeamNumber() != m_pBotPlayer->GetTeam() */) {
                return ActionSapBuilding(pTargetEntity, pCmd);
            }
            return false; // No valid building target for sap
        default:
            return CFFBaseAI::UseAbility(abilitySlot, pTargetEntity, pCmd); // Call base if not handled
    }
}

bool CSpyAI_FF::ActionAttemptBackstab(CBaseEntity* pTarget, CUserCmd* pCmd) {
    if (!pTarget || !pTarget->IsAlive() || !m_pBotPlayer || !pCmd) return false;
    float currentTime = GetCurrentWorldTime_conceptual();
    if (currentTime < m_flNextStabTime) return true; // Waiting for cooldown

    m_pBotPlayer->SelectWeaponByName_Conceptual(WEAPON_NAME_KNIFE_FF, pCmd);
    // if (!m_pBotPlayer->IsWeaponActive_Conceptual(WEAPON_NAME_KNIFE_FF)) {
    //     m_flNextStabTime = currentTime + WEAPON_SWITCH_DELAY_FF; // Wait for switch
    //     return true;
    // }

    // AimAt(pTarget->GetHitboxPosition_Conceptual("HITBOX_SPINE"), pCmd); // More precise
    AimAt(pTarget->GetWorldSpaceCenter(), pCmd); // Simpler aim
    m_pBotPlayer->AddButton(pCmd, IN_ATTACK);
    m_flNextStabTime = currentTime + KNIFE_SWING_RATE_FF;
    return true;
}

bool CSpyAI_FF::ActionCloak(CUserCmd* pCmd) {
    if (!m_pBotPlayer || !pCmd) return false;
    float currentTime = GetCurrentWorldTime_conceptual();

    // if (m_pBotPlayer->IsCloaked_Conceptual() || currentTime < m_flNextCloakToggleTime || !m_pBotPlayer->CanCloak_Conceptual()) {
    //     return false; // Already cloaked, on cooldown, or cannot cloak (e.g. no energy)
    // }

    m_pBotPlayer->SelectWeaponByName_Conceptual(WEAPON_NAME_INVIS_WATCH_FF, pCmd);
    // if (!m_pBotPlayer->IsWeaponActive_Conceptual(WEAPON_NAME_INVIS_WATCH_FF)) {
    //     m_flNextCloakToggleTime = currentTime + WEAPON_SWITCH_DELAY_FF;
    //     return true; // Switching weapon
    // }

    m_pBotPlayer->ActionCloakToggle_Conceptual(pCmd); // This uses IN_ATTACK2
    m_flNextCloakToggleTime = currentTime + SPY_CLOAK_ABILITY_COOLDOWN_FF;
    // m_bIsCurrentlyCloaked_internal = true; // CFFPlayer's IsCloaked_Conceptual should reflect change
    return true;
}

bool CSpyAI_FF::ActionDecloak(CUserCmd* pCmd) {
    if (!m_pBotPlayer || !pCmd) return false;
    float currentTime = GetCurrentWorldTime_conceptual();

    // if (!m_pBotPlayer->IsCloaked_Conceptual() || currentTime < m_flNextCloakToggleTime) {
    //     return false; // Not cloaked or on cooldown
    // }

    // Ensure watch is active if decloak is tied to it (some watches auto-decloak on fire)
    m_pBotPlayer->SelectWeaponByName_Conceptual(WEAPON_NAME_INVIS_WATCH_FF, pCmd);
    // if (!m_pBotPlayer->IsWeaponActive_Conceptual(WEAPON_NAME_INVIS_WATCH_FF)) {
    //      m_flNextCloakToggleTime = currentTime + WEAPON_SWITCH_DELAY_FF;
    //      return true;
    // }

    m_pBotPlayer->ActionCloakToggle_Conceptual(pCmd); // Same button toggles
    m_flNextCloakToggleTime = currentTime + SPY_DECLOAK_TIME_FF; // Different cooldown for decloak
    // m_bIsCurrentlyCloaked_internal = false;
    return true;
}

bool CSpyAI_FF::ActionDisguise(DisguiseTeamType_FF team, DisguiseClassType_FF disguiseClass, CUserCmd* pCmd) {
    if (!m_pBotPlayer || !pCmd) return false;
    float currentTime = GetCurrentWorldTime_conceptual();
    // if (currentTime < m_flNextDisguiseTime || !m_pBotPlayer->CanDisguise_Conceptual()) {
    //     return false; // On cooldown or cannot disguise
    // }

    // Disguise Kit might be a passive action or require selecting it.
    // m_pBotPlayer->SelectWeaponByName_Conceptual(WEAPON_NAME_DISGUISE_KIT_FF, pCmd);
    // if (!m_pBotPlayer->IsWeaponActive_Conceptual(WEAPON_NAME_DISGUISE_KIT_FF)) {
    //     m_flNextDisguiseTime = currentTime + WEAPON_SWITCH_DELAY_FF;
    //     return true; // Switching
    // }

    // Convert conceptual team/class to game's actual IDs
    int gameTeamId = (team == DisguiseTeamType_FF::ENEMY_TEAM) ? TEAM_ID_BLUE : TEAM_ID_RED; // Simplified
    int gameClassId = static_cast<int>(disguiseClass); // Needs proper mapping

    m_pBotPlayer->IssueDisguiseCommand_Conceptual(gameTeamId, gameClassId);
    m_flNextDisguiseTime = currentTime + SPY_DISGUISE_APPLY_TIME_FF;
    // m_eCurrentDisguiseTeam_internal = team; // Update internal state after command issued
    // m_eCurrentDisguiseClass_internal = disguiseClass;
    return true;
}

bool CSpyAI_FF::ActionSapBuilding(CBaseEntity* pBuildingEntity, CUserCmd* pCmd) {
    if (!pBuildingEntity /* || !pBuildingEntity->IsSappable_Conceptual() */ || !m_pBotPlayer || !pCmd) return false;
    float currentTime = GetCurrentWorldTime_conceptual();
    // if (currentTime < m_flNextSapTime) return true; // Cooldown

    // const BuildingInfo* buildingState = m_pKnowledgeBase ? m_pKnowledgeBase->GetBuildingInfo(pBuildingEntity->GetEdict_conceptual()) : nullptr;
    // if (buildingState && (buildingState->isSapped || buildingState->teamId == m_pBotPlayer->GetTeam())) return false; // Already sapped or friendly

    // if ((m_pBotPlayer->GetOrigin() - pBuildingEntity->GetPosition()).LengthSqr() > SAPPER_RANGE_SQR_FF) {
    //     // This task should be preceded by a MoveTo subtask. If AI is here, it assumes it's in range.
    //     // If not, it means movement failed or this action was called prematurely.
    //     // std::cout << "Spy: Target building out of sapper range." << std::endl;
    //     return false; // Cannot sap, too far.
    // }

    m_pBotPlayer->SelectWeaponByName_Conceptual(WEAPON_NAME_SAPPER_FF, pCmd);
    // if (!m_pBotPlayer->IsWeaponActive_Conceptual(WEAPON_NAME_SAPPER_FF)) {
    //     m_flNextSapTime = currentTime + WEAPON_SWITCH_DELAY_FF;
    //     return true; // Switching
    // }

    AimAt(pBuildingEntity->GetWorldSpaceCenter(), pCmd);
    m_pBotPlayer->ActionDeploySapper_Conceptual(pCmd); // This calls AddButton(IN_ATTACK)
    m_flNextSapTime = currentTime + SPY_SAPPER_DEPLOY_TIME_FF;
    return true;
}

// --- Helper Methods (Conceptual Implementations) ---
CBaseEntity* CSpyAI_FF::FindBestBackstabTarget() const { /* ... iterate TrackedEnemies, check class, facing, range, IsCloaked ... */ return nullptr; }
CBaseEntity* CSpyAI_FF::FindBestSappingTarget() const { /* ... iterate TrackedBuildings (enemy), check type (sentry first), range, defenses ... */ return nullptr; }
bool CSpyAI_FF::ShouldCloakNow() const { /* ... based on enemy proximity, current action, cloak energy, !IsCloaked ... */ return GetCloakEnergy() > SPY_CLOAK_MIN_ENERGY_FF && !IsCurrentlyCloaked(); }
bool CSpyAI_FF::ShouldDecloakNow(CBaseEntity* pPotentialTarget) const { /* ... based on target opportunity (backstab/sap), safety, cloak energy ... */ return IsCurrentlyCloaked(); }
DisguiseClassType_FF CSpyAI_FF::SelectBestDisguiseClass(const BotKnowledgeBase* kb) const { return DisguiseClassType_FF::SOLDIER; }
bool CSpyAI_FF::IsBehindTarget(CBaseEntity* pTarget) const { /* ... dot product / angle checks relative to target's forward ... */ return false; }
bool CSpyAI_FF::IsInRangeForBackstab(CBaseEntity* pTarget) const { /* ... distance check using KNIFE_BACKSTAB_RANGE_SQR_FF ... */ return false; }
bool CSpyAI_FF::IsInRangeForSapping(CBaseEntity* pBuilding) const { /* ... distance check using SAPPER_RANGE_SQR_FF ... */ return false; }

// --- State Checks (Rely on CFFPlayer) ---
bool CSpyAI_FF::IsCurrentlyCloaked() const { return m_pBotPlayer ? m_pBotPlayer->IsCloaked_Conceptual() : m_bIsCurrentlyCloaked_internal; }
bool CSpyAI_FF::IsCurrentlyDisguised() const { return m_pBotPlayer ? m_pBotPlayer->IsDisguised_Conceptual() : m_bIsCurrentlyDisguised_internal; }
DisguiseTeamType_FF CSpyAI_FF::GetCurrentDisguiseTeam() const { return m_pBotPlayer ? static_cast<DisguiseTeamType_FF>(m_pBotPlayer->GetDisguiseTeam_Conceptual()) : m_eCurrentDisguiseTeam_internal; }
DisguiseClassType_FF CSpyAI_FF::GetCurrentDisguiseClass() const { return m_pBotPlayer ? static_cast<DisguiseClassType_FF>(m_pBotPlayer->GetDisguiseClass_Conceptual()) : m_eCurrentDisguiseClass_internal; }
float CSpyAI_FF::GetCloakEnergy() const { return m_pBotPlayer ? m_pBotPlayer->GetCloakEnergy_Conceptual() : m_fCloakEnergy_internal; }

// SwitchToWeapon is not needed here if CFFPlayer::SelectWeaponByName_Conceptual is used.
// void CSpyAI_FF::SwitchToWeapon(int weaponId, CUserCmd* pCmd) { /* ... */ }
