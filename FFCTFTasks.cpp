#include "FFCTFTasks.h"
#include "FFBaseAI.h"       // For CFFBaseAI and its methods
#include "CFFPlayer.h"      // For CFFPlayer methods
#include "BotKnowledgeBase.h" // For accessing game state like flag status
#include "BotDefines.h"     // For constants
// #include "bot_schedule.h" // Assumed for pSchedule parameter in fail/complete (RCBot2 context)

#include <string>
#include <iostream> // For placeholder debug prints

// --- Conceptual Time & Bot Access (placeholders if not available from CBot/CFFBaseAI) ---
// Using placeholders defined in FFBaseAI.cpp or FFControlPointTasks.cpp if they are global.
// For self-containment, let's assume CFFBaseAI provides GetCurrentWorldTime_conceptual()
// and CFFPlayer provides GetOrigin(), HasFlag_conceptual(), AddButton_conceptual().
// Actual CUserCmd is passed to CFFBaseAI::Update and then to CFFBaseAI::ExecuteSubTask.
// These tasks, if executed by an RCBot2-style CBotSchedule, would get CBot* which we cast to CFFBaseAI*.
// The CFFBaseAI would then be responsible for passing its CUserCmd to these tasks if they directly modify it,
// or the tasks call higher-level actions on CFFBaseAI which then modify the UserCmd.
// For this implementation, tasks will call action methods on CFFBaseAI.

// --- CPickupFlagTask_FF Implementation ---
CPickupFlagTask_FF::CPickupFlagTask_FF(CBaseEntity* pFlagEntity_conceptual, const Vector& vFlagKnownPosition)
    : CBotTask(),
      m_pFlagEntity_conceptual(pFlagEntity_conceptual),
      m_vFlagKnownPosition(vFlagKnownPosition),
      m_fTaskTimeout(15.0f), // Max 15 seconds to try and pick up once at location
      m_fTaskStartTime(0.0f)
{
    // setID(TASK_FF_PICKUP_FLAG); // Conceptual RCBot2 Task ID
}

void CPickupFlagTask_FF::init(CFFBaseAI* botAI) {
    CBotTask::init(botAI); // Base class init if it takes CFFBaseAI* or void*
    setState(TASK_STATE_RUNNING); // Assuming TASK_STATE_RUNNING from RCBot2's CBotTask
    m_fTaskStartTime = botAI ? botAI->GetCurrentWorldTime_conceptual() : 0.0f;

    if (botAI) {
        // Path planning to m_vFlagKnownPosition is handled by CFFBaseAI::ExecuteSubTask
        // when it sees a SubTask of type PICKUP_FLAG_FF whose targetPosition is m_vFlagKnownPosition.
        // This task assumes the bot is already at or very near m_vFlagKnownPosition
        // due to a preceding MOVE_TO_POSITION subtask.
        // If not, this task might need to initiate movement itself, or fail.
        // For now, let's assume the AI's ExecuteSubTask for PICKUP_FLAG will first ensure proximity.
        // std::cout << botAI->GetBotPlayer()->GetNamePlaceholder() << ": Init PickupFlagTask for flag at ("
        //           << m_vFlagKnownPosition.x << ", " << m_vFlagKnownPosition.y << ")" << std::endl;
    }
}

