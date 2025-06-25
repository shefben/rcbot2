#ifndef FF_CLASS_SPY_H
#define FF_CLASS_SPY_H

#include "ff_class_base.h"

class CFFClassSpy : public CFFPlayerClass
{
public:
    explicit CFFClassSpy(CBot *owner) : CFFPlayerClass(owner), m_bCloaked(false), m_fCloak(100.0f) {}

    void OnSpawn() override;
    bool Think() override;
    bool UseSpecial() override;
    EBotTask GetIdealTask() override;

    bool IsCloaked() const { return m_bCloaked; }
    float GetCloakLevel() const { return m_fCloak; }

private:
    bool  m_bCloaked;
    float m_fCloak;
};

#endif // FF_CLASS_SPY_H
