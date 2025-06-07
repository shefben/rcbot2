#ifndef FF_CTF_TASKS_H
#define FF_CTF_TASKS_H

#include "bot_task.h"         // Assumed base class from RCBot2. Needs CBotTask definition.
#include "FFStateStructs.h"   // For Vector.
#include "BotDefines.h"       // For constants like PICKUP_RADIUS_SQR_FF

// Forward declarations
class CFFBaseAI;        // The AI class that will execute these tasks
class CBotSchedule;     // RCBot2 schedule system
class CBaseEntity;      // Conceptual game entity

// --- CPickupFlagTask_FF ---
// Task to move to a flag's (enemy or dropped friendly) location and attempt to pick it up.
class CPickupFlagTask_FF : public CBotTask {
public:
    CPickupFlagTask_FF(CBaseEntity* pFlagEntity_conceptual, const Vector& vFlagKnownPosition);
    virtual ~CPickupFlagTask_FF() {}

    virtual const char* getName() const override { return "Task: PickupFlag_FF"; }
    virtual void init(CFFBaseAI* botAI) override; // Changed CBot* to CFFBaseAI*
    virtual void execute(CFFBaseAI* botAI, CBotSchedule* pSchedule) override;
    virtual void reset(CFFBaseAI* botAI) override;
    virtual std::string debugString(CFFBaseAI* botAI) const override;

private:
    CBaseEntity* m_pFlagEntity_conceptual; // Pointer or ID to the flag entity
    Vector m_vFlagKnownPosition;           // Position to move to for pickup
    float m_fTaskTimeout;                  // Max time to attempt this task
    float m_fTaskStartTime;
    // int m_iFlagId_conceptual; // If using IDs instead of direct entity pointers
};

// --- CCaptureFlagTask_FF ---
// Task to move to the friendly capture zone with the enemy flag and complete the capture.
class CCaptureFlagTask_FF : public CBotTask {
public:
    CCaptureFlagTask_FF(CBaseEntity* pCaptureZoneEntity_conceptual, const Vector& vCaptureZonePosition,
                        float fCaptureRadius = DEFAULT_FLAG_CAPTURE_RADIUS_FF,
                        float fTimeout = DEFAULT_FLAG_CAPTURE_TIMEOUT_FF);
    virtual ~CCaptureFlagTask_FF() {}

    virtual const char* getName() const override { return "Task: CaptureFlag_FF"; }
    virtual void init(CFFBaseAI* botAI) override;
    virtual void execute(CFFBaseAI* botAI, CBotSchedule* pSchedule) override;
    virtual void reset(CFFBaseAI* botAI) override;
    virtual std::string debugString(CFFBaseAI* botAI) const override;

private:
    CBaseEntity* m_pCaptureZoneEntity_conceptual; // Entity representing the capture zone (if any)
    Vector m_vCaptureZonePosition;
    float m_fCaptureRadiusSqr;
    float m_fTimeout;
    float m_fTaskStartTime;
};

// --- CMoveToEntityDynamic_FF ---
// Task to follow a potentially moving entity, updating path as needed.
class CMoveToEntityDynamic_FF : public CBotTask {
public:
    CMoveToEntityDynamic_FF(CBaseEntity* pTargetEntity_conceptual,
                            float fFollowDistance = ESCORT_FOLLOW_DISTANCE_FF,
                            float fUpdatePathInterval = 1.0f);
    virtual ~CMoveToEntityDynamic_FF() {}

    virtual const char* getName() const override { return "Task: MoveToEntityDynamic_FF"; }
    virtual void init(CFFBaseAI* botAI) override;
    virtual void execute(CFFBaseAI* botAI, CBotSchedule* pSchedule) override;
    virtual void reset(CFFBaseAI* botAI) override;
    virtual std::string debugString(CFFBaseAI* botAI) const override;

private:
    CBaseEntity* m_pTargetEntity_conceptual;
    float m_fFollowDistance;
    float m_fFollowDistanceSqr; // Store squared for efficiency
    float m_fUpdatePathInterval;
    float m_fNextPathUpdateTime;
    Vector m_vLastTargetPosition; // To check if target moved significantly
};

#endif // FF_CTF_TASKS_H