void CPickupFlagTask_FF::execute(CFFBaseAI* botAI, CBotSchedule* pSchedule) {
    if (!botAI || !botAI->GetBotPlayer() || !botAI->GetKnowledgeBase()) {
        fail(pSchedule, "PickupFlag: BotAI, BotPlayer, or KB is null"); return;
    }
    CFFPlayer* ffPlayer = botAI->GetBotPlayer();
    const BotKnowledgeBase* kb = botAI->GetKnowledgeBase();
    CUserCmd* pCmd = botAI->GetUserCmd_conceptual(); // AI needs to provide UserCmd for actions

    // Conceptual: Check if flag still exists or has moved significantly from expectation
    // const FlagInfo* actualFlagInfo = kb->GetFlagInfoByEntity_conceptual(m_pFlagEntity_conceptual);
    // if (!actualFlagInfo || actualFlagInfo->status == FlagStatus::CARRIED_BY_ALLY || actualFlagInfo->status == FlagStatus::CARRIED_BY_ENEMY) {
    //     fail(pSchedule, "Flag no longer available or already carried"); return;
    // }
    // if ((actualFlagInfo->currentPosition - m_vFlagKnownPosition).LengthSqr() > 50.0f*50.0f) {
    //     fail(pSchedule, "Flag moved significantly from expected pickup spot"); return;
    // }


    // Check if bot already has the flag (conceptual: ENEMY_FLAG_ID or OUR_FLAG_ID based on task context)
    // int flagIdToPickup = (m_pFlagEntity_conceptual == kb->GetEnemyFlagInfo_conceptual().pFlagEntity) ? ENEMY_FLAG_ID_CONCEPTUAL : OUR_FLAG_ID_CONCEPTUAL;
    // if (ffPlayer->HasFlag_conceptual(flagIdToPickup)) {
    //     complete(); return;
    // }

    if (botAI->GetCurrentWorldTime_conceptual() - m_fTaskStartTime > m_fTaskTimeout) {
        fail(pSchedule, "Timeout trying to pickup flag"); return;
    }

    float distToFlagSq = (ffPlayer->GetOrigin().x - m_vFlagKnownPosition.x)*(ffPlayer->GetOrigin().x - m_vFlagKnownPosition.x) +
                         (ffPlayer->GetOrigin().y - m_vFlagKnownPosition.y)*(ffPlayer->GetOrigin().y - m_vFlagKnownPosition.y);

    if (distSq < PICKUP_RADIUS_SQR_FF) {
        botAI->StopMoving_conceptual(pCmd); // Stop movement
        // Attempt to "use" the flag or rely on auto-pickup by being close.
        // Some games require +use, others are auto-touch.
        // ffPlayer->AddButton(pCmd, IN_USE); // Conceptual: Press +use key
        // std::cout << ffPlayer->GetNamePlaceholder() << ": Attempting to pickup flag (at pickup radius)." << std::endl;

        // For this placeholder, assume being close is enough and it completes quickly if flag is there.
        // A real system would check game state again next frame or listen for a flag pickup event.
        complete(); // Simplified: if close, assume pickup happens
    } else {
        // Bot is not at the flag position yet. This task shouldn't be running if a MoveTo task didn't precede it.
        // This indicates an issue with schedule decomposition or MoveTo task completion criteria.
        // However, if CFFBaseAI::ExecuteSubTask for PICKUP_FLAG handles movement:
        // if (!botAI->IsCurrentlyMovingTo(m_vFlagKnownPosition)) { // Conceptual check
        //    botAI->MoveTo(m_vFlagKnownPosition, pCmd); // Re-issue move if not already doing so
        // }
        // For now, assume CFFBaseAI's ExecuteSubTask for PICKUP_FLAG will call MoveTo then this task's logic.
        // If this execute is called, it means the MoveTo part of the subtask in CFFBaseAI is done.
        // If still not close enough, it means MoveTo didn't get close enough.
        fail(pSchedule, "Not close enough to flag for pickup task execution");
    }
}

void CPickupFlagTask_FF::reset(CFFBaseAI* botAI) { CBotTask::reset(botAI); m_fTaskStartTime = 0.0f; }
std::string CPickupFlagTask_FF::debugString(CFFBaseAI* botAI) const { return "CPickupFlagTask_FF"; }


// --- CCaptureFlagTask_FF Implementation ---
CCaptureFlagTask_FF::CCaptureFlagTask_FF(CBaseEntity* pCaptureZoneEntity_conceptual, const Vector& vCaptureZonePosition, float fCaptureRadius, float fTimeout)
    : CBotTask(),
      m_pCaptureZoneEntity_conceptual(pCaptureZoneEntity_conceptual),
      m_vCaptureZonePosition(vCaptureZonePosition),
      m_fCaptureRadiusSqr(fCaptureRadius * fCaptureRadius),
      m_fTimeout(fTimeout),
      m_fTaskStartTime(0.0f) {}

