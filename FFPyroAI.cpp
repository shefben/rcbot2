#include "FFPyroAI.h"
#include "CFFPlayer.h"
#include "BotKnowledgeBase.h"
#include "ObjectivePlanner.h"
#include "GameDefines_Placeholder.h" // For CUserCmd, IN_ATTACK etc.
#include "FFStateStructs.h"          // For Vector, ClassConfigInfo
#include "BotDefines.h"              // For Pyro weapon names, ranges, cooldowns

#include <algorithm> // For std::min/max if needed

// Helper function (conceptual, could be in a utility class or CFFBaseAI)
// float GetWorldTime_conceptual() {
//     // In a real engine, this would come from gpGlobals->curtime or similar
//     return static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(
//                                   std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000.0f;
// }

CPyroAI_FF::CPyroAI_FF(CFFPlayer* pBotPlayer, CObjectivePlanner* pPlanner,
                       BotKnowledgeBase* pKnowledgeBase, const ClassConfigInfo* pClassConfig)
    : CFFBaseAI(pBotPlayer, pPlanner, pKnowledgeBase, pClassConfig),
      m_fNextFlamethrowerTime(0.f),
      m_fNextShotgunTime(0.f),
      m_fNextAxeTime(0.f),
      m_fNextAirblastTime(0.f) {
    // Initialize any other Pyro-specific members here
}

TrackedEntityInfo* CPyroAI_FF::SelectTarget() {
    CFFPlayer* player = GetBotPlayer();
    BotKnowledgeBase* kb = GetKnowledgeBase();
    if (!player || !kb) return nullptr;

    SetCurrentTarget(nullptr); // Reset current target from base class

    // 1. Reflectable projectiles (highest priority)
    TrackedEntityInfo* reflectTarget = FindReflectableProjectile(); // Uses updated KB
    if (reflectTarget) {
        SetCurrentTarget(reflectTarget);
        // TODO: Set a sub-task type or planner hint to prioritize airblasting this projectile.
        // For now, AttackTarget will handle if target is projectile.
        return GetCurrentTarget();
    }

    // 2. Extinguish burning teammates
    TrackedEntityInfo* burningAlly = FindNearbyBurningAlly(); // Uses updated IsTargetOnFire -> TrackedEntityInfo
    if (burningAlly) {
        SetCurrentTarget(burningAlly);
        // TODO: Set a sub-task type or planner hint to prioritize airblasting this ally.
        // For now, AttackTarget will handle if target is burning ally.
        return GetCurrentTarget();
    }

    // 3. Spy-checking teammates (conceptual, using ShouldSpyCheck)
    // const auto& allies = kb->GetTrackedAllies();
    // for (const auto& allyInfo : allies) { // allyInfo is TrackedEntityInfo
    //     if (ShouldSpyCheck(&allyInfo)) { // Pass pointer to TrackedEntityInfo
    //         SetCurrentTarget(const_cast<TrackedEntityInfo*>(&allyInfo)); // Need to manage constness if KB stores const TrackedEntityInfo directly
    //         // TODO: Set sub-task for spy-checking (puff of flame)
    //         return GetCurrentTarget();
    //     }
    // }

    // 4. Enemies (general targeting logic)
    SetCurrentTarget(CFFBaseAI::SelectTarget()); // Base class selects an enemy

    // if (GetCurrentTarget() && GetCurrentTarget()->IsHostileTo(player->GetTeam())) { // Conceptual GetTeam()
        // Potentially adjust priority based on if target is on fire (target->isOnFire), distance, etc.
        // Example: if (GetCurrentTarget()->isOnFire && IsTargetRetreating(GetCurrentTarget())) { /* increase priority */ }
    // }

    return GetCurrentTarget();
}

