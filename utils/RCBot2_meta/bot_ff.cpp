#include "bot_ff.h"
#include "bot_globals.h"
#include "bot_navigation.h"
#include "bot_manager.h"
#include "bot_event.h"
#include "bot_util.h"
#include "bot_math.h"
#include "bot_waypoint.h"

#include "game_ff.h" // For FF specific game constants / enums (assuming this exists)
#include "game_shared/ff/ff_playerclass_parse.h" // For CFFPlayerClassInfo
#include "bot_const.h" // For FF_CLASS_ enums (assuming they are moved here or accessible)
#include "bot_weapon_defs.h" // For g_weaponDefs

// If game_ff.h doesn't exist, you might need to include the base game.h or specific headers for entity properties

// Link to engine functions
#if defined(COMPILE_METAMOD)
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#else
// For standalone builds, ensure these are available or stubbed
#include <stdio.h> // For printf, etc.
// May need stubs for engine functions if not linking against HLSDK
#endif

// Define s_IsHuntedModeForTesting if not already defined globally for testing
bool CBotFF::s_IsHuntedModeForTesting = false;

// Constructor
CBotFF::CBotFF() : CBotFortress() {
    m_fGrenadePrimeStartTime = 0.0f;
    m_bIsPrimingGrenade = false;
    m_pGrenadeTargetEnt = NULL;
    m_vGrenadeTargetPos = Vector(0,0,0);
    m_fPrimeDuration = 0.0f;
    m_pVIP = NULL;
    m_bIsVIP = false;
    m_hBuiltMancannon = NULL;
    m_fNextMancannonBuildTime = 0.0f;
    m_bHasActivePipes = false;
    m_vLastPipeTrapLocation = Vector(0,0,0);
    m_iPipesToLay = 0;
    m_fNextPipeLayTime = 0.0f;
    m_bWantsToAirblast = false; // For Pyro
    m_fNextAirblastTime = 0.0f; // For Pyro
    m_pNearestArmorItem.Set(NULL);
    m_fNextArmorCheckTime = 0.0f;
    // Initialize other FF specific variables
}

// Destructor
CBotFF::~CBotFF() {
}

// modThink: Called periodically for FF specific thinking
void CBotFF::modThink() {
    CBotFortress::modThink(); // Call base class think

    // FF Specific Thinking Logic
    FF_ClassID currentFFClass_modThink = CClassInterface::getFFClass(m_pEdict);

    // HWGuy Assault Cannon speed adjustment
    // Ensure m_fIdealMoveSpeed is initialized to default for the class before this
    CBotWeapon* currentWeapon_modThink = getCurrentWeapon();

    if (currentFFClass_modThink == FF_CLASS_HWGUY && currentWeapon_modThink &&
        currentWeapon_modThink->getWeaponInfo() &&
        strcmp(currentWeapon_modThink->getWeaponInfo()->getWeaponName(), "weapon_ff_assaultcannon") == 0 &&
        (m_pButtons->holdingButton(IN_ATTACK) || (m_pEnemy.Get() && isVisible(m_pEnemy.Get()) && wantToShoot())) )
    {
        m_fIdealMoveSpeed *= 0.4f; // Reduce speed by 60% when firing/winding up Assault Cannon
    }
    // Note: If m_fIdealMoveSpeed is not reset elsewhere each frame, add an else to restore it.
    // However, it's common for it to be set from playerinfo maxspeed each frame.

    // Example: Check if bot is VIP in Hunted mode
    if (g_pGameRules && CVAR_GET_FLOAT("mp_hunted_mode") > 0 || s_IsHuntedModeForTesting) { // s_IsHuntedModeForTesting for local tests
        // This is a simplified check. Actual VIP status might be stored in m_pPlayer or elsewhere.
        // For now, let's assume the first player on team 2 (if it exists) is VIP for testing.
        if (m_iTeam == 2 && gpGlobals->time > 5.0f) { // Example: Team 2 is VIP team
             if (!m_pVIP) { // Try to find VIP if not already known
                for (int i = 1; i <= gpGlobals->maxClients; i++) {
                    CBasePlayer *pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( i );
                    if (pPlayer && pPlayer->IsPlayer() && pPlayer->pev->team == 2 && pPlayer->IsAlive()) { // Check if player is on VIP team and alive
                        // In a real scenario, you'd check a specific flag like pPlayer->m_bIsVIP
                        // For testing, let's assume the player with the lowest entity index on team 2 is VIP
                        if (!m_pVIP || pPlayer->entindex() < m_pVIP->entindex()) {
                             m_pVIP = ENT(pPlayer->pev);
                        }
                    }
                }
            }
            if (m_pVIP == edict()) { // If this bot is the VIP
                m_bIsVIP = true;
            } else {
                m_bIsVIP = false;
            }
        } else {
            m_bIsVIP = false;
            m_pVIP = NULL;
        }
    } else {
        m_bIsVIP = false;
        m_pVIP = NULL;
    }

    // Detonate demo pipes if conditions met
    if (currentFFClass_modThink == FF_CLASS_DEMOMAN && m_bHasActivePipes) {
        CBotWeapon *pPipeLauncher = getWeapon(WEAPON_FF_PIPEBOMBLAUNCHER);
        if (pPipeLauncher && pPipeLauncher->canUse()) {
            // Simple logic: if enemies near pipes and bot is not in immediate danger
            bool bEnemyNearPipes = false;
            for (int i = 1; i <= gpGlobals->maxClients; i++) {
                CBasePlayer *pPlayer = (CBasePlayer*)UTIL_PlayerByIndex(i);
                if (pPlayer && pPlayer->IsPlayer() && pPlayer->IsAlive() && isEnemy(ENT(pPlayer->pev))) {
                    if ((pPlayer->pev->origin - m_vLastPipeTrapLocation).Length() < 300) { // 300 units radius
                        bEnemyNearPipes = true;
                        break;
                    }
                }
            }
            if (bEnemyNearPipes && !m_pCurrentEnemy) { // Detonate if enemy near and bot not fighting
                 setSchedule(new CSchedFFDemoDetonatePipes(this));
            }
        }
    }

    // Airblast logic for Pyro
    if (currentFFClass_modThink == FF_CLASS_PYRO && gpGlobals->time >= m_fNextAirblastTime) {
        CBotWeapon* pFlamer = getWeapon(WEAPON_FF_FLAMETHROWER);
        if (pFlamer && pFlamer->canUseSecondary()) { // Assuming secondary fire is airblast
            // Check for projectiles to deflect
            // This is a simplified check. Real projectile detection is complex.
            // You'd iterate through entities, check classnames, velocity, etc.
            edict_t* pProjectile = NULL;
            // TODO: Implement actual projectile detection. For now, this is a placeholder.
            // Example: CBaseEntity *pEnt = NULL; while ((pEnt = UTIL_FindEntityInSphere(pEnt, pev->origin, 200)) != NULL) { ... }

            if (pProjectile && FVisible(pProjectile, edict())) {
                // Check if projectile is moving towards bot or teammate
                // Vector vProjVel = pProjectile->v.velocity.Normalize();
                // Vector vToBot = (pev->origin - pProjectile->v.origin).Normalize();
                // if (DotProduct(vProjVel, vToBot) > 0.7) { // Projectile coming towards bot
                    m_bWantsToAirblast = true;
                    setSchedule(new CSchedFFPyroAirblast(this, pProjectile));
                    m_fNextAirblastTime = gpGlobals->time + 1.0f; // Cooldown
                // }
            } else {
                 // Check for burning teammates to extinguish
                for (int i = 1; i <= gpGlobals->maxClients; i++) {
                    CBasePlayer *pPlayer = (CBasePlayer*)UTIL_PlayerByIndex(i);
                    if (pPlayer && pPlayer->IsPlayer() && pPlayer->IsAlive() && pPlayer->pev->team == pev->team && (pPlayer->pev->effects & EF_ONFIRE)) {
                        if (FVisible(ENT(pPlayer->pev), edict()) && (pPlayer->pev->origin - pev->origin).Length() < 300) {
                             m_bWantsToAirblast = true;
                             setSchedule(new CSchedFFPyroAirblast(this, ENT(pPlayer->pev))); // Target teammate to extinguish
                             m_fNextAirblastTime = gpGlobals->time + 1.0f;
                             break;
                        }
                    }
                }
            }
        }
        if (!m_bWantsToAirblast && m_pCurrentSchedule && m_pCurrentSchedule->getScheduleId() == SCHED_FF_PYRO_AIRBLAST_DEFEND) {
            clearSchedule("Airblast condition no longer met");
        }
    }


}

// isEnemy: FF specific enemy detection
bool CBotFF::isEnemy(edict_t *pEdict, bool bCheckWeapons) {
    if (!pEdict || pEdict->v.flags & FL_NOTARGET || pEdict->v.deadflag != DEAD_NO) {
        return false;
    }

    // FF specific: Check for Spy's disguise
    if (pEdict->v.playerclass == FF_CLASS_SPY) { // Assuming playerclass stores current class/disguise
        // If spy is disguised as own team, but bot has means to detect (e.g. pyro spy-check)
        // This is a complex feature. For now, basic team check.
        if (pEdict->v.team == m_iTeam && pEdict->v.effects & EF_DISGUISED) { // EF_DISGUISED is hypothetical
            // Potentially treat as neutral or suspicious, not outright enemy unless revealed
            // For simplicity, let's say if disguised as own team, not an enemy unless attacking.
            return false;
        }
    }
    return CBotFortress::isEnemy(pEdict, bCheckWeapons); // Fallback to base class
}

// Modified setVisible to include armor detection
// Assuming CBotFF does not have its own setVisible yet, so we define it.
// If it did, we'd merge this logic.
bool CBotFF::setVisible(edict_t *pEntity, bool bVisible) {
    bool bBaseVisible = CBotFortress::setVisible(pEntity, bVisible); // Call base

    if (bVisible && pEntity && !pEntity->IsFree()) {
        const char* pszClassname = pEntity->GetClassName();
        // Assuming common FF armor classnames - these need verification
        if (strcmp(pszClassname, "item_armor_shard") == 0 ||
            strcmp(pszClassname, "item_armor_small") == 0 ||
            strcmp(pszClassname, "item_armor_medium") == 0 ||
            strcmp(pszClassname, "item_armor_large") == 0 ||
            strcmp(pszClassname, "item_ff_armor_shard") == 0 ||
            strcmp(pszClassname, "item_ff_armor_small") == 0 ||
            strcmp(pszClassname, "item_ff_armor_medium") == 0 ||
            strcmp(pszClassname, "item_ff_armor_large") == 0) {

            if (!m_pNearestArmorItem.Get() ||
                (distanceFrom(pEntity) < distanceFrom(m_pNearestArmorItem.Get()))) {
                m_pNearestArmorItem.Set(pEntity);
            }
        }
    } else if (!bVisible && pEntity == m_pNearestArmorItem.Get()) {
        m_pNearestArmorItem.Set(NULL); // Clear if it becomes invisible
    }
    return bBaseVisible;
}