void CCaptureFlagTask_FF::init(CFFBaseAI* botAI) {
    CBotTask::init(botAI);
    setState(TASK_STATE_RUNNING);
    m_fTaskStartTime = botAI ? botAI->GetCurrentWorldTime_conceptual() : 0.0f;
    // Movement to m_vCaptureZonePosition is handled by CFFBaseAI::ExecuteSubTask for this SubTaskType
}

void CCaptureFlagTask_FF::execute(CFFBaseAI* botAI, CBotSchedule* pSchedule) {
    if (!botAI || !botAI->GetBotPlayer() || !botAI->GetKnowledgeBase()) { fail(pSchedule, "CaptureFlag: Invalid bot state"); return; }
    CFFPlayer* ffPlayer = botAI->GetBotPlayer();
    const BotKnowledgeBase* kb = botAI->GetKnowledgeBase();
    // CUserCmd* pCmd = botAI->GetUserCmd_conceptual();

    // Conceptual: Check if bot is actually carrying the ENEMY flag.
    // if (!ffPlayer->HasFlag_conceptual(ENEMY_FLAG_ID_CONCEPTUAL)) {
    //     fail(pSchedule, "Bot lost the enemy flag"); return;
    // }

    if (botAI->GetCurrentWorldTime_conceptual() - m_fTaskStartTime > m_fTimeout) {
        fail(pSchedule, "Timeout trying to capture flag at zone"); return;
    }

    float distToZoneSq = (ffPlayer->GetOrigin().x - m_vCaptureZonePosition.x)*(ffPlayer->GetOrigin().x - m_vCaptureZonePosition.x) +
                         (ffPlayer->GetOrigin().y - m_vCaptureZonePosition.y)*(ffPlayer->GetOrigin().y - m_vCaptureZonePosition.y);

    if (distToZoneSq < m_fCaptureRadiusSqr) {
        botAI->StopMoving_conceptual(nullptr /*pCmd*/);
        // botAI->SetLookAtTask_conceptual(LOOK_AROUND_ON_POINT, m_vCaptureZonePosition); // Look around while capping

        // Actual capture is a game event. This task just holds the bot in the zone.
        // The HLT should complete when the game event "flag_captured" for bot's team occurs.
        // For testing, we might complete if our own flag is at base (implying a cap is possible/just happened).
        // const FlagInfo& ourFlag = kb->GetOurFlagInfo_conceptual();
        // if (ourFlag.status == FlagStatus::AT_BASE) {
        //     complete();
        //     return;
        // }
        // For this placeholder, assume it completes after a short duration on point
        static float timeOnPoint = 0.0f;
        if (m_fTaskStartTime == botAI->GetCurrentWorldTime_conceptual()) timeOnPoint = 0.0f; // Reset if task just started
        timeOnPoint += botAI->GetFrameInterval_conceptual(); // Conceptual
        if (timeOnPoint > 2.0f) { complete(); timeOnPoint = 0.0f; return;}

    } else {
        // Bot is not in the capture zone. This task assumes prior MoveTo got it here.
        // If CFFBaseAI::ExecuteSubTask for this type handles movement, this means it failed to stay.
        fail(pSchedule, "Not in capture zone for CaptureFlag task");
    }
}
void CCaptureFlagTask_FF::reset(CFFBaseAI* botAI) { CBotTask::reset(botAI); m_fTaskStartTime = 0.0f; }
std::string CCaptureFlagTask_FF::debugString(CFFBaseAI* botAI) const { return "CCaptureFlagTask_FF"; }


