#include "engine_wrappers.h"
#include "bot.h"
#include "bot_weapons.h"
#include "bot_getprop.h"
#include "locomotion_ff.h"

bool FFScoutConcJump(CBot &bot)
{
    // ensure bot is on ground
    if (!(CClassInterface::getFlags(bot.getEdict()) & FL_ONGROUND))
        return false;

    CBotWeapon *weapon = bot.getCurrentWeapon();
    if (!weapon || !weapon->getWeaponInfo() || !weapon->getWeaponInfo()->isGrenade())
    {
        // must be holding concussion grenade
        return false;
    }

    bot.tapButton(IN_ATTACK);
    bot.jump();
    return true;
}
