#ifndef FF_CLASS_ENGINEER_H
#define FF_CLASS_ENGINEER_H

#include "ff_class_base.h"

class CFFClassEngineer : public CFFPlayerClass
{
public:
    explicit CFFClassEngineer(CBot *owner);

    void OnSpawn() override;
    bool Think() override;
    bool UseSpecial() override;
    EBotTask GetIdealTask() override;

    int GetSentryLevel() const;

private:
    int m_iBuildCycle;
};

#endif // FF_CLASS_ENGINEER_H
