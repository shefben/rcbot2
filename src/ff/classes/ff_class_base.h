#ifndef FF_CLASS_BASE_H
#define FF_CLASS_BASE_H

#include <vector>
#include <map>
#include "bot.h"

enum EBotTask {
    BOT_TASK_NONE = 0
};

class CFFPlayerClass {
public:
    explicit CFFPlayerClass(CBot *owner);
    virtual ~CFFPlayerClass() {}

    virtual void OnSpawn();
    virtual bool Think();
    virtual bool UseSpecial();
    virtual EBotTask GetIdealTask() = 0;

    void RegisterSpecial(float cooldown);

    const std::vector<int> &GetWeapons() const { return m_weapons; }
    int GetAmmo(int type) const;
    int GetClip(int weapon) const;

protected:
    void GiveWeapon(int id);
    void GiveAmmo(int type, int amount);
    void GiveClip(int weapon, int amount);

    CBot *m_pOwner;
    std::vector<int> m_weapons;
    std::map<int,int> m_ammo;
    std::map<int,int> m_clips;
    float m_fNextSpecialUse;
};

#endif // FF_CLASS_BASE_H
