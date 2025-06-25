#include "special_scheduler.h"
#include "bot_globals.h"

CSpecialAbilityScheduler &CSpecialAbilityScheduler::Get()
{
    static CSpecialAbilityScheduler g_Scheduler;
    return g_Scheduler;
}

void CSpecialAbilityScheduler::Register(CFFPlayerClass *cls, float cooldown)
{
    for (auto &e : m_entries)
    {
        if (e.cls == cls)
        {
            e.cooldown = cooldown;
            return;
        }
    }
    Entry e{cls, cooldown, 0.0f, 0.0f};
    m_entries.push_back(e);
}

void CSpecialAbilityScheduler::PauseSpecials(CFFPlayerClass *cls, float seconds)
{
    for (auto &e : m_entries)
    {
        if (e.cls == cls)
        {
            e.pausedUntil = gpGlobals->time + seconds;
            break;
        }
    }
}

void CSpecialAbilityScheduler::BotFrame(float now)
{
    for (auto &e : m_entries)
    {
        if (now < e.pausedUntil)
            continue;
        if (now < e.next)
            continue;
        if (e.cls->GetIdealTask() == BOT_TASK_ATTACK)
        {
            if (e.cls->UseSpecial())
                e.next = now + e.cooldown;
        }
    }
}
