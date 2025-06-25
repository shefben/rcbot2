#ifndef FF_BOT_CLASS_FACTORY_H
#define FF_BOT_CLASS_FACTORY_H

#include "ff/classes/ff_class_base.h"
class CBot;

CFFPlayerClass *CreateFFPlayerClass(int classId, CBot *owner);

#endif // FF_BOT_CLASS_FACTORY_H
