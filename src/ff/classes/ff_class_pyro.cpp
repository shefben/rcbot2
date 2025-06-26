#include "ff_class_pyro.h"
#include "bot_ff.h"
#include "bot_globals.h"

extern float g_ff_test_time;

void CFFClassPyro::OnSpawn()
{
    GiveWeapon(FF_WEAPON_FLAMETHROWER);
    GiveWeapon(FF_WEAPON_SHOTGUN);
    GiveWeapon(FF_WEAPON_CROWBAR);
    GiveAmmo(AMMO_FUEL, 100);
    GiveAmmo(FF_GRENADE_NAPALM, 1);
    RegisterSpecial(1.0f);
}

bool CFFClassPyro::Think()
{
    if (!m_pOwner)
        return false;

    edict_t *enemy = m_pOwner->getEnemy();
    if (enemy && (m_pOwner->distanceFrom(enemy) < 800.0f))
    {
        CBotGlobals::botMessage(NULL, 0, "Pyro defending flag");
        return true;
    }

    CBotGlobals::botMessage(NULL, 0, "Pyro assisting squad objective");
    return true;
}

bool CFFClassPyro::UseSpecial()
{
    float now = g_ff_test_time;
    if (now < m_fNextSpecialUse)
        return false;
    if (GetAmmo(AMMO_FUEL) < 10)
        return false;
    m_ammo[AMMO_FUEL] -= 10;
    m_fNextSpecialUse = now + 5.0f;
    CBotGlobals::botMessage(NULL, 0, "Pyro afterburn burst");
    return true;
}

EBotTask CFFClassPyro::GetIdealTask()
{
    return BOT_TASK_NONE;
}

