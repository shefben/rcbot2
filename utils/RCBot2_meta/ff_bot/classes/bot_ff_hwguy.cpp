// bot_ff_hwguy.cpp
// HWGuy-specific logic for CBotFF

#include "../../bot_ff.h"
#include "../../bot_globals.h"
#include "../../bot_util.h"      // For ADD_UTILITY_WEAPON
#include "../../bot_weapon_defs.h" // For weapon_ff_assaultcannon
#include "../../game_shared/ff/ff_shareddefs.h" // For FF_CLASS_HWGUY
#include "../../bot_game.h" // For BotSupport

void CBotFF::hwguyModThink() {
    // Note: currentFFClass_modThink is established in the calling CBotFF::modThink()
    // We can assume this function is only called if the class is HWGuy.

    CBotWeapon* currentWeapon = getCurrentWeapon(); // Use member function

    if (currentWeapon &&
        currentWeapon->getWeaponInfo() &&
        strcmp(currentWeapon->getWeaponInfo()->getWeaponName(), "weapon_ff_assaultcannon") == 0 &&
        (m_pButtons->holdingButton(IN_ATTACK) || (m_pEnemy.Get() && isVisible(m_pEnemy.Get()) && wantToShoot())) )
    {
        m_fIdealMoveSpeed *= 0.4f;
    }
}

void CBotFF::getHWGuyTasks(unsigned int iIgnore) {
    // Note: currentFFClass_getTasks is established in the calling CBotFF::getTasks()
    // We can assume this function is only called if the class is HWGuy.

    CBotWeapon* pAssaultCannon = m_pWeapons->getWeaponByName("weapon_ff_assaultcannon");
    if (pAssaultCannon && pAssaultCannon->hasWeapon() && !pAssaultCannon->outOfAmmo(this)) {
        float utility = 0.7f;
        if (m_pEnemy.Get() && CBotGlobals::entityIsAlive(m_pEnemy.Get())) {
            int enemiesNearbyTarget = CBotGlobals::countPlayersNearOrigin(CBotGlobals::entityOrigin(m_pEnemy.Get()), 300.0f, BotSupport::getEnemyTeam(getTeam()), m_pEdict, true);
            if (enemiesNearbyTarget > 1) {
                utility += 0.2f;
            }
            if (hasSomeConditions(CONDITION_DEFEND_POINT | CONDITION_DEFEND_FLAG)) {
                utility += 0.15f;
            }
        } else if (hasSomeConditions(CONDITION_DEFEND_POINT | CONDITION_DEFEND_FLAG)) {
            utility += 0.10f;
        }
        ADD_UTILITY_WEAPON(BOT_UTIL_ATTACK, true, utility, pAssaultCannon);
    }
    // ... (logic for other HWGuy weapons like shotgun can follow) ...
}
