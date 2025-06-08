#include "FFCTFTasks.h"
#include "FFBaseAI.h"       // For CFFBaseAI (now SDK-aware)
#include "CFFPlayer.h"      // For CFFPlayerWrapper (header name might still be CFFPlayer.h)
#include "BotKnowledgeBase.h"
#include "BotDefines.h"     // For constants like PICKUP_RADIUS_SQR_FF, PATH_RECALC_TARGET_MOVED_DIST_SQR_FF
#include "game/server/cbase.h" // For CBaseEntity (if task targets are CBaseEntity*)
// #include "GameDefines_Placeholder.h" // No longer needed

#include <string>
#include <iostream> // For placeholder debug prints

// --- CPickupFlagTask_FF Implementation ---
// --- CPickupFlagTask_FF Implementation ---
CPickupFlagTask_FF::CPickupFlagTask_FF(CBaseEntity* pFlagEntity_SDK, const Vector& vFlagKnownPosition) // Use CBaseEntity* for SDK entity
    : CBotTask(),
      m_pFlagEntity_SDK(pFlagEntity_SDK),
      m_vFlagKnownPosition(vFlagKnownPosition),
      m_fTaskTimeout(15.0f),
      m_fTaskStartTime(0.0f)
{
}

void CPickupFlagTask_FF::init(CFFBaseAI* botAI) {
    CBotTask::init(botAI);
    setState(TASK_STATE_RUNNING);
    m_fTaskStartTime = botAI ? botAI->GetWorldTime() : 0.0f; // Use SDK-aware time
}

void CPickupFlagTask_FF::execute(CFFBaseAI* botAI, CBotSchedule* pSchedule) {
    if (!botAI || !botAI->GetBotPlayer() || !botAI->GetKnowledgeBase()) {
        fail(pSchedule, "PickupFlag: BotAI, BotPlayerWrapper, or KB is null"); return;
    }
    CFFPlayerWrapper* ffPlayer = botAI->GetBotPlayer(); // Now CFFPlayerWrapper
    BotKnowledgeBase* kb = botAI->GetKnowledgeBase(); // Non-const

    // Conceptual: Check if flag (m_pFlagEntity_SDK) still exists or has moved.
    // const TrackedEntityInfo* actualFlagTrackedInfo = kb->GetTrackedEntity(m_pFlagEntity_SDK->edict());
    // if (!actualFlagTrackedInfo || actualFlagTrackedInfo->isCarried_Conceptual) { // Assuming TrackedEntityInfo has isCarried_Conceptual
    //     fail(pSchedule, "Flag no longer available or already carried"); return;
    // }
    // if (actualFlagTrackedInfo->lastKnownPosition.DistToSqr(m_vFlagKnownPosition) > 50.0f*50.0f) {
    //     fail(pSchedule, "Flag moved significantly"); return;
    // }

    // if (ffPlayer->HasFlag_SDK(ENEMY_FLAG_ID_CONCEPTUAL)) { // Conceptual check using SDK flag ID
    //     complete(); return;
    // }

    if (botAI->GetWorldTime() - m_fTaskStartTime > m_fTaskTimeout) {
        fail(pSchedule, "Timeout trying to pickup flag"); return;
    }

    if (ffPlayer->GetOrigin().DistToSqr2D(m_vFlagKnownPosition) < PICKUP_RADIUS_SQR_FF) {
        // No StopMoving call; CFFBaseAI::ExecuteSubTask handles movement based on subtask type.
        // If this task is active, AI should be trying to "use" or touch the flag.
        // For auto-touch flags, being close is enough. For +use flags:
        // CUserCmd* pCmd = botAI->GetUserCmdForNextFrame_Conceptual(); // If AI provides this
        // if (pCmd) ffPlayer->AddButton(pCmd, IN_USE);

        complete(); // Simplified: assume pickup if close. Game events would confirm.
    } else {
        // This task expects the bot to be at the flag. If not, the preceding MoveTo failed or schedule is wrong.
        // CFFBaseAI::ExecuteSubTask for PICKUP_FLAG should ensure movement.
        fail(pSchedule, "Not close enough to flag for pickup task");
    }
}

void CPickupFlagTask_FF::reset(CFFBaseAI* botAI) { CBotTask::reset(botAI); m_fTaskStartTime = 0.0f; }
std::string CPickupFlagTask_FF::debugString(CFFBaseAI* botAI) const { return "CPickupFlagTask_FF"; }


// --- CCaptureFlagTask_FF Implementation ---
CCaptureFlagTask_FF::CCaptureFlagTask_FF(CBaseEntity* pCaptureZoneEntity_SDK, const Vector& vCaptureZonePosition, float fCaptureRadius, float fTimeout)
    : CBotTask(),
      m_pCaptureZoneEntity_SDK(pCaptureZoneEntity_SDK),
      m_vCaptureZonePosition(vCaptureZonePosition),
      m_fCaptureRadiusSqr(fCaptureRadius * fCaptureRadius),
      m_fTimeout(fTimeout),
      m_fTaskStartTime(0.0f) {}

void CCaptureFlagTask_FF::init(CFFBaseAI* botAI) {
    CBotTask::init(botAI);
    setState(TASK_STATE_RUNNING);
    m_fTaskStartTime = botAI ? botAI->GetWorldTime() : 0.0f;
}

