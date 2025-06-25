#include "engine_wrappers.h"
#include "bot_weapons.h"
#include "bot_ff.h"
#include "bot_globals.h"

WeaponsData_t FFWeaps[] = {
    // slot, id, name, flags, minDist, maxDist, ammo index, preference, projectile speed
    {1, FF_WEAPON_CROWBAR,        "ff_weapon_crowbar",        WEAP_FL_MELEE|WEAP_FL_PRIM_ATTACK,0,64,-1,1,0},
    {1, FF_WEAPON_SHOTGUN,        "ff_weapon_shotgun",        WEAP_FL_PRIM_ATTACK,0,768,-1,2,0},
    {1, FF_WEAPON_SUPERSHOTGUN,   "ff_weapon_supershotgun",   WEAP_FL_PRIM_ATTACK,0,768,-1,2,0},
    {1, FF_WEAPON_NAILGUN,        "ff_weapon_nailgun",        WEAP_FL_PRIM_ATTACK|WEAP_FL_PROJECTILE,0,1024,-1,2,1200},
    {1, FF_WEAPON_SUPERNAILGUN,   "ff_weapon_supernailgun",   WEAP_FL_PRIM_ATTACK|WEAP_FL_PROJECTILE,0,1024,-1,2,1200},
    {1, FF_WEAPON_GRENADELAUNCHER,"ff_weapon_grenadelauncher",WEAP_FL_PRIM_ATTACK|WEAP_FL_EXPLOSIVE|WEAP_FL_PROJECTILE,0,1500,-1,2,1200},
    {1, FF_WEAPON_PIPELAUNCHER,   "ff_weapon_pipelauncher",   WEAP_FL_PRIM_ATTACK|WEAP_FL_EXPLOSIVE|WEAP_FL_PROJECTILE,0,1500,-1,2,900},
    {1, FF_WEAPON_FLAMETHROWER,   "ff_weapon_flamethrower",   WEAP_FL_PRIM_ATTACK,0,400,-1,2,0},
    {1, FF_WEAPON_RPG,            "ff_weapon_rpg",            WEAP_FL_PRIM_ATTACK|WEAP_FL_EXPLOSIVE|WEAP_FL_PROJECTILE,0,2000,-1,2,1100},
    {0,0,"\0",0,0,0,0,0,0}
};

void RegisterFFWeapons()
{
    CWeapons::loadWeapons("FF", FFWeaps);
}

int FireFlamethrower(CBot *bot)
{
    if (!bot)
        return 0;

    bot->pressButton(IN_ATTACK);
    bot->setLookAt(bot->getEnemy() ? CBotGlobals::entityOrigin(bot->getEnemy()) : bot->getEyePosition());

    return 20; // fire for a short burst
}

int LaunchDetpack(CBot *bot)
{
    if (!bot)
        return 0;

    static float s_fFuseEnd = 0.0f;

    if (s_fFuseEnd <= gpGlobals->time)
    {
        bot->pressButton(IN_ATTACK);
        s_fFuseEnd = gpGlobals->time + 5.0f; // detonate in 5 seconds
        return 50;
    }

    if (gpGlobals->time >= s_fFuseEnd)
    {
        bot->pressButton(IN_ATTACK2); // detonate
        s_fFuseEnd = 0.0f;
    }

    return 5;
}

int NailgunBurst(CBot *bot)
{
    if (!bot)
        return 0;

    bot->pressButton(IN_ATTACK);
    return 10;
}
