#include "bot.h"
#include "bot_ff.h"
#include "bot_weapons.h"
#include "bot_getprop.h"

// Parse Fortress Forever player loadout information
void ParseFFClassLoadout(CBot &bot)
{
    // get current weapon index via game interface if available
    int weapon = CClassInterface::getCurrentWeapon(bot.getEdict());
    const char *name = FFWeaponName(weapon);

    if (CWeapon *wpn = CWeapons::getWeaponByShortName(name))
        bot.getWeapons()->addWeapon(wpn);

    // ensure bot team is valid for 4-team play
    int team = CClassInterface::getTeam(bot.getEdict());
    if (team < FF_TEAM_BLUE || team > FF_TEAM_GREEN)
        bot.changeTeam(randomInt(FF_TEAM_BLUE, FF_TEAM_GREEN));
}
