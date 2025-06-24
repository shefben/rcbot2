#include "ff_class_hwguy.h"
#include "bot_ff.h"

extern float g_ff_test_time;

void CFFClassHWGuy::OnSpawn()
{
    GiveWeapon(FF_WEAPON_ASSAULTCANNON);
    GiveWeapon(FF_WEAPON_SHOTGUN);
    GiveWeapon(FF_WEAPON_CROWBAR);
    GiveAmmo(FF_GRENADE_NORMAL, 2);
    m_bSpinning = false;
    RegisterSpecial(1.0f);
}

bool CFFClassHWGuy::Think()
{
    if (!m_pOwner)
        return false;

    edict_t *enemy = m_pOwner->getEnemy();
    if (enemy && (m_pOwner->distanceFrom(enemy) < 800.0f))
    {
        CBotGlobals::botMessage(NULL, 0, "HWGuy defending flag");
        return true;
    }

    CBotGlobals::botMessage(NULL, 0, "HWGuy assisting squad objective");
    return true;
}

bool CFFClassHWGuy::UseSpecial()
{
    float now = g_ff_test_time;
    if (now < m_fNextSpecialUse)
        return false;

    m_bSpinning = !m_bSpinning;
    m_fNextSpecialUse = now + 0.8f;
    CBotGlobals::botMessage(NULL, 0, m_bSpinning ? "HWGuy spin up" : "HWGuy spin down");
    return true;
}

EBotTask CFFClassHWGuy::GetIdealTask()
{
    return BOT_TASK_NONE;
}
