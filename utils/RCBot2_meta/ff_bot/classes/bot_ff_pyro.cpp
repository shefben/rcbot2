// bot_ff_pyro.cpp
// Pyro-specific logic for CBotFF

#include "../../bot_ff.h"
#include "../../bot_globals.h"
#include "../../bot_navigation.h"
#include "../../bot_manager.h"
#include "../../bot_event.h"
#include "../../bot_util.h"
#include "../../bot_math.h"
#include "../../bot_task.h"
#include "../../bot_schedule.h"
#include "../../bot_weapon_defs.h"
#include "../../game_shared/ff/ff_shareddefs.h" // For FF_CLASS_PYRO if not in bot_const.h
#include "../../bot_projectiles.h" // For m_NearestEnemyRocket (if still used) and projectile detection helpers

void CBotFF::pyroModThink() {
    // Airblast logic for Pyro
    // Note: currentFFClass_modThink is established in the calling CBotFF::modThink()
    if (gpGlobals->time >= m_fNextAirblastTime) {
        CBotWeapon* pFlamer = getWeapon(WEAPON_FF_FLAMETHROWER);
        if (pFlamer && pFlamer->canUseSecondary()) { // Assuming secondary fire is airblast
            edict_t* pProjectile = NULL;
            // TODO: Implement actual projectile detection.
            // For now, this placeholder relies on external population of potential projectiles or a more direct check.
            // Example: Iterating visible entities if m_pVisibles is available and contains non-player entities.
            // for (int i = 0; i < m_pVisibles->numVisibles(); ++i) {
            //     edict_t* pVisibleEnt = m_pVisibles->getVisible(i);
            //     if (pVisibleEnt && pVisibleEnt != m_pEdict && !(pVisibleEnt->v.flags & FL_CLIENT) &&
            //         CClassInterface::isFFReflectableProjectile(pVisibleEnt)) {
            //         pProjectile = pVisibleEnt; // Found a potential projectile
            //         break;
            //     }
            // }

            if (pProjectile && FVisible(pProjectile, edict())) {
                 m_bWantsToAirblast = true;
                 setSchedule(new CSchedFFPyroAirblast(this, pProjectile));
                 m_fNextAirblastTime = gpGlobals->time + 1.0f; // Cooldown
            } else {
                for (int i = 1; i <= gpGlobals->maxClients; i++) {
                    CBasePlayer *pPlayer = (CBasePlayer*)UTIL_PlayerByIndex(i);
                    if (pPlayer && pPlayer->IsPlayer() && pPlayer->IsAlive() && pPlayer->pev->team == pev->team && (pPlayer->pev->effects & EF_ONFIRE)) {
                        if (FVisible(ENT(pPlayer->pev), edict()) && (pPlayer->pev->origin - pev->origin).Length() < 300) {
                             m_bWantsToAirblast = true;
                             setSchedule(new CSchedFFPyroAirblast(this, ENT(pPlayer->pev)));
                             m_fNextAirblastTime = gpGlobals->time + 1.0f;
                             break;
                        }
                    }
                }
            }
        }
        if (!m_bWantsToAirblast && m_pCurrentSchedule && m_pCurrentSchedule->getScheduleId() == SCHED_FF_PYRO_AIRBLAST_DEFEND) {
            clearSchedule("Airblast condition no longer met for pyroModThink");
        }
    }
}

void CBotFF::getPyroTasks(unsigned int iIgnore) {
    // Note: currentFFClass_getTasks is established in the calling CBotFF::getTasks()
    // This function is called when currentFFClass_getTasks == FF_CLASS_PYRO

    CBotWeapon* pFlamer = m_pWeapons->getWeaponByName("weapon_ff_flamethrower");
    CBotWeapon* pIC = m_pWeapons->getWeaponByName("weapon_ff_ic");
    edict_t* pCurrentEnemy = m_pEnemy.Get();
    bool enemyValidAndVisible = (pCurrentEnemy && CBotGlobals::entityIsAlive(pCurrentEnemy) && isVisible(pCurrentEnemy));
    float enemyDist = enemyValidAndVisible ? distanceFrom(pCurrentEnemy) : 9999.0f;

    // Airblast Logic
    if (pFlamer && pFlamer->hasWeapon() && engine->Time() >= m_fNextAirblastTime) {
        bool airblastConditionMet = false;
        edict_t* airblastTargetEntity = NULL;

        // 1. Projectile Reflection
        for (int i = 0; i < m_pVisibles->numVisibles(); ++i) {
            edict_t* pVisibleEnt = m_pVisibles->getVisible(i);
            if (pVisibleEnt && pVisibleEnt != m_pEdict && CBotGlobals::entityIsValid(pVisibleEnt) && !(pVisibleEnt->v.flags & FL_CLIENT) && pVisibleEnt->v.movetype != MOVETYPE_NONE) {
                if (CClassInterface::isFFReflectableProjectile(pVisibleEnt)) {
                    if (distanceFrom(pVisibleEnt) < 250.0f && isFacingEdict(pVisibleEnt, 0.7f)) {
                        airblastConditionMet = true;
                        airblastTargetEntity = pVisibleEnt;
                        break;
                    }
                }
            }
        }

        // 2. Push enemy
        if (!airblastConditionMet && enemyValidAndVisible && enemyDist < 180.0f) {
            airblastConditionMet = true;
            airblastTargetEntity = pCurrentEnemy;
        }

        // 3. Extinguish Teammate (Conceptual)
        // if (!airblastConditionMet) {
        //    for (int i = 1; i <= gpGlobals->maxClients; ++i) {
        //        edict_t* pTeammate = INDEXENT(i);
        //        if (pTeammate && pTeammate != m_pEdict && CBotGlobals::entityIsValid(pTeammate) && CBotGlobals::entityIsAlive(pTeammate) &&
        //            getTeam() == CBotGlobals::getTeam(pTeammate) && isVisible(pTeammate) && distanceFrom(pTeammate) < 200.0f &&
        //            CClassInterface::IsOnFire(pTeammate)) {
        //            airblastConditionMet = true; airblastTargetEntity = pTeammate; break;
        //        }
        //    }
        // }

        if (airblastConditionMet) {
            ADD_UTILITY_WEAPON_TARGET(BOT_UTIL_FF_PYRO_AIRBLAST, true, 0.9f, pFlamer, airblastTargetEntity);
        }
    }

    // Incendiary Cannon (IC) Logic
    if (pIC && pIC->hasWeapon() && !pIC->outOfAmmo(this)) {
        if (enemyValidAndVisible && enemyDist > 250.0f && enemyDist < 1200.0f) {
             if (!m_pUtil->hasUtility(BOT_UTIL_FF_PYRO_AIRBLAST)) {
                ADD_UTILITY_WEAPON(BOT_UTIL_FF_PYRO_USE_IC, true, 0.75f, pIC);
             }
        }
    }

    // Flamethrower Primary Attack Logic
    if (pFlamer && pFlamer->hasWeapon() && !pFlamer->outOfAmmo(this) && enemyValidAndVisible && enemyDist < 350.0f) {
        if (!m_pUtil->hasUtility(BOT_UTIL_FF_PYRO_AIRBLAST) && !m_pUtil->hasUtility(BOT_UTIL_FF_PYRO_USE_IC)) {
           ADD_UTILITY_WEAPON(BOT_UTIL_ATTACK, true, 0.8f, pFlamer);
        }
    }
}

