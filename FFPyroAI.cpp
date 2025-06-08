#include "FFPyroAI.h"
#include "CFFPlayer.h" // Will be CFFPlayerWrapper.h
#include "BotKnowledgeBase.h"
#include "ObjectivePlanner.h"
// #include "GameDefines_Placeholder.h" // No longer needed
#include "FFStateStructs.h"          // For Vector, ClassConfigInfo
#include "BotDefines.h"              // For Pyro weapon names, ranges, cooldowns, AMMO_ defines
#include "FFBot_SDKDefines.h"        // For weapon classname defines
#include "game/shared/usercmd.h"     // For CUserCmd
#include "game/shared/in_buttons.h"  // For IN_ATTACK, IN_ATTACK2
#include "TrackedEntityInfo.h"       // For TrackedEntityInfo

#include <algorithm> // For std::min/max if needed

// GetWorldTime() is now in CFFBaseAI

CPyroAI_FF::CPyroAI_FF(CFFPlayerWrapper* pBotPlayer, CObjectivePlanner* pPlanner, // Updated type
                       BotKnowledgeBase* pKnowledgeBase, const ClassConfigInfo* pClassConfig)
    : CFFBaseAI(pBotPlayer, pPlanner, pKnowledgeBase, pClassConfig),
      m_fNextFlamethrowerTime(0.f),
      m_fNextShotgunTime(0.f),
      m_fNextAxeTime(0.f),
      m_fNextAirblastTime(0.f) {
    // Initialize
}

TrackedEntityInfo* CPyroAI_FF::SelectTarget() {
    // CFFPlayerWrapper* player = GetBotPlayer(); // Correct type from CFFBaseAI
    // BotKnowledgeBase* kb = GetKnowledgeBase();
    // if (!player || !kb) return nullptr;

    // SetCurrentTarget(nullptr);

    // TrackedEntityInfo* reflectTarget = FindReflectableProjectile();
    // if (reflectTarget) {
    //     SetCurrentTarget(kb->GetTrackedEntity(reflectTarget->pEdict)); // Assuming FindReflectableProjectile returns temporary or needs full TrackedEntityInfo
    //     return GetCurrentTarget() ? GetCurrentTarget()->pEdict ? CBaseEntity::Instance(GetCurrentTarget()->pEdict) : nullptr : nullptr; // Return CBaseEntity*
    // }

    // TrackedEntityInfo* burningAlly = FindNearbyBurningAlly();
    // if (burningAlly) {
    //     SetCurrentTarget(burningAlly);
    //     return burningAlly->pEdict ? CBaseEntity::Instance(burningAlly->pEdict) : nullptr;
    // }

    // CBaseEntity* enemyTarget = CFFBaseAI::SelectTarget(); // Base class selects an enemy CBaseEntity*
    // SetCurrentTarget(enemyTarget); // Store it in base
    // return enemyTarget;
    return CFFBaseAI::SelectTarget(); // Placeholder
}

