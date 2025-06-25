#include "ff_class_medic.h"
#include "bot_ff.h"

extern float g_ff_test_time;

void CFFClassMedic::OnSpawn()
{
    GiveWeapon(FF_WEAPON_MEDKIT);
    GiveWeapon(FF_WEAPON_SUPERNAILGUN);
    GiveWeapon(FF_WEAPON_TRANQUILISER);
    GiveAmmo(FF_GRENADE_CONCUSS, 2);
    m_fUberCharge = 0.0f;
    RegisterSpecial(1.0f);
}

bool CFFClassMedic::Think()
{
    if (!m_pOwner)
        return false;

    if (m_fUberCharge < 100.0f)
    {
        m_fUberCharge += 1.0f;
        CBotGlobals::botMessage(NULL, 0, "Medic uber charge += 1");
    }

    edict_t *enemy = m_pOwner->getEnemy();
    if (enemy && (m_pOwner->distanceFrom(enemy) < 800.0f))
    {
        CBotGlobals::botMessage(NULL, 0, "Medic defending flag");
        return true;
    }

    CBotGlobals::botMessage(NULL, 0, "Medic assisting squad objective");
    return true;
}

bool CFFClassMedic::UseSpecial()
{
    float now = g_ff_test_time;
    if (now < m_fNextSpecialUse)
        return false;
    if (m_fUberCharge < 100.0f)
        return false;
    m_fUberCharge = 0.0f;
    m_fNextSpecialUse = now + 6.0f;
    CBotGlobals::botMessage(NULL, 0, "Medic uber activated");
    return true;
}

EBotTask CFFClassMedic::GetIdealTask()
{
    return BOT_TASK_NONE;
}
