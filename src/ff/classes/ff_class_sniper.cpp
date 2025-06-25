#include "ff_class_sniper.h"
#include "bot_ff.h"

extern float g_ff_test_time;

void CFFClassSniper::OnSpawn()
{
    GiveWeapon(FF_WEAPON_SNIPERRIFLE);
    GiveWeapon(FF_WEAPON_NAILGUN);
    GiveWeapon(FF_WEAPON_CROWBAR);
    GiveAmmo(FF_GRENADE_NORMAL, 2);
    m_bCharging = false;
    m_fChargeStart = 0.0f;
    RegisterSpecial(1.0f);
}

bool CFFClassSniper::Think()
{
    if (!m_pOwner)
        return false;

    edict_t *enemy = m_pOwner->getEnemy();
    if (enemy && (m_pOwner->distanceFrom(enemy) < 800.0f))
    {
        CBotGlobals::botMessage(NULL, 0, "Sniper defending flag");
        return true;
    }

    CBotGlobals::botMessage(NULL, 0, "Sniper assisting squad objective");
    return true;
}

bool CFFClassSniper::UseSpecial()
{
    float now = g_ff_test_time;
    if (now < m_fNextSpecialUse)
        return false;

    if (!m_bCharging)
    {
        m_bCharging = true;
        m_fChargeStart = now;
        CBotGlobals::botMessage(NULL, 0, "Sniper charging");
        return false;
    }

    if ((now - m_fChargeStart) >= 1.2f)
    {
        m_bCharging = false;
        m_fNextSpecialUse = now + 2.0f;
        CBotGlobals::botMessage(NULL, 0, "Sniper headshot");
        return true;
    }
    return false;
}

EBotTask CFFClassSniper::GetIdealTask()
{
    return BOT_TASK_NONE;
}
