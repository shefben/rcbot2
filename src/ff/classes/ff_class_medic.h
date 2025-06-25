#ifndef FF_CLASS_MEDIC_H
#define FF_CLASS_MEDIC_H

#include "ff_class_base.h"

class CFFClassMedic : public CFFPlayerClass
{
public:
    explicit CFFClassMedic(CBot *owner) : CFFPlayerClass(owner), m_fUberCharge(0.0f) {}

    void OnSpawn() override;
    bool Think() override;
    bool UseSpecial() override;
    EBotTask GetIdealTask() override;

    float GetUberCharge() const { return m_fUberCharge; }

private:
    float m_fUberCharge;
};

#endif // FF_CLASS_MEDIC_H
