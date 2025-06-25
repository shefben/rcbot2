#ifndef __FF_FLAG_TRACKER_H__
#define __FF_FLAG_TRACKER_H__
#include "bot_ff.h"
struct FlagState
{
    bool atBase;
    bool carried;
    bool dropped;
    int carrier;
    FlagState() : atBase(true), carried(false), dropped(false), carrier(0) {}
};
class CFFFlagTracker
{
public:
    CFFFlagTracker();
    void Reset();
    void FlagPickedUp(int team, int userid);
    void FlagReturned(int team);
    void FlagCaptured(int team);
    const FlagState &GetFlag(int team) const;
private:
    FlagState m_flags[6];
};
extern CFFFlagTracker g_FFFlagTracker;
#endif
