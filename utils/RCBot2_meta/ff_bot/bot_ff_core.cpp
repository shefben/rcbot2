// bot_ff_core.cpp
// Core implementations for CBotFF methods and generic FF tasks/schedules.

#include "../bot_ff.h"
#include "../bot_globals.h"
#include "../bot_navigation.h"
#include "../bot_manager.h"
#include "../bot_event.h"
#include "../bot_util.h"
#include "../bot_math.h"
#include "../bot_waypoint.h"
#include "../bot_task.h"
#include "../bot_schedule.h"
#include "../bot_weapon_defs.h"
#include "../bot_projectiles.h"
#include "../bot_game.h"
#include "../player.h" // For CBasePlayer
#include "../weapons.h" // For CBasePlayerWeapon
#include "../gamerules.h" // For g_pGameRules
#include "../bot_waypoints.h" // For CWaypoints, CWaypointTypes

#include "game_ff.h"
#include "game_shared/ff/ff_playerclass_parse.h"
#include "game_shared/ff/ff_shareddefs.h"
#include "game_shared/ff/ff_player.h"

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
    m_bWantsToAirblast = false;
    m_fNextAirblastTime = 0.0f;
    m_pNearestArmorItem.Set(NULL);
    m_fNextArmorCheckTime = 0.0f;
}

// Destructor
CBotFF::~CBotFF() {
}

// modThink: Called periodically for FF specific thinking
void CBotFF::modThink() {
    CBotFortress::modThink();

    FF_ClassID currentFFClass_modThink = CClassInterface::getFFClass(m_pEdict);

    if (currentFFClass_modThink == FF_CLASS_HWGUY) {
        this->hwguyModThink();
    }

    // VIP Identification Logic (Refined)
    if (g_pGameRules && CVAR_GET_FLOAT("mp_hunted_mode") > 0 || s_IsHuntedModeForTesting) {
        // TODO-FF: Replace with game-specific IsVIP() check if available.
        // For now, using heuristic: lowest entity index on team 2.
        // This is a significant simplification and may not be accurate.
        edict_t* foundVIP = nullptr;
        if (m_iTeam == 2) { // Assuming Team 2 is the VIP team for this heuristic
            for (int i = 1; i <= gpGlobals->maxClients; i++) {
                CBasePlayer *pPlayer = (CBasePlayer*)UTIL_PlayerByIndex(i); // Consider CBotGlobals::playerByIndex
                if (pPlayer && pPlayer->IsPlayer() && pPlayer->IsAlive() && pPlayer->pev->team == 2) {
                    if (!foundVIP || pPlayer->entindex() < foundVIP->entindex()) {
                        foundVIP = ENT(pPlayer->pev);
                    }
                }
            }
        }
        m_pVIP = foundVIP;
        m_bIsVIP = (m_pVIP == edict());

    } else {
        m_bIsVIP = false;
        m_pVIP = NULL;
    }

    if (currentFFClass_modThink == FF_CLASS_DEMOMAN) {
         // this->demomanModThink(); // If any modThink logic for Demoman is needed
    }

    if (currentFFClass_modThink == FF_CLASS_PYRO) {
        this->pyroModThink();
    }
}

bool CBotFF::isEnemy(edict_t *pEdict, bool bCheckWeapons) {
    if (!pEdict || pEdict->v.flags & FL_NOTARGET || pEdict->v.deadflag != DEAD_NO) {
        return false;
    }
    bool bBaseIsEnemy = CBotFortress::isEnemy(pEdict, bCheckWeapons);
    FF_ClassID enemyFFClass = CClassInterface::getFFClass(pEdict);
    if (enemyFFClass == FF_CLASS_SPY) {
        if (CClassInterface::isFFSpyDisguised(pEdict) &&
            CClassInterface::getFFSpyDisguiseTeam(pEdict) == getTeam()) {
            return false;
        }
    }
    return bBaseIsEnemy;
}

