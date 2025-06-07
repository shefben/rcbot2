#include "bot_fortress_schedules.h"
#include "bot_fortress_tasks.h" // Will contain CSecureAreaTask_FF, CStandOnPointTask_FF
#include "bot_task_move.h"    // Assumed: contains CMoveToTask or similar
// #include "bot_task_combat.h" // If a generic CSecureAreaTask is used from RCBot2

// Assumed: CBot is base, CBotFortress is derived. Need to cast for FF specific things if any.
#include "bot_fortress.h"

// Assumed: Vector is defined somewhere accessible (e.g. mathlib.h or ff_state_structs.h)
// If not, a simple struct Vector { float x,y,z; }; would be needed.


CCaptureControlPointSched_FF::CCaptureControlPointSched_FF(CBaseEntity* pTargetCPentity, const Vector& vTargetCPPosition)
    : CBotSchedule(), // Call base constructor
      m_pTargetCpEntity(pTargetCPentity),
      m_vTargetCpPosition(vTargetCPPosition)
{
    // Constructor
}

bool CCaptureControlPointSched_FF::init(CBot* pBot) {
    if (!CBotSchedule::init(pBot)) { // Call base class init
        return false;
    }

    // setID(SCHED_FF_CAPTURE_POINT); // Set the schedule ID

    // Ensure bot pointer is valid (though base init might do this)
    CBotFortress* pBotFF = dynamic_cast<CBotFortress*>(pBot);
    if (!pBotFF) {
        // Log_Error("CCaptureControlPointSched_FF::init: Bot is not a CBotFortress instance!\n");
        return false;
    }
    if (!m_pTargetCpEntity) {
        // Log_Error("CCaptureControlPointSched_FF::init: Target CP entity is null!\n");
        return false;
    }

    // --- Task Sequence ---

    // 1. Move to the Control Point's position.
    // Assuming CMoveToTask exists and takes a target Vector.
    // If RCBot2 uses CFindPathTask which then creates movement, that's also fine.
    CMoveToTask* moveTask = new CMoveToTask(m_vTargetCpPosition);
    // moveTask->setMovementType(CMoveToTask::RUN); // Example if task supports it
    // moveTask->setArrivalTolerance(50.0f); // How close to get
    AddTask(moveTask);

    // 2. Secure the area around the Control Point.
    // This new task will make the bot look for and engage enemies for a short duration
    // or until no enemies are nearby, within a certain radius of the point.
    CSecureAreaTask_FF* secureTask = new CSecureAreaTask_FF(m_vTargetCpPosition, 128.0f, 8.0f); // Secure 128u radius for up to 8s
    AddTask(secureTask);

    // 3. Stand on the point to capture it.
    // This new task will make the bot stay on the point, checking for capture completion.
    CStandOnPointTask_FF* standTask = new CStandOnPointTask_FF(m_pTargetCpEntity, m_vTargetCpPosition, 20.0f); // Max 20s on point
    AddTask(standTask);

    return true; // Successfully initialized
}
