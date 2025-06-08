// bot_ff_sniper.cpp
// Sniper-specific logic for CBotFF

#include "../../bot_ff.h"
#include "../../bot_globals.h"
#include "../../bot_navigation.h"
#include "../../bot_manager.h"
#include "../../bot_event.h"
#include "../../bot_util.h"      // For CBotUtility
#include "../../bot_math.h"
#include "../../bot_task.h"
#include "../../bot_schedule.h"
#include "../../bot_weapon_defs.h" // For WEAPON_FF_SNIPERRIFLE etc.
#include "../../game_shared/ff/ff_shareddefs.h" // For FF_CLASS_SNIPER if needed

bool CBotFF::executeSniperAction(CBotUtility *util) {
    switch(util->m_iUtility) {
        case BOT_UTIL_SNIPE:
            if (m_pCurrentEnemy) {
                 CBotWeapon* pSniperRifle = getWeapon(WEAPON_FF_SNIPERRIFLE);
                 if (!pSniperRifle) pSniperRifle = getWeapon(WEAPON_FF_RAILGUN);

                 if (pSniperRifle && pSniperRifle->canUse()) {
                    setSchedule(new CSchedFFSnipe(this, m_pCurrentEnemy));
                    return true;
                 }
            }
            break;
        default:
            return false;
    }
    return false;
}

// CTaskFFSnipeAttackSequence
CTaskFFSnipeAttackSequence::CTaskFFSnipeAttackSequence(edict_t* pTarget) : m_hTarget(pTarget), m_iState(0), m_fNextActionTime(0.0f) {}
void CTaskFFSnipeAttackSequence::init(CBot* pBot) {
    CBotFF* pFFBot = (CBotFF*)pBot;
    CBotWeapon* pSniperWep = pFFBot->getWeapon(WEAPON_FF_SNIPERRIFLE);
    if (!pSniperWep) pSniperWep = pFFBot->getWeapon(WEAPON_FF_RAILGUN);

    if (pSniperWep) {
        pFFBot->selectWeapon(pSniperWep->m_iWeaponID);
        m_iState = 0;
        m_fNextActionTime = gpGlobals->time;
    } else {
        m_iState = 2; // No weapon, task is basically complete (fail)
    }
}
void CTaskFFSnipeAttackSequence::execute(CBot* pBot) {
    CBotFF* pFFBot = (CBotFF*)pBot;
    edict_t* pTargetEdict = m_hTarget.Get();

    if (!pTargetEdict || pTargetEdict->v.deadflag != DEAD_NO) {
        m_iState = 2;
        pFFBot->pev->button &= ~IN_ATTACK2;
        pFFBot->pev->button &= ~IN_ATTACK;
        return;
    }

    pFFBot->setIdealYaw(pTargetEdict->v.origin);

    if (gpGlobals->time < m_fNextActionTime) return;

    switch (m_iState) {
        case 0:
            pFFBot->pev->button |= IN_ATTACK2;
            m_fNextActionTime = gpGlobals->time + 0.5f;
            m_iState = 1;
            break;
        case 1:
            pFFBot->pev->button &= ~IN_ATTACK2;
            if (pFFBot->isZoomed()) {
                 pFFBot->pev->button |= IN_ATTACK;
                 m_fNextActionTime = gpGlobals->time + 1.0f;
                 m_iState = 2;
            } else {
                 m_fNextActionTime = gpGlobals->time + 0.2f;
                 m_iState = 0;
            }
            break;
        case 2:
            pFFBot->pev->button &= ~IN_ATTACK;
            break;
    }
}
bool CTaskFFSnipeAttackSequence::isTaskComplete(CBot* pBot) {
    if (m_iState == 2 && gpGlobals->time >= m_fNextActionTime) {
         ((CBotFF*)pBot)->pev->button &= ~IN_ATTACK;
        return true;
    }
    return false;
}
const char* CTaskFFSnipeAttackSequence::getTaskName() { return "TaskFFSnipeAttackSequence"; }

// CSchedFFSnipe
CSchedFFSnipe::CSchedFFSnipe(CBotFF* pBot, edict_t* pTarget) :
    CBotSchedule(SCHED_FF_SNIPE_ATTACK, BOT_UTIL_SNIPE, PRIORITY_HIGH) {
    if (pTarget) {
        addTask(new CTaskFaceEntity(pTarget));
        addTask(new CTaskFFSnipeAttackSequence(pTarget));
    }
}
const char* CSchedFFSnipe::getScheduleName() { return "SchedFFSnipe"; }
