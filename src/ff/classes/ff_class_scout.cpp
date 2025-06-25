#include "ff_class_scout.h"
#include "locomotion_ff.h"
#include "ff_flag_tracker.h"
#include "bot_ff.h"
#include "bot_getprop.h"
#include "bot_globals.h"
#include "bot_task.h"

void CFFClassScout::OnSpawn()
{
    GiveWeapon(FF_WEAPON_SUPERNAILGUN);
    GiveWeapon(FF_WEAPON_SHOTGUN);
    GiveWeapon(FF_WEAPON_CROWBAR);
    GiveAmmo(FF_GRENADE_CONCUSS, 2);
    GiveAmmo(FF_GRENADE_NORMAL, 2);
    RegisterSpecial(1.0f);
}

bool CFFClassScout::Think()
{
    if (!m_pOwner)
        return false;

    if (m_pOwner->isCarryingFlag())
    {
        // head to capture point quickly
        CBotGlobals::botMessage(NULL,0,"Scout heading to capture point");
        CBotTask *path = new CFindPathTask(m_pOwner->getOrigin());
        m_pOwner->getSchedules()->addFront(new CBotSchedule(path));
        return true;
    }

    // assume enemy flag index is opposite team of owner
    int enemyTeam = (m_pOwner->getTeam() == FF_TEAM_RED) ? FF_TEAM_BLUE : FF_TEAM_RED;
    const FlagState &state = g_FFFlagTracker.GetFlag(enemyTeam);

    if (state.at_base && (m_pOwner->getHealth() > 75))
    {
        FFScoutConcJump(*m_pOwner);
        return true;
    }

    // otherwise follow squad leader
    CBotGlobals::botMessage(NULL, 0, "Following squad leader");
    return true;
}

bool CFFClassScout::UseSpecial()
{
    if (!m_pOwner)
        return false;

    float now = gpGlobals->curtime;
    if (now < m_fNextSpecialUse)
        return false;

    if (m_pOwner->getAmmo(FF_GRENADE_CONCUSS) <= 0)
        return false;

    if (!(CClassInterface::getFlags(m_pOwner->getEdict()) & FL_ONGROUND))
        return false;

    m_pOwner->primeGrenade(FF_GRENADE_CONCUSS);
    m_pOwner->jump();
    m_pOwner->releaseGrenade(0.6f);

    m_fNextSpecialUse = now + 10.0f;
    return true;
}

EBotTask CFFClassScout::GetIdealTask()
{
    return BOT_TASK_NONE;
}