bool CPyroAI_FF::AttackTarget(TrackedEntityInfo* pTargetInfo, CUserCmd* pCmd) { // Changed param to TrackedEntityInfo
    // CFFPlayerWrapper* player = GetBotPlayer();
    // BotKnowledgeBase* kb = GetKnowledgeBase();
    // if (!player || !pTargetInfo || !pTargetInfo->pEdict || !pCmd || !kb || !m_pClassConfig) return false;

    // CBaseEntity* pTarget = CBaseEntity::Instance(pTargetInfo->pEdict); // Get CBaseEntity for SDK calls
    // if (!pTarget || !pTarget->IsAlive()) return false;

    // float worldTime = GetWorldTime();

    // // Special target handling (projectiles, burning allies from TrackedEntityInfo)
    // // if (pTargetInfo->entityType_conceptual == PROJECTILE_TYPE && pTargetInfo->isHostile) { // Conceptual check
    // //     return ActionAirblast(pCmd, pTargetInfo->lastKnownPosition);
    // // }
    // // if (pTargetInfo->team == player->GetTeam() && pTargetInfo->isOnFire) {
    // //     return ActionAirblast(pCmd, pTargetInfo->lastKnownPosition);
    // // }

    // Vector botPos = player->GetOrigin();
    // Vector targetPos = pTarget->GetAbsOrigin(); // Use CBaseEntity's position
    // float distSq = botPos.DistToSqr(targetPos);

    // std::string desiredWeaponClassname = WEAPON_CLASSNAME_AXE_PYRO_FF; // Default to axe if all else fails

    // if (player->GetAmmo(AMMO_FLAMETHROWER_FF) > 0 && // Uses AMMO_ defines from BotDefines.h -> mapped by GetAmmoIndexForName_FF
    //     distSq < FLAMETHROWER_EFFECTIVE_RANGE_SQR_FF && worldTime > m_fNextFlamethrowerTime) {
    //     desiredWeaponClassname = WEAPON_CLASSNAME_FLAMETHROWER_FF;
    // } else if (player->GetAmmo(AMMO_SHOTGUN_FF) > 0 &&
    //     distSq < SHOTGUN_EFFECTIVE_RANGE_SQR_FF && worldTime > m_fNextShotgunTime) {
    //     desiredWeaponClassname = WEAPON_CLASSNAME_SHOTGUN_PYRO_FF;
    // } else if (distSq < AXE_RANGE_SQR_FF && worldTime > m_fNextAxeTime) {
    //     desiredWeaponClassname = WEAPON_CLASSNAME_AXE_PYRO_FF;
    // } else if (player->GetAmmo(AMMO_SHOTGUN_FF) > 0) { // Fallback to shotgun
    //      desiredWeaponClassname = WEAPON_CLASSNAME_SHOTGUN_PYRO_FF;
    // } else if (player->GetAmmo(AMMO_FLAMETHROWER_FF) > 0) { // Fallback to FT
    //      desiredWeaponClassname = WEAPON_CLASSNAME_FLAMETHROWER_FF;
    // }

    // player->SelectWeapon(pCmd, desiredWeaponClassname);
    // // if (player->IsSwitchingWeapon_Conceptual()) return true; // Add conceptual delay

    // Vector aimPos = targetPos;
    // if (desiredWeaponClassname == WEAPON_CLASSNAME_FLAMETHROWER_FF) {
    //     float flameSpeed = m_pClassConfig ? m_pClassConfig->GetWeaponParamFloat(desiredWeaponClassname, "projectile_speed") : FLAMETHROWER_FLAME_SPEED_FF;
    //     aimPos = PredictTargetPosition(pTarget, flameSpeed); // PredictTargetPosition takes CBaseEntity*
    //     player->AddButton(pCmd, IN_ATTACK);
    //     m_fNextFlamethrowerTime = worldTime + FLAMETHROWER_FIRE_INTERVAL_FF;
    // } else if (desiredWeaponClassname == WEAPON_CLASSNAME_SHOTGUN_PYRO_FF) {
    //     aimPos = PredictTargetPosition(pTarget, 9999.0f); // Hitscan
    //     player->AddButton(pCmd, IN_ATTACK);
    //     m_fNextShotgunTime = worldTime + SHOTGUN_FIRE_INTERVAL_FF;
    // } else if (desiredWeaponClassname == WEAPON_CLASSNAME_AXE_PYRO_FF) {
    //     if (distSq < AXE_RANGE_SQR_FF * 1.2f) { // Generous range for axe swing
    //         aimPos = pTarget->GetAbsOrigin(); // Melee aim direct
    //         player->AddButton(pCmd, IN_ATTACK);
    //         m_fNextAxeTime = worldTime + AXE_FIRE_INTERVAL_FF;
    //     } else { return true; /* Too far for axe, don't attack, maybe move closer */ }
    // }
    // AimAt(aimPos, pCmd); // CFFBaseAI method
    return true;
}

bool CPyroAI_FF::UseAbility(int abilitySlot, TrackedEntityInfo* pTargetEntityInfo, const Vector& targetOrDirection, CUserCmd* pCmd) {
    // CFFPlayerWrapper* player = GetBotPlayer();
    // if (!player || !pCmd ) return false;

    // // Assuming abilitySlot 0 is Airblast for Pyro
    // if (abilitySlot == 0 /* ABILITY_SLOT_AIRBLAST_PYRO_CONCEPTUAL */) {
    //     Vector aimTargetPos = targetOrDirection;
    //     if (pTargetEntityInfo) {
    //         aimTargetPos = pTargetEntityInfo->lastKnownPosition;
    //     } else if (aimTargetPos.IsZero()) {
    //         // Vector forward; AngleVectors(player->GetEyeAngles(), &forward); // SDK Call
    //         // aimTargetPos = player->GetOrigin() + forward * AIRBLAST_RANGE_FF;
    //     }
    //     return ActionAirblast(pCmd, aimTargetPos);
    // }
    return false;
}

