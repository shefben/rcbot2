#ifndef FF_CLASS_DEMOMAN_H
#define FF_CLASS_DEMOMAN_H

#include "ff_class_base.h"

class CFFClassDemoman : public CFFPlayerClass
{
public:
    explicit CFFClassDemoman(CBot *owner) : CFFPlayerClass(owner) {}

    void OnSpawn() override;
    bool Think() override;
    bool UseSpecial() override;
    EBotTask GetIdealTask() override;

private:
    enum { AMMO_DETPACK = 2 };
};

#endif // FF_CLASS_DEMOMAN_H
