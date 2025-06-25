#include "ff_class_engineer.h"
#include "engy/building_manager.h"
#include "bot_ff.h"

extern float g_ff_test_time;

CFFClassEngineer::CFFClassEngineer(CBot *owner)
    : CFFPlayerClass(owner), m_iBuildCycle(0)
{
}

void CFFClassEngineer::OnSpawn()
{
    GiveWeapon(FF_WEAPON_RAILGUN);
    GiveWeapon(FF_WEAPON_SHOTGUN);
    GiveWeapon(FF_WEAPON_CROWBAR);
    GiveAmmo(FF_GRENADE_EMP, 2);
    m_iBuildCycle = 0;
    RegisterSpecial(1.0f);
}

bool CFFClassEngineer::Think()
{
    // restock or repair logic stub
    return true;
}

bool CFFClassEngineer::UseSpecial()
{
    switch(m_iBuildCycle)
    {
    case 0:
        g_FFBuildingManager.BuildDispenser();
        break;
    case 1:
        g_FFBuildingManager.BuildSentry();
        break;
    case 2:
        g_FFBuildingManager.BuildJumpPad();
        break;
    }
    m_iBuildCycle = (m_iBuildCycle + 1) % 3;
    m_fNextSpecialUse = g_ff_test_time + 5.0f;
    return true;
}

EBotTask CFFClassEngineer::GetIdealTask()
{
    return BOT_TASK_NONE;
}

int CFFClassEngineer::GetSentryLevel() const
{
    return g_FFBuildingManager.GetSentryLevel();
}
