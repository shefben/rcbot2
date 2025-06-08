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

void CBotFF::getScoutTasks(unsigned int iIgnore) {
    // Note: currentFFClass_getTasks is already defined in the calling CBotFF::getTasks()
    // We can assume this function is only called if the class is Scout.

    CBotWeapon* pCaltrops = getWeapon(WEAPON_FF_GREN_CALTROP);
    if (pCaltrops && pCaltrops->canUse()) {
        if (m_pCurrentEnemy && (m_pCurrentEnemy->v.origin - pev->origin).Length() < 300) {
            addUtility(BOT_UTIL_FF_SCOUT_USE_CALTROPS, BOT_UTIL_FF_SCOUT_USE_CALTROPS, 40);
        }
    }

    CBotWeapon* pConcGrenade = getWeapon(WEAPON_FF_GREN_CONC);
    if (pConcGrenade && pConcGrenade->canUse()) {
         addUtility(BOT_UTIL_FF_CONC_JUMP_MOBILITY, BOT_UTIL_FF_CONC_JUMP_MOBILITY, 30);
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
            if (m_pCurrentEnemy) { // Check m_pCurrentEnemy directly as it's a member
                 CBotWeapon* pCaltrops = getWeapon(WEAPON_FF_GREN_CALTROP);
                 if (pCaltrops) { // pCaltrops should be valid if this utility was chosen
                    setSchedule(new CSchedFFPrimeThrowGrenade(m_pCurrentEnemy->v.origin, pCaltrops, 0.1f));
                    return true;
                 }
            }
            break;
        default:
            return false; // Not a scout-specific action handled here
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
    // Assuming pConcGrenade is the WEAPON_FF_GREN_CONC passed in
    addTask(new CTaskFFPrimeGrenade(pBot->pev->origin + Vector(0,0,-50), 0.1f));
    addTask(new CTaskFFThrowGrenade(pBot->pev->origin + Vector(0,0,-50), pConcGrenade));
    addTask(new CTaskDelay(0.05f));
    addTask(new CTaskJump());
}
const char* CSchedFFConcJumpSelf::getScheduleName() { return "SchedFFConcJumpSelf"; }