bool CBotFF::setVisible(edict_t *pEntity, bool bVisible) {
    bool bBaseVisible = CBotFortress::setVisible(pEntity, bVisible);
    if (bVisible && pEntity && !pEntity->IsFree()) {
        const char* pszClassname = pEntity->GetClassName();
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
        m_pNearestArmorItem.Set(NULL);
    }
    return bBaseVisible;
}

void CBotFF::getTasks(unsigned int iIgnore) {
    CBotFortress::getTasks(iIgnore);
    FF_ClassID currentFFClass_getTasks = CClassInterface::getFFClass(m_pEdict);

    if (currentFFClass_getTasks == FF_CLASS_HWGUY) {
        this->getHWGuyTasks(iIgnore);
    } else if (currentFFClass_getTasks == FF_CLASS_PYRO) {
        this->getPyroTasks(iIgnore);
    } else if (currentFFClass_getTasks == FF_CLASS_ENGINEER) {
        this->getEngineerTasks(iIgnore);
    } else if (currentFFClass_getTasks == FF_CLASS_DEMOMAN) {
        this->getDemomanTasks(iIgnore);
    } else if (currentFFClass_getTasks == FF_CLASS_SPY) {
        this->getSpyTasks(iIgnore);
    } else if (currentFFClass_getTasks == FF_CLASS_SCOUT) {
        this->getScoutTasks(iIgnore);
    }

    if (g_pGameRules && CVAR_GET_FLOAT("mp_hunted_mode") > 0 || s_IsHuntedModeForTesting) {
        if (m_bIsVIP) {
            addUtility(BOT_UTIL_FF_HUNTED_VIP_ESCAPE, BOT_UTIL_FF_HUNTED_VIP_ESCAPE, 110);
            for(size_t i=0; i < m_pUtil->numUtils(); ++i) {
                CBotUtility* pUtil = m_pUtil->getUtility(i);
                if(pUtil && pUtil->getId() == BOT_UTIL_ATTACK) {
                    pUtil->setUtility(pUtil->getUtility()*0.1f);
                }
            }
        } else if (m_pVIP && m_pVIP->v.team == m_iTeam) { // Protector
            addUtility(BOT_UTIL_FF_HUNTED_PROTECT_VIP, BOT_UTIL_FF_HUNTED_PROTECT_VIP, 85);
            if (m_pVIP && m_pVIP->v.deadflag == DEAD_NO) {
                for (int i = 1; i <= gpGlobals->maxClients; i++) {
                    CBasePlayer *pPlayer = CBotGlobals::playerByIndex(i);
                    if (pPlayer && CBotGlobals::isAlive(ENT(pPlayer->pev)) && isEnemy(ENT(pPlayer->pev)) && isVisible(ENT(pPlayer->pev))) {
                        if ((pPlayer->pev->origin - m_pVIP->pev->origin).LengthSqr() < (800.0f * 800.0f)) {
                            ADD_UTILITY_ATTACK(ENT(pPlayer->pev), 95);
                        }
                    }
                }
            }
        } else if (m_pVIP && m_pVIP->v.team != m_iTeam) { // Hunter
            addUtility(BOT_UTIL_FF_HUNTED_KILL_VIP, BOT_UTIL_FF_HUNTED_KILL_VIP, 95);
        }
    }

    if (engine->Time() >= m_fNextArmorCheckTime) {
        int currentArmor = CClassInterface::getPlayerArmor(m_pEdict);
        int maxArmor = CClassInterface::getPlayerMaxArmor(m_pEdict);
        if (maxArmor <= 0) maxArmor = 200;
        if (currentArmor < maxArmor * 0.8f || currentArmor < 75) {
            if (m_pNearestArmorItem.Get() && CBotGlobals::entityIsValid(m_pNearestArmorItem.Get()) && isVisible(m_pNearestArmorItem.Get())) {
                float distToArmor = distanceFrom(m_pNearestArmorItem.Get());
                if (distToArmor < 1500.0f) {
                    float utility = 0.5f + (1.0f - (float)currentArmor / maxArmor) * 0.3f;
                    if (m_bIsVIP && utility > 0.3f) utility = 0.3f;
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
            if (m_pCurrentEnemy && FVisible(m_pCurrentEnemy, edict())) {
                float distToEnemy = (m_pCurrentEnemy->v.origin - pev->origin).Length();
                if (distToEnemy > pGrenade->getMinDistance() && distToEnemy < pGrenade->getMaxDistance()) {
                    addUtility(grenUtils[i], grenUtils[i], 40 + pGrenade->getPreference());
                }
            }
        }
    }
}

bool CBotFF::executeAction(CBotUtility *util) {
    FF_ClassID currentFFClass = CClassInterface::getFFClass(m_pEdict);

    if (currentFFClass == FF_CLASS_SCOUT) {
        if (this->executeScoutAction(util)) return true;
    } else if (currentFFClass == FF_CLASS_DEMOMAN) {
        if (this->executeDemomanAction(util)) return true;
    } else if (currentFFClass == FF_CLASS_ENGINEER) {
        if (this->executeEngineerAction(util)) return true;
    } else if (currentFFClass == FF_CLASS_PYRO) {
        if (this->executePyroAction(util)) return true;
    } else if (currentFFClass == FF_CLASS_MEDIC) {
         if (util->m_iUtility == BOT_UTIL_MEDIC_HEAL && this->executeMedicAction(util)) return true;
    } else if (currentFFClass == FF_CLASS_SNIPER) {
         if (util->m_iUtility == BOT_UTIL_SNIPE && this->executeSniperAction(util)) return true;
    } else if (currentFFClass == FF_CLASS_SPY) {
         if (this->executeSpyAction(util)) return true;
    }

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
                else if (util->m_iUtility == BOT_UTIL_FF_USE_GRENADE_EMP) pGrenade = getWeapon(WEAPON_FF_GREN_EMP);
                else if (util->m_iUtility == BOT_UTIL_FF_USE_GRENADE_GAS) pGrenade = getWeapon(WEAPON_FF_GREN_GAS);
                else if (util->m_iUtility == BOT_UTIL_FF_USE_GRENADE_CALTROP) pGrenade = getWeapon(WEAPON_FF_GREN_CALTROP);

                if (pGrenade && m_pCurrentEnemy) {
                    setSchedule(new CSchedFFPrimeThrowGrenade(m_pCurrentEnemy->v.origin, pGrenade, 1.0f));
                    return true;
                }
            }
            break;
        case BOT_UTIL_FF_HUNTED_VIP_ESCAPE:
            setSchedule(new CSchedFFHuntedVIPEscape(this));
            return true;
        case BOT_UTIL_FF_HUNTED_KILL_VIP:
             if (m_pVIP && m_pVIP->v.deadflag == DEAD_NO) {
                setAttackTarget(m_pVIP, BOT_UTIL_FF_HUNTED_KILL_VIP);
                return true;
             }
            break;
        case BOT_UTIL_FF_HUNTED_PROTECT_VIP:
            setSchedule(new CSchedFFHuntedProtectVIP(this));
            return true;
        case BOT_UTIL_FF_GET_ARMOR:
            {
                edict_t* pArmorItem = util->getTaskEdictTarget();
                if (pArmorItem && CBotGlobals::entityIsValid(pArmorItem)) {
                    m_pSchedules->add(new CBotPickupSched(pArmorItem));
                    return true;
                }
            }
            break;
        default:
            return CBotFortress::executeAction(util);
    }
    return false;
}

