// bot_ff_engineer.cpp
// Engineer-specific logic for CBotFF

#include "../../bot_ff.h"
#include "../../bot_globals.h"
#include "../../bot_navigation.h"
#include "../../bot_manager.h"
#include "../../bot_event.h"
#include "../../bot_util.h"
#include "../../bot_math.h"
#include "../../bot_task.h"
#include "../../bot_schedule.h"
#include "../../bot_weapon_defs.h"
#include "../../bot_waypoints.h" // For CWaypoints::getWaypointIndex, CWaypointTypes, CWaypoint
#include "../../game_shared/ff/ff_shareddefs.h" // For FF_CLASS_ENGINEER etc.
#include "../../bot_game.h" // For BotSupport

void CBotFF::getEngineerTasks(unsigned int iIgnore) {
    // EMP Grenade Logic for Engineer
    if (!m_bIsPrimingGrenade) {
        CBotWeapon* pEMPGrenade = m_pWeapons->getWeaponByName("weapon_ff_gren_emp");
        if (pEMPGrenade && pEMPGrenade->hasWeapon() && !pEMPGrenade->outOfAmmo(this)) {
            edict_t* pCurrentEnemy = m_pEnemy.Get();
            if (pCurrentEnemy && CBotGlobals::entityIsAlive(pCurrentEnemy) && isVisible(pCurrentEnemy) &&
                distanceFrom(pCurrentEnemy) < 1000.0f && distanceFrom(pCurrentEnemy) > 200.0f) {
                float utilityScore = 0.65f;
                int enemiesNearbyTarget = CBotGlobals::countPlayersNearOrigin(CBotGlobals::entityOrigin(pCurrentEnemy), 250.0f, BotSupport::getEnemyTeam(getTeam()), m_pEdict, true);
                if (enemiesNearbyTarget > 1) {
                    utilityScore += 0.2f;
                }
                ADD_UTILITY_WEAPON_TARGET(BOT_UTIL_FF_USE_GRENADE_EMP, true, utilityScore, pEMPGrenade, pCurrentEnemy);
            }
        }
    }
    // Refined Engineer Mancannon logic
    if (engine->Time() >= m_fNextMancannonBuildTime && !m_hBuiltMancannon.Get()) {
        CWaypoint* pBuildSpot = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_FF_MANCANNON_SPOT, getTeam(), 0, true, this);
        if (pBuildSpot) {
            ADD_UTILITY_DATA(BOT_UTIL_FF_ENGRI_BUILD_MANCANNON, true, 0.7f, CWaypoints::getWaypointIndex(pBuildSpot));
        }
    }
}

bool CBotFF::executeEngineerAction(CBotUtility *util) {
    switch(util->m_iUtility) {
        case BOT_UTIL_FF_ENGRI_BUILD_MANCANNON:
            {
                int waypointIndex = util->getIntData();
                CWaypoint* pBuildSpot = CWaypoints::getWaypoint(waypointIndex);
                if (pBuildSpot) {
                    setSchedule(new CSchedFFBuildMancannon(this, pBuildSpot));
                    return true;
                }
            }
            break;
        // Note: BOT_UTIL_FF_USE_GRENADE_EMP is handled by the generic grenade throwing logic in the main executeAction,
        // as it uses CSchedFFPrimeThrowGrenade. If Engineer had a *unique* way to execute the EMP throw,
        // that case would be moved here.
        default:
            return false;
    }
    return false;
}

// CTaskFFEngineerBuild
CTaskFFEngineerBuild::CTaskFFEngineerBuild(int buildableId, const Vector& buildPos) :
    m_buildableId(buildableId), m_vBuildPos(buildPos), m_bCommandSent(false) {}
void CTaskFFEngineerBuild::execute(CBot* pBot) {
    CBotFF* pFFBot = (CBotFF*)pBot;
    if (!m_bCommandSent) {
        CBotWeapon* pSpanner = pFFBot->getWeapon(WEAPON_FF_SPANNER_ENGI);
        if (pSpanner) {
            pFFBot->selectWeapon(pSpanner->m_iWeaponID);
            pFFBot->setIdealYaw(m_vBuildPos);
            char cmd[64];
            // FF_ENGIBUILD_MANCANNON is a placeholder ID from bot_ff.h, ensure it's correct
            sprintf(cmd, "build %d", m_buildableId == FF_ENGIBUILD_MANCANNON ? FF_ENGIBUILD_MANCANNON : m_buildableId );
            BotSendCommand(pBot->edict(), cmd);
            m_bCommandSent = true;
            pFFBot->m_fNextMancannonBuildTime = gpGlobals->time + 30.0f;
        }
    }
}
bool CTaskFFEngineerBuild::isTaskComplete(CBot* pBot) {
    return m_bCommandSent;
}
const char* CTaskFFEngineerBuild::getTaskName() { return "TaskFFEngineerBuild"; }

// CSchedFFBuildMancannon
CSchedFFBuildMancannon::CSchedFFBuildMancannon(CBotFF* pBot, CWaypoint* pBuildSpot) :
    CBotSchedule(SCHED_FF_ENGRI_BUILD_MANCANNON, BOT_UTIL_FF_ENGRI_BUILD_MANCANNON, PRIORITY_NORMAL) {
    if (pBuildSpot) {
        addTask(new CTaskMoveTo(pBuildSpot->m_vOrigin, BOT_UTIL_FF_ENGRI_BUILD_MANCANNON, 0.5f));
        addTask(new CTaskLookAt(pBuildSpot->m_vOrigin + Vector(0,0,50),0.3f));
        // FF_ENGIBUILD_MANCANNON is placeholder, should be defined in bot_const or game_ff
        addTask(new CTaskFFEngineerBuild(FF_ENGIBUILD_MANCANNON, pBuildSpot->m_vOrigin));
    }
}
const char* CSchedFFBuildMancannon::getScheduleName() { return "SchedFFBuildMancannon"; }
