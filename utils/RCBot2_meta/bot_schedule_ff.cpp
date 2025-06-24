#include "bot_schedule.h"
#include "bot_task_ff.h"
#include "ff_flag_tracker.h"

CBotFFCaptureFlagSched::CBotFFCaptureFlagSched(Vector vOrigin, int enemyTeam)
{
    addTask(new CFindPathTask(vOrigin));
    addTask(new CBotTaskFFCaptureFlag(enemyTeam));
}

void CBotFFCaptureFlagSched::init()
{
    setID(SCHED_FF_CAPTURE_FLAG);
}

CBotFFDefendFlagSched::CBotFFDefendFlagSched(int wpt, int team)
{
    addTask(new CFindPathTask(wpt));
    addTask(new CBotTaskFFDefendFlag(team));
}

void CBotFFDefendFlagSched::init()
{
    setID(SCHED_FF_DEFEND_FLAG);
}