void CBotFF::handleWeapons() {
    CBotFortress::handleWeapons();
    FF_ClassID currentFFClass_handleWeapons = CClassInterface::getFFClass(m_pEdict);
    if (currentFFClass_handleWeapons == FF_CLASS_SPY) {
        // this->spyHandleWeapons();
    }
}

bool CBotFF::handleAttack(CBotWeapon *pWeapon, edict_t *pEnemy) {
    if (!pWeapon || !pEnemy) return false;
    FF_ClassID currentFFClass_handleAttack = CClassInterface::getFFClass(m_pEdict);

    if (currentFFClass_handleAttack == FF_CLASS_PYRO &&
        pWeapon && pWeapon->getWeaponInfo() &&
        strcmp(pWeapon->getWeaponInfo()->getWeaponName(), "weapon_ff_flamethrower") == 0) {
        if (m_bWantsToAirblast && engine->Time() >= m_fNextAirblastTime) {
            secondaryAttack();
            m_bWantsToAirblast = false;
            m_fNextAirblastTime = engine->Time() + 0.75f;
            m_pAttackingEnemy.Set(pEnemy);
            return true;
        }
    }
    if (currentFFClass_handleAttack == FF_CLASS_DEMOMAN && pWeapon->m_iWeaponID == WEAPON_FF_PIPEBOMBLAUNCHER) {
        if (m_bHasActivePipes) {
            if (m_pCurrentSchedule && m_pCurrentSchedule->getScheduleId() == SCHED_FF_DEMO_DETONATE_PIPES) {
                return true;
            }
        }
    }
    return CBotFortress::handleAttack(pWeapon, pEnemy);
}