void CCaptureFlagTask_FF::execute(CFFBaseAI* botAI, CBotSchedule* pSchedule) {
    if (!botAI || !botAI->GetBotPlayer() || !botAI->GetKnowledgeBase()) { fail(pSchedule, "CaptureFlag: Invalid bot state"); return; }
    CFFPlayerWrapper* ffPlayer = botAI->GetBotPlayer();
    BotKnowledgeBase* kb = botAI->GetKnowledgeBase();

    // if (!ffPlayer->HasFlag_SDK(ENEMY_FLAG_ID_CONCEPTUAL)) { // Conceptual check
    //     fail(pSchedule, "Bot lost the enemy flag"); return;
    // }

    if (botAI->GetWorldTime() - m_fTaskStartTime > m_fTimeout) {
        fail(pSchedule, "Timeout trying to capture flag at zone"); return;
    }

    if (ffPlayer->GetOrigin().DistToSqr2D(m_vCaptureZonePosition) < m_fCaptureRadiusSqr) {
        // Bot is in the zone. CFFBaseAI::ExecuteSubTask for CAPTURE_FLAG_AT_POINT_FF should handle behavior (e.g. look around).
        // Actual capture is a game event. This task completes if criteria met (e.g. flag is home).
        // const FlagInfo* ourFlag = kb->GetFlagDetails_SDK(OUR_FLAG_ID_CONCEPTUAL); // Conceptual
        // if (ourFlag && ourFlag->status == FlagStatus::AT_BASE) {
        //     complete();
        //     return;
        // }
        // For placeholder, complete after a short duration on point if still holding flag.
        // static float timeOnPoint = 0.0f;
        // if (m_fTaskStartTime == botAI->GetWorldTime()) timeOnPoint = 0.0f;
        // timeOnPoint += botAI->GetFrameInterval(); // Conceptual: from CFFBaseAI or Globals
        // if (timeOnPoint > 2.0f) { complete(); timeOnPoint = 0.0f; return;}
    } else {
        fail(pSchedule, "Not in capture zone for CaptureFlag task");
    }
}
void CCaptureFlagTask_FF::reset(CFFBaseAI* botAI) { CBotTask::reset(botAI); m_fTaskStartTime = 0.0f; }
std::string CCaptureFlagTask_FF::debugString(CFFBaseAI* botAI) const { return "CCaptureFlagTask_FF"; }


// --- CMoveToEntityDynamic_FF Implementation ---
CMoveToEntityDynamic_FF::CMoveToEntityDynamic_FF(CBaseEntity* pTargetEntity_SDK, float fFollowDistance, float fUpdatePathInterval)
    : CBotTask(),
      m_pTargetEntity_SDK(pTargetEntity_SDK),
      m_fFollowDistance(fFollowDistance),
      m_fFollowDistanceSqr(fFollowDistance * fFollowDistance),
      m_fUpdatePathInterval(fUpdatePathInterval),
      m_fNextPathUpdateTime(0.0f) {}

void CMoveToEntityDynamic_FF::init(CFFBaseAI* botAI) {
    CBotTask::init(botAI);
    setState(TASK_STATE_RUNNING);
    m_fNextPathUpdateTime = 0.0f;
}

void CMoveToEntityDynamic_FF::execute(CFFBaseAI* botAI, CBotSchedule* pSchedule) {
    if (!botAI || !botAI->GetBotPlayer()) { fail(pSchedule, "MoveDynamic: Invalid bot state"); return; }

    // if (!m_pTargetEntity_SDK || !m_pTargetEntity_SDK->IsAlive()) { // Use SDK IsAlive
    //     fail(pSchedule, "Target entity for dynamic move lost or dead");
    //     return;
    // }

    // Vector targetEntityPos = m_pTargetEntity_SDK->GetAbsOrigin(); // SDK Call
    // Vector myPos = botAI->GetBotPlayer()->GetOrigin(); // SDK Call
    // float distToTargetSq = myPos.DistToSqr2D(targetEntityPos);

    // if (distToTargetSq < m_fFollowDistanceSqr) {
    //     // Bot is close enough. CFFBaseAI::ExecuteSubTask for MOVE_TO_ENTITY_DYNAMIC
    //     // should handle behavior (e.g. stop moving, look at target).
    //     m_fNextPathUpdateTime = 0; // Allow immediate re-plan if target moves far again.
    // } else {
    //     float currentTime = botAI->GetWorldTime();
    //     bool targetMovedSignificantly = m_vLastTargetPosition.DistToSqr(targetEntityPos) > PATH_RECALC_TARGET_MOVED_DIST_SQR_FF;

    //     if (currentTime >= m_fNextPathUpdateTime || !botAI->IsCurrentlyWithPath() || targetMovedSignificantly) {
    //         botAI->MoveTo(targetEntityPos, nullptr); // CFFBaseAI::MoveTo now only plans
    //         m_fNextPathUpdateTime = currentTime + m_fUpdatePathInterval;
    //         m_vLastTargetPosition = targetEntityPos;
    //     }
    //     // Movement commands are set by CFFBaseAI's main update loop via FollowPath.
    // }
}

void CMoveToEntityDynamic_FF::reset(CFFBaseAI* botAI) {
    CBotTask::reset(botAI);
    m_fNextPathUpdateTime = 0.0f;
    if(botAI) botAI->ClearCurrentPath();
}
std::string CMoveToEntityDynamic_FF::debugString(CFFBaseAI* botAI) const { return "CMoveToEntityDynamic_FF"; }