// --- CMoveToEntityDynamic_FF Implementation ---
CMoveToEntityDynamic_FF::CMoveToEntityDynamic_FF(CBaseEntity* pTargetEntity_conceptual, float fFollowDistance, float fUpdatePathInterval)
    : CBotTask(),
      m_pTargetEntity_conceptual(pTargetEntity_conceptual),
      m_fFollowDistance(fFollowDistance),
      m_fFollowDistanceSqr(fFollowDistance * fFollowDistance),
      m_fUpdatePathInterval(fUpdatePathInterval),
      m_fNextPathUpdateTime(0.0f) {}

void CMoveToEntityDynamic_FF::init(CFFBaseAI* botAI) {
    CBotTask::init(botAI);
    setState(TASK_STATE_RUNNING);
    m_fNextPathUpdateTime = 0.0f; // Force initial path update
    if (botAI) {
        // std::cout << botAI->GetBotPlayer()->GetNamePlaceholder() << ": Init MoveToEntityDynamic." << std::endl;
    }
}

void CMoveToEntityDynamic_FF::execute(CFFBaseAI* botAI, CBotSchedule* pSchedule) {
    if (!botAI || !botAI->GetBotPlayer()) { fail(pSchedule, "MoveDynamic: Invalid bot state"); return; }
    // CFFPlayer* ffPlayer = botAI->GetBotPlayer();
    // CUserCmd* pCmd = botAI->GetUserCmd_conceptual();

    if (!m_pTargetEntity_conceptual /* || !m_pTargetEntity_conceptual->IsAlive_conceptual() */) {
        fail(pSchedule, "Target entity for dynamic move lost or dead");
        return;
    }

    Vector targetEntityPos = m_pTargetEntity_conceptual->GetPosition(); // Conceptual
    Vector myPos = botAI->GetBotPlayer()->GetOrigin();
    float distToTargetSq = (myPos.x - targetEntityPos.x)*(myPos.x - targetEntityPos.x) + (myPos.y - targetEntityPos.y)*(myPos.y - targetEntityPos.y);

    if (distSq < m_fFollowDistanceSqr) {
        botAI->StopMoving_conceptual(nullptr /*pCmd*/);
        // botAI->AimAt(targetEntityPos, pCmd); // Look towards target
        m_fNextPathUpdateTime = 0; // Allow immediate re-plan if target moves far again
        // Task is ongoing, successfully maintaining follow distance.
        // Completion of this task is usually handled by HLT logic (e.g., flag captured, or duration from HLT expires).
    } else {
        // Target is too far, or we need to update path
        float currentTime = botAI->GetCurrentWorldTime_conceptual();
        bool targetMovedSignificantly = (m_vLastTargetPosition.x - targetEntityPos.x)*(m_vLastTargetPosition.x - targetEntityPos.x) + (m_vLastTargetPosition.y - targetEntityPos.y)*(m_vLastTargetPosition.y - targetEntityPos.y) > PATH_RECALC_TARGET_MOVED_DIST_SQR_FF;

        if (currentTime >= m_fNextPathUpdateTime || !botAI->IsCurrentlyWithPath_conceptual() || targetMovedSignificantly) {
            // std::cout << "CMoveToEntityDynamic: Replanning path to target." << std::endl;
            botAI->MoveTo(targetEntityPos, nullptr /* pCmd - CFFBaseAI::MoveTo doesn't take pCmd now */);
            m_fNextPathUpdateTime = currentTime + m_fUpdatePathInterval;
            m_vLastTargetPosition = targetEntityPos;
        }
        // Actual movement commands are set by CFFBaseAI::Update -> ExecuteSubTask -> FollowPath
    }
    // This task type is usually ongoing until the HLT changes or fails.
    // It doesn't "complete" on its own unless target is lost.
}

void CMoveToEntityDynamic_FF::reset(CFFBaseAI* botAI) {
    CBotTask::reset(botAI);
    m_fNextPathUpdateTime = 0.0f;
    if(botAI) botAI->ClearCurrentPath();
}
std::string CMoveToEntityDynamic_FF::debugString(CFFBaseAI* botAI) const { return "CMoveToEntityDynamic_FF"; }
