#ifndef FF_CLASS_SCOUT_H
#define FF_CLASS_SCOUT_H

#include "ff_class_base.h"

class CFFClassScout : public CFFPlayerClass
{
public:
    explicit CFFClassScout(CBot *owner) : CFFPlayerClass(owner) {}

    void OnSpawn() override;
    bool Think() override;
    bool UseSpecial() override;
    EBotTask GetIdealTask() override;
};

#endif // FF_CLASS_SCOUT_H
