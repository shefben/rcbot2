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

// --- Conceptual Placeholder Types ---
#ifndef CUSERCMD_CONCEPTUAL_DEF_FFBASEAI_CPP
#define CUSERCMD_CONCEPTUAL_DEF_FFBASEAI_CPP
struct CUserCmd { int buttons = 0; float forwardmove = 0.0f; float sidemove = 0.0f; Vector viewangles; int weaponselect = 0; };
#endif
#ifndef CBASEENTITY_CONCEPTUAL_DEF_FFBASEAI_CPP
#define CBASEENTITY_CONCEPTUAL_DEF_FFBASEAI_CPP
class CBaseEntity {
public:
    virtual ~CBaseEntity() {}
    Vector GetPosition() const { return Vector(0,0,0); }
    Vector GetWorldSpaceCenter() const { return Vector(0,0,32); }
    Vector GetAbsVelocity() const { return Vector(0,0,0); }
    bool IsAlive() const { return true; }
    int GetTeamNumber() const { return 0; }
};
#endif
#ifndef CFFPLAYER_CONCEPTUAL_DEF_FFBASEAI_CPP
#define CFFPLAYER_CONCEPTUAL_DEF_FFBASEAI_CPP
class CFFPlayer { public: CFFPlayer(edict_t* ed=nullptr) : m_pEdict(ed) {} edict_t* GetEdict() const { return m_pEdict; } bool IsValid() const {return m_pEdict != nullptr;} bool IsAlive() const { return true; } Vector GetPosition() const { return m_CurrentPosition_placeholder; } Vector GetEyePosition() const { return Vector(m_CurrentPosition_placeholder.x, m_CurrentPosition_placeholder.y, m_CurrentPosition_placeholder.z + 64); } Vector GetViewAngles() const { return m_CurrentViewAngles_placeholder; } void SetViewAngles(const Vector& ang) { m_CurrentViewAngles_placeholder = ang; } int GetTeamNumber() const { return 1; } int GetFlags() const { return (1 << 0); } Vector m_CurrentPosition_placeholder; Vector m_CurrentViewAngles_placeholder; private: edict_t* m_pEdict; };
#endif

const float DEFAULT_ARRIVAL_TOLERANCE = 48.0f;
const float DEFAULT_PATH_NODE_REACHED_TOLERANCE_SQ = 64.0f * 64.0f;
// --- End Conceptual Placeholders ---


CFFBaseAI::CFFBaseAI(CFFPlayer* pBotPlayer, CObjectivePlanner* pPlanner,
                   const BotKnowledgeBase* pKnowledgeBase, const ClassConfigInfo* pClassConfig)
    : m_pBotPlayer(pBotPlayer),
      m_pObjectivePlanner(pPlanner),
      m_pKnowledgeBase(pKnowledgeBase),
      m_pClassConfig(pClassConfig),
      m_pCurrentTarget(nullptr),
      m_CurrentPathIndex(-1),
      m_pPathfinder(nullptr)
{
    if (!m_pObjectivePlanner) {
        std::cerr << "CFFBaseAI Error: CObjectivePlanner is NULL!" << std::endl;
    }
    if (!m_pBotPlayer) {
        std::cerr << "CFFBaseAI Warning: CFFPlayer is NULL during construction." << std::endl;
    }
    if (m_pKnowledgeBase && m_pKnowledgeBase->GetNavGraph()) {
        m_pPathfinder = std::make_unique<AStarPathfinder>(m_pKnowledgeBase->GetNavGraph());
    } else {
        std::cerr << "CFFBaseAI Error: No NavGraph available in KnowledgeBase for Pathfinder initialization!" << std::endl;
    }
}

CFFBaseAI::~CFFBaseAI() {}