bool CPyroAI_FF::ActionAirblast(CUserCmd* pCmd, const Vector& aimTargetPos) {
    // CFFPlayerWrapper* player = GetBotPlayer();
    // if (!player || !pCmd || !m_pClassConfig) return false;

    // float worldTime = GetWorldTime();

    // if (player->CanAirblast() && worldTime > m_fNextAirblastTime) { // CanAirblast uses CFFPlayerWrapper
    //     // bool needsWeaponSwitch = !player->IsWeaponActive_Conceptual(WEAPON_CLASSNAME_FLAMETHROWER_FF);
    //     // if (needsWeaponSwitch) {
    //     //     player->SelectWeapon(pCmd, WEAPON_CLASSNAME_FLAMETHROWER_FF);
    //     //     m_fNextAirblastTime = worldTime + WEAPON_SWITCH_DELAY_FF + 0.1f;
    //     //     return true;
    //     // }

    //     AimAtPosition(aimTargetPos, pCmd);
    //     player->SecondaryAttack(pCmd); // Use CFFPlayerWrapper helper for IN_ATTACK2
    //     m_fNextAirblastTime = worldTime + AIRBLAST_COOLDOWN_FF;
    //     // Conceptual: player->ConsumeAmmo(AMMO_ID_FLAMETHROWER_FF_CONCEPTUAL, AIRBLAST_AMMO_COST_FF_CONCEPTUAL);
    //     return true;
    // }
    return false;
}

// --- Helper Implementations ---

bool CPyroAI_FF::IsTargetOnFire(const TrackedEntityInfo* pTarget) const {
    if (!pTarget) return false;
    return pTarget->isOnFire;
}

TrackedEntityInfo* CPyroAI_FF::FindNearbyBurningAlly() const {
    // CFFPlayerWrapper* player = GetBotPlayer();
    // BotKnowledgeBase* kb = GetKnowledgeBase();
    // if (!player || !kb) return nullptr;

    // const auto& allies = kb->GetTrackedAllies();
    // for (const auto& allyInfo : allies) {
    //     if (allyInfo.health > 0 && allyInfo.isOnFire) {
    //         Vector botPos = player->GetOrigin();
    //         if (botPos.DistToSqr(allyInfo.lastKnownPosition) < AIRBLAST_RANGE_FF * AIRBLAST_RANGE_FF * 1.5f*1.5f) {
    //             return const_cast<TrackedEntityInfo*>(&allyInfo);
    //         }
    //     }
    // }
    return nullptr;
}

TrackedEntityInfo* CPyroAI_FF::FindReflectableProjectile() const {
    // CFFPlayerWrapper* player = GetBotPlayer();
    // BotKnowledgeBase* kb = GetKnowledgeBase();
    // if (!player || !kb) return nullptr;

    // const std::vector<ReflectableProjectileInfo>& projectiles = kb->GetTrackedProjectiles();
    // TrackedEntityInfo* bestProjectileTEI = nullptr; // Need to return TrackedEntityInfo for consistency
    // float closestThreatDistanceSq = AIRBLAST_RANGE_FF * AIRBLAST_RANGE_FF;

    // for (const auto& projInfo : projectiles) {
    //     if (!projInfo.isHostile || !projInfo.pEntity) continue;
    //     // Conceptual: if (!IsProjectileTypeReflectable_SDK(projInfo.projectileType)) continue;

    //     Vector botPos = player->GetOrigin();
    //     float distSq = projInfo.position.DistToSqr(botPos);

    //     if (distSq < closestThreatDistanceSq) {
    //         // Conceptual: if (IsProjectileThreatening_SDK(projInfo, player)) {
    //             // if (player->IsInFOV(projInfo.position, AIRBLAST_FOV_DEG_FF)) { // Assuming CFFPlayerWrapper::IsInFOV
    //                 // bestProjectileTEI = kb->GetTrackedEntity(projInfo.pEntity); // This gets the full TrackedEntityInfo
    //                 // closestThreatDistanceSq = distSq;
    //             // }
    //         // }
    //     }
    // }
    // return bestProjectileTEI;
    return nullptr;
}

bool CPyroAI_FF::ShouldSpyCheck(const TrackedEntityInfo* pTeammate) const {
    // CFFPlayerWrapper* player = GetBotPlayer();
    // BotKnowledgeBase* kb = GetKnowledgeBase();
    // if (!pTeammate || !player || !kb) return false;

    // if (pTeammate->isOnFire) return false;

    // // Conceptual checks
    // // if (pTeammate->isBehavingSuspiciously_Conceptual) return true;
    // // if (kb->WasEnemySpyRecentlyNear_Conceptual(pTeammate->lastKnownPosition)) return true;
    // // FF_BotPlayerClassID teammateClass = GetBotClassIDFromString_FF(pTeammate->className);
    // // if (teammateClass == FF_BOT_CLASS_ENGINEER || teammateClass == FF_BOT_CLASS_MEDIC) { /* ... */ }
    // // if (kb->IsPlayerCallingForSpy_Conceptual(pTeammate->entityId)) return true;

    return false;
}
