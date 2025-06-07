#ifndef FF_CONTROL_POINT_TASKS_H
#define FF_CONTROL_POINT_TASKS_H

#include "bot_task.h"         // Assumed base class from RCBot2. Needs definition for CBotTask.
#include "FFStateStructs.h"   // For Vector.

// Forward declarations (assuming these are part of RCBot2 or engine/game SDK)
class CBot;
class CBotSchedule;
class CBaseEntity; // Conceptual game entity

// --- CMoveToPositionTask ---
// A more generic movement task that could replace CMoveToTask_FF if preferred.
// Assumes CBot has pathfinding capabilities integrated into SetMoveTo or similar.
class CMoveToPositionTask : public CBotTask {
public:
    CMoveToPositionTask(const Vector& targetPos, float arrivalTolerance = 32.0f);
    virtual ~CMoveToPositionTask() {}

    virtual const char* getName() const override { return "Task: MoveToPosition"; }
    virtual void init(CBot* pBot) override;
    virtual void execute(CBot* pBot, CBotSchedule* pSchedule) override;
    virtual void reset(CBot* pBot) override;
    virtual std::string debugString(CBot* pBot) const override;

protected:
    Vector m_vTargetPosition;
    float m_fArrivalTolerance;
    float m_fStuckTimer; // To detect if bot is stuck
};

// --- CSecureAreaTask_FF ---
class CSecureAreaTask_FF : public CBotTask {
public:
    CSecureAreaTask_FF(const Vector& vAreaCenter, float fRadius, float fDuration);
    virtual ~CSecureAreaTask_FF() {}

    virtual const char* getName() const override { return "Task: SecureArea_FF"; }
    virtual void init(CBot* pBot) override;
    virtual void execute(CBot* pBot, CBotSchedule* pSchedule) override;
    virtual void reset(CBot* pBot) override;
    virtual std::string debugString(CBot* pBot) const override;

private:
    Vector m_vAreaCenter;
    float m_fRadius;        // Radius to consider for enemy engagement.
    float m_fDuration;      // How long to actively secure if no initial threats.
    float m_fTaskStartTime;
    bool m_bFoundEnemyDuringTask; // Tracks if an enemy was engaged.
};

// --- CStandOnPointTask_FF ---
class CStandOnPointTask_FF : public CBotTask {
public:
    CStandOnPointTask_FF(CBaseEntity* pCPEntity, const Vector& vCPPosition, float fMaxTime, float captureRadius = 64.0f);
    virtual ~CStandOnPointTask_FF() {}

    virtual const char* getName() const override { return "Task: StandOnPoint_FF"; }
    virtual void init(CBot* pBot) override;
    virtual void execute(CBot* pBot, CBotSchedule* pSchedule) override;
    virtual void reset(CBot* pBot) override;
    virtual std::string debugString(CBot* pBot) const override;

private:
    CBaseEntity* m_pTargetCPEntity; // The CP entity itself.
    Vector m_vCPPosition;           // Target position on the point.
    float m_fMaxTime;               // Max time to spend on this task attempting to capture.
    float m_fCaptureRadius;         // How close bot needs to be to m_vCPPosition.
    float m_fTaskStartTime;
};

// --- CHoldPositionTask_FF ---
class CHoldPositionTask_FF : public CBotTask {
public:
    CHoldPositionTask_FF(const Vector& vPositionToHold, float fDuration, float fHoldRadius = 128.0f);
    virtual ~CHoldPositionTask_FF() {}

    virtual const char* getName() const override { return "Task: HoldPosition_FF"; }
    virtual void init(CBot* pBot) override;
    virtual void execute(CBot* pBot, CBotSchedule* pSchedule) override;
    virtual void reset(CBot* pBot) override;
    virtual std::string debugString(CBot* pBot) const override;

private:
    Vector m_vPositionToHold;
    float m_fDuration;      // How long to hold the position.
    float m_fHoldRadius;    // How far bot can stray while still "holding".
    float m_fTaskStartTime;
};

#endif // FF_CONTROL_POINT_TASKS_H