void CFFBaseAI::Update(CUserCmd* pCmd) {
    if (!pCmd) return;
    pCmd->buttons = 0; pCmd->forwardmove = 0.0f; pCmd->sidemove = 0.0f;

    if (!m_pObjectivePlanner || !m_pBotPlayer || !m_pBotPlayer->IsValid() || !m_pBotPlayer->IsAlive()) {
        return;
    }
    m_pObjectivePlanner->EvaluateAndSelectTask();

    // Get a pointer to the current subtask. This pointer may become dangling if the planner resets the HLT.
    // So, we need to be careful or copy the necessary info if planner can change HLT mid-execution.
    // For now, assume planner->OnSubTaskOutcomeReported is the main path for HLT change.
    const SubTask* pConstCurrentSubTask = m_pObjectivePlanner->GetCurrentSubTask();

    if (pConstCurrentSubTask && !pConstCurrentSubTask->isCompleted) {
        // Make a copy for safety if ExecuteSubTask or subsequent calls might change the HLT in planner
        // SubTask currentSubTaskCopy = *pConstCurrentSubTask;
        // bool subTaskIsStillOngoing = ExecuteSubTask(&currentSubTaskCopy, pCmd);
        // For now, pass the const pointer and trust ExecuteSubTask to use it carefully.
        // ExecuteSubTask will need to get a mutable version from planner if it needs to set isCompleted.

        bool subTaskIsStillOngoing = ExecuteSubTask(pConstCurrentSubTask, pCmd);

        // After execution, get the subtask state again as ExecuteSubTask might have marked it complete.
        SubTask* pMutableCurrentSubTask = m_pObjectivePlanner->GetCurrentSubTaskMutable();

        if (!subTaskIsStillOngoing) {
            // SubTask has finished (either success or failure this frame)
            // The SubTask's isCompleted flag should be set by ExecuteSubTask or its callees if successful.
            float duration = 0.0f;
            if (pMutableCurrentSubTask && pMutableCurrentSubTask->startTime.time_since_epoch().count() != 0) {
                duration = std::chrono::duration<float>(
                                std::chrono::system_clock::now() - pMutableCurrentSubTask->startTime).count();
            }

            // Report outcome to planner
            if (pMutableCurrentSubTask) { // Check if it's still valid after execution
                 m_pObjectivePlanner->OnSubTaskOutcomeReported(pMutableCurrentSubTask->isCompleted,
                                                              pMutableCurrentSubTask->isCompleted ? "" : "Subtask_Failed_Or_Not_Marked_Complete_By_AI");
            } else { // Subtask became null, means HLT was probably reset by ExecuteSubTask itself (e.g. via failing)
                 // This case might indicate an HLT failure already processed.
            }
        }
    } else if (pConstCurrentSubTask && pConstCurrentSubTask->isCompleted) {
         // This subtask was already completed, tell planner to advance or re-evaluate.
         // This path ensures that if a task completes and planner doesn't immediately switch, it gets notified again.
         m_pObjectivePlanner->OnSubTaskOutcomeReported(true, "Previously_Completed");
    } else {
        // No current subtask from planner (HLT might be NONE or have no subtasks)
        if (m_pCurrentTarget && m_pCurrentTarget->IsAlive()) {
            // AttackTarget(m_pCurrentTarget, pCmd); // Base AttackTarget is pure virtual
        } else {
            // Idle: look forward
            if (m_pBotPlayer) AimAt(m_pBotPlayer->GetPosition() + Vector(100,0,0), pCmd);
        }
    }
}

