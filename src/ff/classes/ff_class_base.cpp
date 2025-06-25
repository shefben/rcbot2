#include "ff_class_base.h"
#include "special_scheduler.h"

CFFPlayerClass::CFFPlayerClass(CBot *owner)
    : m_pOwner(owner), m_fNextSpecialUse(0.0f)
{
}

void CFFPlayerClass::OnSpawn()
{
    // base does nothing
}

bool CFFPlayerClass::Think()
{
    return false;
}

bool CFFPlayerClass::UseSpecial()
{
    // not ready by default
    return false;
}

void CFFPlayerClass::GiveWeapon(int id)
{
    m_weapons.push_back(id);
}

void CFFPlayerClass::GiveAmmo(int type, int amount)
{
    m_ammo[type] += amount;
}

void CFFPlayerClass::GiveClip(int weapon, int amount)
{
    m_clips[weapon] += amount;
}

int CFFPlayerClass::GetAmmo(int type) const
{
    auto it = m_ammo.find(type);
    return (it == m_ammo.end()) ? 0 : it->second;
}

int CFFPlayerClass::GetClip(int weapon) const
{
    auto it = m_clips.find(weapon);
    return (it == m_clips.end()) ? 0 : it->second;
}

void CFFPlayerClass::RegisterSpecial(float cooldown)
{
    CSpecialAbilityScheduler::Get().Register(this, cooldown);
}