bool CBotFF::executePyroAction(CBotUtility *util) {
    switch(util->m_iUtility) {
        case BOT_UTIL_FF_PYRO_AIRBLAST:
            if (util->getWeaponChoice() && util->getWeaponChoice()->getWeaponInfo()->isWeaponName("weapon_ff_flamethrower")) {
                m_bWantsToAirblast = true;
                edict_t* airblastTarget = util->getTaskEdictTarget();

                CBotSchedule* pAirblastSchedule = new CSchedFFPyroAirblast(this, airblastTarget);
                // pAirblastSchedule->setID(SCHED_FF_PYRO_AIRBLAST_DEFEND); // ID is set in CSchedFFPyroAirblast constructor
                m_pSchedules->add(pAirblastSchedule);
                return true;
            }
            break;
        case BOT_UTIL_FF_PYRO_USE_IC:
            if (util->getWeaponChoice() && util->getWeaponChoice()->getWeaponInfo()->isWeaponName("weapon_ff_ic")) {
                if (m_pEnemy.Get() && CBotGlobals::entityIsAlive(m_pEnemy.Get())) {
                    m_pSchedules->add(new CBotAttackSched(m_pEnemy.Get()));
                    return true;
                }
            }
            break;
        default:
            return false;
    }
    return false;
}

// CTaskFFPyroAirblast
CTaskFFPyroAirblast::CTaskFFPyroAirblast(edict_t* pTarget) : m_hTargetEntity(pTarget) {
    setTaskName("TaskFFPyroAirblast");
    m_bTaskComplete = false;
}
void CTaskFFPyroAirblast::init(CBot* pBot) {
    CBotTask::init(pBot);
    if (!m_hTargetEntity.Get() && m_pTaskDataTargetEdict) {
        m_hTargetEntity.Set(m_pTaskDataTargetEdict);
    }
}
void CTaskFFPyroAirblast::execute(CBot* pBot) {
    CBotFF* pFFBot = static_cast<CBotFF*>(pBot);
    if (!pFFBot) {
        failTask("Bot pointer is null");
        m_bTaskComplete = true;
        return;
    }
    edict_t* pTarget = m_hTargetEntity.Get();
    if (pTarget && CBotGlobals::entityIsValid(pTarget) && (CBotGlobals::entityIsAlive(pTarget) || !(pTarget->v.flags & FL_ONGROUND)))) {
        pFFBot->setLookAt(pTarget);
        pFFBot->setLookAtTask(LOOK_EDICT_PRIORITY);
    } else {
        pFFBot->setLookAtTask(LOOK_NONE);
    }
    pFFBot->m_bWantsToAirblast = true;
    setTaskStatus(TASK_COMPLETE);
    m_bTaskComplete = true;
}
// getTaskName is inherited

// CSchedFFPyroAirblast
CSchedFFPyroAirblast::CSchedFFPyroAirblast(CBotFF* pBot, edict_t* pTarget) {
    setID(SCHED_FF_PYRO_AIRBLAST_DEFEND);
    setScheduleName("SchedFFPyroAirblast");
    if (!pBot) { failSchedule(); return; }
    CBotWeapon* pFlamer = pBot->getWeapons()->getWeaponByName("weapon_ff_flamethrower");
    if (!pFlamer || !pFlamer->hasWeapon() || !pFlamer->getWeaponInfo()) {
        failSchedule();
        return;
    }
    addTask(new CSelectWeaponTask(pFlamer->getWeaponInfo()));
    addTask(new CTaskFFPyroAirblast(pTarget));
}
// getScheduleName is inherited