bool CPyroAI_FF::AttackTarget(TrackedEntityInfo* pTarget, CUserCmd* pCmd) {
    CFFPlayer* player = GetBotPlayer();
    BotKnowledgeBase* kb = GetKnowledgeBase(); // Unused for now, but good practice
    if (!player || !pTarget || !pCmd || !kb || !m_pClassConfig) return false;

    float worldTime = 0.0f; // Conceptual: kb->GetCurrentTime() or g_pGlobals->curtime;

    // Handle special targets first (projectiles, burning allies)
    // Conceptual check for projectile type, assuming pTarget could be a TrackedEntityInfo for a projectile
    // if (pTarget->entityType_conceptual == ENTITY_TYPE_PROJECTILE_FF && pTarget->isHostile) {
    //     return ActionAirblast(pCmd, pTarget->lastKnownPosition);
    // }
    // if (pTarget->team == player->GetTeam() && pTarget->isOnFire) { // Extinguish ally
    //     return ActionAirblast(pCmd, pTarget->lastKnownPosition);
    // }


    // Standard enemy engagement
    Vector botPos = player->GetOrigin(); // Using refined CFFPlayer getter
    Vector targetPos = pTarget->lastKnownPosition;
    float distSq = (botPos - targetPos).LengthSqr();

    // Primary Weapon: Flamethrower
    const ClassWeaponInfo* flamethrowerInfo = GetWeaponInfoByName(WEAPON_NAME_FLAMETHROWER_FF);
    // if (flamethrowerInfo && player->HasWeaponBySlot_Conceptual(flamethrowerInfo->slot) &&
    //     player->GetAmmo(AMMO_ID_FLAMETHROWER_FF_CONCEPTUAL) > 0 &&
    //     distSq < FLAMETHROWER_EFFECTIVE_RANGE_SQR_FF && worldTime > m_fNextFlamethrowerTime) {

    //     player->SelectWeaponByName_Conceptual(WEAPON_NAME_FLAMETHROWER_FF, pCmd);
    //     AimAtTarget(pTarget, pCmd, true, FLAMETHROWER_FLAME_SPEED_FF); // AimAtTarget is from CFFBaseAI
    //     player->AddButton(pCmd, IN_ATTACK);
    //     m_fNextFlamethrowerTime = worldTime + FLAMETHROWER_FIRE_INTERVAL_FF;
    //     return true;
    // }

    // Secondary Weapon: Shotgun
    const ClassWeaponInfo* shotgunInfo = GetWeaponInfoByName(WEAPON_NAME_SHOTGUN_PYRO_FF);
    // if (shotgunInfo && player->HasWeaponBySlot_Conceptual(shotgunInfo->slot) &&
    //     player->GetAmmo(AMMO_SHOTGUN_FF) > 0 &&
    //     distSq < SHOTGUN_EFFECTIVE_RANGE_SQR_FF && worldTime > m_fNextShotgunTime) {

    //     player->SelectWeaponByName_Conceptual(WEAPON_NAME_SHOTGUN_PYRO_FF, pCmd);
    //     AimAtTarget(pTarget, pCmd, true, 0.0f);
    //     player->AddButton(pCmd, IN_ATTACK);
    //     m_fNextShotgunTime = worldTime + SHOTGUN_FIRE_INTERVAL_FF;
    //     return true;
    // }

    // Melee Weapon: Axe
    const ClassWeaponInfo* axeInfo = GetWeaponInfoByName(WEAPON_NAME_AXE_PYRO_FF);
    // if (axeInfo && player->HasWeaponBySlot_Conceptual(axeInfo->slot) &&
    //     distSq < AXE_RANGE_SQR_FF && worldTime > m_fNextAxeTime) {

    //     player->SelectWeaponByName_Conceptual(WEAPON_NAME_AXE_PYRO_FF, pCmd);
    //     AimAtTarget(pTarget, pCmd);
    //     player->AddButton(pCmd, IN_ATTACK);
    //     m_fNextAxeTime = worldTime + AXE_FIRE_INTERVAL_FF;
    //     return true;
    // }

    return false; // No valid attack executed
}

bool CPyroAI_FF::UseAbility(int abilitySlot, TrackedEntityInfo* pTargetEntity, const Vector& targetOrDirection, CUserCmd* pCmd) {
    CFFPlayer* player = GetBotPlayer();
    // BotKnowledgeBase* kb = GetKnowledgeBase(); // Unused for now
    if (!player || !pCmd ) return false;

    // Conceptual: abilitySlot 0 for Airblast
    // if (abilitySlot == ABILITY_SLOT_AIRBLAST_FF_CONCEPTUAL) {
    //     Vector aimTargetPos = targetOrDirection;
    //     if (pTargetEntity) {
    //         aimTargetPos = pTargetEntity->lastKnownPosition;
    //     } else if (aimTargetPos.IsZero()) {
    //         // aimTargetPos = player->GetOrigin() + player->GetForwardVector_Conceptual() * AIRBLAST_RANGE_FF;
    //     }
    //     return ActionAirblast(pCmd, aimTargetPos);
    // }
    return false;
}

