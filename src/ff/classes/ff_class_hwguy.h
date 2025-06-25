#ifndef FF_CLASS_HWGUY_H
#define FF_CLASS_HWGUY_H

#include "ff_class_base.h"

class CFFClassHWGuy : public CFFPlayerClass
{
public:
    explicit CFFClassHWGuy(CBot *owner) : CFFPlayerClass(owner), m_bSpinning(false) {}

    void OnSpawn() override;
    bool Think() override;
    bool UseSpecial() override;
    EBotTask GetIdealTask() override;

    bool IsSpinning() const { return m_bSpinning; }

private:
    bool m_bSpinning;
};

#endif // FF_CLASS_HWGUY_H
