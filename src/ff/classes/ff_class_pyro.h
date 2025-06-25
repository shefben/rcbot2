#ifndef FF_CLASS_PYRO_H
#define FF_CLASS_PYRO_H

#include "ff_class_base.h"

class CFFClassPyro : public CFFPlayerClass
{
public:
    explicit CFFClassPyro(CBot *owner) : CFFPlayerClass(owner) {}

    void OnSpawn() override;
    bool Think() override;
    bool UseSpecial() override;
    EBotTask GetIdealTask() override;

private:
    enum { AMMO_FUEL = 1 };
};

#endif // FF_CLASS_PYRO_H
