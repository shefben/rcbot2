#ifndef FF_SPECIAL_SCHEDULER_H
#define FF_SPECIAL_SCHEDULER_H

#include <vector>
#include "ff/classes/ff_class_base.h"

class CSpecialAbilityScheduler
{
public:
    static CSpecialAbilityScheduler &Get();

    void Register(CFFPlayerClass *cls, float cooldown);
    void PauseSpecials(CFFPlayerClass *cls, float seconds);
    void BotFrame(float now);

private:
    struct Entry
    {
        CFFPlayerClass *cls;
        float cooldown;
        float next;
        float pausedUntil;
    };

    std::vector<Entry> m_entries;
};

#endif // FF_SPECIAL_SCHEDULER_H
