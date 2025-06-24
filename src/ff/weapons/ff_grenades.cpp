#include "bot_ff.h"
#include "bot_globals.h"

enum EGrenadeType
{
    GRENADE_FRAG,
    GRENADE_CONC,
    GRENADE_EMP,
    GRENADE_INCID
};

static void onFragExplode(CBot *bot)
{
    CBotGlobals::botMessage(NULL,0,"FRAG explode");
}

static void onConcExplode(CBot *bot)
{
    CBotGlobals::botMessage(NULL,0,"CONC explode");
}

static void onEmpExplode(CBot *bot)
{
    CBotGlobals::botMessage(NULL,0,"EMP explode");
}

static void onIncidExplode(CBot *bot)
{
    CBotGlobals::botMessage(NULL,0,"INCENDIARY explode");
}

int FireGrenade(CBot *bot, EGrenadeType type)
{
    switch(type)
    {
    case GRENADE_FRAG:
        onFragExplode(bot);
        break;
    case GRENADE_CONC:
        onConcExplode(bot);
        break;
    case GRENADE_EMP:
        onEmpExplode(bot);
        break;
    case GRENADE_INCID:
        onIncidExplode(bot);
        break;
    }
    return 0;
}