bool CFFBaseAI::ExecuteSubTask(const SubTask* pSubTask, CUserCmd* pCmd) {
    if (!pSubTask || !m_pBotPlayer || !pCmd) return false;
    bool isOngoing = true;
    SubTask* pMutableSubTask = m_pObjectivePlanner->GetCurrentSubTaskMutable(); // For setting isCompleted

    // Ensure mutable subtask is valid and matches the const one being processed
    if (!pMutableSubTask || pMutableSubTask->type != pSubTask->type) {
        // This might happen if the HLT was changed by an interrupt or other logic
        // between CFFBaseAI::Update getting pConstCurrentSubTask and this call.
        // Or if pConstCurrentSubTask was the last subtask and planner reset the HLT.
        return false; // Cannot operate if mutable version is gone or different
    }


    switch (pSubTask->type) {
        case SubTaskType::MOVE_TO_POSITION:
        case SubTaskType::MOVE_TO_ENTITY: {
            Vector targetPos = (pSubTask->type == SubTaskType::MOVE_TO_ENTITY && pSubTask->pTargetEntity) ?
                               pSubTask->pTargetEntity->GetPosition() : pSubTask->targetPosition;

            bool targetPositionChangedSignificantly = true;
            if (!m_CurrentPath_NavAreaIDs.empty()) {
                float dx = m_vCurrentMoveToTarget.x - targetPos.x;
                float dy = m_vCurrentMoveToTarget.y - targetPos.y;
                float dz = m_vCurrentMoveToTarget.z - targetPos.z;
                targetPositionChangedSignificantly = (dx*dx + dy*dy + dz*dz > 10.0f*10.0f);
            }
            if (m_CurrentPath_NavAreaIDs.empty() || targetPositionChangedSignificantly) {
                if (!MoveTo(targetPos, pCmd)) {
                    isOngoing = false; pMutableSubTask->isCompleted = false; break;
                }
            }
            isOngoing = FollowPath(pCmd);
            if (!isOngoing) pMutableSubTask->isCompleted = true;
            break;
        }
        case SubTaskType::ATTACK_TARGET:
            if (pSubTask->pTargetEntity && pSubTask->pTargetEntity->IsAlive()) {
                isOngoing = AttackTarget(pSubTask->pTargetEntity, pCmd);
                if (pSubTask->pTargetEntity && !pSubTask->pTargetEntity->IsAlive()) {
                    isOngoing = false; pMutableSubTask->isCompleted = true;
                } else if (!isOngoing) { // Attack action itself finished (e.g. one shot weapon, or decided to stop)
                     pMutableSubTask->isCompleted = false; // Or true if objective of attack met
                }
            } else {
                isOngoing = false;
                pMutableSubTask->isCompleted = true;
            }
            break;

        case SubTaskType::CAPTURE_OBJECTIVE:
        case SubTaskType::STAND_ON_POINT: {
            // Use radiusParam from SubTask for capture/stand radius
            float standRadiusSq = pSubTask->radiusParam * pSubTask->radiusParam;
            if (standRadiusSq <= 0.01f) standRadiusSq = DEFAULT_ARRIVAL_TOLERANCE * DEFAULT_ARRIVAL_TOLERANCE; // Fallback

            Vector currentPos = m_pBotPlayer->GetPosition();
            float distSq = (currentPos.x - pSubTask->targetPosition.x) * (currentPos.x - pSubTask->targetPosition.x) +
                           (currentPos.y - pSubTask->targetPosition.y) * (currentPos.y - pSubTask->targetPosition.y);

            if (distSq < standRadiusSq) {
                AimAt(m_pBotPlayer->GetPosition() + Vector(100,0,0), pCmd);
                // Conceptual: if (IsObjectiveCaptured_Conceptual(pSubTask->pTargetEntity, m_pBotPlayer->GetTeam())) {
                //    pMutableSubTask->isCompleted = true; isOngoing = false; break;
                // }
                // Check duration for STAND_ON_POINT or timed CAPTURE_OBJECTIVE
                if (pSubTask->desiredDuration > 0.0f &&
                    pSubTask->startTime.time_since_epoch().count() != 0 &&
                    std::chrono::duration<float>(std::chrono::system_clock::now() - pSubTask->startTime).count() > pSubTask->desiredDuration) {
                    pMutableSubTask->isCompleted = true; // Duration met
                    isOngoing = false;
                } else {
                    isOngoing = true; // On point, continue task (e.g. waiting for cap or duration)
                }
            } else {
                bool targetPosChanged = (m_vCurrentMoveToTarget.x != pSubTask->targetPosition.x || m_vCurrentMoveToTarget.y != pSubTask->targetPosition.y);
                if (m_CurrentPath_NavAreaIDs.empty() || targetPosChanged) {
                    if (!MoveTo(pSubTask->targetPosition, pCmd)) { isOngoing = false; pMutableSubTask->isCompleted = false; break;}
                }
                isOngoing = FollowPath(pCmd);
                if(!isOngoing) { // Path ended
                    // Check again if now at point, otherwise movement failed to reach
                    distSq = (m_pBotPlayer->GetPosition().x - pSubTask->targetPosition.x) * (m_pBotPlayer->GetPosition().x - pSubTask->targetPosition.x) +
                             (m_pBotPlayer->GetPosition().y - pSubTask->targetPosition.y) * (m_pBotPlayer->GetPosition().y - pSubTask->targetPosition.y);
                    pMutableSubTask->isCompleted = (distSq < standRadiusSq);
                }
            }
            break;
        }
        case SubTaskType::USE_ABILITY_ON_TARGET:
        case SubTaskType::USE_ABILITY_AT_POSITION:
            isOngoing = UseAbility(pSubTask->abilitySlot, pSubTask->pTargetEntity, pCmd);
            if (!isOngoing) pMutableSubTask->isCompleted = true;
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
                // Conceptual: Scan for enemies, attack if found. If not, look around.
                // CBaseEntity* enemy = SelectTarget(); // Could use a specific scan for area
                // if(enemy) AttackTarget(enemy, pCmd); else AimAt(m_pBotPlayer->GetPosition() + Vector(0,100,0), pCmd);
                isOngoing = true; // Assume ongoing until duration
            }
            break;
        }
        default:
            isOngoing = false;
            pMutableSubTask->isCompleted = false; // Mark as failed if unknown
            break;
    }
    return isOngoing;
}