// getTasks: FF specific tasks
void CBotFF::getTasks(unsigned int iIgnore) {
    CBotFortress::getTasks(iIgnore); // Base class tasks

    // FF specific tasks
    FF_ClassID currentFFClass_getTasks = CClassInterface::getFFClass(m_pEdict);

    // HWGuy tasks
    if (currentFFClass_getTasks == FF_CLASS_HWGUY) {
        CBotWeapon* pAssaultCannon = m_pWeapons->getWeaponByName("weapon_ff_assaultcannon");
        if (pAssaultCannon && pAssaultCannon->hasWeapon() && !pAssaultCannon->outOfAmmo(this)) {
            float utility = 0.7f; // Base utility for Assault Cannon
            if (m_pEnemy.Get() && CBotGlobals::entityIsAlive(m_pEnemy.Get())) {
                // Check for multiple enemies near the primary target
                // Assuming CTeamFortress2Mod::getEnemyTeam can be replaced by a generic function or FF specific one later
                // Replaced CGame::getEnemyTeam with a more plausible BotSupport::getEnemyTeam
                int enemiesNearbyTarget = CBotGlobals::countPlayersNearOrigin(CBotGlobals::entityOrigin(m_pEnemy.Get()), 300.0f, BotSupport::getEnemyTeam(getTeam()), m_pEdict, true);
                if (enemiesNearbyTarget > 1) {
                    utility += 0.2f; // Bonus for multiple enemies
                }
                // Bonus if defending a point/flag
                if (hasSomeConditions(CONDITION_DEFEND_POINT | CONDITION_DEFEND_FLAG)) { // Assuming these conditions are relevant
                    utility += 0.15f;
                }
            } else if (hasSomeConditions(CONDITION_DEFEND_POINT | CONDITION_DEFEND_FLAG)) {
                // Higher utility for suppressive fire if defending, even without a direct enemy atm
                utility += 0.10f;
            }
            // BOT_UTIL_ATTACK is a generic utility, the weapon choice directs the specifics
            ADD_UTILITY_WEAPON(BOT_UTIL_ATTACK, true, utility, pAssaultCannon);
        }
        // ... (logic for other HWGuy weapons like shotgun can follow) ...
    }

    // Pyro tasks
    if (currentFFClass_getTasks == FF_CLASS_PYRO) {
        CBotWeapon* pFlamer = m_pWeapons->getWeaponByName("weapon_ff_flamethrower");
        CBotWeapon* pIC = m_pWeapons->getWeaponByName("weapon_ff_ic");
        edict_t* pCurrentEnemy = m_pEnemy.Get(); // Use the member variable directly
        bool enemyValidAndVisible = (pCurrentEnemy && CBotGlobals::entityIsAlive(pCurrentEnemy) && isVisible(pCurrentEnemy));
        float enemyDist = enemyValidAndVisible ? distanceFrom(pCurrentEnemy) : 9999.0f;

        // Airblast Logic
        if (pFlamer && pFlamer->hasWeapon() && engine->Time() >= m_fNextAirblastTime) {
            bool airblastConditionMet = false;
            edict_t* airblastTargetEntity = NULL; // Store the entity to aim airblast at

            // 1. Projectile Reflection
            // Iterate through visible entities to find reflectable projectiles
            // This assumes m_pVisibles holds relevant entities. A sphere check might be more robust.
            for (int i = 0; i < m_pVisibles->numVisibles(); ++i) {
                edict_t* pVisibleEnt = m_pVisibles->getVisible(i);
                if (pVisibleEnt && pVisibleEnt != m_pEdict && CBotGlobals::entityIsValid(pVisibleEnt) && !(pVisibleEnt->v.flags & FL_CLIENT) && pVisibleEnt->v.movetype != MOVETYPE_NONE) { // Not a player, valid, and moving
                    if (CClassInterface::isFFReflectableProjectile(pVisibleEnt)) {
                        if (distanceFrom(pVisibleEnt) < 250.0f && isFacingEdict(pVisibleEnt, 0.7f)) {
                            // Potentially check if projectile is hostile (e.g., by checking team of owner if available)
                            airblastConditionMet = true;
                            airblastTargetEntity = pVisibleEnt;
                            break; // Found a projectile
                        }
                    }
                }
            }

            // 2. Push enemy (defensive/environmental)
            if (!airblastConditionMet && enemyValidAndVisible && enemyDist < 180.0f) { // Reduced range for pushing
                airblastConditionMet = true;
                airblastTargetEntity = pCurrentEnemy;
            }

            // 3. Extinguish Teammate
            // if (!airblastConditionMet) {
            //    edict_t* burningTeammate = findBurningTeammate(); // findBurningTeammate() needs implementation
            //    if (burningTeammate && distanceFrom(burningTeammate) < 200.0f && isFacingEdict(burningTeammate, 0.8f)) {
            //        airblastConditionMet = true; airblastTarget = burningTeammate;
            //    }
            // }

            if (airblastConditionMet) {
                // BOT_UTIL_FF_PYRO_AIRBLAST should be a defined enum
                ADD_UTILITY_WEAPON_TARGET(BOT_UTIL_FF_PYRO_AIRBLAST, true, 0.9f, pFlamer, airblastTargetEntity);
            }
        }

        // Incendiary Cannon (IC) Logic
        if (pIC && pIC->hasWeapon() && !pIC->outOfAmmo(this)) {
            if (enemyValidAndVisible && enemyDist > 250.0f && enemyDist < 1200.0f) { // IC for mid-range
                 if (!m_pUtil->hasUtility(BOT_UTIL_FF_PYRO_AIRBLAST)) { // Don't use IC if already planning to airblast
                    ADD_UTILITY_WEAPON(BOT_UTIL_FF_PYRO_USE_IC, true, 0.75f, pIC);
                 }
            }
        }

        // Flamethrower Primary Attack Logic
        if (pFlamer && pFlamer->hasWeapon() && !pFlamer->outOfAmmo(this) && enemyValidAndVisible && enemyDist < 350.0f) { // FF Flamer range
            // Don't primary fire if an airblast utility was already added for the flamer for this thinking cycle
            if (!m_pUtil->hasUtility(BOT_UTIL_FF_PYRO_AIRBLAST)) {
               // BOT_UTIL_ATTACK is a generic attack utility
               ADD_UTILITY_WEAPON(BOT_UTIL_ATTACK, true, 0.8f, pFlamer); // High utility for close range
            }
        }
    }

    // Example: Hunted Mode VIP tasks
    if (g_pGameRules && CVAR_GET_FLOAT("mp_hunted_mode") > 0 || s_IsHuntedModeForTesting) {
        if (m_bIsVIP) { // If bot is VIP
            addUtility(BOT_UTIL_FF_HUNTED_VIP_ESCAPE, BOT_UTIL_FF_HUNTED_VIP_ESCAPE, 100); // Highest priority to escape
        } else if (m_pVIP && m_pVIP->v.team == m_iTeam) { // If VIP is on bot's team
            addUtility(BOT_UTIL_FF_HUNTED_PROTECT_VIP, BOT_UTIL_FF_HUNTED_PROTECT_VIP, 80);
        } else if (m_pVIP && m_pVIP->v.team != m_iTeam) { // If VIP is on enemy team
            addUtility(BOT_UTIL_FF_HUNTED_KILL_VIP, BOT_UTIL_FF_HUNTED_KILL_VIP, 90);
        }
    }

    // Engineer tasks
    if (currentFFClass_getTasks == FF_CLASS_ENGINEER) {
        // EMP Grenade Logic for Engineer
        if (!m_bIsPrimingGrenade) { // Don't consider throwing another grenade if already priming one
            CBotWeapon* pEMPGrenade = m_pWeapons->getWeaponByName("weapon_ff_gren_emp");
            if (pEMPGrenade && pEMPGrenade->hasWeapon() && !pEMPGrenade->outOfAmmo(this)) {
                edict_t* pCurrentEnemy = m_pEnemy.Get();
                if (pCurrentEnemy && CBotGlobals::entityIsAlive(pCurrentEnemy) && isVisible(pCurrentEnemy) &&
                    distanceFrom(pCurrentEnemy) < 1000.0f && distanceFrom(pCurrentEnemy) > 200.0f) { // Effective EMP range

                    float utilityScore = 0.65f;
                    // Potentially increase utility against certain classes if EMP is particularly effective
                    // TF_Class enemyClass = (TF_Class)CClassInterface::getTF2Class(pCurrentEnemy); // Needs FF class detection
                    // if (enemyClass == TF_CLASS_HWGUY || enemyClass == TF_CLASS_SPY) utilityScore += 0.1f;

                    int enemiesNearbyTarget = CBotGlobals::countPlayersNearOrigin(CBotGlobals::entityOrigin(pCurrentEnemy), 250.0f, BotSupport::getEnemyTeam(getTeam()), m_pEdict, true);
                    if (enemiesNearbyTarget > 1) {
                        utilityScore += 0.2f; // Bonus for multiple enemies
                    }

                    // BOT_UTIL_FF_USE_GRENADE_EMP should be a defined enum
                    ADD_UTILITY_WEAPON_TARGET(BOT_UTIL_FF_USE_GRENADE_EMP, true, utilityScore, pEMPGrenade, pCurrentEnemy);
                }
            }
        }
        // Refined Engineer Mancannon logic
        if (engine->Time() >= m_fNextMancannonBuildTime && !m_hBuiltMancannon.Get()) { // Check cooldown and if bot already has one
            // W_FL_FF_MANCANNON_SPOT is a conceptual waypoint flag
            CWaypoint* pBuildSpot = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_FF_MANCANNON_SPOT, getTeam(), 0, true, this);
            if (pBuildSpot) {
                // Check if a friendly mancannon is already very close to this spot by this bot (already handled by m_hBuiltMancannon check)
                // or could iterate all friendly mancannons if a global list existed.
                // For now, simply checking if this bot's mancannon isn't already there is sufficient.
                ADD_UTILITY_DATA(BOT_UTIL_FF_ENGRI_BUILD_MANCANNON, true, 0.7f, CWaypoints::getWaypointIndex(pBuildSpot));
            }
        }
    }

    // Demoman tasks
    if (currentFFClass_getTasks == FF_CLASS_DEMOMAN) {
        CBotWeapon* pPipeLauncher = getWeapon(WEAPON_FF_PIPEBOMBLAUNCHER);
        if (pPipeLauncher && pPipeLauncher->canUse()) {
            if (!m_bHasActivePipes || m_iPipesToLay > 0) { // If no active trap or wants to lay more
                 // Find strategic spot for pipe trap (e.g., chokepoint, near objective)
                 // This is complex. For now, use a placeholder or last known good spot.
                 CWaypoint* pTrapSpot = WaypointFindNearest(pev->origin, NULL, WAYPOINT_FLAG_CHOKEPOINT); // Example flag
                 if (pTrapSpot) {
                    addUtility(BOT_UTIL_FF_DEMO_LAY_PIPE_TRAP, BOT_UTIL_FF_DEMO_LAY_PIPE_TRAP, 50, (void*)pTrapSpot);
                 }
            }
           if (m_bHasActivePipes) {
                addUtility(BOT_UTIL_FF_DEMO_DETONATE_PIPES, BOT_UTIL_FF_DEMO_DETONATE_PIPES, 55); // Slightly higher than laying if pipes are ready
           }
        }
    }

    // Spy tasks
    if (currentFFClass_getTasks == FF_CLASS_SPY) {
        CBotWeapon* pTranqGun = getWeapon(WEAPON_FF_TRANQGUN);
        if (pTranqGun && pTranqGun->canUse() && m_pCurrentEnemy && FVisible(m_pCurrentEnemy,edict())) {
            if ((m_pCurrentEnemy->v.origin - pev->origin).Length() < pTranqGun->getMaxDistance()) {
                 addUtility(BOT_UTIL_FF_SPY_USE_TRANQ, BOT_UTIL_FF_SPY_USE_TRANQ, 65); // High priority if enemy visible
            }
        }
    }

    // Scout tasks
    if (currentFFClass_getTasks == FF_CLASS_SCOUT) {
        CBotWeapon* pCaltrops = getWeapon(WEAPON_FF_GREN_CALTROP); // Assuming caltrops are a weapon
        if (pCaltrops && pCaltrops->canUse()) {
            // Use caltrops defensively or in chokepoints
            if (m_pCurrentEnemy && (m_pCurrentEnemy->v.origin - pev->origin).Length() < 300) { // If enemy is close
                addUtility(BOT_UTIL_FF_SCOUT_USE_CALTROPS, BOT_UTIL_FF_SCOUT_USE_CALTROPS, 40);
            }
        }
        // Conc jump for mobility (if has conc grenade)
        CBotWeapon* pConcGrenade = getWeapon(WEAPON_FF_GREN_CONC);
        if (pConcGrenade && pConcGrenade->canUse()) {
            // Example: Use conc jump to reach high places or escape
            // This requires pathfinding knowledge of conc jump spots.
            // For now, a generic utility.
             addUtility(BOT_UTIL_FF_CONC_JUMP_MOBILITY, BOT_UTIL_FF_CONC_JUMP_MOBILITY, 30);
        }
    }

    // Pyro tasks
    // This second Pyro block in getTasks is redundant as the primary Pyro logic is already above.
    // This will be removed by the nature of the diff if the above Pyro block is the one kept and modified.
    // For safety, explicitly removing it or ensuring the replacement block is comprehensive.
    // The earlier, more detailed Pyro tasks block is the one being kept and enhanced.
    /* if (currentFFClass_getTasks == FF_CLASS_PYRO) {
        CBotWeapon* pIC = getWeapon(WEAPON_FF_IC); // Incendiary Cannon
        if (pIC && pIC->canUse() && m_pCurrentEnemy) {
    */
    // Removing the search for this specific redundant block as it's complex to match if slightly varied.
    // The main Pyro block above is the target for modifications.
    // The following is the END of the second (redundant) Pyro block that should be gone after correct changes to the first Pyro block.
