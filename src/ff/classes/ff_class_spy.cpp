#include "ff_class_spy.h"
#include "bot_ff.h"
#include "engy/building_manager.h"
#include "bot_globals.h"

extern float g_ff_test_time;

void CFFClassSpy::OnSpawn()
{
    GiveWeapon(FF_WEAPON_KNIFE);
    GiveWeapon(FF_WEAPON_TRANQUILISER);
    GiveWeapon(FF_WEAPON_NAILGUN);
    GiveAmmo(AMMO_DETPACK, 2);
    GiveAmmo(FF_GRENADE_NORMAL, 2);
    m_fCloak = 100.0f;
    m_bCloaked = false;
    RegisterSpecial(1.0f);
}

bool CFFClassSpy::Think()
{
    float now = g_ff_test_time;
    if (m_bCloaked)
    {
        if (m_fCloak > 0.0f)
            m_fCloak -= 4.0f;
        if (m_fCloak <= 0.0f)
        {
            m_bCloaked = false;
            m_fNextSpecialUse = now + 20.0f;
            m_fCloak = 0.0f;
        }
    }
    else if ((now >= m_fNextSpecialUse) && (m_fCloak < 100.0f))
    {
        m_fCloak = 100.0f;
    }
    return true;
}

bool CFFClassSpy::UseSpecial()
{
    float now = g_ff_test_time;
    if (m_bCloaked)
    {
        m_bCloaked = false;
        m_fNextSpecialUse = now + 20.0f;
        m_fCloak = 0.0f;
        CBotGlobals::botMessage(NULL,0,"Spy uncloaked");
        return true;
    }
    if (now < m_fNextSpecialUse || m_fCloak <= 0.0f)
        return false;
    m_bCloaked = true;
    CBotGlobals::botMessage(NULL,0,"Spy cloaked");
    return true;
}

EBotTask CFFClassSpy::GetIdealTask()
{
    return BOT_TASK_NONE;
}
