#ifndef BOT_FORTRESS_SCHEDULES_H
#define BOT_FORTRESS_SCHEDULES_H

#include "bot_schedule.h" // Assumed base class header from RCBot2
// #include "edict.h" // Assuming edict_t is defined here or in a base type header
// For now, use CBaseEntity as a proxy for edict_t if not available
class CBaseEntity; // Forward declaration, assuming it's RCBot2's entity representation
struct Vector; // Assuming Vector is defined (e.g. in mathlib or ff_state_structs.h)

// Placeholder for schedule IDs if eBotSchedule enum is not directly modifiable
#define SCHED_FF_CAPTURE_POINT 1001
#define SCHED_FF_DEFEND_POINT 1002

class CCaptureControlPointSched_FF : public CBotSchedule {
public:
    // Using CBaseEntity* as a substitute for edict_t* if edict_t is not readily available
    CCaptureControlPointSched_FF(CBaseEntity* pTargetCPentity, const Vector& vTargetCPPosition);
    virtual ~CCaptureControlPointSched_FF() {}

    virtual const char* getName() const override { return "Schedule: CaptureControlPoint_FF"; }
    virtual bool init(CBot* pBot) override;
    // execute() is typically handled by base CBotSchedule by running tasks

private:
    CBaseEntity* m_pTargetCpEntity;
    Vector m_vTargetCpPosition;
};


class CDefendControlPointSched_FF : public CBotSchedule {
public:
    CDefendControlPointSched_FF(CBaseEntity* pTargetCPentity, const Vector& vTargetCPPosition);
    virtual ~CDefendControlPointSched_FF() {}

    virtual const char* getName() const override { return "Schedule: DefendControlPoint_FF"; }
    virtual bool init(CBot* pBot) override;

private:
    CBaseEntity* m_pTargetCpEntity;
    Vector m_vTargetCpPosition;
};


#endif // BOT_FORTRESS_SCHEDULES_H
