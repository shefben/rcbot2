// bot_ff_medic.cpp
// Medic-specific logic for CBotFF

#include "../../bot_ff.h"
#include "../../bot_globals.h"
#include "../../bot_navigation.h"
#include "../../bot_manager.h"
#include "../../bot_event.h"
#include "../../bot_util.h"      // For CBotUtility
#include "../../bot_math.h"
#include "../../bot_task.h"
#include "../../bot_schedule.h"
#include "../../bot_weapon_defs.h" // For WEAPON_FF_MEDKIT
#include "../../game_shared/ff/ff_shareddefs.h" // For FF_CLASS_MEDIC if needed by logic here

bool CBotFF::executeMedicAction(CBotUtility *util) {
    switch(util->m_iUtility) {
        case BOT_UTIL_MEDIC_HEAL:
            if (m_pPatient) {
                CBotWeapon *pMedkit = getWeapon(WEAPON_FF_MEDKIT);
                if (pMedkit && pMedkit->canUse()) {
                    setSchedule(new CSchedFFMedicHealTeammate(this, m_pPatient));
                    return true;
                }
            }
            break;
        default:
            return false;
    }
    return false;
}

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
            pFFBot->pev->button |= IN_ATTACK;
        } else {
            pFFBot->pev->button &= ~IN_ATTACK;
        }
    } else {
        pFFBot->pev->button &= ~IN_ATTACK;
    }
}
bool CTaskFFMedicAimAndHeal::isTaskComplete(CBot* pBot) {
    edict_t* pTargetEdict = m_hTarget.Get();
    if (!pTargetEdict || pTargetEdict->v.deadflag != DEAD_NO || pTargetEdict->v.health >= pTargetEdict->v.max_health) {
        pBot->pev->button &= ~IN_ATTACK;
        return true;
    }
    return false;
}
const char* CTaskFFMedicAimAndHeal::getTaskName() { return "TaskFFMedicAimAndHeal"; }

// CSchedFFMedicHealTeammate
CSchedFFMedicHealTeammate::CSchedFFMedicHealTeammate(CBotFF* pBot, edict_t* pTargetTeammate) :
    CBotSchedule(SCHED_FF_MEDIC_HEAL_TEAMMATE, BOT_UTIL_MEDIC_HEAL, PRIORITY_MED) {
    if (pTargetTeammate) {
        addTask(new CTaskMoveTo(pTargetTeammate->v.origin, BOT_UTIL_MEDIC_HEAL, 0.7f, true, 300.0f));
        addTask(new CTaskFFMedicAimAndHeal(pTargetTeammate));
    }
}
const char* CSchedFFMedicHealTeammate::getScheduleName() { return "SchedFFMedicHealTeammate"; }
