// bot_ff_scout.cpp
// Scout-specific logic for CBotFF

#include "../../bot_ff.h"
#include "../../bot_globals.h"
#include "../../bot_navigation.h" // For CWaypoint, CWaypoints if used by Scout logic
#include "../../bot_manager.h"   // For BotSendCommand if used
#include "../../bot_event.h"     // If Scout logic uses events
#include "../../bot_util.h"      // For ADD_UTILITY_X macros
#include "../../bot_math.h"
#include "../../bot_task.h"      // For CBotTask and derived
#include "../../bot_schedule.h"  // For CBotSchedule and derived
#include "../../bot_weapon_defs.h" // For weapon IDs if needed by tasks/schedules
#include "../../bot_waypoints.h" // For CWaypointTypes (used in commented out code)

void CBotFF::getScoutTasks(unsigned int iIgnore) {
    // Note: currentFFClass_getTasks is established in the calling CBotFF::getTasks()
    // We can assume this function is only called if the class is Scout.

    CBotWeapon* pCaltrops = getWeapon(WEAPON_FF_GREN_CALTROP);
    if (pCaltrops && pCaltrops->canUse()) {
        // Offensive Use
        if (m_pCurrentEnemy && (m_pCurrentEnemy->v.origin - pev->origin).Length() < 300) {
            // ADD_UTILITY_WEAPON_TARGETPOS expects a weapon, but we also need to pass the target edict for default throw
            // For simplicity, if offensive, target current enemy.
            ADD_UTILITY_WEAPON_TARGET(BOT_UTIL_FF_SCOUT_USE_CALTROPS, true, 45, pCaltrops, m_pCurrentEnemy.Get());
        }
        // Defensive Use
        if (m_iHealth < 50 && m_pLastEnemy.Get() && (m_pLastEnemy.Get()->v.origin - pev->origin).Length() < 400 && isMovingAwayFrom(m_pLastEnemy.Get())) {
            Vector vDirToEnemy = (m_pLastEnemy.Get()->v.origin - pev->origin).Normalize();
            Vector vCaltropPos = pev->origin - vDirToEnemy * 100;
            ADD_UTILITY_TARGETPOS(BOT_UTIL_FF_SCOUT_USE_CALTROPS, true, 55, pCaltrops, vCaltropPos);
        }
        // Area Denial (Placeholder)
        // CWaypoint* pChoke = WaypointFindNearest(pev->origin, nullptr, W_FL_CHOKEPOINT | W_FL_DEFEND_HERE);
        // if (pChoke && (pChoke->m_vOrigin - pev->origin).Length() < 200) {
        //    ADD_UTILITY_TARGETPOS(BOT_UTIL_FF_SCOUT_USE_CALTROPS, true, 40, pCaltrops, pChoke->m_vOrigin);
        // }
    }

    CBotWeapon* pConcGrenade = getWeapon(WEAPON_FF_GREN_CONC);
    if (pConcGrenade && pConcGrenade->canUse()) {
        // Escape Danger
        if (m_iHealth < 60 && m_pCurrentEnemy && (m_pCurrentEnemy->v.origin - pev->origin).Length() < 250) {
            ADD_UTILITY(BOT_UTIL_FF_CONC_JUMP_MOBILITY, true, 75);
        }
        // Reach High Ground/Objective (Placeholder)
        // if (m_pCurrentPath && m_pCurrentPath->getGoalWpt()) {
        //     CWaypoint* pConcJumpNode = WaypointFindNearest(pev->origin, nullptr, W_FL_CONC_JUMP_SPOT);
        //     if (pConcJumpNode && (pConcJumpNode->m_vOrigin - pev->origin).Length() < 150) {
        //         ADD_UTILITY(BOT_UTIL_FF_CONC_JUMP_MOBILITY, true, 60);
        //     }
        // }
        // Generic Mobility (if no higher priority conc jump was added)
        if (!m_pUtil->hasUtility(BOT_UTIL_FF_CONC_JUMP_MOBILITY)) {
             ADD_UTILITY(BOT_UTIL_FF_CONC_JUMP_MOBILITY, true, 25);
        }
    }
}