//        if (pFlamer && pFlamer->canUseSecondary()) { // Airblast
//            addUtility(BOT_UTIL_FF_PYRO_AIRBLAST, BOT_UTIL_FF_PYRO_AIRBLAST, 45); // General airblast utility
//        }
//    }

    // Grenade throwing tasks (generic for all classes that can use them)
            if (FVisible(m_pCurrentEnemy, edict()) && (m_pCurrentEnemy->v.origin - pev->origin).Length() < pIC->getMaxDistance()) {
                addUtility(BOT_UTIL_FF_PYRO_USE_IC, BOT_UTIL_FF_PYRO_USE_IC, 50);
            }
        }
        CBotWeapon* pFlamer = getWeapon(WEAPON_FF_FLAMETHROWER);
        if (pFlamer && pFlamer->canUseSecondary()) { // Airblast
            addUtility(BOT_UTIL_FF_PYRO_AIRBLAST, BOT_UTIL_FF_PYRO_AIRBLAST, 45); // General airblast utility
        }
    }

    // Grenade throwing tasks (generic for all classes that can use them)
    // Note: TF_Class is used in CBotFortress, FF classes need mapping or direct use of m_iPlayerClass
    // For this section, assuming m_iPlayerClass holds the direct FF class enum
    // This is a simplified example. Actual grenade throwing is more complex.

    // Armor pickup logic
    if (engine->Time() >= m_fNextArmorCheckTime) {
        int currentArmor = CClassInterface::getPlayerArmor(m_pEdict);
        int maxArmor = CClassInterface::getPlayerMaxArmor(m_pEdict);
        if (maxArmor <= 0) maxArmor = 200; // Default max armor if not provided or invalid (FF typically 200 for max)

        if (currentArmor < maxArmor * 0.8f || currentArmor < 75) { // Get armor if below 80% or less than 75 absolute
            if (m_pNearestArmorItem.Get() && CBotGlobals::entityIsValid(m_pNearestArmorItem.Get()) && isVisible(m_pNearestArmorItem.Get())) {
                float distToArmor = distanceFrom(m_pNearestArmorItem.Get());
                if (distToArmor < 1500.0f) {
                    float utility = 0.5f + (1.0f - (float)currentArmor / maxArmor) * 0.3f;
                    ADD_UTILITY_TARGET(BOT_UTIL_FF_GET_ARMOR, true, utility, m_pNearestArmorItem.Get());
                }
            }
        }
        m_fNextArmorCheckTime = engine->Time() + 0.5f;
    }

    const char* grenTypes[] = { "weapon_ff_gren_std", "weapon_ff_gren_conc", "weapon_ff_gren_nail", "weapon_ff_gren_mirv", "weapon_ff_gren_emp", "weapon_ff_gren_gas", "weapon_ff_gren_caltrop"};
    int grenUtils[] = {BOT_UTIL_FF_USE_GRENADE_STD, BOT_UTIL_FF_USE_GRENADE_CONC, BOT_UTIL_FF_USE_GRENADE_NAIL, BOT_UTIL_FF_USE_GRENADE_MIRV, BOT_UTIL_FF_USE_GRENADE_EMP, BOT_UTIL_FF_USE_GRENADE_GAS, BOT_UTIL_FF_USE_GRENADE_CALTROP};

    for (int i=0; i < sizeof(grenTypes)/sizeof(grenTypes[0]); ++i) {
        CBotWeapon* pGrenade = getWeaponByName(grenTypes[i]);
        if (pGrenade && pGrenade->canUse()) {
            if (m_pCurrentEnemy && FVisible(m_pCurrentEnemy, edict())) { // If has enemy and visible
                float distToEnemy = (m_pCurrentEnemy->v.origin - pev->origin).Length();
                if (distToEnemy > pGrenade->getMinDistance() && distToEnemy < pGrenade->getMaxDistance()) {
                    // Check if grenade throw is viable (clear path, etc.) - complex, omitted for now
                    addUtility(grenUtils[i], grenUtils[i], 40 + pGrenade->getPreference()); // Base utility + weapon preference
                }
            }
        }
    }
}