// ... (MoveTo, FollowPath, PlanPathTo, GetNextPathNodePosition, ClearCurrentPath, AttackTarget, IsTargetInRange, IsFacingTarget, AimAt remain largely the same as Task 11, Step 2)
// Ensure placeholder CFFPlayer methods like GetOrigin() are used consistently.
bool CFFBaseAI::MoveTo(const Vector& targetPos, CUserCmd* pCmd) { /* ... unchanged ... */
    ClearCurrentPath();
    m_vCurrentMoveToTarget = targetPos;
    if (!PlanPathTo(targetPos)) {
        return false;
    }
    return !m_CurrentPath_NavAreaIDs.empty();
}
bool CFFBaseAI::FollowPath(CUserCmd* pCmd) { /* ... unchanged ... */
    if (!m_pBotPlayer || !pCmd) return false;
    if (m_CurrentPath_NavAreaIDs.empty() || m_CurrentPathIndex < 0 ) {
        return false;
    }
    Vector botOrigin = m_pBotPlayer->GetPosition();
    float distToFinalTargetSq = (botOrigin.x - m_vCurrentMoveToTarget.x)*(botOrigin.x - m_vCurrentMoveToTarget.x) +
                                (botOrigin.y - m_vCurrentMoveToTarget.y)*(botOrigin.y - m_vCurrentMoveToTarget.y);
    if (distToFinalTargetSq < DEFAULT_ARRIVAL_TOLERANCE * DEFAULT_ARRIVAL_TOLERANCE) {
        ClearCurrentPath();
        return false;
    }
    Vector nextNodeWorldPos;
    if (!GetNextPathNodePosition(nextNodeWorldPos)) {
        ClearCurrentPath();
        return false;
    }
    AimAt(nextNodeWorldPos, pCmd);
    pCmd->forwardmove = 400;
    pCmd->sidemove = 0;
    return true;
}
bool CFFBaseAI::PlanPathTo(const Vector& targetPos) { /* ... unchanged ... */
    ClearCurrentPath();
    if (!m_pPathfinder || !m_pBotPlayer || !m_pKnowledgeBase || !m_pKnowledgeBase->GetNavGraph()) {
        return false;
    }
    Vector startPos = m_pBotPlayer->GetPosition();
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
bool CFFBaseAI::GetNextPathNodePosition(Vector& outNextNodePos) { /* ... unchanged ... */
    if (m_CurrentPath_NavAreaIDs.empty() || m_CurrentPathIndex < 0 || m_CurrentPathIndex >= m_CurrentPath_NavAreaIDs.size()) {
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
    Vector botPos = m_pBotPlayer ? m_pBotPlayer->GetPosition() : Vector(0,0,0);
    float dx = botPos.x - outNextNodePos.x;
    float dy = botPos.y - outNextNodePos.y;
    if ((dx*dx + dy*dy) < DEFAULT_PATH_NODE_REACHED_TOLERANCE_SQ) {
        m_CurrentPathIndex++;
        if (m_CurrentPathIndex >= m_CurrentPath_NavAreaIDs.size()) {
            return false;
        }
        currentNodeNavId = m_CurrentPath_NavAreaIDs[m_CurrentPathIndex];
        if (m_pKnowledgeBase && m_pKnowledgeBase->GetNavGraph()) {
            pCurrentNodeDef = m_pKnowledgeBase->GetNavGraph()->GetNode(currentNodeNavId);
        }
        if (!pCurrentNodeDef) {
            ClearCurrentPath(); return false;
        }
        outNextNodePos = pCurrentNodeDef->center;
    }
    return true;
}
void CFFBaseAI::ClearCurrentPath() { /* ... unchanged ... */
    m_CurrentPath_NavAreaIDs.clear();
    m_CurrentPathIndex = -1;
    m_vCurrentMoveToTarget = Vector(0,0,0);
}
bool CFFBaseAI::AttackTarget(CBaseEntity* pTarget, CUserCmd* pCmd) { /* ... unchanged, pure virtual ... */ return false;}
bool CFFBaseAI::IsTargetInRange(CBaseEntity* pTarget, float range) const { /* ... unchanged ... */ return true; }
bool CFFBaseAI::IsFacingTarget(CBaseEntity* pTarget, float fovDegrees) const { /* ... unchanged ... */ return true; }
void CFFBaseAI::AimAt(const Vector& targetPos, CUserCmd* pCmd) { /* ... unchanged ... */ }
