#include "FFBaseAI.h"
#include "ObjectivePlanner.h"
#include "BotTasks.h"         // For SubTask and its members like ::startTime
#include "FFStateStructs.h"
#include "NavSystem.h"

#include <iostream>
#include <cmath>
#include <chrono> // For std::chrono::system_clock, std::chrono::duration

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// SDK Includes (conceptual)
// #include "game/shared/usercmd.h" // For CUserCmd
// #include "game/server/cbase.h"   // For CBaseEntity
// #include "public/globalvars_base.h" // For gpGlobals
// #include "CFFPlayer.h" // Now CFFPlayerWrapper.h (renamed) - this should be CFFPlayerWrapper.h
#include "CFFPlayer.h" // Will be CFFPlayerWrapper.h after rename step
#include "BotKnowledgeBase.h" // For non-const BotKnowledgeBase

// --- Conceptual Placeholder Types ---
// These should now come from SDK or our refactored headers.
// For example, CUserCmd is now from SDK. CBaseEntity from SDK.
// CFFPlayer is now CFFPlayerWrapper.
// --- End Conceptual Placeholders ---

// Constants for pathing
const float DEFAULT_ARRIVAL_TOLERANCE_SQR = 48.0f * 48.0f;
const float DEFAULT_PATH_NODE_REACHED_TOLERANCE_SQR = 64.0f * 64.0f;

// Assume gpGlobals is available if GetWorldTime() needs it.
// extern CGlobalVarsBase* gpGlobals; // Should be available from SDK's globalvars_base.h


CFFBaseAI::CFFBaseAI(CFFPlayerWrapper* pBotPlayer, CObjectivePlanner* pPlanner,
                   BotKnowledgeBase* pKnowledgeBase, const ClassConfigInfo* pClassConfig) // Updated types
    : m_pBotPlayer(pBotPlayer),
      m_pObjectivePlanner(pPlanner),
      m_pKnowledgeBase(pKnowledgeBase), // Non-const
      m_pClassConfig(pClassConfig),
      m_pCurrentTarget(nullptr),
      m_CurrentPathIndex(-1),
      m_pPathfinder(nullptr)
{
    // if (!m_pObjectivePlanner) {
    //     // std::cerr << "CFFBaseAI Error: CObjectivePlanner is NULL!" << std::endl;
    // }
    // if (!m_pBotPlayer) {
    //     // std::cerr << "CFFBaseAI Warning: CFFPlayerWrapper is NULL during construction." << std::endl;
    // }
    if (m_pKnowledgeBase && m_pKnowledgeBase->GetNavGraph()) {
        m_pPathfinder = std::make_unique<AStarPathfinder>(m_pKnowledgeBase->GetNavGraph());
    } else {
        // std::cerr << "CFFBaseAI Error: No NavGraph available in KnowledgeBase for Pathfinder initialization!" << std::endl;
    }
}

CFFBaseAI::~CFFBaseAI() {}

float CFFBaseAI::GetWorldTime() const {
    // if (m_pKnowledgeBase && m_pKnowledgeBase->GetGlobals_Conceptual()) { // Assuming KB can provide CGlobalVarsBase*
    //     return m_pKnowledgeBase->GetGlobals_Conceptual()->curtime;
    // }
    // Or if gpGlobals is directly accessible here (e.g. via include "globalvars_base.h")
    // if (gpGlobals) return gpGlobals->curtime;
    return 0.0f; // Placeholder
}

bool CFFBaseAI::IsLocalPlayerAlive() const {
    return m_pBotPlayer && m_pBotPlayer->IsAlive();
}