// executeAction: FF specific action execution
bool CBotFF::executeAction(CBotUtility *util) {
    switch(util->m_iUtility) {
        case BOT_UTIL_FF_USE_GRENADE_STD:
        case BOT_UTIL_FF_USE_GRENADE_CONC:
        case BOT_UTIL_FF_USE_GRENADE_NAIL:
        case BOT_UTIL_FF_USE_GRENADE_MIRV:
        case BOT_UTIL_FF_USE_GRENADE_EMP:
        case BOT_UTIL_FF_USE_GRENADE_GAS:
        case BOT_UTIL_FF_USE_GRENADE_CALTROP:
            {
                CBotWeapon* pGrenade = NULL;
                if (util->m_iUtility == BOT_UTIL_FF_USE_GRENADE_STD) pGrenade = getWeapon(WEAPON_FF_GREN_STD);
                else if (util->m_iUtility == BOT_UTIL_FF_USE_GRENADE_CONC) pGrenade = getWeapon(WEAPON_FF_GREN_CONC);
                else if (util->m_iUtility == BOT_UTIL_FF_USE_GRENADE_NAIL) pGrenade = getWeapon(WEAPON_FF_GREN_NAIL);
                else if (util->m_iUtility == BOT_UTIL_FF_USE_GRENADE_MIRV) pGrenade = getWeapon(WEAPON_FF_GREN_MIRV);
                // ... (add others)

                if (pGrenade && m_pCurrentEnemy) {
                    // FF might have primeable grenades. This is a simplified throw.
                    // Assumes grenades are primed and thrown with primary attack.
                    // float primeTime = pGrenade->getFloatData(BOT_WEAPON_DATA_PRIME_TIME, 0.0f); // Get prime time if applicable
                    setSchedule(new CSchedFFPrimeThrowGrenade(m_pCurrentEnemy->v.origin, pGrenade, 1.0f /*primeTime*/));
                    return true;
                }
            }
            break;
        case BOT_UTIL_FF_CONC_JUMP_MOBILITY:
            {
                 CBotWeapon* pConcGrenade = getWeapon(WEAPON_FF_GREN_CONC);
                 if (pConcGrenade && pConcGrenade->canUse()) {
                    setSchedule(new CSchedFFConcJumpSelf(this, pConcGrenade));
                    return true;
                 }
            }
            break;
        case BOT_UTIL_FF_HUNTED_VIP_ESCAPE:
            // Simplified: Move to a far waypoint (escape point)
            // Real implementation needs designated escape routes/zones.
            if (m_pCurrentPath && m_pCurrentPath->getGoalWpt()) { // If already has a path
                 CWaypoint* pEscapePoint = WaypointFindFarthest(pev->origin, m_pCurrentPath->getGoalWpt()->m_vOrigin);
                 if (pEscapePoint) {
                    findPath(pEscapePoint->m_iWaypointID, BOT_UTIL_FF_HUNTED_VIP_ESCAPE);
                    setSchedule(new CBotSchedule(SCHED_FF_HUNTED_ESCAPE, BOT_UTIL_FF_HUNTED_VIP_ESCAPE, PRIORITY_HIGH, new CBotTaskFollowPath()));
                    return true;
                 }
            } else { // Find a new escape point
                CWaypoint* pEscapePoint = WaypointFindFarthest(pev->origin, pev->origin); // Farthest from current pos
                if (pEscapePoint) {
                    findPath(pEscapePoint->m_iWaypointID, BOT_UTIL_FF_HUNTED_VIP_ESCAPE);
                    setSchedule(new CBotSchedule(SCHED_FF_HUNTED_ESCAPE, BOT_UTIL_FF_HUNTED_VIP_ESCAPE, PRIORITY_HIGH, new CBotTaskFollowPath()));
                    return true;
                }
            }
            break;
        case BOT_UTIL_FF_HUNTED_KILL_VIP:
             if (m_pVIP && m_pVIP->v.deadflag == DEAD_NO) {
                setAttackTarget(m_pVIP, BOT_UTIL_FF_HUNTED_KILL_VIP); // Use existing attack schedule
                return true;
             }
            break;
        case BOT_UTIL_FF_HUNTED_PROTECT_VIP:
            if (m_pVIP && m_pVIP->v.deadflag == DEAD_NO) {
                // Stay near VIP, attack enemies threatening VIP
                // This is complex. Simplified: Move to VIP's location.
                findPathToEntity(m_pVIP, BOT_UTIL_FF_HUNTED_PROTECT_VIP);
                setSchedule(new CBotSchedule(SCHED_FF_HUNTED_GUARD_VIP, BOT_UTIL_FF_HUNTED_PROTECT_VIP, PRIORITY_HIGH, new CBotTaskFollowPath()));
                // Could add tasks to look for enemies near VIP.
                return true;
            }
            break;
        case BOT_UTIL_FF_ENGRI_BUILD_MANCANNON:
            {
                CWaypoint* pBuildSpot = (CWaypoint*)util->m_pVoidData;
                if (pBuildSpot) {
                    setSchedule(new CSchedFFBuildMancannon(this, pBuildSpot));
                    return true;
                }
            }
            break;
        case BOT_UTIL_FF_DEMO_LAY_PIPE_TRAP:
            {
                CWaypoint* pTrapSpotWpt = (CWaypoint*)util->m_pVoidData;
                if (pTrapSpotWpt) {
                     setSchedule(new CSchedFFDemoLayPipeTrap(this, pTrapSpotWpt, 3)); // Lay 3 pipes
                     return true;
                }
            }
            break;
        case BOT_UTIL_FF_DEMO_DETONATE_PIPES:
            {
                setSchedule(new CSchedFFDemoDetonatePipes(this));
                return true;
            }
            break;
        case BOT_UTIL_FF_SPY_USE_TRANQ:
            if (m_pCurrentEnemy) {
                // Simple attack schedule, assuming tranq gun is handled by handleAttack
                setAttackTarget(m_pCurrentEnemy, BOT_UTIL_FF_SPY_USE_TRANQ);
                return true;
            }
            break;
        case BOT_UTIL_FF_SCOUT_USE_CALTROPS:
            // This is like a grenade throw.
            // For simplicity, assume it's thrown like a grenade at enemy's feet.
            if (m_pCurrentEnemy) {
                 CBotWeapon* pCaltrops = getWeapon(WEAPON_FF_GREN_CALTROP);
                 if (pCaltrops) {
                    setSchedule(new CSchedFFPrimeThrowGrenade(m_pCurrentEnemy->v.origin, pCaltrops, 0.1f)); // Short prime time
                    return true;
                 }
            }
            break;
        case BOT_UTIL_FF_PYRO_AIRBLAST:
            if (util->getWeaponChoice() && util->getWeaponChoice()->getWeaponInfo()->isWeaponName("weapon_ff_flamethrower")) {
                m_bWantsToAirblast = true;
                // The actual airblast IN_ATTACK2 will be triggered in handleAttack.
                // We still might want to "attack" the target (e.g. a rocket) to ensure the bot faces it.
                edict_t* airblastTarget = util->getTaskEdictTarget();
                if (airblastTarget) {
                    m_pSchedules->add(new CBotAttackSched(airblastTarget, 0.1f)); // Short schedule to face target
                } else { // If no specific target (e.g. defensive airblast), just ensure state is set
                    m_pSchedules->add(new CBotSchedule()); // Dummy schedule to process m_bWantsToAirblast
                }
                return true;
            }
            break;

        case BOT_UTIL_FF_PYRO_USE_IC:
            if (util->getWeaponChoice() && util->getWeaponChoice()->getWeaponInfo()->isWeaponName("weapon_ff_ic")) {
                if (m_pEnemy.Get() && CBotGlobals::entityIsAlive(m_pEnemy.Get())) {
                    m_pSchedules->add(new CBotAttackSched(m_pEnemy.Get())); // Standard attack schedule
                    return true;
                }
            }
            break;
        case BOT_UTIL_FF_PYRO_AIRBLAST:
            if (util->getWeaponChoice() && util->getWeaponChoice()->getWeaponInfo()->isWeaponName("weapon_ff_flamethrower")) {
                m_bWantsToAirblast = true;
                edict_t* airblastTarget = util->getTaskEdictTarget();

                // Use the new CSchedFFPyroAirblast
                CBotSchedule* pAirblastSchedule = new CSchedFFPyroAirblast(this, airblastTarget);
                m_pSchedules->add(pAirblastSchedule);
                return true;
            }
            break;

        case BOT_UTIL_FF_PYRO_USE_IC:
            if (util->getWeaponChoice() && util->getWeaponChoice()->getWeaponInfo()->isWeaponName("weapon_ff_ic")) {
                if (m_pEnemy.Get() && CBotGlobals::entityIsAlive(m_pEnemy.Get())) {
                    m_pSchedules->add(new CBotAttackSched(m_pEnemy.Get()));
                    return true;
                }
            }
            break;
        case BOT_UTIL_FF_USE_GRENADE_EMP: // Use actual enum value
            {
                CBotWeapon* pEmpGrenadeWeapon = util->getWeaponChoice();
                edict_t* pTargetEnemy = util->getTaskEdictTarget(); // Target enemy passed from getTasks

                if (pEmpGrenadeWeapon && pEmpGrenadeWeapon->getWeaponInfo() &&
                    pTargetEnemy && CBotGlobals::entityIsAlive(pTargetEnemy)) {

                    Vector targetPos = CBotGlobals::entityOrigin(pTargetEnemy);
                    float primeTime = 1.0f; // Standard prime time for EMP, adjust if needed
                    m_pSchedules->add(new CSchedFFPrimeThrowGrenade(targetPos, pEmpGrenadeWeapon, primeTime));
                    return true;
                }
            }
            break;
        case BOT_UTIL_FF_GET_ARMOR:
            {
                edict_t* pArmorItem = util->getTaskEdictTarget();
                if (pArmorItem && CBotGlobals::entityIsValid(pArmorItem)) {
                    m_pSchedules->add(new CBotPickupSched(pArmorItem));
                    return true;
                }
            }
            break;
        case BOT_UTIL_MEDIC_HEAL: // Overriding base medic heal for FF specific
            if (m_pPatient) { // m_pPatient would be set by findPatient or similar logic
                CBotWeapon *pMedkit = getWeapon(WEAPON_FF_MEDKIT);
                if (pMedkit && pMedkit->canUse()) {
                    setSchedule(new CSchedFFMedicHealTeammate(this, m_pPatient));
                    return true;
                }
            }
            break;
        case BOT_UTIL_SNIPE: // Overriding base snipe for FF specific
            if (m_pCurrentEnemy) {
                 CBotWeapon* pSniperRifle = getWeapon(WEAPON_FF_SNIPERRIFLE); // Or Railgun for Sniper
                 if (!pSniperRifle) pSniperRifle = getWeapon(WEAPON_FF_RAILGUN);

                 if (pSniperRifle && pSniperRifle->canUse()) {
                    setSchedule(new CSchedFFSnipe(this, m_pCurrentEnemy));
                    return true;
                 }
            }
            break;
        default:
            return CBotFortress::executeAction(util); // Fallback to base class
    }
    return false;
}

// handleWeapons: FF specific weapon handling
void CBotFF::handleWeapons() {
    CBotFortress::handleWeapons(); // Base class handling

    // FF Specific Weapon Logic (e.g. Spy disguise kit, Engineer build PDA)
    // Example: Spy disguise
    FF_ClassID currentFFClass_handleWeapons = CClassInterface::getFFClass(m_pEdict);
    if (currentFFClass_handleWeapons == FF_CLASS_SPY) {
        // Logic for choosing disguise, activating it, etc.
        // This is highly game-specific and complex.
        // e.g., if (needsToDisguise()) { selectWeapon(WEAPON_FF_DISGUISE_KIT); issueCommand(CMD_PRIMARY_ATTACK); }
    }
}

// handleAttack: FF specific attack logic
bool CBotFF::handleAttack(CBotWeapon *pWeapon, edict_t *pEnemy) {
    if (!pWeapon || !pEnemy) return false;

    // FF Specific Attack Logic
    // Pyro Airblast Handling
    // This logic was already present from a previous step, ensure it's correct and integrated.
    if (pWeapon && pWeapon->getWeaponInfo() &&
        strcmp(pWeapon->getWeaponInfo()->getWeaponName(), "weapon_ff_flamethrower") == 0) {
        if (m_bWantsToAirblast && engine->Time() >= m_fNextAirblastTime) { // Check cooldown
            secondaryAttack();
            m_bWantsToAirblast = false;
            m_fNextAirblastTime = engine->Time() + 0.75f; // Airblast cooldown
            m_pAttackingEnemy.Set(pEnemy);
            return true; // Airblast handled
        }
        // If not airblasting, normal flamethrower logic (hold attack) will be handled by base or subsequent logic
    }
    // Example: Demoman pipebomb launcher secondary fire (detonation)
    if (pWeapon->m_iWeaponID == WEAPON_FF_PIPEBOMBLAUNCHER) {
        if (m_bHasActivePipes && /* conditions to detonate */ true) {
            // If schedule is already trying to detonate, let it.
            if (m_pCurrentSchedule && m_pCurrentSchedule->getScheduleId() == SCHED_FF_DEMO_DETONATE_PIPES) {
                // Schedule will handle the actual press of secondary attack.
                return true; // Allow schedule to continue
            }
            // Otherwise, if we just decided to detonate (e.g. reactive)
            // issueCommand(CMD_SECONDARY_ATTACK); // This might be too direct. Prefer schedules.
        }
    }

    // Example: Pyro airblast for deflection / extinguishing
    if (pWeapon->m_iWeaponID == WEAPON_FF_FLAMETHROWER && m_bWantsToAirblast) {
         // This is usually handled by a schedule like CSchedFFPyroAirblast
         // The schedule would issue CMD_SECONDARY_ATTACK at the right time.
         // For direct handling (less ideal):
         // issueCommand(CMD_SECONDARY_ATTACK);
         // m_bWantsToAirblast = false; // Reset flag
         // m_fNextAirblastTime = gpGlobals->time + 1.0f; // Cooldown
         // return true;
         // Better to let the schedule handle it.
         if (m_pCurrentSchedule && m_pCurrentSchedule->getScheduleId() == SCHED_FF_PYRO_AIRBLAST_DEFEND) {
             return true; // Let schedule do its thing
         }
    }


    // FF specific: Grenade priming logic (if applicable)
    // This is a very simplified example. Real priming is complex.
    if (pWeapon->getBoolData(BOT_WEAPON_DATA_IS_GRENADE, false) && pWeapon->getFloatData(BOT_WEAPON_DATA_PRIME_TIME, 0.0f) > 0) {
        if (m_bIsPrimingGrenade) {
            if (gpGlobals->time >= m_fGrenadePrimeStartTime + m_fPrimeDuration) {
                // Time to throw
                // issueCommand(CMD_PRIMARY_ATTACK); // Release button (if needed by game mechanics)
                // m_bIsPrimingGrenade = false;
                // BotManager_RemoveDangerousEntity(edict()); // No longer holding primed nade
                // return true;
                // This logic is better inside a schedule/task like CTaskFFPrimeGrenade & CTaskFFThrowGrenade
            } else {
                // Continue holding primary attack (if needed)
                // pev->button |= IN_ATTACK;
                // return true; // Still priming
            }
        }
    }

    return CBotFortress::handleAttack(pWeapon, pEnemy); // Fallback
}

