#ifndef CSTAND_ON_POINT_TASK_FF_H
#define CSTAND_ON_POINT_TASK_FF_H

#include "bot_task.h"         // Assumed base class header from RCBot2
#include "ff_state_structs.h" // For Vector

// Forward declarations
class CBaseEntity;
class CBotFortress; // The bot instance

class CStandOnPointTask_FF : public CBotTask {
public:
    CStandOnPointTask_FF(CBaseEntity* pTargetCP, const Vector& pointPosition, float maxDuration = 30.0f);
    virtual ~CStandOnPointTask_FF();

    virtual const char* getName() const { return "Task: StandOnPoint_FF"; }

    virtual void init(CBot* pBot) override;
    virtual void execute(CBot* pBot) override;
    virtual void reset(CBot* pBot) override;

    // Condition checks for interruption (these would be defined in RCBot2's condition system)
    // virtual bool checkConditions(CBot* pBot) override;

private:
    CBaseEntity* m_pTargetCpEntity; // The CP entity itself
    Vector m_vPointPosition;        // Target position on the point
    float m_flMaxDuration;          // Max time to spend on this task
    float m_flStartTime;            // Time this task started execution

    // Internal state
    bool m_bAtPosition;
};

#endif // CSTAND_ON_POINT_TASK_FF_H
