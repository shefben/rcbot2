#ifndef FF_CLASS_SOLDIER_H
#define FF_CLASS_SOLDIER_H

#include "ff_class_base.h"

class CFFClassSoldier : public CFFPlayerClass
{
public:
    explicit CFFClassSoldier(CBot *owner) : CFFPlayerClass(owner) {}

    void OnSpawn() override;
    bool Think() override;
    bool UseSpecial() override;
    EBotTask GetIdealTask() override;

private:
    enum { AMMO_ROCKET = 0 };
};

#endif // FF_CLASS_SOLDIER_H