// modAim: FF specific aiming adjustments
void CBotFF::modAim(edict_t *pEntity, Vector &v_origin, Vector *v_desired_offset, Vector &v_size, float fDist, float fDist2D) {
    CBotFortress::modAim(pEntity, v_origin, v_desired_offset, v_size, fDist, fDist2D); // Base adjustments

    // FF Specific Aiming
    // Example: Leading for FF projectiles if different from base
    CBotWeapon* pCurrentWeapon = getCurrentWeapon();
    if (pCurrentWeapon && pCurrentWeapon->getBoolData(BOT_WEAPON_DATA_FIRES_PROJECTILE,false)) {
        float projSpeed = pCurrentWeapon->getFloatData(BOT_WEAPON_DATA_PROJ_SPEED, 0.0f);
        if (projSpeed > 0 && pEntity && pEntity->v.velocity.Length() > 50) { // If target is moving
            // FF might have unique projectile properties (gravity, arcs)
            // This is a simple linear prediction.
            Vector vPredictedPos = pEntity->v.origin + (pEntity->v.velocity * (fDist / projSpeed));
            // Could add prediction for player's movement patterns, jumps, etc.

            // Adjust v_origin towards vPredictedPos
            // This requires careful calculation to avoid overshooting, jitter, etc.
            // Example (very basic adjustment):
            Vector vecToPredicted = vPredictedPos - (pev->origin + pev->view_ofs);
            Vector anglesToPredicted;
            UTIL_VecToAngles(vecToPredicted, anglesToPredicted);

            // Get current view angles
            Vector currentViewAngles = pev->v_angle; // Or m_vCurrentAngle if that's used for aiming

            // Smoothly adjust towards predicted angles or directly set view angles
            // This is where sophisticated aim smoothing/filtering would go.
            // For simplicity, this example doesn't directly modify v_origin here,
            // as that's usually the output of the base aiming. Instead, it might influence
            // internal aim targets or suggest offsets. The actual aiming mechanism
            // (setting pev->v_angle or using engine aim functions) would use this.

            // If v_desired_offset is used by the aiming system to fine-tune:
            // *v_desired_offset = vPredictedPos - pEntity->v.origin; // Set offset to the predicted future position relative to current.
            // This is just one way to use it. The exact mechanism depends on how CBot::aimAt is implemented.
        }
    }
}

// chooseClass: FF specific class selection logic
void CBotFF::chooseClass() {
    // FF has more classes than TF2. Needs specific logic.
    // Example: Random selection for now, or based on team needs.
    int available_classes[] = {
        FF_CLASS_SCOUT, FF_CLASS_SNIPER, FF_CLASS_SOLDIER, FF_CLASS_DEMOMAN,
        FF_CLASS_MEDIC, FF_CLASS_HWGUY, FF_CLASS_PYRO, FF_CLASS_SPY,
        FF_CLASS_ENGINEER, FF_CLASS_CIVILIAN // If civilian is playable by bots
    };
    int num_classes = sizeof(available_classes) / sizeof(available_classes[0]);

    // Simple strategy: try to balance classes, or pick based on map/gamemode.
    // This is a placeholder for more complex logic.
    // For now, pick randomly from available classes if no specific strategy.
    m_iDesiredClass = available_classes[RANDOM_LONG(0, num_classes - 1)];

    // More advanced:
    // - Check team composition.
    // - Consider map objectives (e.g., more engineers on defense).
    // - Player's preference if set.

    // If in Hunted mode, different logic might apply
    if (g_pGameRules && CVAR_GET_FLOAT("mp_hunted_mode") > 0 || s_IsHuntedModeForTesting) {
        // Example: If bot's team needs a VIP, and bot is chosen, pick a suitable VIP class (e.g. Civilian)
        // Or, if not VIP, pick classes good for escorting or hunting.
        // This depends on game rules of Hunted.
    }
    selectClass(); // Call selectClass to send the command
}

// selectClass: Sends the class selection command to the server
void CBotFF::selectClass() {
    char cbuf[32];
    sprintf(cbuf, "select_class %d", m_iDesiredClass); // Assuming "select_class <id>" is the FF command
    BotSendCommand(edict(), cbuf);
}

// isZoomed: Check if bot is currently zoomed (e.g. Sniper Rifle)
bool CBotFF::isZoomed() {
    // FF specific check if different from base TF2 (e.g. different weapon IDs or zoom mechanism)
    CBotWeapon* pCurrentWeapon = getCurrentWeapon();
    if (pCurrentWeapon) {
        if (pCurrentWeapon->m_iWeaponID == WEAPON_FF_SNIPERRIFLE ||
            pCurrentWeapon->m_iWeaponID == WEAPON_FF_RAILGUN) { // Assuming Railgun might have a scope/zoom
            // Check player's FOV or a specific flag set by the weapon's zoom function.
            // This is game-dependent. In TF2, pev->fov is used.
            return (pev->fov < CVAR_GET_FLOAT("default_fov")); // Standard check
        }
    }
    return false; // Default if not a zoomable weapon or not zoomed
}


// --- Task Implementations ---
// CTaskFFPrimeGrenade
CTaskFFPrimeGrenade::CTaskFFPrimeGrenade(const Vector &vTargetPos, float fDuration) :
    m_vTargetPos(vTargetPos), m_fPrimeDuration(fDuration), m_fPrimeStartTime(0.0f) {}

void CTaskFFPrimeGrenade::init(CBot* pBot) {
    CBotFF* pFFBot = (CBotFF*)pBot;
    pFFBot->m_bIsPrimingGrenade = true;
    pFFBot->m_fGrenadePrimeStartTime = gpGlobals->time;
    pFFBot->m_vGrenadeTargetPos = m_vTargetPos; // Store for aiming during prime
    pFFBot->m_fPrimeDuration = m_fPrimeDuration;
    // BotManager_AddDangerousEntity(pBot->edict()); // Bot is holding a live grenade
    pBot->pev->button |= IN_ATTACK; // Hold attack to prime
    pBot->setIdealYaw(m_vTargetPos); // Aim while priming
}
void CTaskFFPrimeGrenade::execute(CBot* pBot) {
    if (!isTaskComplete(pBot)) {
        pBot->pev->button |= IN_ATTACK; // Ensure attack button is held
        pBot->setIdealYaw(m_vTargetPos); // Keep aiming
    }
}
bool CTaskFFPrimeGrenade::isTaskComplete(CBot* pBot) {
    return gpGlobals->time >= ((CBotFF*)pBot)->m_fGrenadePrimeStartTime + m_fPrimeDuration;
}
const char* CTaskFFPrimeGrenade::getTaskName() { return "TaskFFPrimeGrenade"; }

// CTaskFFThrowGrenade
CTaskFFThrowGrenade::CTaskFFThrowGrenade(const Vector &vTargetPos, CBotWeapon* pGrenadeWeapon) :
    m_vTargetPos(vTargetPos), m_pGrenadeWeapon(pGrenadeWeapon) {}

void CTaskFFThrowGrenade::execute(CBot* pBot) {
    CBotFF* pFFBot = (CBotFF*)pBot;
    pFFBot->pev->button &= ~IN_ATTACK; // Release attack to throw (common mechanism)
    // Some games might require another command or specific handling.
    // For FF, this might be sufficient if primary fire throws after priming.

    pFFBot->m_bIsPrimingGrenade = false;
    // BotManager_RemoveDangerousEntity(pBot->edict()); // No longer dangerous

    // If weapon needs to be explicitly fired after priming (e.g. TF2 sticky launcher)
    // pBot->issueCommand(CMD_PRIMARY_ATTACK); // Or similar if just releasing isn't enough

    // Mark task as complete immediately after attempting to throw
    // Actual grenade entity creation and physics are handled by the game.
}
bool CTaskFFThrowGrenade::isTaskComplete(CBot* pBot) { return true; } // Task is to initiate the throw
const char* CTaskFFThrowGrenade::getTaskName() { return "TaskFFThrowGrenade"; }

// CTaskFFExecuteConcJump
CTaskFFExecuteConcJump::CTaskFFExecuteConcJump() {}
void CTaskFFExecuteConcJump::execute(CBot* pBot) {
    CBotFF* pFFBot = (CBotFF*)pBot;
    CBotWeapon* pConc = pFFBot->getWeapon(WEAPON_FF_GREN_CONC);
    if (pConc && pConc->canUse()) {
        // Aim down (or specific angle for jump type)
        Vector vAngle = pFFBot->pev->v_angle;
        vAngle.x = 90; // Aim straight down (simplistic)
        pFFBot->setIdealAngle(vAngle);

        // Could have a short prime task here if concs are primeable
        pFFBot->pev->button |= IN_ATTACK; // Press attack
        // After a very short delay (e.g. next frame or using a timer task), release IN_ATTACK
        // And then press IN_JUMP at the right moment. This is complex timing.
        // For simplicity, this task just presses attack. A schedule would manage the sequence.
    }
}
bool CTaskFFExecuteConcJump::isTaskComplete(CBot* pBot) { return true; } // Task is to initiate
const char* CTaskFFExecuteConcJump::getTaskName() { return "TaskFFExecuteConcJump"; }


// CTaskFFEngineerBuild
CTaskFFEngineerBuild::CTaskFFEngineerBuild(int buildableId, const Vector& buildPos) :
    m_buildableId(buildableId), m_vBuildPos(buildPos), m_bCommandSent(false) {}
