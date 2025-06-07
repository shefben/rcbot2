#ifndef BOT_FORTRESS_TASKS_H
#define BOT_FORTRESS_TASKS_H

#include "bot_task.h" // Assumed base class header from RCBot2
// #include "vector.h" // Assuming Vector is defined here or similar
struct Vector; // Forward declare if full definition not available/needed here
class CBaseEntity; // Forward declaration

// Placeholder for task IDs if eBotTask enum is not directly modifiable
#define TASK_FF_SECURE_AREA 101
#define TASK_FF_STAND_ON_POINT 102
#define TASK_FF_MOVE_TO_TARGET 103 // If a specific FF move task is needed

// --- CSecureAreaTask_FF ---
class CSecureAreaTask_FF : public CBotTask {
public:
    CSecureAreaTask_FF(const Vector& vAreaCenter, float fRadius, float fDuration);
    virtual ~CSecureAreaTask_FF() {}

    virtual const char* getName() const override { return "Task: SecureArea_FF"; }

    virtual void init(CBot* pBot) override;
    virtual void execute(CBot* pBot, CBotSchedule* pSchedule) override;
    virtual void reset(CBot* pBot) override;
    // virtual bool checkConditions(CBot* pBot) override; // For interrupts

private:
    Vector m_vAreaCenter;
    float m_fRadius;
    float m_fDuration; // How long to actively secure if no threats initially
    float m_fTaskStartTime;
    bool m_bFoundEnemy; // Did we find an enemy during this task?
};


// --- CStandOnPointTask_FF ---
class CStandOnPointTask_FF : public CBotTask {
public:
    CStandOnPointTask_FF(CBaseEntity* pCPEntity, const Vector& vCPPosition, float fMaxTime);
    virtual ~CStandOnPointTask_FF() {}

    virtual const char* getName() const override { return "Task: StandOnPoint_FF"; }

    virtual void init(CBot* pBot) override;
    virtual void execute(CBot* pBot, CBotSchedule* pSchedule) override;
    virtual void reset(CBot* pBot) override;
    // virtual bool checkConditions(CBot* pBot) override;

private:
    CBaseEntity* m_pTargetCPEntity;
    Vector m_vCPPosition;
    float m_fMaxTime; // Max time to attempt capture/hold
    float m_fTaskStartTime;
};


// --- CRocketJumpTask_FF ---
class CRocketJumpTask_FF : public CBotTask {
public:
    // TargetApex might be where bot wants to land or the general direction of the jump.
    // LaunchDirectionHint can be used if a specific facing direction is desired before jumping (e.g. for wall jumps)
    CRocketJumpTask_FF(const Vector& vTargetApexOrDirection, bool isDirection = false);
    virtual ~CRocketJumpTask_FF() {}

    virtual const char* getName() const override { return "Task: RocketJump_FF"; }

    virtual void init(CBot* pBot) override;
    virtual void execute(CBot* pBot, CBotSchedule* pSchedule) override;
    virtual void reset(CBot* pBot) override;
    // virtual bool checkConditions(CBot* pBot) override; // e.g., is on ground, has health, has RL, has ammo

private:
    Vector m_vTarget; // Could be an apex, or a direction vector if isDirection is true
    bool m_bIsDirection;   // If true, m_vTarget is a direction, not a specific apex.

    enum class RJState {
        IDLE,
        AIMING,
        CROUCHING, // Optional: for crouch jumps
        JUMPING_AND_FIRING,
        IN_AIR_CONTROL, // Optional: for air strafing
        COMPLETED,
        FAILED
    };
    RJState m_eCurrentState;
    float m_fStateEntryTime;

    // Helper to perform the jump actions
    bool AttemptJumpAction(CBot* pBot, UserCmd* pCmd);
};


// --- (Optional) CMoveToTask_FF ---
// If RCBot2's CMoveToTask isn't suitable or needs FF-specific logic (e.g. jump pads)
// For now, assume generic CMoveToTask from bot_task_move.h is sufficient.
/*
class CMoveToTask_FF : public CBotTask { // Or derive from CMoveToTask if that's a base
public:
    CMoveToTask_FF(const Vector& vTargetPosition);
    virtual ~CMoveToTask_FF() {}
    // ...
};
*/

#endif // BOT_FORTRESS_TASKS_H
