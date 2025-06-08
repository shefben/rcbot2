// bot_ff_demoman.cpp
// Demoman-specific logic for CBotFF

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
#include "../../bot_game.h" // For BotSupport
#include "../../player.h" // For CBasePlayer for iteration

void CBotFF::getDemomanTasks(unsigned int iIgnore) {
    CBotWeapon* pPipeLauncher = getWeapon(WEAPON_FF_PIPEBOMBLAUNCHER);
    if (pPipeLauncher && pPipeLauncher->canUse()) {
        // Pipe Laying Logic
        if (!m_bHasActivePipes || m_iPipesToLay > 0) {
             CWaypoint* pTrapSpot = WaypointFindNearest(pev->origin, NULL, WAYPOINT_FLAG_CHOKEPOINT); // Example flag
             if (pTrapSpot) {
                addUtility(BOT_UTIL_FF_DEMO_LAY_PIPE_TRAP, BOT_UTIL_FF_DEMO_LAY_PIPE_TRAP, 50, (void*)pTrapSpot);
             }
        }

        // Pipe Detonation Logic
        if (m_bHasActivePipes) {
            bool bEnemyNearPipes = false;
            edict_t* pTargetEnemyForDetonation = nullptr;
            if (m_vLastPipeTrapLocation != vec3_origin) {
                for (int i = 1; i <= gpGlobals->maxClients; i++) {
                    CBasePlayer *pPlayer = (CBasePlayer*)UTIL_PlayerByIndex(i);
                    if (pPlayer && pPlayer->IsPlayer() && pPlayer->IsAlive() && isEnemy(ENT(pPlayer->pev))) {
                        if ((pPlayer->pev->origin - m_vLastPipeTrapLocation).Length() < 250.0f) { // Radius for detonation
                            bEnemyNearPipes = true;
                            pTargetEnemyForDetonation = ENT(pPlayer->pev);
                            break;
                        }
                    }
                }
            }
            if (bEnemyNearPipes) {
                float distToTrapSqr = (m_vLastPipeTrapLocation - pev->origin).LengthSqr();
                if (distToTrapSqr > (200.0f * 200.0f) || m_iHealth > 75) {
                    ADD_UTILITY_TARGET(BOT_UTIL_FF_DEMO_DETONATE_PIPES, true, 80, getWeapon(WEAPON_FF_PIPEBOMBLAUNCHER), pTargetEnemyForDetonation);
                }
            }
            // Optional: Detonate old pipes (currently commented out)
            // else if (gpGlobals->time > m_fNextPipeLayTime + 10.0f && m_iPipesToLay == 0) {
            //    ADD_UTILITY(BOT_UTIL_FF_DEMO_DETONATE_PIPES, true, 20);
            // }
        }
    }
}

bool CBotFF::executeDemomanAction(CBotUtility *util) {
    switch(util->m_iUtility) {
        case BOT_UTIL_FF_DEMO_LAY_PIPE_TRAP:
            {
                CWaypoint* pTrapSpotWpt = (CWaypoint*)util->m_pVoidData;
                if (pTrapSpotWpt) {
                     setSchedule(new CSchedFFDemoLayPipeTrap(this, pTrapSpotWpt, 3));
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
        default:
            return false;
    }
    return false;
}

// CTaskFFDemoLaySinglePipe
CTaskFFDemoLaySinglePipe::CTaskFFDemoLaySinglePipe(const Vector& vTargetPos) : m_vTargetPos(vTargetPos), m_bFired(false) {}
void CTaskFFDemoLaySinglePipe::execute(CBot* pBot) {
    CBotFF* pFFBot = (CBotFF*)pBot;
    CBotWeapon* pPipeLauncher = pFFBot->getWeapon(WEAPON_FF_PIPEBOMBLAUNCHER);
    if (pPipeLauncher && pPipeLauncher->canUsePrimary() && !m_bFired) {
        pFFBot->selectWeapon(pPipeLauncher->m_iWeaponID);
        pFFBot->setIdealYaw(m_vTargetPos);
        pFFBot->pev->button |= IN_ATTACK;
        m_bFired = true;
        pFFBot->m_bHasActivePipes = true;
        // pFFBot->m_vLastPipeTrapLocation = m_vTargetPos; // Removed: Trap location set by schedule
        pFFBot->m_fNextPipeLayTime = gpGlobals->time + 0.8f;
    }
}
bool CTaskFFDemoLaySinglePipe::isTaskComplete(CBot* pBot) {
    if (m_bFired && !(pBot->pev->button & IN_ATTACK)) return true;
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
        pFFBot->pev->button |= IN_SECONDARY_ATTACK;
        pFFBot->m_bHasActivePipes = false;
    }
}
bool CTaskFFDemoDetonatePipes::isTaskComplete(CBot* pBot) {
    if (!(pBot->pev->button & IN_SECONDARY_ATTACK)) return true;
    return false;
}
const char* CTaskFFDemoDetonatePipes::getTaskName() { return "TaskFFDemoDetonatePipes"; }

// CSchedFFDemoLayPipeTrap
CSchedFFDemoLayPipeTrap::CSchedFFDemoLayPipeTrap(CBotFF* pBot, CWaypoint* pTrapSpotWpt, int numPipes) :
    CBotSchedule(SCHED_FF_DEMO_LAY_PIPE_TRAP, BOT_UTIL_FF_DEMO_LAY_PIPE_TRAP, PRIORITY_NORMAL) {
    if (pTrapSpotWpt) {
        pBot->m_vLastPipeTrapLocation = pTrapSpotWpt->m_vOrigin; // Set center of trap
        pBot->m_iPipesToLay = numPipes;
        addTask(new CTaskMoveTo(pTrapSpotWpt->m_vOrigin, BOT_UTIL_FF_DEMO_LAY_PIPE_TRAP, 0.5f));
        for (int i=0; i < numPipes; ++i) {
            Vector pipePos = pTrapSpotWpt->m_vOrigin + Vector(RANDOM_FLOAT(-30,30),RANDOM_FLOAT(-30,30), 10);
            addTask(new CTaskFFDemoLaySinglePipe(pipePos));
            if (i < numPipes -1) addTask(new CTaskDelay(0.8f));
        }
    }
}
const char* CSchedFFDemoLayPipeTrap::getScheduleName() { return "SchedFFDemoLayPipeTrap"; }

// CSchedFFDemoDetonatePipes
CSchedFFDemoDetonatePipes::CSchedFFDemoDetonatePipes(CBotFF* pBot) :
    CBotSchedule(SCHED_FF_DEMO_DETONATE_PIPES, BOT_UTIL_FF_DEMO_DETONATE_PIPES, PRIORITY_HIGH) {
    addTask(new CTaskFFDemoDetonatePipes());
}
const char* CSchedFFDemoDetonatePipes::getScheduleName() { return "SchedFFDemoDetonatePipes"; }