void CTaskFFEngineerBuild::execute(CBot* pBot) {
    CBotFF* pFFBot = (CBotFF*)pBot;
    if (!m_bCommandSent) {
        // Select build weapon (e.g., Spanner or a PDA)
        // This depends on FF's Engineer mechanics. Assume Spanner for now.
        CBotWeapon* pSpanner = pFFBot->getWeapon(WEAPON_FF_SPANNER_ENGI); // Or a generic build tool ID
        if (pSpanner) {
            pFFBot->selectWeapon(pSpanner->m_iWeaponID);
            // Aim at build position (if required by game)
            pFFBot->setIdealYaw(m_vBuildPos);

            // Issue build command
            // Format: "build <building_id>" or similar. FF_ENGIBUILD_MANCANNON is a placeholder for the actual ID.
            char cmd[64];
            sprintf(cmd, "build %d", m_buildableId); // m_buildableId should match game's internal ID for mancannon
            BotSendCommand(pBot->edict(), cmd);
            m_bCommandSent = true;
            pFFBot->m_fNextMancannonBuildTime = gpGlobals->time + 30.0f; // Cooldown before trying again
        }
    }
    // Bot might need to stay and "hit" the building with spanner. This task only sends the initial command.
    // A follow-up schedule/task would handle the actual construction process if needed.
}
bool CTaskFFEngineerBuild::isTaskComplete(CBot* pBot) {
    // Task is complete once command is sent. Construction is game-handled or by another task.
    // Or, could check if building exists: pFFBot->m_hBuiltMancannon = findEntity(m_vBuildPos, "ff_mancannon_obj");
    return m_bCommandSent;
}
const char* CTaskFFEngineerBuild::getTaskName() { return "TaskFFEngineerBuild"; }


// CTaskFFDemoLaySinglePipe
CTaskFFDemoLaySinglePipe::CTaskFFDemoLaySinglePipe(const Vector& vTargetPos) : m_vTargetPos(vTargetPos), m_bFired(false) {}
void CTaskFFDemoLaySinglePipe::execute(CBot* pBot) {
    CBotFF* pFFBot = (CBotFF*)pBot;
    CBotWeapon* pPipeLauncher = pFFBot->getWeapon(WEAPON_FF_PIPEBOMBLAUNCHER);
    if (pPipeLauncher && pPipeLauncher->canUsePrimary() && !m_bFired) {
        pFFBot->selectWeapon(pPipeLauncher->m_iWeaponID);
        pFFBot->setIdealYaw(m_vTargetPos); // Aim at the spot

        // May need to account for projectile arc
        // Vector aimPos = CalculatePipebombAim(pFFBot->pev->origin, m_vTargetPos, pPipeLauncher->getFloatData(BOT_WEAPON_DATA_PROJ_SPEED));
        // pFFBot->setIdealYaw(aimPos);

        pFFBot->pev->button |= IN_ATTACK; // Fire one pipe
        m_bFired = true; // Only fire once per task execution
        pFFBot->m_bHasActivePipes = true; // Assume at least one pipe is now active
        pFFBot->m_vLastPipeTrapLocation = m_vTargetPos; // Update trap location (can be refined)
        pFFBot->m_fNextPipeLayTime = gpGlobals->time + 0.8f; // Cooldown between pipes
    }
}
bool CTaskFFDemoLaySinglePipe::isTaskComplete(CBot* pBot) {
    if (m_bFired && !(pBot->pev->button & IN_ATTACK)) return true; // Complete once fired and button released
    return false;
}
const char* CTaskFFDemoLaySinglePipe::getTaskName() { return "TaskFFDemoLaySinglePipe"; }

// CTaskFFDemoDetonatePipes
CTaskFFDemoDetonatePipes::CTaskFFDemoDetonatePipes() {}
void CTaskFFDemoDetonatePipes::execute(CBot* pBot) {
    CBotFF* pFFBot = (CBotFF*)pBot;
    CBotWeapon* pPipeLauncher = pFFBot->getWeapon(WEAPON_FF_PIPEBOMBLAUNCHER);
    if (pPipeLauncher && pPipeLauncher->canUseSecondary()) {
        pFFBot->selectWeapon(pPipeLauncher->m_iWeaponID);
        pFFBot->pev->button |= IN_SECONDARY_ATTACK; // Press secondary fire to detonate
        pFFBot->m_bHasActivePipes = false; // Pipes are gone after detonation
    }
}
bool CTaskFFDemoDetonatePipes::isTaskComplete(CBot* pBot) {
    if (!(pBot->pev->button & IN_SECONDARY_ATTACK)) return true; // Complete once button released
    return false;
}
const char* CTaskFFDemoDetonatePipes::getTaskName() { return "TaskFFDemoDetonatePipes"; }

// CTaskFFMedicAimAndHeal
CTaskFFMedicAimAndHeal::CTaskFFMedicAimAndHeal(edict_t* pTarget) : m_hTarget(pTarget) {}
void CTaskFFMedicAimAndHeal::init(CBot* pBot) {
    CBotFF* pFFBot = (CBotFF*)pBot;
    CBotWeapon* pMedkit = pFFBot->getWeapon(WEAPON_FF_MEDKIT);
    if (pMedkit) {
        pFFBot->selectWeapon(pMedkit->m_iWeaponID);
    }
}
void CTaskFFMedicAimAndHeal::execute(CBot* pBot) {
    CBotFF* pFFBot = (CBotFF*)pBot;
    edict_t* pTargetEdict = m_hTarget.Get();
    if (pTargetEdict && pTargetEdict->v.deadflag == DEAD_NO && pTargetEdict->v.health < pTargetEdict->v.max_health) {
        if (FVisible(pTargetEdict, pBot->edict())) {
            pFFBot->setIdealYaw(pTargetEdict->v.origin);
            pFFBot->pev->button |= IN_ATTACK; // Hold attack to heal
        } else {
            pFFBot->pev->button &= ~IN_ATTACK; // Stop healing if target not visible
        }
    } else {
        pFFBot->pev->button &= ~IN_ATTACK; // Stop healing if target is dead or full health
    }
}
bool CTaskFFMedicAimAndHeal::isTaskComplete(CBot* pBot) {
    edict_t* pTargetEdict = m_hTarget.Get();
    if (!pTargetEdict || pTargetEdict->v.deadflag != DEAD_NO || pTargetEdict->v.health >= pTargetEdict->v.max_health) {
        pBot->pev->button &= ~IN_ATTACK; // Ensure button is released
        return true; // Target gone, dead, or fully healed
    }
    // Could also have a timeout or if bot needs to do something else
    return false;
}
const char* CTaskFFMedicAimAndHeal::getTaskName() { return "TaskFFMedicAimAndHeal"; }

// CTaskFFSnipeAttackSequence
CTaskFFSnipeAttackSequence::CTaskFFSnipeAttackSequence(edict_t* pTarget) : m_hTarget(pTarget), m_iState(0), m_fNextActionTime(0.0f) {}
void CTaskFFSnipeAttackSequence::init(CBot* pBot) {
    CBotFF* pFFBot = (CBotFF*)pBot;
    CBotWeapon* pSniperWep = pFFBot->getWeapon(WEAPON_FF_SNIPERRIFLE); // Or Railgun
    if (!pSniperWep) pSniperWep = pFFBot->getWeapon(WEAPON_FF_RAILGUN);

    if (pSniperWep) {
        pFFBot->selectWeapon(pSniperWep->m_iWeaponID);
        m_iState = 0; // Start with zooming
        m_fNextActionTime = gpGlobals->time;
    } else {
        m_iState = 2; // No weapon, task is basically complete (fail)
    }
}
void CTaskFFSnipeAttackSequence::execute(CBot* pBot) {
    CBotFF* pFFBot = (CBotFF*)pBot;
    edict_t* pTargetEdict = m_hTarget.Get();

    if (!pTargetEdict || pTargetEdict->v.deadflag != DEAD_NO) {
        m_iState = 2; // Target gone
        pFFBot->pev->button &= ~IN_ATTACK2; // Unzoom
        pFFBot->pev->button &= ~IN_ATTACK;   // Stop firing
        return;
    }

    pFFBot->setIdealYaw(pTargetEdict->v.origin); // Keep aiming

    if (gpGlobals->time < m_fNextActionTime) return; // Wait for next action time

    switch (m_iState) {
        case 0: // Zoom in
            pFFBot->pev->button |= IN_ATTACK2; // Press secondary to zoom
            m_fNextActionTime = gpGlobals->time + 0.5f; // Time to zoom
            m_iState = 1;
            break;
        case 1: // Fire
            pFFBot->pev->button &= ~IN_ATTACK2; // Stop pressing zoom (usually toggle or hold)
                                             // If it's a hold-to-zoom, this needs to be managed carefully.
                                             // Assuming it's a toggle or stays zoomed once IN_ATTACK2 is used.
            if (pFFBot->isZoomed()) { // Check if actually zoomed
                 pFFBot->pev->button |= IN_ATTACK; // Fire
                 m_fNextActionTime = gpGlobals->time + 1.0f; // Time for weapon refire
                 m_iState = 2; // Move to unzoom or finish
            } else { // Not zoomed, maybe retry zoom or fail
                 m_fNextActionTime = gpGlobals->time + 0.2f; // Quick retry
                 m_iState = 0; // Re-try zoom
            }
            break;
        case 2: // Finished firing or failed
            pFFBot->pev->button &= ~IN_ATTACK;
            // Optionally unzoom immediately, or let another task/logic handle it
            // pFFBot->pev->button |= IN_ATTACK2; // Press again to unzoom if it's a toggle
            break;
    }
}
bool CTaskFFSnipeAttackSequence::isTaskComplete(CBot* pBot) {
    if (m_iState == 2 && gpGlobals->time >= m_fNextActionTime) {
         ((CBotFF*)pBot)->pev->button &= ~IN_ATTACK; // Ensure fire button is released
         // ((CBotFF*)pBot)->pev->button &= ~IN_ATTACK2; // Ensure zoom button is managed if held
        return true;
    }
    return false;
}
const char* CTaskFFSnipeAttackSequence::getTaskName() { return "TaskFFSnipeAttackSequence"; }

// CTaskFFPyroAirblast
CTaskFFPyroAirblast::CTaskFFPyroAirblast(edict_t* pTarget) : m_hTargetEntity(pTarget) {}
void CTaskFFPyroAirblast::init(CBot* pBot) {
    CBotFF* pFFBot = (CBotFF*)pBot;
    CBotWeapon* pFlamer = pFFBot->getWeapon(WEAPON_FF_FLAMETHROWER);
    if (pFlamer) {
        pFFBot->selectWeapon(pFlamer->m_iWeaponID);
    }
}
void CTaskFFPyroAirblast::execute(CBot* pBot) {
    CBotFF* pFFBot = (CBotFF*)pBot;
    edict_t* pTarget = m_hTargetEntity.Get();

    if (pTarget) { // If specific target (projectile or burning player)
        pFFBot->setIdealYaw(pTarget->v.origin);
    } else {
        // General defensive airblast, aim forward or towards anticipated threat
        if (pFFBot->m_pCurrentEnemy && FVisible(pFFBot->m_pCurrentEnemy, pBot->edict())) {
            pFFBot->setIdealYaw(pFFBot->m_pCurrentEnemy->v.origin);
        } else {
            // Aim forward based on current path or facing
            Vector vForward;
            UTIL_MakeVectors(pBot->pev->v_angle, vForward, NULL, NULL);
            pFFBot->setIdealYaw(pBot->pev->origin + vForward * 100);
        }
    }
    pFFBot->pev->button |= IN_SECONDARY_ATTACK; // Perform airblast
    pFFBot->m_bWantsToAirblast = false; // Reset the general flag
    pFFBot->m_fNextAirblastTime = gpGlobals->time + 1.0f; // Cooldown
}
bool CTaskFFPyroAirblast::isTaskComplete(CBot* pBot) {
    // Complete once secondary attack button is released (or after a short duration)
    if (!(pBot->pev->button & IN_SECONDARY_ATTACK)) {
        return true;
    }
    // Or add a timer: return gpGlobals->time > m_fActionCompleteTime;
    return false; // For now, assumes it's pressed and schedule will remove on next think
}
const char* CTaskFFPyroAirblast::getTaskName() { return "TaskFFPyroAirblast"; }


