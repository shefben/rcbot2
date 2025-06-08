// bot_ff_spy.cpp
// Spy-specific logic for CBotFF

#include "../../bot_ff.h"
#include "../../bot_globals.h"
// #include "../../bot_navigation.h" // Not immediately needed
// #include "../../bot_manager.h"   // Not immediately needed
// #include "../../bot_event.h"     // Not immediately needed
#include "../../bot_util.h"      // For ADD_UTILITY_WEAPON
// #include "../../bot_math.h"
#include "../../bot_task.h"      // For CBotAttackSched if used by setAttackTarget
#include "../../bot_schedule.h"
#include "../../bot_weapon_defs.h" // For WEAPON_FF_TRANQGUN
#include "../../game_shared/ff/ff_shareddefs.h" // For FF_CLASS_SPY

void CBotFF::getSpyTasks(unsigned int iIgnore) {
    // Note: currentFFClass_getTasks is established in the calling CBotFF::getTasks()
    // We can assume this function is only called if the class is Spy.

    CBotWeapon* pTranqGun = getWeapon(WEAPON_FF_TRANQGUN);
    if (pTranqGun && pTranqGun->canUse() && m_pCurrentEnemy && FVisible(m_pCurrentEnemy,edict())) {
        if ((m_pCurrentEnemy->v.origin - pev->origin).Length() < pTranqGun->getMaxDistance()) {
             addUtility(BOT_UTIL_FF_SPY_USE_TRANQ, BOT_UTIL_FF_SPY_USE_TRANQ, 65);
        }
    }
    // Future spy tasks (disguise, sap) would go here
}

bool CBotFF::executeSpyAction(CBotUtility *util) {
    switch(util->m_iUtility) {
        case BOT_UTIL_FF_SPY_USE_TRANQ:
            if (m_pCurrentEnemy) {
                setAttackTarget(m_pCurrentEnemy, BOT_UTIL_FF_SPY_USE_TRANQ);
                return true;
            }
            break;
        // Future spy actions (disguise, sap) would go here
        default:
            return false;
    }
    return false;
}
