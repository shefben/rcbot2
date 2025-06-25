#include "ff/classes/ff_class_scout.h"
#include "ff/classes/ff_class_soldier.h"
#include "ff/classes/ff_class_pyro.h"
#include "ff/classes/ff_class_demoman.h"
#include "ff/classes/ff_class_medic.h"
#include "ff/classes/ff_class_hwguy.h"
#include "ff/classes/ff_class_sniper.h"
#include "ff/classes/ff_class_spy.h"
#include "ff/classes/ff_class_engineer.h"
#include "bot_ff.h"

CFFPlayerClass *CreateFFPlayerClass(int classId, CBot *owner)
{
    switch (classId)
    {
    case FF_CLASS_SCOUT:
        return new CFFClassScout(owner);
    case FF_CLASS_SOLDIER:
        return new CFFClassSoldier(owner);
    case FF_CLASS_PYRO:
        return new CFFClassPyro(owner);
    case FF_CLASS_DEMOMAN:
        return new CFFClassDemoman(owner);
    case FF_CLASS_MEDIC:
        return new CFFClassMedic(owner);
    case FF_CLASS_HWGUY:
        return new CFFClassHWGuy(owner);
    case FF_CLASS_SNIPER:
        return new CFFClassSniper(owner);
    case FF_CLASS_SPY:
        return new CFFClassSpy(owner);
    case FF_CLASS_ENGINEER:
        return new CFFClassEngineer(owner);
    default:
        return nullptr;
    }
}