// --- Schedule Implementations ---
// CSchedFFPrimeThrowGrenade
CSchedFFPrimeThrowGrenade::CSchedFFPrimeThrowGrenade(const Vector &vTargetPos, CBotWeapon* pGrenadeWeapon, float fPrimeTime) :
    CBotSchedule(SCHED_FF_PRIME_THROW_GRENADE, BOT_UTIL_FF_USE_GRENADE_STD + (pGrenadeWeapon->m_iWeaponID - WEAPON_FF_GREN_STD), PRIORITY_NORMAL) { // Adjust util ID based on grenade type
    addTask(new CTaskFFPrimeGrenade(vTargetPos, fPrimeTime));
    addTask(new CTaskFFThrowGrenade(vTargetPos, pGrenadeWeapon));
    // May need a short CTaskDelay after throw before bot can do other things
}
const char* CSchedFFPrimeThrowGrenade::getScheduleName() { return "SchedFFPrimeThrowGrenade"; }

// CSchedFFConcJumpSelf
CSchedFFConcJumpSelf::CSchedFFConcJumpSelf(CBotFF* pBot, CBotWeapon* pConcGrenade) :
    CBotSchedule(SCHED_FF_CONC_JUMP_SELF, BOT_UTIL_FF_CONC_JUMP_MOBILITY, PRIORITY_HIGH) {
    // Complex sequence: look down, throw conc, jump.
    // This is a simplified representation.
    addTask(new CTaskLookAt(Vector(pBot->pev->origin.x, pBot->pev->origin.y, pBot->pev->origin.z - 100), 0.2f)); // Look down
    addTask(new CTaskFFPrimeGrenade(pBot->pev->origin + Vector(0,0,-50), 0.1f)); // Short prime at feet
    addTask(new CTaskFFThrowGrenade(pBot->pev->origin + Vector(0,0,-50), pConcGrenade));
    addTask(new CTaskDelay(0.05f)); // Tiny delay for grenade to be active
    addTask(new CTaskJump()); // Jump
    // Timing is critical and game-specific for conc jumps.
}
const char* CSchedFFConcJumpSelf::getScheduleName() { return "SchedFFConcJumpSelf"; }

// CSchedFFBuildMancannon
CSchedFFBuildMancannon::CSchedFFBuildMancannon(CBotFF* pBot, CWaypoint* pBuildSpot) :
    CBotSchedule(SCHED_FF_ENGRI_BUILD_MANCANNON, BOT_UTIL_FF_ENGRI_BUILD_MANCANNON, PRIORITY_NORMAL) {
    if (pBuildSpot) {
        addTask(new CTaskMoveTo(pBuildSpot->m_vOrigin, BOT_UTIL_FF_ENGRI_BUILD_MANCANNON, 0.5f)); // Move to spot
        addTask(new CTaskLookAt(pBuildSpot->m_vOrigin + Vector(0,0,50),0.3f)); // Look at general build area
        addTask(new CTaskFFEngineerBuild(FF_ENGIBUILD_MANCANNON, pBuildSpot->m_vOrigin)); // Build it (FF_ENGIBUILD_MANCANNON is placeholder ID)
        // Could add tasks to defend the spot while building if it takes time.
    }
}
const char* CSchedFFBuildMancannon::getScheduleName() { return "SchedFFBuildMancannon"; }

// CSchedFFDemoLayPipeTrap
CSchedFFDemoLayPipeTrap::CSchedFFDemoLayPipeTrap(CBotFF* pBot, CWaypoint* pTrapSpotWpt, int numPipes) :
    CBotSchedule(SCHED_FF_DEMO_LAY_PIPE_TRAP, BOT_UTIL_FF_DEMO_LAY_PIPE_TRAP, PRIORITY_NORMAL) {
    if (pTrapSpotWpt) {
        pBot->m_iPipesToLay = numPipes;
        addTask(new CTaskMoveTo(pTrapSpotWpt->m_vOrigin, BOT_UTIL_FF_DEMO_LAY_PIPE_TRAP, 0.5f));
        for (int i=0; i < numPipes; ++i) {
            // Slightly vary position for each pipe
            Vector pipePos = pTrapSpotWpt->m_vOrigin + Vector(RANDOM_FLOAT(-30,30),RANDOM_FLOAT(-30,30), 10);
            addTask(new CTaskFFDemoLaySinglePipe(pipePos));
            if (i < numPipes -1) addTask(new CTaskDelay(0.8f)); // Delay between pipes (matches m_fNextPipeLayTime)
        }
        // After laying, bot might want to move away or guard the trap.
    }
}
const char* CSchedFFDemoLayPipeTrap::getScheduleName() { return "SchedFFDemoLayPipeTrap"; }

// CSchedFFDemoDetonatePipes
CSchedFFDemoDetonatePipes::CSchedFFDemoDetonatePipes(CBotFF* pBot) :
    CBotSchedule(SCHED_FF_DEMO_DETONATE_PIPES, BOT_UTIL_FF_DEMO_DETONATE_PIPES, PRIORITY_HIGH) {
    addTask(new CTaskFFDemoDetonatePipes());
    // Could add a task to quickly look away or take cover if detonation is dangerous to self.
}
const char* CSchedFFDemoDetonatePipes::getScheduleName() { return "SchedFFDemoDetonatePipes"; }

// CSchedFFMedicHealTeammate
CSchedFFMedicHealTeammate::CSchedFFMedicHealTeammate(CBotFF* pBot, edict_t* pTargetTeammate) :
    CBotSchedule(SCHED_FF_MEDIC_HEAL_TEAMMATE, BOT_UTIL_MEDIC_HEAL, PRIORITY_MED) { // Using SCHED_HEAL from base for now if appropriate
    if (pTargetTeammate) {
        addTask(new CTaskMoveTo(pTargetTeammate->v.origin, BOT_UTIL_MEDIC_HEAL, 0.7f, true, 300.0f)); // Move within 300 units
        addTask(new CTaskFFMedicAimAndHeal(pTargetTeammate));
        // This schedule will loop as CTaskFFMedicAimAndHeal won't complete until target is healed/gone.
        // Or, can add a CTaskFaceEntity(pTargetTeammate) if movement is separate from aiming.
    }
}
const char* CSchedFFMedicHealTeammate::getScheduleName() { return "SchedFFMedicHealTeammate"; }


// CSchedFFSnipe
CSchedFFSnipe::CSchedFFSnipe(CBotFF* pBot, edict_t* pTarget) :
    CBotSchedule(SCHED_FF_SNIPE_ATTACK, BOT_UTIL_SNIPE, PRIORITY_HIGH) { // Using SCHED_SNIPE from base
    if (pTarget) {
        // Optional: Move to a sniping spot first
        // CWaypoint* pSnipeSpot = WaypointFindNearest(pBot->pev->origin, NULL, WAYPOINT_SNIPE_SPOT);
        // if (pSnipeSpot) addTask(new CTaskMoveTo(pSnipeSpot->m_vOrigin));

        addTask(new CTaskFaceEntity(pTarget)); // Ensure facing target
        addTask(new CTaskFFSnipeAttackSequence(pTarget));
        // After attack, might unzoom or reposition.
    }
}
const char* CSchedFFSnipe::getScheduleName() { return "SchedFFSnipe"; }


// CSchedFFPyroAirblast
CSchedFFPyroAirblast::CSchedFFPyroAirblast(CBotFF* pBot, edict_t* pTarget) :
    CBotSchedule(SCHED_FF_PYRO_AIRBLAST_DEFEND, BOT_UTIL_FF_PYRO_AIRBLAST, PRIORITY_VERY_HIGH) {
    if (pTarget) { // Target for airblast (projectile or player)
        addTask(new CTaskFaceEntity(pTarget)); // Quickly face the target
    }
    addTask(new CTaskFFPyroAirblast(pTarget));
    // Schedule is short, just the airblast action itself.
    // Cooldowns (m_fNextAirblastTime) will prevent immediate re-scheduling.
}
const char* CSchedFFPyroAirblast::getScheduleName() { return "SchedFFPyroAirblast"; }

// TODO: Implement other FF specific schedules:
// SCHED_FF_HUNTED_ESCAPE
// SCHED_FF_HUNTED_GUARD_VIP
// SCHED_FF_PYRO_EXTINGUISH_TEAMMATE (might be part of CSchedFFPyroAirblast if target is teammate)

// Note: Many class IDs (e.g. FF_CLASS_DEMOMAN), weapon IDs (e.g. WEAPON_FF_PIPEBOMBLAUNCHER),
// waypoint flags (e.g. WAYPOINT_FF_BUILD_MANCANNON), and game specific commands ("build <ID>")
// are placeholders and need to be defined according to Fortress Forever's actual implementation.
// The existence of files like "game_ff.h" is assumed for such constants.
// Placeholder BOT_UTIL_ and SCHED_ values also need to be integrated with bot_const.h/bot_schedule.h.

// Placeholder define for utility - this should go into bot_const.h eventually
#define BOT_UTIL_FF_GET_ARMOR 10019 // Ensure this ID is unique (Using a new ID from the original plan)


