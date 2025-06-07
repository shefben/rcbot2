#include "CCaptureControlPointSched_FF.h"
#include "CStandOnPointTask_FF.h" // The new task we'll define next
#include "bot_task_move.h"      // Assuming CMoveToTask or CFindPathTask exists in RCBot2
#include "bot_task_combat.h"    // Assuming CSecureAreaTask or CAttackTask exists

// Assuming CBot is the base class for CBotFortress
#include "bot_fortress.h" // For casting CBot* to CBotFortress* if needed

CCaptureControlPointSched_FF::CCaptureControlPointSched_FF(CBaseEntity* pTargetCPentity, int cpID, const Vector& cpPosition)
    : m_pTargetCpEntity(pTargetCPentity), m_iCpID(cpID), m_vCpPosition(cpPosition) {
    // Constructor logic if any
}

CCaptureControlPointSched_FF::~CCaptureControlPointSched_FF() {
    // Destructor logic if any
}

bool CCaptureControlPointSched_FF::init(CBot* pBot) {
    if (!CBotSchedule::init(pBot)) { // Call base class init
        return false;
    }

    CBotFortress* pBotFF = dynamic_cast<CBotFortress*>(pBot);
    if (!pBotFF) {
        // log error
        return false;
    }

    // Sequence of tasks:
    // 1. (Optional) Secure the area around the point first if enemies are likely present.
    //    This could be a CSecureAreaTask or similar. For simplicity, we might skip this
    //    or make it conditional.
    //    Example: if (pBotFF->GetVisionInterface()->GetNearbyKnownEnemies(m_vCpPosition, 500.0f).Count() > 0) {
    //        AddTask(new CSecureAreaTask(m_vCpPosition, 500.0f));
    //    }

    // 2. Move to the control point.
    //    Using a generic CMoveToTask, assuming it takes a Vector.
    //    If RCBot2 has CFindPathTask that then leads to movement, that would be used.
    CMoveToTask* pMoveTask = new CMoveToTask(m_vCpPosition);
    if (pMoveTask) {
        pMoveTask->setMovementSpeed(MOVETYPE_RUN); // Example if task supports it
        pMoveTask->setPreciseArrival(true);       // Stop close to the point
        AddTask(pMoveTask);
    } else {
        return false; // Failed to create task
    }

    // 3. Stand on the point to capture it.
    CStandOnPointTask_FF* pStandTask = new CStandOnPointTask_FF(m_pTargetCpEntity, m_vCpPosition, 30.0f); // Capture for up to 30s or until success
    if (pStandTask) {
        AddTask(pStandTask);
    } else {
        // If move task was added, it should be cleaned up if schedule fails init
        // Or rely on ClearSchedule in CBotSchedule base for cleanup on failure
        return false;
    }

    // Note: Combat tasks (like CAttackTask) are often not added explicitly in sequence
    // but are handled by an "action" layer or interrupts if an enemy is encountered
    // during MoveTo or StandOnPoint. Or, CStandOnPointTask_FF might internally allow
    // combat while trying to hold the point.

    return true; // Successfully initialized with tasks
}
