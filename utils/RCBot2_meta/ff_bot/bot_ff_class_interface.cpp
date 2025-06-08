// bot_ff_class_interface.cpp

#include "../bot_ff.h"
#include "game_shared/ff/ff_playerclass_parse.h"
#include "../bot_const.h"
#include "../bot_weapon_defs.h"
#include "../bot_globals.h"
#include "player.h"
#include "game_shared/ff/ff_player.h"

const CFFPlayerClassInfo* CBotFF::GetClassGameData() const {
    if (!m_pEdict) return nullptr;
    FF_ClassID currentClass = (FF_ClassID)m_iBotClass;
    const char* className = nullptr;
    switch (currentClass) {
        case FF_CLASS_SCOUT:    className = "ff_playerclass_scout"; break;
        case FF_CLASS_SNIPER:   className = "ff_playerclass_sniper"; break;
        case FF_CLASS_SOLDIER:  className = "ff_playerclass_soldier"; break;
        case FF_CLASS_DEMOMAN:  className = "ff_playerclass_demoman"; break;
        case FF_CLASS_MEDIC:    className = "ff_playerclass_medic"; break;
        case FF_CLASS_HWGUY:    className = "ff_playerclass_hwguy"; break;
        case FF_CLASS_PYRO:     className = "ff_playerclass_pyro"; break;
        case FF_CLASS_SPY:      className = "ff_playerclass_spy"; break;
        case FF_CLASS_ENGINEER: className = "ff_playerclass_engineer"; break;
        case FF_CLASS_CIVILIAN: className = "ff_playerclass_civilian"; break;
        default: return nullptr;
    }
    if (!className) return nullptr;
    int handle = LookupPlayerClassInfoSlot(className);
    if (handle == -1) return nullptr;
    return GetFilePlayerClassInfoFromHandle(handle);
}

int CBotFF::GetMaxHP() const {
    const CFFPlayerClassInfo* pCD = GetClassGameData();
    return pCD ? pCD->m_iHealth : 100;
}

int CBotFF::GetMaxAP() const {
    const CFFPlayerClassInfo* pCD = GetClassGameData();
    return pCD ? pCD->m_iMaxArmour : 0;
}

float CBotFF::GetMaxSpeed() const {
    const CFFPlayerClassInfo* pCD = GetClassGameData();
    return pCD ? (float)pCD->m_iSpeed : 320.0f;
}

weapon_t CBotFF::GetWeaponByIndex(int index) const {
    const CFFPlayerClassInfo* pCD = GetClassGameData();
    if (!pCD || index < 0 || index >= pCD->m_iNumWeapons) return WEAPON_NONE;
    return g_weaponDefs.getWeaponID(pCD->m_aWeapons[index]);
}

weapon_t CBotFF::GetGrenade1WeaponID() const {
    const CFFPlayerClassInfo* pCD = GetClassGameData();
    if (!pCD || strcmp(pCD->m_szPrimaryClassName, "None") == 0) return WEAPON_NONE;
    return g_weaponDefs.getWeaponID(pCD->m_szPrimaryClassName);
}

weapon_t CBotFF::GetGrenade2WeaponID() const {
    const CFFPlayerClassInfo* pCD = GetClassGameData();
    if (!pCD || strcmp(pCD->m_szSecondaryClassName, "None") == 0) return WEAPON_NONE;
    return g_weaponDefs.getWeaponID(pCD->m_szSecondaryClassName);
}

int CBotFF::GetMaxGren1() const {
    const CFFPlayerClassInfo* pCD = GetClassGameData();
    return pCD ? pCD->m_iPrimaryMax : 0;
}

int CBotFF::GetMaxGren2() const {
    const CFFPlayerClassInfo* pCD = GetClassGameData();
    return pCD ? pCD->m_iSecondaryMax : 0;
}

int CBotFF::GetInitialGren1() const {
    const CFFPlayerClassInfo* pCD = GetClassGameData();
    return pCD ? pCD->m_iPrimaryInitial : 0;
}

int CBotFF::GetInitialGren2() const {
    const CFFPlayerClassInfo* pCD = GetClassGameData();
    return pCD ? pCD->m_iSecondaryInitial : 0;
}

int CBotFF::GetMaxAmmo(int ammoIndex) const {
    const CFFPlayerClassInfo* pCD = GetClassGameData();
    if (!pCD) return 0;
    if (ammoIndex == m_iAmmoShells) return pCD->m_iMaxShells;
    if (ammoIndex == m_iAmmoNails) return pCD->m_iMaxNails;
    if (ammoIndex == m_iAmmoCells) return pCD->m_iMaxCells;
    if (ammoIndex == m_iAmmoRockets) return pCD->m_iMaxRockets;
    if (ammoIndex == m_iAmmoDetpack) return pCD->m_iMaxDetpack;
    return CBotFortress::GetMaxAmmo(ammoIndex);
}

#define self ( (CFFPlayer*)m_pPlayer )

int CBotFF::GetCurrentHP() const {
    if (!self) return 0;
    return self->GetHealth();
}

int CBotFF::GetCurrentAP() const {
    if (!self) return 0;
    return self->pev->armorvalue;
}

float CBotFF::GetCurrentSpeed() const {
    if (!self) return 0.0f;
    return self->pev->velocity.Length2D();
}

int CBotFF::GetAmmoCount(int ammoIndex) const {
    if (!self) return 0;
    return self->m_rgAmmo[ammoIndex];
}

int CBotFF::GetGrenade1Count() const {
    if (!self) return 0;
    return self->m_iPrimary;
}

int CBotFF::GetGrenade2Count() const {
    if (!self) return 0;
    return self->m_iSecondary;
}

bool CBotFF::IsPlayerCloaked() const {
    if (!self) return false;
    FF_ClassID currentClass = CClassInterface::getFFClass(self->edict());
    if (currentClass != FF_CLASS_SPY) return false;
    return self->IsCloaked();
}

int CBotFF::GetPlayerJetpackFuel() const {
    if (!self) return 0;
    FF_ClassID currentClass = CClassInterface::getFFClass(self->edict());
    if (currentClass != FF_CLASS_PYRO) return 0;
    return self->m_iJetpackFuel;
}

bool CBotFF::IsPlayerBuilding() const {
    if (!self) return false;
    FF_ClassID currentClass = CClassInterface::getFFClass(self->edict());
    if (currentClass != FF_CLASS_ENGINEER) return false;
    return self->IsBuilding();
}

bool CBotFF::IsPlayerPrimingGrenade() const {
    if (!self) return false;
    return self->IsGrenadePrimed();
}

#undef self