const CFFPlayerClassInfo* CBotFF::GetClassGameData() const {
    if (!m_pEdict) return nullptr;

    // This m_iBotClass needs to be the FF_ClassID enum type, not TF_ClassID from base.
    // Assuming m_iBotClass is correctly populated with FF_ClassID during bot creation/class selection.
    FF_ClassID currentClass = (FF_ClassID)m_iBotClass;
    const char* className = nullptr;

    switch (currentClass) {
        case FF_CLASS_SCOUT:    className = "ff_playerclass_scout"; break;
        case FF_CLASS_SNIPER:   className = "ff_playerclass_sniper"; break;
        case FF_CLASS_SOLDIER:  className = "ff_playerclass_soldier"; break;
        case FF_CLASS_DEMOMAN:  className = "ff_playerclass_demoman"; break;
        case FF_CLASS_MEDIC:    className = "ff_playerclass_medic"; break;
        case FF_CLASS_HWGUY:    className = "ff_playerclass_hwguy"; break;
        case FF_CLASS_PYRO:     className = "ff_playerclass_pyro"; break;
        case FF_CLASS_SPY:      className = "ff_playerclass_spy"; break;
        case FF_CLASS_ENGINEER: className = "ff_playerclass_engineer"; break;
        case FF_CLASS_CIVILIAN: className = "ff_playerclass_civilian"; break;
        default: return nullptr;
    }

    if (!className) return nullptr;

    int handle = LookupPlayerClassInfoSlot(className);
    if (handle == -1) return nullptr;

    return GetFilePlayerClassInfoFromHandle(handle);
}

int CBotFF::GetMaxHP() const {
    const CFFPlayerClassInfo* pCD = GetClassGameData();
    return pCD ? pCD->m_iHealth : 100; // Default 100 if data not found
}

int CBotFF::GetMaxAP() const {
    const CFFPlayerClassInfo* pCD = GetClassGameData();
    return pCD ? pCD->m_iMaxArmour : 0; // Default 0 if data not found
}

float CBotFF::GetMaxSpeed() const {
    const CFFPlayerClassInfo* pCD = GetClassGameData();
    return pCD ? (float)pCD->m_iSpeed : 320.0f; // Default 320 if data not found
}

weapon_t CBotFF::GetWeaponByIndex(int index) const {
    const CFFPlayerClassInfo* pCD = GetClassGameData();
    if (!pCD || index < 0 || index >= pCD->m_iNumWeapons) return WEAPON_NONE; // WEAPON_NONE or 0
    return g_weaponDefs.getWeaponID(pCD->m_aWeapons[index]);
}

weapon_t CBotFF::GetGrenade1WeaponID() const {
    const CFFPlayerClassInfo* pCD = GetClassGameData();
    if (!pCD || strcmp(pCD->m_szPrimaryClassName, "None") == 0) return WEAPON_NONE;
    return g_weaponDefs.getWeaponID(pCD->m_szPrimaryClassName);
}

weapon_t CBotFF::GetGrenade2WeaponID() const {
    const CFFPlayerClassInfo* pCD = GetClassGameData();
    if (!pCD || strcmp(pCD->m_szSecondaryClassName, "None") == 0) return WEAPON_NONE;
    return g_weaponDefs.getWeaponID(pCD->m_szSecondaryClassName);
}

int CBotFF::GetMaxGren1() const {
    const CFFPlayerClassInfo* pCD = GetClassGameData();
    return pCD ? pCD->m_iPrimaryMax : 0;
}

int CBotFF::GetMaxGren2() const {
    const CFFPlayerClassInfo* pCD = GetClassGameData();
    return pCD ? pCD->m_iSecondaryMax : 0;
}

int CBotFF::GetInitialGren1() const {
    const CFFPlayerClassInfo* pCD = GetClassGameData();
    return pCD ? pCD->m_iPrimaryInitial : 0;
}

int CBotFF::GetInitialGren2() const {
    const CFFPlayerClassInfo* pCD = GetClassGameData();
    return pCD ? pCD->m_iSecondaryInitial : 0;
}

int CBotFF::GetMaxAmmo(int ammoIndex) const {
    const CFFPlayerClassInfo* pCD = GetClassGameData();
    if (!pCD) return 0;

    // These ammo indices (m_iAmmoShells, etc.) are member variables of CBot (or CBotFortress)
    // and should correspond to the game's internal ammo type indices.
    // The CFFPlayerClassInfo struct uses its own names (m_iMaxShells, etc.)
    if (ammoIndex == m_iAmmoShells) return pCD->m_iMaxShells;
    if (ammoIndex == m_iAmmoNails) return pCD->m_iMaxNails;
    if (ammoIndex == m_iAmmoCells) return pCD->m_iMaxCells;
    if (ammoIndex == m_iAmmoRockets) return pCD->m_iMaxRockets;
    if (ammoIndex == m_iAmmoDetpack) return pCD->m_iMaxDetpack; // Assuming m_iAmmoDetpack matches Detpack ammo index
    // FF specific - if m_iAmmoMancannon is defined for Mancannon builder ammo type
    // if (ammoIndex == m_iAmmoMancannon) return pCD->m_iMaxManCannon; // This field might not exist directly for all classes

    // Fallback for other ammo types if not directly mapped above, or if they are not class-specific
    // This might require looking up a global ammo definition if not in CFFPlayerClassInfo
    return CBotFortress::GetMaxAmmo(ammoIndex); // Or a more generic lookup
}


// CTaskFFPyroAirblast
// (Assuming CPlayer is base for CFFPlayer or self can be cast)
#define self ( (CFFPlayer*)m_pPlayer ) // Macro for convenience if direct CFFPlayer access is needed and safe

int CBotFF::GetCurrentHP() const {
    if (!self) return 0;
    return self->GetHealth(); // Standard CBasePlayer method
}

int CBotFF::GetCurrentAP() const {
    if (!self) return 0;
    return self->pev->armorvalue; // Standard CBasePlayer access
}

float CBotFF::GetCurrentSpeed() const {
    if (!self) return 0.0f;
    return self->pev->velocity.Length2D(); // Standard CBasePlayer access
}

int CBotFF::GetAmmoCount(int ammoIndex) const {
    if (!self) return 0;
    return self->m_rgAmmo[ammoIndex]; // Standard CBasePlayer access
}

int CBotFF::GetGrenade1Count() const {
    if (!self) return 0;
    // Assuming CFFPlayer specific member m_iPrimaryAmmo or similar for primary grenades
    // and that GetGrenade1WeaponID() returns the weapon for which this ammo is relevant.
    // This might need a more robust way if ammo types are not directly tied to grenade slots.
    // For now, using the direct member access as hinted by the prompt's example.
    // This requires self to be safely castable to CFFPlayer and m_iPrimary to be the count.
    return self->m_iPrimary;
}

int CBotFF::GetGrenade2Count() const {
    if (!self) return 0;
    // Similar assumption as GetGrenade1Count for m_iSecondaryAmmo or m_iSecondary
    return self->m_iSecondary;
}

bool CBotFF::IsPlayerCloaked() const {
    if (!self) return false;
    FF_ClassID currentClass = CClassInterface::getFFClass(self->edict());
    if (currentClass != FF_CLASS_SPY) return false;
    // Assuming CFFPlayer has an IsCloaked() method or a flag like m_bCloaked
    return self->IsCloaked(); // This method needs to exist on CFFPlayer or its hierarchy
}

int CBotFF::GetPlayerJetpackFuel() const {
    if (!self) return 0;
    FF_ClassID currentClass = CClassInterface::getFFClass(self->edict());
    if (currentClass != FF_CLASS_PYRO) return 0;
    // Assuming CFFPlayer has m_iJetpackFuel or an accessor
    return self->m_iJetpackFuel; // This member needs to exist on CFFPlayer
}

bool CBotFF::IsPlayerBuilding() const {
    if (!self) return false;
    FF_ClassID currentClass = CClassInterface::getFFClass(self->edict());
    if (currentClass != FF_CLASS_ENGINEER) return false;
    // Assuming CFFPlayer has an IsBuilding() method or a flag like m_bIsBuilding
    return self->IsBuilding(); // This method needs to exist on CFFPlayer or its hierarchy
}

bool CBotFF::IsPlayerPrimingGrenade() const {
    if (!self) return false;
    // Assuming CFFPlayer has an IsGrenadePrimed() method or a flag like m_bIsPrimingGrenade
    return self->IsGrenadePrimed(); // This method needs to exist on CFFPlayer or its hierarchy
}

#undef self // Undefine the macro to avoid conflicts elsewhere

CTaskFFPyroAirblast::CTaskFFPyroAirblast(edict_t* pTarget) : m_hTargetEntity(pTarget) {
    setTaskName("TaskFFPyroAirblast"); // Set a name for debugging
    m_bTaskComplete = false; // Initialize completion status
}
void CTaskFFPyroAirblast::init(CBot* pBot) {
    CBotTask::init(pBot); // Call base class init
    // If target wasn't passed in constructor but is available from schedule data
    if (!m_hTargetEntity.Get() && m_pTaskDataTargetEdict) {
        m_hTargetEntity.Set(m_pTaskDataTargetEdict);
    }
}
void CTaskFFPyroAirblast::execute(CBot* pBot) {
    CBotFF* pFFBot = static_cast<CBotFF*>(pBot);
    if (!pFFBot) {
        failTask("Bot pointer is null"); // Use failTask for consistency
        m_bTaskComplete = true;
        return;
    }

    // The actual IN_ATTACK2 is now expected to be done by handleAttack when m_bWantsToAirblast is true.
    // This task mainly ensures the bot is oriented if there's a target.
    edict_t* pTarget = m_hTargetEntity.Get();
    if (pTarget && CBotGlobals::entityIsValid(pTarget) && (CBotGlobals::entityIsAlive(pTarget) || !(pTarget->v.flags & FL_ONGROUND)))) { // Also aim at projectiles not on ground
        pFFBot->setLookAt(pTarget);
        pFFBot->setLookAtTask(LOOK_EDICT_PRIORITY);
    } else {
        pFFBot->setLookAtTask(LOOK_NONE);
    }

    // This task signals the intent to airblast for handleAttack.
    // It completes quickly, letting handleAttack perform the action based on m_bWantsToAirblast.
    // If m_bWantsToAirblast is not set by executeAction, it should be set here.
    pFFBot->m_bWantsToAirblast = true;

    setTaskStatus(TASK_COMPLETE);
    m_bTaskComplete = true;
}
// isTaskComplete is inherited, or can be overridden if specific logic is needed beyond m_bTaskComplete
// bool CTaskFFPyroAirblast::isTaskComplete(CBot* pBot) { return m_bTaskComplete; }
// getTaskName is inherited

// CSchedFFPyroAirblast
CSchedFFPyroAirblast::CSchedFFPyroAirblast(CBotFF* pBot, edict_t* pTarget) {
    setID(SCHED_FF_PYRO_AIRBLAST_DEFEND);
    setScheduleName("SchedFFPyroAirblast"); // Set name for debugging
    if (!pBot) { failSchedule(); return; } // Use failSchedule

    CBotWeapon* pFlamer = pBot->getWeapons()->getWeaponByName("weapon_ff_flamethrower");
    if (!pFlamer || !pFlamer->hasWeapon() || !pFlamer->getWeaponInfo()) {
        failSchedule(); // Use failSchedule
        return;
    }
    addTask(new CSelectWeaponTask(pFlamer->getWeaponInfo()));
    addTask(new CTaskFFPyroAirblast(pTarget)); // This task will signal handleAttack
}
// getScheduleName is inherited
