#ifndef __RCBOT_FF_H__
#define __RCBOT_FF_H__

#include "bot_fortress.h"

// Fortress Forever team identifiers
#define FF_TEAM_UNASSIGNED 0
#define FF_TEAM_SPEC      1
#define FF_TEAM_BLUE      2
#define FF_TEAM_RED       3
#define FF_TEAM_YELLOW    4
#define FF_TEAM_GREEN     5

enum FFClass
{
    FF_CLASS_UNASSIGNED = 0,
    FF_CLASS_SCOUT = 1,
    FF_CLASS_SNIPER,
    FF_CLASS_SOLDIER,
    FF_CLASS_DEMOMAN,
    FF_CLASS_MEDIC,
    FF_CLASS_HWGUY,
    FF_CLASS_PYRO,
    FF_CLASS_SPY,
    FF_CLASS_ENGINEER,
    FF_CLASS_CIVILIAN,
    FF_CLASS_MAX
};

enum FFWeaponID
{
    FF_WEAPON_NONE = 0,
    FF_WEAPON_CROWBAR,
    FF_WEAPON_KNIFE,
    FF_WEAPON_MEDKIT,
    FF_WEAPON_SPANNER,
    FF_WEAPON_UMBRELLA,
    FF_WEAPON_SHOTGUN,
    FF_WEAPON_SUPERSHOTGUN,
    FF_WEAPON_NAILGUN,
    FF_WEAPON_SUPERNAILGUN,
    FF_WEAPON_GRENADELAUNCHER,
    FF_WEAPON_PIPELAUNCHER,
    FF_WEAPON_AUTORIFLE,
    FF_WEAPON_SNIPERRIFLE,
    FF_WEAPON_FLAMETHROWER,
    FF_WEAPON_IC,
    FF_WEAPON_RAILGUN,
    FF_WEAPON_JUMPGUN,
    FF_WEAPON_TRANQUILISER,
    FF_WEAPON_ASSAULTCANNON,
    FF_WEAPON_RPG,
    FF_WEAPON_TOMMYGUN,
    FF_WEAPON_CUBEMAP,
    FF_WEAPON_DEPLOYDISPENSER,
    FF_WEAPON_DEPLOYSENTRYGUN,
    FF_WEAPON_DEPLOYDETPACK,
    FF_WEAPON_DEPLOYMANCANNON,
    FF_WEAPON_DEPLOYJUMPPAD,
    FF_WEAPON_FLAG,
    FF_WEAPON_MAX
};
// Grenade type identifiers
enum FFGrenadeType
{
    FF_GRENADE_NORMAL = 1,
    FF_GRENADE_CONCUSS,
    FF_GRENADE_FLASH,
    FF_GRENADE_FLARE,
    FF_GRENADE_NAIL,
    FF_GRENADE_CLUSTER,
    FF_GRENADE_CLUSTERSECTION,
    FF_GRENADE_NAPALM,
    FF_GRENADE_GAS,
    FF_GRENADE_EMP,
    FF_GRENADE_MAX
};

// Buildable object identifiers
enum FFBuildable
{
    FF_BUILD_NONE = 0,
    FF_BUILD_DISPENSER,
    FF_BUILD_SENTRYGUN,
    FF_BUILD_DETPACK,
    FF_BUILD_MANCANNON
};

// Objective goal identifiers
enum FFGoalType
{
    FF_GOAL_SENTRYPOINT = 0,
    FF_GOAL_DISPENSERPOINT,
    FF_GOAL_PIPETRAPPOINT,
    FF_GOAL_DETPACKPOINT,
    FF_GOAL_RJ,
    FF_GOAL_CJ,
    FF_GOAL_HUNTEDESCAPE
};

enum FFMapMode
{
    FF_MODE_CTF = 0,
    FF_MODE_VIP,
    FF_MODE_AD,
    FF_MODE_TC,
    FF_MODE_INVADE
};

enum FFVoiceCmd
{
    FF_VC_INCOMING = 0,
    FF_VC_NEED_DISPENSER,
    FF_VC_MAX
};

class CBotFortressForever : public CBotFortress
{
public:
    CBotFortressForever();

    bool startGame() override;
    void chooseClass();
    void selectTeam();

    bool isTF() { return true; }

    void hearVoiceCommand(edict_t *pPlayer, byte cmd) override;
};

const char *FFWeaponName(int id);
struct lua_State;
FFMapMode DetectFFMapMode(const std::string &mapname, lua_State *L);
FFVoiceCmd ProcessFFVoiceCommand(const char *text);

#endif // __RCBOT_FF_H__