void CFFBaseAI::Update(CUserCmd* pCmd) {
    if (!pCmd) return;
    // SDK CUserCmd should be zeroed by game engine before filling or by bot at start of its thinking.
    // pCmd->buttons = 0; pCmd->forwardmove = 0.0f; pCmd->sidemove = 0.0f; // etc.

    if (!m_pObjectivePlanner || !IsLocalPlayerAlive()) {
        return;
    }
    m_pObjectivePlanner->EvaluateAndSelectTask();

    const SubTask* pConstCurrentSubTask = m_pObjectivePlanner->GetCurrentSubTask();

    if (pConstCurrentSubTask && !pConstCurrentSubTask->isCompleted) {
        bool subTaskIsStillOngoing = ExecuteSubTask(pConstCurrentSubTask, pCmd);
        SubTask* pMutableCurrentSubTask = m_pObjectivePlanner->GetCurrentSubTaskMutable();

        if (!subTaskIsStillOngoing) {
            float duration = 0.0f;
            if (pMutableCurrentSubTask && pMutableCurrentSubTask->startTime.time_since_epoch().count() != 0) {
                 duration = std::chrono::duration<float>(
                               std::chrono::system_clock::now() - pMutableCurrentSubTask->startTime).count();
            }
            if (pMutableCurrentSubTask) {
                 m_pObjectivePlanner->OnSubTaskOutcomeReported(pMutableCurrentSubTask->isCompleted,
                                                              pMutableCurrentSubTask->isCompleted ? "" : "Subtask_Failed_Or_Not_Marked_Complete_By_AI");
            }
        }
    } else if (pConstCurrentSubTask && pConstCurrentSubTask->isCompleted) {
         m_pObjectivePlanner->OnSubTaskOutcomeReported(true, "Previously_Completed");
    } else {
        if (m_pCurrentTarget /* && m_pCurrentTarget->IsAlive() using SDK method */) {
            // Derived class might implement default attack here if no task
        } else {
            if (m_pBotPlayer) AimAt(m_pBotPlayer->GetOrigin() + Vector(100,0,0), pCmd); // Aim forward
        }
    }
}

bool CFFBaseAI::ExecuteSubTask(const SubTask* pSubTask, CUserCmd* pCmd) {
    if (!pSubTask || !m_pBotPlayer || !pCmd) return false;
    bool isOngoing = true;
    SubTask* pMutableSubTask = m_pObjectivePlanner->GetCurrentSubTaskMutable();

    if (!pMutableSubTask || pMutableSubTask->type != pSubTask->type) {
        return false;
    }

    switch (pSubTask->type) {
        case SubTaskType::MOVE_TO_POSITION:
        case SubTaskType::MOVE_TO_ENTITY_DYNAMIC_FF: { // Assuming MOVE_TO_ENTITY was renamed or similar
            Vector targetPos;
            if (pSubTask->type == SubTaskType::MOVE_TO_ENTITY_DYNAMIC_FF && pSubTask->pTargetEntity_SDK) { // Assuming pTargetEntity_SDK is CBaseEntity*
                // targetPos = pSubTask->pTargetEntity_SDK->GetAbsOrigin(); // SDK call
            } else {
                targetPos = pSubTask->targetPosition;
            }

            bool targetPositionChangedSignificantly = true;
            if (!m_CurrentPath_NavAreaIDs.empty()) {
                targetPositionChangedSignificantly = (m_vCurrentMoveToTarget.DistToSqr(targetPos) > 10.0f*10.0f);
            }
            if (m_CurrentPath_NavAreaIDs.empty() || targetPositionChangedSignificantly) {
                // MoveTo now just calls PlanPathTo and sets m_vCurrentMoveToTarget. CUserCmd is not used by it.
                if (!MoveTo(targetPos, nullptr /*pCmd not needed for MoveTo's new role*/)) {
                    isOngoing = false; pMutableSubTask->isCompleted = false; break;
                }
            }
            isOngoing = FollowPath(pCmd); // FollowPath fills pCmd
            if (!isOngoing && m_pBotPlayer->GetOrigin().DistToSqr(targetPos) < DEFAULT_ARRIVAL_TOLERANCE_SQR) {
                 pMutableSubTask->isCompleted = true;
            } else if (!isOngoing) { // Path ended but not at target
                 pMutableSubTask->isCompleted = false;
            }
            break;
        }
        case SubTaskType::ATTACK_TARGET:
            // if (pSubTask->pTargetEntity_SDK && pSubTask->pTargetEntity_SDK->IsAlive()) { // SDK Check
            //     isOngoing = AttackTarget(pSubTask->pTargetEntity_SDK, pCmd);
            //     if (pSubTask->pTargetEntity_SDK && !pSubTask->pTargetEntity_SDK->IsAlive()) {
            //         isOngoing = false; pMutableSubTask->isCompleted = true;
            //     } else if (!isOngoing) {
            //          pMutableSubTask->isCompleted = false;
            //     }
            // } else {
            //     isOngoing = false;
            //     pMutableSubTask->isCompleted = true;
            // }
            break;

        case SubTaskType::CAPTURE_OBJECTIVE:
        case SubTaskType::STAND_ON_POINT: {
            float standRadiusSq = pSubTask->radiusParam * pSubTask->radiusParam;
            if (standRadiusSq <= 0.01f) standRadiusSq = DEFAULT_ARRIVAL_TOLERANCE_SQR;

            Vector currentPos = m_pBotPlayer->GetOrigin();
            if (currentPos.DistToSqr(pSubTask->targetPosition) < standRadiusSq) {
                AimAt(m_pBotPlayer->GetOrigin() + Vector(100,0,0), pCmd); // Look forward
                if (pSubTask->desiredDuration > 0.0f &&
                    pSubTask->startTime.time_since_epoch().count() != 0 &&
                    std::chrono::duration<float>(std::chrono::system_clock::now() - pSubTask->startTime).count() > pSubTask->desiredDuration) {
                    pMutableSubTask->isCompleted = true;
                    isOngoing = false;
                } else {
                    isOngoing = true;
                }
            } else {
                bool targetPosChanged = (m_vCurrentMoveToTarget.DistToSqr(pSubTask->targetPosition) > 1.0f);
                if (m_CurrentPath_NavAreaIDs.empty() || targetPosChanged) {
                    if (!MoveTo(pSubTask->targetPosition, nullptr)) { isOngoing = false; pMutableSubTask->isCompleted = false; break;}
                }
                isOngoing = FollowPath(pCmd);
                if(!isOngoing) {
                    pMutableSubTask->isCompleted = (m_pBotPlayer->GetOrigin().DistToSqr(pSubTask->targetPosition) < standRadiusSq);
                }
            }
            break;
        }
        case SubTaskType::USE_ABILITY_ON_TARGET: // Renamed from USE_ABILITY_ON_TARGET
        case SubTaskType::USE_ABILITY_AT_POSITION: // Renamed from USE_ABILITY_AT_POSITION
            // isOngoing = UseAbility(pSubTask->abilitySlot, pSubTask->pTargetEntity_SDK, pSubTask->targetPosition, pCmd);
            // if (!isOngoing) pMutableSubTask->isCompleted = true; // Assume ability is one-shot or sets completion itself
            break;
        case SubTaskType::SECURE_AREA:
        case SubTaskType::DEFEND_POSITION:
        case SubTaskType::HOLD_POSITION: {
            if (pSubTask->desiredDuration > 0.0f &&
                pSubTask->startTime.time_since_epoch().count() != 0 &&
                std::chrono::duration<float>(std::chrono::system_clock::now() - pSubTask->startTime).count() > pSubTask->desiredDuration) {
                pMutableSubTask->isCompleted = true;
                isOngoing = false;
            } else {
                // CBaseEntity* enemy = SelectTarget();
                // if(enemy) AttackTarget(enemy, pCmd); else AimAt(m_pBotPlayer->GetOrigin() + Vector(0,100,0), pCmd); // Look around
                isOngoing = true;
            }
            break;
        }
        default:
            isOngoing = false;
            pMutableSubTask->isCompleted = false;
            break;
    }
    return isOngoing;
}

