#include "engine_wrappers.h"
#include "bot_mods.h"
#include "bot_events.h"
#include "bot_weapons.h"
#include "bot_ff.h"
#include "special_scheduler.h"
#include "ff_flag_tracker.h"
#include "bot.h"
#include <vector>
#include <string>

// Fortress Forever mod skeleton implementing basic event hooks

const char *CFortressForeverMod::ModID()
{
    return "ff";
}

FFMapMode CFortressForeverMod::m_Mode = FF_MODE_CTF;

void CFortressForeverMod::initMod()
{
    CBotMod::initMod();

    RegisterFFWeapons();

    // register essential events for Fortress Forever gameplay
    CBotEvents::addEvent(new CPlayerDeathEvent());
    CBotEvents::addEvent(new CFFFlagPickupEvent());
    CBotEvents::addEvent(new CFFFlagReturnEvent());
    CBotEvents::addEvent(new CFFFlagCaptureEvent());
    g_FFFlagTracker.Reset();
}

void CFortressForeverMod::mapInit()
{
    CBotMod::mapInit();
    string_t map = gpGlobals->mapname;
    m_Mode = DetectFFMapMode(map.ToCStr(), NULL);
}

void CFortressForeverMod::modFrame()
{
    CSpecialAbilityScheduler::Get().BotFrame(gpGlobals->time);
    AssignSquadsFF();
    switch(m_Mode)
    {
        case FF_MODE_VIP:
            ModeAI_VIP();
            break;
        case FF_MODE_AD:
            ModeAI_AD();
            break;
        case FF_MODE_TC:
            ModeAI_TC();
            break;
        case FF_MODE_INVADE:
            ModeAI_Invade();
            break;
        default:
            ModeAI_CTF();
            break;
    }
}

void CFortressForeverMod::ModeAI_CTF()
{
    for(int i=0;i<CBots::numBots();++i)
    {
        CBot *bot = CBots::get(i);
        if(!bot || !bot->inUse())
            continue;

        if(bot->getCurrentSchedule())
            continue;

        int team = CClassInterface::getTeam(bot->getEdict());
        if(team < FF_TEAM_BLUE || team > FF_TEAM_GREEN)
            continue;

        int enemy = (team==FF_TEAM_BLUE)?FF_TEAM_RED:FF_TEAM_BLUE;
        const FlagState &flag = g_FFFlagTracker.GetFlag(enemy);

        if(flag.atBase)
            bot->getSchedules()->addSchedule(new CBotFFCaptureFlagSched(bot->getOrigin(), enemy));
        else
            bot->getSchedules()->addSchedule(new CBotFFDefendFlagSched(bot->getCurrentWaypoint(), team));
    }
}

void CFortressForeverMod::ModeAI_VIP()
{
    CBotGlobals::botMessage(NULL,0,"Running VIP mode AI");
}

void CFortressForeverMod::ModeAI_AD()
{
    CBotGlobals::botMessage(NULL,0,"Running Attack & Defend AI");
}

void CFortressForeverMod::ModeAI_TC()
{
    CBotGlobals::botMessage(NULL,0,"Running Territorial Control AI");
}

void CFortressForeverMod::ModeAI_Invade()
{
    CBotGlobals::botMessage(NULL,0,"Running Invade mode AI");
}

namespace {
static float g_NextSquadTime = 0;
}

void CFortressForeverMod::AssignSquadsFF()
{
    if (gpGlobals->time < g_NextSquadTime)
        return;

    g_NextSquadTime = gpGlobals->time + 30.0f;

    std::vector<std::string> offense;
    std::vector<std::string> defense;

    for (int i=0;i<CBots::numBots();i++)
    {
        CBot *bot = CBots::get(i);
        if (!bot || !bot->inUse())
            continue;

        const char *name = bot->getPlayerInfo()->GetName();

        if ((int)offense.size() < 3)
            offense.push_back(name);
        else
            defense.push_back(name);
    }

    std::string olist;
    for(size_t i=0;i<offense.size();i++)
    {
        if (i) olist += ",";
        olist += offense[i];
    }

    std::string dlist;
    for(size_t i=0;i<defense.size();i++)
    {
        if (i) dlist += ",";
        dlist += defense[i];
    }

    CBotGlobals::botMessage(NULL,0,"Offense squad: %s",olist.c_str());
    CBotGlobals::botMessage(NULL,0,"Defense squad: %s",dlist.c_str());
}

