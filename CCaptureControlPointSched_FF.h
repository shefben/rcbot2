#ifndef CCAPTURE_CONTROL_POINT_SCHED_FF_H
#define CCAPTURE_CONTROL_POINT_SCHED_FF_H

#include "bot_schedule.h" // Assumed base class header from RCBot2
#include "bot_task.h"     // For CBotTask
#include "ff_state_structs.h" // For Vector (if not in RCBot2's mathlib)

// Forward declarations (assuming these are part of RCBot2 or engine)
class CBaseEntity;
class CBotFortress; // The bot instance

class CCaptureControlPointSched_FF : public CBotSchedule {
public:
    CCaptureControlPointSched_FF(CBaseEntity* pTargetCPentity, int cpID, const Vector& cpPosition);
    virtual ~CCaptureControlPointSched_FF();

    virtual const char* getName() const { return "Schedule: CaptureControlPoint_FF"; }
    virtual bool init(CBot* pBot) override; // CBot* should be CBotFortress*

protected:
    CBaseEntity* m_pTargetCpEntity; // The actual CP entity
    int m_iCpID;                    // An identifier for the CP
    Vector m_vCpPosition;           // Position of the CP
};

#endif // CCAPTURE_CONTROL_POINT_SCHED_FF_H