void CBotFF::modAim(edict_t *pEntity, Vector &v_origin, Vector *v_desired_offset, Vector &v_size, float fDist, float fDist2D) {
    CBotFortress::modAim(pEntity, v_origin, v_desired_offset, v_size, fDist, fDist2D);
}

void CBotFF::chooseClass() {
    int available_classes[] = {
        FF_CLASS_SCOUT, FF_CLASS_SNIPER, FF_CLASS_SOLDIER, FF_CLASS_DEMOMAN,
        FF_CLASS_MEDIC, FF_CLASS_HWGUY, FF_CLASS_PYRO, FF_CLASS_SPY,
        FF_CLASS_ENGINEER, FF_CLASS_CIVILIAN
    };
    int num_classes = sizeof(available_classes) / sizeof(available_classes[0]);
    m_iDesiredClass = available_classes[RANDOM_LONG(0, num_classes - 1)];
    selectClass();
}

void CBotFF::selectClass() {
    char cbuf[32];
    sprintf(cbuf, "select_class %d", m_iDesiredClass);
    BotSendCommand(edict(), cbuf);
}

bool CBotFF::isZoomed() {
    CBotWeapon* pCurrentWeapon = getCurrentWeapon();
    if (pCurrentWeapon) {
        if (pCurrentWeapon->m_iWeaponID == WEAPON_FF_SNIPERRIFLE ||
            pCurrentWeapon->m_iWeaponID == WEAPON_FF_RAILGUN) {
            return (pev->fov < CVAR_GET_FLOAT("default_fov"));
        }
    }
    return false;
}

// --- Generic Task Implementations ---
CTaskFFPrimeGrenade::CTaskFFPrimeGrenade(const Vector &vTargetPos, float fDuration) :
    m_vTargetPos(vTargetPos), m_fPrimeDuration(fDuration), m_fPrimeStartTime(0.0f) {}

void CTaskFFPrimeGrenade::init(CBot* pBot) {
    CBotFF* pFFBot = (CBotFF*)pBot;
    pFFBot->m_bIsPrimingGrenade = true;
    pFFBot->m_fGrenadePrimeStartTime = gpGlobals->time;
    pFFBot->m_vGrenadeTargetPos = m_vTargetPos;
    pFFBot->m_fPrimeDuration = m_fPrimeDuration;
    pBot->pev->button |= IN_ATTACK;
    pBot->setIdealYaw(m_vTargetPos);
}
void CTaskFFPrimeGrenade::execute(CBot* pBot) {
    if (!isTaskComplete(pBot)) {
        pBot->pev->button |= IN_ATTACK;
        pBot->setIdealYaw(((CBotFF*)pBot)->m_vGrenadeTargetPos);
    }
}
bool CTaskFFPrimeGrenade::isTaskComplete(CBot* pBot) {
    return gpGlobals->time >= ((CBotFF*)pBot)->m_fGrenadePrimeStartTime + ((CBotFF*)pBot)->m_fPrimeDuration;
}
const char* CTaskFFPrimeGrenade::getTaskName() { return "TaskFFPrimeGrenade"; }