bool CFFBaseAI::MoveTo(const Vector& targetPos, CUserCmd* pCmd) {
    ClearCurrentPath();
    m_vCurrentMoveToTarget = targetPos;
    if (!PlanPathTo(targetPos)) {
        return false;
    }
    // Path is planned, FollowPath will be called next tick by ExecuteSubTask normally.
    // No CUserCmd to fill here directly for MoveTo itself.
    return !m_CurrentPath_NavAreaIDs.empty();
}

bool CFFBaseAI::FollowPath(CUserCmd* pCmd) {
    if (!m_pBotPlayer || !pCmd) return false;
    if (m_CurrentPath_NavAreaIDs.empty() || m_CurrentPathIndex < 0 ) {
        return false;
    }

    Vector botOrigin = m_pBotPlayer->GetOrigin(); // SDK Call via wrapper
    if (botOrigin.DistToSqr(m_vCurrentMoveToTarget) < DEFAULT_ARRIVAL_TOLERANCE_SQR) {
        ClearCurrentPath();
        return false; // Arrived at final target
    }

    Vector nextNodeWorldPos;
    if (!GetNextPathNodePosition(nextNodeWorldPos)) { // This advances m_CurrentPathIndex or clears path
        ClearCurrentPath();
        return false; // Path exhausted or failed to get next node
    }

    AimAt(nextNodeWorldPos, pCmd);
    // Set movement. Actual speed should come from class data or be a define.
    // float speed = m_pClassConfig ? m_pClassConfig->speed : 300.0f;
    pCmd->forwardmove = 400; // Assuming a default forward speed
    pCmd->sidemove = 0;

    return true; // Still following path
}