bool CBotFF::executeScoutAction(CBotUtility *util) {
    switch(util->m_iUtility) {
        case BOT_UTIL_FF_CONC_JUMP_MOBILITY:
            {
                 CBotWeapon* pConcGrenade = getWeapon(WEAPON_FF_GREN_CONC);
                 if (pConcGrenade && pConcGrenade->canUse()) {
                    setSchedule(new CSchedFFConcJumpSelf(this, pConcGrenade));
                    return true;
                 }
            }
            break;
        case BOT_UTIL_FF_SCOUT_USE_CALTROPS:
            {
                CBotWeapon* pCaltropsWeapon = util->getWeaponChoice();
                if (!pCaltropsWeapon) pCaltropsWeapon = getWeapon(WEAPON_FF_GREN_CALTROP);

                if (pCaltropsWeapon && pCaltropsWeapon->canUse()) { // Ensure bot still has caltrops
                    Vector vTargetLocation = util->getTargetPos();
                    edict_t* pTargetEdict = util->getTaskEdictTarget();

                    if (vTargetLocation == vec3_origin && pTargetEdict) {
                        vTargetLocation = CBotGlobals::entityOrigin(pTargetEdict);
                    } else if (vTargetLocation == vec3_origin && m_pCurrentEnemy.Get()) {
                        vTargetLocation = CBotGlobals::entityOrigin(m_pCurrentEnemy.Get());
                    } else if (vTargetLocation == vec3_origin) {
                        Vector vForward;
                        UTIL_MakeVectors(pev->v_angle, vForward, NULL, NULL);
                        vTargetLocation = pev->origin + vForward * 200;
                    }
                    setSchedule(new CSchedFFPrimeThrowGrenade(vTargetLocation, pCaltropsWeapon, 0.1f));
                    return true;
                }
            }
            break;
        default:
            return false;
    }
    return false;
}

// CTaskFFExecuteConcJump
CTaskFFExecuteConcJump::CTaskFFExecuteConcJump() {}
void CTaskFFExecuteConcJump::execute(CBot* pBot) {
    CBotFF* pFFBot = (CBotFF*)pBot;
    CBotWeapon* pConc = pFFBot->getWeapon(WEAPON_FF_GREN_CONC);
    if (pConc && pConc->canUse()) {
        Vector vAngle = pFFBot->pev->v_angle;
        vAngle.x = 90;
        pFFBot->setIdealAngle(vAngle);
        pFFBot->pev->button |= IN_ATTACK;
    }
}
bool CTaskFFExecuteConcJump::isTaskComplete(CBot* pBot) { return true; }
const char* CTaskFFExecuteConcJump::getTaskName() { return "TaskFFExecuteConcJump"; }

// CSchedFFConcJumpSelf
CSchedFFConcJumpSelf::CSchedFFConcJumpSelf(CBotFF* pBot, CBotWeapon* pConcGrenade) :
    CBotSchedule(SCHED_FF_CONC_JUMP_SELF, BOT_UTIL_FF_CONC_JUMP_MOBILITY, PRIORITY_HIGH) {
    addTask(new CTaskLookAt(Vector(pBot->pev->origin.x, pBot->pev->origin.y, pBot->pev->origin.z - 100), 0.2f));
    addTask(new CTaskFFPrimeGrenade(pBot->pev->origin + Vector(0,0,-50), 0.1f));
    addTask(new CTaskFFThrowGrenade(pBot->pev->origin + Vector(0,0,-50), pConcGrenade));
    addTask(new CTaskDelay(0.05f));
    addTask(new CTaskJump());
}
const char* CSchedFFConcJumpSelf::getScheduleName() { return "SchedFFConcJumpSelf"; }