CTaskFFThrowGrenade::CTaskFFThrowGrenade(const Vector &vTargetPos, CBotWeapon* pGrenadeWeapon) :
    m_vTargetPos(vTargetPos), m_pGrenadeWeapon(pGrenadeWeapon) {}

void CTaskFFThrowGrenade::execute(CBot* pBot) {
    CBotFF* pFFBot = (CBotFF*)pBot;
    pFFBot->pev->button &= ~IN_ATTACK;
    pFFBot->m_bIsPrimingGrenade = false;
}
bool CTaskFFThrowGrenade::isTaskComplete(CBot* pBot) { return true; }
const char* CTaskFFThrowGrenade::getTaskName() { return "TaskFFThrowGrenade"; }

// --- Generic Schedule Implementations ---
CSchedFFPrimeThrowGrenade::CSchedFFPrimeThrowGrenade(const Vector &vTargetPos, CBotWeapon* pGrenadeWeapon, float fPrimeTime) :
    CBotSchedule(SCHED_FF_PRIME_THROW_GRENADE, BOT_UTIL_FF_USE_GRENADE_STD + (pGrenadeWeapon->m_iWeaponID - WEAPON_FF_GREN_STD), PRIORITY_NORMAL) {
    addTask(new CTaskFFPrimeGrenade(vTargetPos, fPrimeTime));
    addTask(new CTaskFFThrowGrenade(vTargetPos, pGrenadeWeapon));
}
const char* CSchedFFPrimeThrowGrenade::getScheduleName() { return "SchedFFPrimeThrowGrenade"; }

// Implementations for Scout, Demoman, Engineer & Pyro specific Tasks/Schedules are now in their respective class files.
// Medic specific tasks/schedules moved to bot_ff_medic.cpp
// Sniper specific tasks/schedules moved to bot_ff_sniper.cpp

// Placeholder define for utility - this should go into bot_const.h eventually
#define BOT_UTIL_FF_GET_ARMOR 10019 // Ensure this ID is unique (Using a new ID from the original plan)

// Implementations of CClassInterface methods have been moved to utils/RCBot2_meta/ff_bot/bot_ff_class_interface.cpp

// Define conceptual constant, ideally this would be in a shared constants header
const float BOT_MAX_PATH_LOOKAHEAD_VIP_ESCAPE = 300.0f;

// --- Hunted Mode Task Implementations ---
CTaskFFFindEscapeRoute::CTaskFFFindEscapeRoute() {
    setTaskName("TaskFFFindEscapeRoute");
    m_bRouteFound = false;
}

void CTaskFFFindEscapeRoute::init(CBot* pBot) {
    CBotTask::init(pBot); // Call base class init
    m_bRouteFound = false;
}

CWaypoint* CTaskFFFindEscapeRoute::WaypointFindFarthestFromVisibleEnemies(CBotFF* pBot) {
    // TODO-FF: Implement proper logic:
    // 1. Get all visible enemies.
    // 2. Calculate a "danger centroid" or average enemy position.
    // 3. Find a waypoint that is farthest from this centroid, but still reachable.
    // Consider cover and teammate proximity as well.
    return nullptr; // Stub
}

void CTaskFFFindEscapeRoute::execute(CBot* pBot) {
    CBotFF* pFFBot = static_cast<CBotFF*>(pBot);
    if (!pFFBot) {
        failTask("Bot pointer is null");
        m_bRouteFound = true; // Mark as complete to avoid loop
        return;
    }

    CWaypoint* pTargetWpt = nullptr;
    // Conceptual flag W_FL_FF_ESCAPE_ZONE would be defined in waypoint system.
    // CWaypoint* pEscapeZoneWpt = CWaypoints::findRandomWaypointWithFlag(W_FL_FF_ESCAPE_ZONE);
    CWaypoint* pEscapeZoneWpt = nullptr; // Placeholder until flag is real
    if (pEscapeZoneWpt) {
        pTargetWpt = pEscapeZoneWpt;
    } else {
        pTargetWpt = WaypointFindFarthestFromVisibleEnemies(pFFBot);
        if (!pTargetWpt) {
            pTargetWpt = WaypointFindFarthest(pFFBot->pev->origin, pFFBot->pev->origin);
        }
    }
    if (pTargetWpt) {
        pFFBot->findPath(pTargetWpt->m_iWaypointID, BOT_UTIL_FF_HUNTED_VIP_ESCAPE);
        if (pFFBot->hasPath()) {
            m_bRouteFound = true;
            setTaskStatus(TASK_COMPLETE);
            return;
        }
    }
    failTask("Could not determine or path to an escape route");
}

