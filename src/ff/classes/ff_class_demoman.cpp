#include "ff_class_demoman.h"
#include "bot_ff.h"

extern float g_ff_test_time;

void CFFClassDemoman::OnSpawn()
{
    GiveWeapon(FF_WEAPON_GRENADELAUNCHER);
    GiveWeapon(FF_WEAPON_PIPELAUNCHER);
    GiveWeapon(FF_WEAPON_CROWBAR);
    GiveAmmo(AMMO_DETPACK, 4);
    RegisterSpecial(1.0f);
}

bool CFFClassDemoman::Think()
{
    if (!m_pOwner)
        return false;

    edict_t *enemy = m_pOwner->getEnemy();
    if (enemy && (m_pOwner->distanceFrom(enemy) < 800.0f))
    {
        CBotGlobals::botMessage(NULL, 0, "Demoman defending flag");
        return true;
    }

    CBotGlobals::botMessage(NULL, 0, "Demoman assisting squad objective");
    return true;
}

bool CFFClassDemoman::UseSpecial()
{
    float now = g_ff_test_time;
    if (now < m_fNextSpecialUse)
        return false;
    if (GetAmmo(AMMO_DETPACK) <= 0)
        return false;
    m_ammo[AMMO_DETPACK] -= 1;
    m_fNextSpecialUse = now + 15.0f;
    CBotGlobals::botMessage(NULL, 0, "Demoman detpack placed");
    return true;
}

EBotTask CFFClassDemoman::GetIdealTask()
{
    return BOT_TASK_NONE;
}

