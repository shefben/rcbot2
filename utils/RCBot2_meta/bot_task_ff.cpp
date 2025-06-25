#include "bot_task_ff.h"
#include "bot_globals.h"
#include "ff_flag_tracker.h"


CBotTaskFFMedicHeal::CBotTaskFFMedicHeal() {}
CBotTaskFFEngineerBuild::CBotTaskFFEngineerBuild() {}

CBotTaskFFCaptureFlag::CBotTaskFFCaptureFlag(int enemyTeam)
{
    m_iEnemyTeam = enemyTeam;
}

CBotTaskFFDefendFlag::CBotTaskFFDefendFlag(int team)
{
    m_iTeam = team;
}

void CBotTaskFFCaptureFlag::execute(CBot *pBot, CBotSchedule *)
{
    const FlagState &state = g_FFFlagTracker.GetFlag(m_iEnemyTeam);

    if (state.atBase)
    {
        fail();
        return;
    }

    if (!state.carried && !state.dropped)
    {
        complete();
        return;
    }

    CBotGlobals::botMessage(NULL, 0, "Capturing flag...");
}

void CBotTaskFFDefendFlag::execute(CBot *pBot, CBotSchedule *)
{
    const FlagState &state = g_FFFlagTracker.GetFlag(m_iTeam);

    if (state.atBase)
    {
        complete();
        return;
    }

    CBotGlobals::botMessage(NULL, 0, "Defending flag...");
}
