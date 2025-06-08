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
    if (currentFFClass_modThink == FF_CLASS_HWGUY) {
        this->hwguyModThink(); // Call to helper function
    }

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
    if (currentFFClass_modThink == FF_CLASS_PYRO) {
        this->pyroModThink(); // Call to helper function
    }

}

// isEnemy: FF specific enemy detection
bool CBotFF::isEnemy(edict_t *pEdict, bool bCheckWeapons) {
    if (!pEdict || pEdict->v.flags & FL_NOTARGET || pEdict->v.deadflag != DEAD_NO) {
        return false;
    }

    // Call base class isEnemy first. If it considers the edict an enemy, then proceed with FF checks.
    // If base class says it's NOT an enemy (e.g. same team, not disguised), then usually respect that.
    // However, FF spy disguise can make an enemy appear as a teammate.

    bool bBaseIsEnemy = CBotFortress::isEnemy(pEdict, bCheckWeapons);

    FF_ClassID enemyFFClass = CClassInterface::getFFClass(pEdict);
    if (enemyFFClass == FF_CLASS_SPY) {
        if (CClassInterface::isFFSpyDisguised(pEdict) &&
            CClassInterface::getFFSpyDisguiseTeam(pEdict) == getTeam()) {
            // This is an enemy spy disguised as a friendly. Base check might say it's friendly.
            // For FF, this should NOT be considered an enemy *for targeting* until revealed or suspicious.
            // However, if the baseIsEnemy was true (e.g. due to FFA mode), a disguised spy is still an enemy.
            // The logic here is tricky: if base says friendly, and it's a disguised spy on my team, it's NOT an enemy.
            // If base says enemy, it's an enemy.
            // If base says friendly, but it's a spy disguised as me, it's NOT an enemy to shoot.
            return false; // Do not target spies disguised as friendlies.
        }
    }

    // If it's not a spy disguised as a teammate, rely on the base class determination.
    return bBaseIsEnemy;
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
        this->getHWGuyTasks(iIgnore); // Call to helper function
    }

    // Pyro tasks
    if (currentFFClass_getTasks == FF_CLASS_PYRO) {
        this->getPyroTasks(iIgnore); // Call to helper function
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
        this->getEngineerTasks(iIgnore); // Call to helper function
    }

    // Demoman tasks
    if (currentFFClass_getTasks == FF_CLASS_DEMOMAN) {
        this->getDemomanTasks(iIgnore); // Call to helper function
    }

    // Spy tasks
    if (currentFFClass_getTasks == FF_CLASS_SPY) {
        this->getSpyTasks(iIgnore); // Call to helper function
    }

    // Scout tasks
    if (currentFFClass_getTasks == FF_CLASS_SCOUT) {
        this->getScoutTasks(iIgnore); // Call to helper function
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
        // Scout specific actions are handled by executeScoutAction
        case BOT_UTIL_FF_CONC_JUMP_MOBILITY: // Fallthrough
        case BOT_UTIL_FF_SCOUT_USE_CALTROPS:
            if (this->executeScoutAction(util)) return true;
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
            if (this->executeEngineerAction(util)) return true;
            break;
        // Demoman specific actions are now handled by executeDemomanAction
        case BOT_UTIL_FF_DEMO_LAY_PIPE_TRAP: // Fallthrough
        case BOT_UTIL_FF_DEMO_DETONATE_PIPES:
            if (this->executeDemomanAction(util)) return true;
            break;
        case BOT_UTIL_FF_SPY_USE_TRANQ:
            if (m_pCurrentEnemy) {
                // Simple attack schedule, assuming tranq gun is handled by handleAttack
                setAttackTarget(m_pCurrentEnemy, BOT_UTIL_FF_SPY_USE_TRANQ);
                return true;
            }
            break;
        // BOT_UTIL_FF_SCOUT_USE_CALTROPS handled above by executeScoutAction
        // BOT_UTIL_FF_SCOUT_USE_CALTROPS handled above by executeScoutAction
        case BOT_UTIL_FF_PYRO_AIRBLAST: // Fallthrough
        case BOT_UTIL_FF_PYRO_USE_IC:
            if (this->executePyroAction(util)) return true;
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
        case BOT_UTIL_MEDIC_HEAL:
            if (this->executeMedicAction(util)) return true;
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

// Implementations for Scout & Demoman specific Tasks/Schedules are now in their respective class files.
// CTaskFFExecuteConcJump was moved to bot_ff_scout.cpp
// CTaskFFDemoLaySinglePipe and CTaskFFDemoDetonatePipes were moved to bot_ff_demoman.cpp

// Implementations for Scout, Demoman, Engineer & Pyro specific Tasks/Schedules are now in their respective class files.
// Medic specific tasks/schedules moved to bot_ff_medic.cpp

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

// CSchedFFConcJumpSelf was moved to bot_ff_scout.cpp
// CSchedFFDemoLayPipeTrap and CSchedFFDemoDetonatePipes were moved to bot_ff_demoman.cpp

// Implementations for Engineer specific Tasks/Schedules are now in bot_ff_engineer.cpp
// Implementations for Pyro specific Tasks/Schedules are now in bot_ff_pyro.cpp
// Medic specific tasks/schedules moved to bot_ff_medic.cpp

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

// Implementations of CClassInterface methods have been moved to utils/RCBot2_meta/ff_bot/bot_ff_class_interface.cpp
