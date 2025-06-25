#include "ff_flag_tracker.h"
#include "engine_wrappers.h"

CFFFlagTracker g_FFFlagTracker;

CFFFlagTracker::CFFFlagTracker()
{
    Reset();
}

void CFFFlagTracker::Reset()
{
    for(int i=0;i<6;i++)
        m_flags[i] = FlagState();
}

void CFFFlagTracker::FlagPickedUp(int team, int userid)
{
    if(team<0||team>=6) return;
    m_flags[team].atBase = false;
    m_flags[team].carried = true;
    m_flags[team].dropped = false;
    m_flags[team].carrier = userid;
}

void CFFFlagTracker::FlagReturned(int team)
{
    if(team<0||team>=6) return;
    m_flags[team] = FlagState();
}

void CFFFlagTracker::FlagCaptured(int team)
{
    FlagReturned(team);
}

const FlagState &CFFFlagTracker::GetFlag(int team) const
{
    static FlagState dummy;
    if(team<0||team>=6) return dummy;
    return m_flags[team];
}
