#include "ff_class_soldier.h"
#include "bot_ff.h"
#include "in_buttons.h"

extern float g_ff_test_time;

void CFFClassSoldier::OnSpawn()
{
    GiveWeapon(FF_WEAPON_RPG);
    GiveWeapon(FF_WEAPON_SHOTGUN);
    GiveWeapon(FF_WEAPON_CROWBAR);
    GiveAmmo(AMMO_ROCKET, 4);
    GiveAmmo(FF_GRENADE_NORMAL, 2);
    RegisterSpecial(1.0f);
}

bool CFFClassSoldier::Think()
{
    if (!m_pOwner)
        return false;

    edict_t *enemy = m_pOwner->getEnemy();
    if (enemy && (m_pOwner->distanceFrom(enemy) < 800.0f))
    {
        CBotGlobals::botMessage(NULL, 0, "Soldier defending flag");
        return true;
    }

    CBotGlobals::botMessage(NULL, 0, "Soldier assisting squad objective");
    return true;
}

bool CFFClassSoldier::UseSpecial()
{
    float now = g_ff_test_time;
    if (now < m_fNextSpecialUse)
        return false;
    if (GetAmmo(AMMO_ROCKET) <= 0)
        return false;
    if (m_pOwner && m_pOwner->getHealth() <= 60)
        return false;

    m_ammo[AMMO_ROCKET] -= 1;
    if (m_pOwner)
    {
        m_pOwner->tapButton(IN_JUMP);
        m_pOwner->hurt(m_pOwner->getEdict(), m_pOwner->getHealth() - 40);
    }
    m_fNextSpecialUse = now + 10.0f;
    return true;
}

EBotTask CFFClassSoldier::GetIdealTask()
{
    return BOT_TASK_NONE;
}