bool CFFBaseAI::PlanPathTo(const Vector& targetPos) {
    ClearCurrentPath();
    if (!m_pPathfinder || !m_pBotPlayer || !m_pKnowledgeBase || !m_pKnowledgeBase->GetNavGraph()) {
        return false;
    }
    Vector startPos = m_pBotPlayer->GetOrigin(); // SDK Call via wrapper
    unsigned int startAreaId = m_pKnowledgeBase->GetNavGraph()->FindNearestNodeID(startPos);
    unsigned int endAreaId = m_pKnowledgeBase->GetNavGraph()->FindNearestNodeID(targetPos);
    if (startAreaId == 0 || endAreaId == 0) {
        return false;
    }
    if (startAreaId == endAreaId) {
        m_CurrentPath_NavAreaIDs.push_back(startAreaId);
        m_CurrentPathIndex = 0;
        return true;
    }
    m_CurrentPath_NavAreaIDs = m_pPathfinder->FindPath(startAreaId, endAreaId, m_pKnowledgeBase);
    if (!m_CurrentPath_NavAreaIDs.empty()) {
        m_CurrentPathIndex = 0;
        return true;
    }
    return false;
}
bool CFFBaseAI::GetNextPathNodePosition(Vector& outNextNodePos) {
    if (m_CurrentPath_NavAreaIDs.empty() || m_CurrentPathIndex < 0 || (size_t)m_CurrentPathIndex >= m_CurrentPath_NavAreaIDs.size()) {
        return false;
    }
    unsigned int currentNodeNavId = m_CurrentPath_NavAreaIDs[m_CurrentPathIndex];
    const NavAreaNode* pCurrentNodeDef = nullptr;
    if (m_pKnowledgeBase && m_pKnowledgeBase->GetNavGraph()) {
        pCurrentNodeDef = m_pKnowledgeBase->GetNavGraph()->GetNode(currentNodeNavId);
    }
    if (!pCurrentNodeDef) {
        ClearCurrentPath();
        return false;
    }
    outNextNodePos = pCurrentNodeDef->center;

    Vector botPos = m_pBotPlayer ? m_pBotPlayer->GetOrigin() : Vector(0,0,0); // SDK Call
    // Check if bot is close enough to the current path node's center (2D distance check)
    if (botPos.DistToSqr2D(outNextNodePos) < DEFAULT_PATH_NODE_REACHED_TOLERANCE_SQ) {
        m_CurrentPathIndex++;
        if ((size_t)m_CurrentPathIndex >= m_CurrentPath_NavAreaIDs.size()) {
            return false; // Reached end of path list
        }
        // Get next node
        currentNodeNavId = m_CurrentPath_NavAreaIDs[m_CurrentPathIndex];
        if (m_pKnowledgeBase && m_pKnowledgeBase->GetNavGraph()) {
            pCurrentNodeDef = m_pKnowledgeBase->GetNavGraph()->GetNode(currentNodeNavId);
        }
        if (!pCurrentNodeDef) {
            ClearCurrentPath(); return false; // Path error
        }
        outNextNodePos = pCurrentNodeDef->center;
    }
    return true;
}

void CFFBaseAI::ClearCurrentPath() {
    m_CurrentPath_NavAreaIDs.clear();
    m_CurrentPathIndex = -1;
    m_vCurrentMoveToTarget = Vector(0,0,0); // Use SDK Vector constructor if different
}

// AttackTarget is pure virtual.

bool CFFBaseAI::IsTargetInRange(CBaseEntity* pTarget, float range) const {
    if (!pTarget || !m_pBotPlayer) return false;
    // float distSq = m_pBotPlayer->GetOrigin().DistToSqr(pTarget->GetAbsOrigin()); // SDK Calls
    // return distSq < (range * range);
    return true; // Placeholder
}

bool CFFBaseAI::IsFacingTarget(CBaseEntity* pTarget, float fovDegrees) const {
    if (!pTarget || !m_pBotPlayer) return false;
    // Vector toTarget = pTarget->GetAbsOrigin() - m_pBotPlayer->GetEyePosition(); // SDK Calls
    // toTarget.NormalizeInPlace();
    // Vector forward;
    // AngleVectors(m_pBotPlayer->GetEyeAngles(), &forward); // SDK Call or QAngle method
    // return forward.Dot(toTarget) > std::cos(fovDegrees * M_PI / 360.0f); // M_PI from cmath
    return true; // Placeholder
}

void CFFBaseAI::AimAt(const Vector& targetPos, CUserCmd* pCmd) {
    if (!m_pBotPlayer || !pCmd) return;
    // Vector eyePos = m_pBotPlayer->GetEyePosition(); // SDK Call
    // Vector aimDir = targetPos - eyePos;
    // QAngle finalAngles;
    // VectorAngles(aimDir, finalAngles); // SDK Call
    // m_pBotPlayer->SetViewAngles(pCmd, finalAngles); // Uses CFFPlayerWrapper method
}
