#include "engine_wrappers.h"
#include "bot_ff.h"
#include "bot_globals.h"
#include "bot_weapons.h"
#include "bot_getprop.h"
#include "bot_client.h"
#include <algorithm>
#include <string>

CBotFortressForever::CBotFortressForever() : CBotFortress()
{
    m_iDesiredClass = -1;
}

bool CBotFortressForever::startGame()
{
    int team = m_pPlayerInfo->GetTeamIndex();
    m_iClass = (TF_Class)CClassInterface::getTF2Class(m_pEdict);

    if ((team < FF_TEAM_BLUE) || (team > FF_TEAM_GREEN))
    {
        helpers->ClientCommand(m_pEdict, "joingame");
        selectTeam();
        chooseClass();
        return false;
    }

    if ((m_iDesiredClass < FF_CLASS_SCOUT) || (m_iDesiredClass > FF_CLASS_CIVILIAN) || (m_iClass != m_iDesiredClass))
    {
        chooseClass();
        return false;
    }

    return true;
}

void CBotFortressForever::chooseClass()
{
    m_iDesiredClass = randomInt(FF_CLASS_SCOUT, FF_CLASS_CIVILIAN);

    char cmd[32];
    snprintf(cmd, sizeof(cmd), "joinclass %d", m_iDesiredClass);
    helpers->ClientCommand(m_pEdict, cmd);
}

void CBotFortressForever::selectTeam()
{
    int team = randomInt(FF_TEAM_BLUE, FF_TEAM_GREEN);

    char cmd[32];
    snprintf(cmd, sizeof(cmd), "jointeam %d", team);
    helpers->ClientCommand(m_pEdict, cmd);
}

const char *FFWeaponName(int id)
{
    static const char *names[] = {
        "none",
        "crowbar",
        "knife",
        "medkit",
        "spanner",
        "umbrella",
        "shotgun",
        "supershotgun",
        "nailgun",
        "supernailgun",
        "grenadelauncher",
        "pipelauncher",
        "autorifle",
        "sniperrifle",
        "flamethrower",
        "incendiarycannon",
        "railgun",
        "jumpgun",
        "tranq",
        "assaultcannon",
        "rpg",
        "tommygun",
        "cubemap",
        "deploydispenser",
        "deploysentrygun",
        "deploydetpack",
        "deploymancannon",
        "deployjumppad",
        "flag"
    };

    if (id < 0 || id >= (int)(sizeof(names)/sizeof(names[0])))
        return "unknown";

    return names[id];
}

FFVoiceCmd ProcessFFVoiceCommand(const char *text)
{
    if (!text)
        return FF_VC_MAX;

    std::string lower = text;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "incoming!")
        return FF_VC_INCOMING;
    if (lower == "need dispenser")
        return FF_VC_NEED_DISPENSER;

    return FF_VC_MAX;
}

void CBotFortressForever::hearVoiceCommand(edict_t *pPlayer, byte cmd)
{
    const char *who = CClients::get(pPlayer)->getName();
    const char *vc = (cmd==FF_VC_INCOMING)?"Incoming!":(cmd==FF_VC_NEED_DISPENSER?"Need Dispenser":"unknown");
    CBotGlobals::botMessage(m_pEdict,0,"heard %s from %s",vc,who);
}