bool CPyroAI_FF::ActionAirblast(CUserCmd* pCmd, const Vector& aimTargetPos) {
    CFFPlayer* player = GetBotPlayer();
    // BotKnowledgeBase* kb = GetKnowledgeBase(); // Unused for now
    if (!player || !pCmd || !m_pClassConfig) return false;

    float worldTime = 0.0f; // Conceptual: kb->GetCurrentTime() or g_pGlobals->curtime;

    // if (player->CanAirblast_Conceptual() && worldTime > m_fNextAirblastTime) {
    //     bool needsWeaponSwitch = !player->IsWeaponActive_Conceptual(WEAPON_NAME_FLAMETHROWER_FF);
    //     if (needsWeaponSwitch) {
    //         player->SelectWeaponByName_Conceptual(WEAPON_NAME_FLAMETHROWER_FF, pCmd);
    //         // TODO: Set a state/timer to wait for weapon switch completion (e.g., WEAPON_SWITCH_DELAY_FF from BotDefines.h)
    //         // For now, assume it switches instantly or the next tick handles the actual airblast.
    //         // Could return a special status like "ACTION_PENDING_WEAPON_SWITCH"
    //         m_fNextAirblastTime = worldTime + WEAPON_SWITCH_DELAY_FF + 0.1f; // Prevent immediate retry
    //         return true; // Indicate action initiated (weapon switch)
    //     }

    //     AimAtPosition(aimTargetPos, pCmd); // AimAtPosition is from CFFBaseAI
    //     player->AddButton(pCmd, IN_ATTACK2);
    //     m_fNextAirblastTime = worldTime + AIRBLAST_COOLDOWN_FF; // Use define from BotDefines.h
        // Conceptual: player->ConsumeAmmo_Conceptual(AMMO_ID_FLAMETHROWER_FF_CONCEPTUAL, AIRBLAST_AMMO_COST_FF_CONCEPTUAL);
    //     return true; // Airblast fired
    // }
    return false;
}

// --- Conceptual Helper Implementations ---

bool CPyroAI_FF::IsTargetOnFire(const TrackedEntityInfo* pTarget) const {
    if (!pTarget) return false;
    return pTarget->isOnFire; // Using the new member from TrackedEntityInfo
}

TrackedEntityInfo* CPyroAI_FF::FindNearbyBurningAlly() const {
    CFFPlayer* player = GetBotPlayer();
    BotKnowledgeBase* kb = GetKnowledgeBase();
    if (!player || !kb) return nullptr;

    const auto& allies = kb->GetTrackedAllies();
    for (const auto& allyInfo : allies) { // allyInfo is TrackedEntityInfo
        // if (allyInfo.IsAlive_Conceptual() && allyInfo.isOnFire) { // Assuming IsAlive_Conceptual is a method or check health > 0
        //     Vector botPos = player->GetOrigin();
        //     if ((allyInfo.lastKnownPosition - botPos).LengthSqr() < AIRBLAST_RANGE_FF * AIRBLAST_RANGE_FF * 1.5f) { // Check within airblast range (1.5x for buffer)
        //         return const_cast<TrackedEntityInfo*>(&allyInfo); // KB stores by value in vector, so address is fine. If it was vector<unique_ptr>, would use .get()
        //     }
        // }
    }
    return nullptr;
}

TrackedEntityInfo* CPyroAI_FF::FindReflectableProjectile() const {
    CFFPlayer* player = GetBotPlayer();
    BotKnowledgeBase* kb = GetKnowledgeBase();
    if (!player || !kb) return nullptr;

    // const std::vector<ReflectableProjectileInfo>& projectiles = kb->GetTrackedProjectiles();
    // TrackedEntityInfo* bestProjectileToReflect = nullptr;
    // float closestThreatDistance = AIRBLAST_RANGE_FF * AIRBLAST_RANGE_FF; // Max reflection range

    // for (const auto& projInfo : projectiles) {
    //     if (!projInfo.isHostile || !projInfo.pEntity) continue;

        // Conceptual: Check if projectile type is reflectable (e.g., rocket, grenade but not bullet)
        // if (!IsTypeReflectable_Conceptual(projInfo.projectileType)) continue;

        // Vector botPos = player->GetOrigin();
        // float distSq = (projInfo.position - botPos).LengthSqr();

        // if (distSq < closestThreatDistance) {
            // Conceptual: Check if projectile is actually threatening (e.g., moving towards player or key area)
            // if (IsProjectileThreatening_Conceptual(projInfo, player)) {
                // if (player->IsInFOV_Conceptual(projInfo.position, AIRBLAST_FOV_DEG_FF)) {
                    // bestProjectileToReflect = kb->GetTrackedEntity(projInfo.pEntity); // Get full TrackedEntityInfo if needed
                    // closestThreatDistance = distSq;
                // }
            // }
    //     }
    // }
    // return bestProjectileToReflect;
    return nullptr; // Placeholder
}

bool CPyroAI_FF::ShouldSpyCheck(const TrackedEntityInfo* pTeammate) const {
    CFFPlayer* player = GetBotPlayer();
    BotKnowledgeBase* kb = GetKnowledgeBase();
    if (!pTeammate || !player || !kb) return false;

    // if (pTeammate->isOnFire) return false; // Already on fire, not a disguised spy (usually)

    // Conceptual checks (as before, but can use pTeammate-> directly):
    // if (pTeammate->IsBehavingSuspiciously_Conceptual()) return true;
    // if (kb->WasEnemySpyRecentlyNear_Conceptual(pTeammate->lastKnownPosition)) return true;
    // if (pTeammate->className == "Engineer" || pTeammate->className == "Medic") { /* ... */ }
    // if (kb->IsPlayerCallingForSpy_Conceptual(pTeammate->entityId)) return true;

    return false;
}
