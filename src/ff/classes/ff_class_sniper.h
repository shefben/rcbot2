#ifndef FF_CLASS_SNIPER_H
#define FF_CLASS_SNIPER_H

#include "ff_class_base.h"

class CFFClassSniper : public CFFPlayerClass
{
public:
    explicit CFFClassSniper(CBot *owner) : CFFPlayerClass(owner), m_bCharging(false), m_fChargeStart(0.0f) {}

    void OnSpawn() override;
    bool Think() override;
    bool UseSpecial() override;
    EBotTask GetIdealTask() override;

    bool IsCharging() const { return m_bCharging; }

private:
    bool m_bCharging;
    float m_fChargeStart;
};

#endif // FF_CLASS_SNIPER_H