bool CTaskFFFindEscapeRoute::isTaskComplete(CBot* pBot) {
    return hasFailed() || m_bRouteFound;
}

const char* CTaskFFFindEscapeRoute::getTaskName() {
    return "TaskFFFindEscapeRoute";
}

// --- Hunted Mode Schedule Implementations ---
CSchedFFHuntedVIPEscape::CSchedFFHuntedVIPEscape(CBotFF* pBot) :
    CBotSchedule(SCHED_FF_HUNTED_ESCAPE, BOT_UTIL_FF_HUNTED_VIP_ESCAPE, PRIORITY_VERY_HIGH) {
    setScheduleName("SchedFFHuntedVIPEscape");
    if (!pBot) { failSchedule(); return; }

    addTask(new CTaskFFFindEscapeRoute());
    addTask(new CBotTaskFollowPath(BOT_MAX_PATH_LOOKAHEAD_VIP_ESCAPE));
}

const char* CSchedFFHuntedVIPEscape::getScheduleName() {
    return "SchedFFHuntedVIPEscape";
}

// --- Protector Task Implementations ---
CTaskFFStayNearVIP::CTaskFFStayNearVIP(float fFollowDistance) :
    m_fFollowDistance(fFollowDistance), m_fNextRepathTime(0.0f) {
    setTaskName("TaskFFStayNearVIP");
}

void CTaskFFStayNearVIP::init(CBot* pBot) {
    CBotTask::init(pBot);
    m_fNextRepathTime = 0.0f;
}

void CTaskFFStayNearVIP::execute(CBot* pBot) {
    CBotFF* pFFBot = static_cast<CBotFF*>(pBot);
    if (!pFFBot || !pFFBot->m_pVIP || pFFBot->m_pVIP->v.deadflag != DEAD_NO) {
        failTask("VIP not found or dead for StayNearVIP");
        return;
    }
    if (gpGlobals->time < m_fNextRepathTime && pFFBot->hasPath()) return;

    float fDistSqrToVIP = (pFFBot->m_pVIP->pev->origin - pFFBot->pev->origin).LengthSqr();
    if (fDistSqrToVIP > (m_fFollowDistance * m_fFollowDistance)) {
        pFFBot->findPathToEntity(pFFBot->m_pVIP, BOT_UTIL_FF_HUNTED_PROTECT_VIP);
    } else if (fDistSqrToVIP < ((m_fFollowDistance * 0.5f) * (m_fFollowDistance * 0.5f)) ) {
        if(pFFBot->hasPath()) pFFBot->clearPath();
    }
    m_fNextRepathTime = gpGlobals->time + 0.5f;
}

bool CTaskFFStayNearVIP::isTaskComplete(CBot* pBot) {
    // This task runs continuously unless explicitly failed (e.g. VIP dies)
    return hasFailed();
}

const char* CTaskFFStayNearVIP::getTaskName() {
    return "TaskFFStayNearVIP";
}

// --- Protector Schedule Implementations ---
CSchedFFHuntedProtectVIP::CSchedFFHuntedProtectVIP(CBotFF* pBot) :
    CBotSchedule(SCHED_FF_HUNTED_GUARD_VIP, BOT_UTIL_FF_HUNTED_PROTECT_VIP, PRIORITY_HIGH) {
    setScheduleName("SchedFFHuntedProtectVIP");
    if (!pBot) { failSchedule(); return; }
    addTask(new CTaskFFStayNearVIP());
}

const char* CSchedFFHuntedProtectVIP::getScheduleName() {
    return "SchedFFHuntedProtectVIP";
}
