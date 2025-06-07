#include "FFBaseAI.h"
#include "ObjectivePlanner.h" // For CObjectivePlanner
// These would be actual engine/game includes
// For now, assume minimal forward declarations or conceptual includes are enough.
// #include "CFFPlayer.h"
// #include "CBaseEntity.h"
// #include "UserCmd.h"
// #include "NavMeshGraph.h"

#include <cmath>              // For fabs, atan2, sqrt etc.
#include <iostream>           // For placeholder logging

// --- CFFBaseAI Implementation ---

// Conceptual placeholders for missing engine types, if not properly included/defined elsewhere
class CFFPlayer {
public:
    Vector GetPosition() const { return Vector(0,0,0); }
    Vector GetEyePosition() const { return Vector(0,0,70); }
    // QAngle GetViewAngles() const { return QAngle(0,0,0); }
    bool IsAlive() const { return true; }
};
class CBaseEntity {
public:
    Vector GetPosition() const { return Vector(100,100,0); }
    Vector GetWorldSpaceCenter() const { return Vector(100,100,32); }
    bool IsAlive() const { return true; }
    int GetTeamNumber() const { return 0; } // 0 for neutral/world
};
struct UserCmd {
    // QAngle viewangles;
    Vector viewangles; // Using vector for simplicity if QAngle not defined
    int buttons = 0;
    float forwardmove = 0.0f;
    float sidemove = 0.0f;
};
class NavMeshGraph { // Minimal placeholder for A* call
public:
    struct NavAreaNodePlaceholder { unsigned int id; Vector center; };
    NavAreaNodePlaceholder* GetNode(unsigned int id) const {
        if (id > 0 && id < 5) { // Allow dummy path up to 4 nodes
             // This is highly problematic for a real system. We need to return a stable pointer.
             // static NavAreaNodePlaceholder tempNode; tempNode.id = id; tempNode.center = Vector(id*100.f, id*50.f, 0); return &tempNode;
             // This static is not safe. For compilation test only.
             return new NavAreaNodePlaceholder{id, Vector(id*100.f, id*50.f, 0)}; // LEAKS for demo
        }
        return nullptr;
    }
    unsigned int GetNavArea(const Vector& pos) const { return 1; } // Dummy
};
const float BOT_REACHED_DISTANCE_THRESHOLD = 48.0f; // Example threshold

CFFBaseAI::CFFBaseAI(CFFPlayer* pBotPlayer, CObjectivePlanner* pPlanner, const BotKnowledgeBase* pKnowledgeBase, const ClassConfigInfo* pClassConfig)
    : m_pBotPlayer(pBotPlayer),
      m_pObjectivePlanner(pPlanner),
      m_pKnowledgeBase(pKnowledgeBase),
      m_pClassConfig(pClassConfig),
      m_pCurrentTarget(nullptr),
      m_CurrentPathIndex(-1) {
    if (!m_pObjectivePlanner) {
        std::cerr << "Error: CFFBaseAI initialized without an ObjectivePlanner!" << std::endl;
    }
    if(!m_pBotPlayer) { // Create a dummy player if null for testing
        // m_pBotPlayer = new CFFPlayer(); // This would leak, just for conceptual testing
    }
}

CFFBaseAI::~CFFBaseAI() {
    // if m_pBotPlayer was newed in constructor for testing, delete it.
}

void CFFBaseAI::Update(UserCmd* pCmd) {
    if (!m_pObjectivePlanner || (m_pBotPlayer && !m_pBotPlayer->IsAlive())) {
        return;
    }

    m_pObjectivePlanner->EvaluateAndSelectTask(); // Let planner determine HLT and current SubTask
    const SubTask* pCurrentSubTask = m_pObjectivePlanner->GetCurrentSubTask();

    if (pCurrentSubTask && !pCurrentSubTask->isCompleted) {
        // std::cout << "Bot executing subtask: " << static_cast<int>(pCurrentSubTask->type) << std::endl;
        bool subTaskIsStillOngoing = ExecuteSubTask(pCurrentSubTask, pCmd);

        if (!subTaskIsStillOngoing) { // AI module indicates task is done or failed
            // The subtask's isCompleted flag should have been set by ExecuteSubTask if successful
            if (pCurrentSubTask->isCompleted) {
                m_pObjectivePlanner->OnSubTaskCompleted();
            } else {
                m_pObjectivePlanner->OnSubTaskFailed();
            }
        }
    } else if (pCurrentSubTask && pCurrentSubTask->isCompleted) {
         m_pObjectivePlanner->OnSubTaskCompleted(); // Ensure planner advances
    } else {
        // No current subtask from planner - AI can idle or perform default actions
        // Example: Look around, or if m_pCurrentTarget is set from a recent event, consider it.
        if (m_pCurrentTarget && m_pCurrentTarget->IsAlive()) {
            AttackTarget(m_pCurrentTarget, pCmd);
        }
    }
}

bool CFFBaseAI::ExecuteSubTask(const SubTask* pSubTask, UserCmd* pCmd) {
    if (!pSubTask) return false;

    bool isOngoing = true;

    switch (pSubTask->type) {
        case SubTaskType::MOVE_TO_POSITION: {
            bool targetPositionChanged = (m_CurrentPathTargetPos.x != pSubTask->targetPosition.x ||
                                          m_CurrentPathTargetPos.y != pSubTask->targetPosition.y ||
                                          m_CurrentPathTargetPos.z != pSubTask->targetPosition.z);
            if (m_CurrentPath.empty() || targetPositionChanged) {
                if (!MoveTo(pSubTask->targetPosition, pCmd)) { // This calls FindPath
                    isOngoing = false; // Pathfinding failed
                    break;
                }
            }
            isOngoing = FollowPath(pCmd);
            if (!isOngoing) ((SubTask*)pSubTask)->isCompleted = true; // Path finished
            break;
        }
        case SubTaskType::MOVE_TO_ENTITY:
            if (pSubTask->pTargetEntity && pSubTask->pTargetEntity->IsAlive()) {
                 bool targetPositionChanged = (m_CurrentPathTargetPos.x != pSubTask->pTargetEntity->GetPosition().x ||
                                              m_CurrentPathTargetPos.y != pSubTask->pTargetEntity->GetPosition().y ||
                                              m_CurrentPathTargetPos.z != pSubTask->pTargetEntity->GetPosition().z);
                if (m_CurrentPath.empty() || targetPositionChanged ) {
                     if(!MoveTo(pSubTask->pTargetEntity->GetPosition(), pCmd)){
                        isOngoing = false; break;
                     }
                }
                isOngoing = FollowPath(pCmd);
                if (!isOngoing) ((SubTask*)pSubTask)->isCompleted = true; // Path finished
            } else {
                isOngoing = false; // Target entity lost or dead
            }
            break;

        case SubTaskType::ATTACK_TARGET:
            if (pSubTask->pTargetEntity && pSubTask->pTargetEntity->IsAlive()) {
                m_pCurrentTarget = pSubTask->pTargetEntity;
                isOngoing = AttackTarget(m_pCurrentTarget, pCmd);
                if (!m_pCurrentTarget->IsAlive()) { // Target died
                    isOngoing = false; ((SubTask*)pSubTask)->isCompleted = true;
                }
            } else {
                 m_pCurrentTarget = nullptr;
                isOngoing = false;
            }
            break;

        case SubTaskType::CAPTURE_OBJECTIVE: {
            // Placeholder: If close enough to target position, consider it done.
            // Real capture logic is game-specific.
            Vector currentPos = m_pBotPlayer ? m_pBotPlayer->GetPosition() : Vector(0,0,0);
            float distSq = (currentPos.x - pSubTask->targetPosition.x) * (currentPos.x - pSubTask->targetPosition.x) +
                           (currentPos.y - pSubTask->targetPosition.y) * (currentPos.y - pSubTask->targetPosition.y);
            if (distSq < (BOT_REACHED_DISTANCE_THRESHOLD * BOT_REACHED_DISTANCE_THRESHOLD)) {
               std::cout << "Conceptual: Reached CAPTURE_OBJECTIVE at " << pSubTask->targetPosition.x << std::endl;
               isOngoing = false;
               ((SubTask*)pSubTask)->isCompleted = true;
            } else { // Need to move closer
               bool targetPositionChanged = (m_CurrentPathTargetPos.x != pSubTask->targetPosition.x ||
                                             m_CurrentPathTargetPos.y != pSubTask->targetPosition.y ||
                                             m_CurrentPathTargetPos.z != pSubTask->targetPosition.z);
                if (m_CurrentPath.empty() || targetPositionChanged) {
                    if (!MoveTo(pSubTask->targetPosition, pCmd)) { isOngoing = false; break;}
                }
                isOngoing = FollowPath(pCmd);
                // If FollowPath itself returns false (path ended), and we are not yet at capture radius, then task failed.
                if(!isOngoing && distSq >= (BOT_REACHED_DISTANCE_THRESHOLD * BOT_REACHED_DISTANCE_THRESHOLD) ){
                     ((SubTask*)pSubTask)->isCompleted = false; // ensure it's marked as failed
                } else if (!isOngoing && distSq < (BOT_REACHED_DISTANCE_THRESHOLD * BOT_REACHED_DISTANCE_THRESHOLD)) {
                     ((SubTask*)pSubTask)->isCompleted = true; // successfully reached
                }
            }
            break;
        }
        default:
            std::cout << "Warning: ExecuteSubTask - Unknown SubTaskType: " << static_cast<int>(pSubTask->type) << std::endl;
            isOngoing = false;
            break;
    }
    return isOngoing;
}


bool CFFBaseAI::MoveTo(const Vector& targetPos, UserCmd* pCmd) {
    Vector currentPos = m_pBotPlayer ? m_pBotPlayer->GetPosition() : Vector(0,0,0);
    m_CurrentPath.clear();
    m_CurrentPathIndex = -1;
    m_CurrentPathTargetPos = targetPos;

    if (FindPath(currentPos, targetPos, m_CurrentPath)) {
        if (!m_CurrentPath.empty()) {
            m_CurrentPathIndex = 0;
            // std::cout << "Path found. Nodes: " << m_CurrentPath.size() << ". Target: " << targetPos.x << std::endl;
            return true;
        }
    }
    // std::cerr << "CFFBaseAI: Pathfinding failed from (" << currentPos.x << ") to (" << targetPos.x << ")" << std::endl;
    return false;
}

bool CFFBaseAI::FollowPath(UserCmd* pCmd) {
    if (m_CurrentPath.empty() || m_CurrentPathIndex < 0 ) {
        return false;
    }
    if (m_CurrentPathIndex >= m_CurrentPath.size()){ // Already at the end of the path
        m_CurrentPath.clear(); // Clear path as we've "finished" it
        m_CurrentPathIndex = -1;
        return false;
    }


    Vector nextNodePosInGraph;
    const NavMeshGraph::NavAreaNodePlaceholder* areaNode = nullptr; // Using placeholder type
    if (m_pKnowledgeBase && m_pKnowledgeBase->navMesh) {
        areaNode = m_pKnowledgeBase->navMesh->GetNode(m_CurrentPath[m_CurrentPathIndex]);
    }

    if (!areaNode) {
        std::cerr << "FollowPath: Invalid node ID in path: " << m_CurrentPath[m_CurrentPathIndex] << std::endl;
        m_CurrentPath.clear(); m_CurrentPathIndex = -1; return false;
    }
    nextNodePosInGraph = areaNode->center;
    // delete areaNode; // Clean up placeholder if newed in GetNode (THIS IS BAD DESIGN FOR GetNode)

    Vector currentPos = m_pBotPlayer ? m_pBotPlayer->GetPosition() : Vector(0,0,0);
    AimAt(nextNodePosInGraph, pCmd);

    float distToNextNodeSq = (currentPos.x - nextNodePosInGraph.x)*(currentPos.x - nextNodePosInGraph.x) +
                             (currentPos.y - nextNodePosInGraph.y)*(currentPos.y - nextNodePosInGraph.y);

    if (distToNextNodeSq < (BOT_REACHED_DISTANCE_THRESHOLD * BOT_REACHED_DISTANCE_THRESHOLD)) {
        m_CurrentPathIndex++;
        if (m_CurrentPathIndex >= m_CurrentPath.size()) {
            m_CurrentPath.clear();
            m_CurrentPathIndex = -1;
            // std::cout << "Reached end of path." << std::endl;
            return false; // Reached end of path
        }
        // Update areaNode and nextNodePosInGraph for the new current path index
        if (m_pKnowledgeBase && m_pKnowledgeBase->navMesh) {
             areaNode = m_pKnowledgeBase->navMesh->GetNode(m_CurrentPath[m_CurrentPathIndex]);
        }
        if (!areaNode) {
             std::cerr << "FollowPath: Invalid node ID after advancing: " << m_CurrentPath[m_CurrentPathIndex] << std::endl;
             m_CurrentPath.clear(); m_CurrentPathIndex = -1; return false;
        }
        nextNodePosInGraph = areaNode->center;
        // delete areaNode; // BAD
        AimAt(nextNodePosInGraph, pCmd); // Re-aim at the new current node
    }

    pCmd->forwardmove = 450;
    // std::cout << "Following path to node " << m_CurrentPath[m_CurrentPathIndex] << " at " << nextNodePosInGraph.x << std::endl;
    return true;
}

bool CFFBaseAI::GetNextPathPosition(Vector& nextPos) {
    // This function is somewhat redundant if FollowPath directly uses m_CurrentPath[m_CurrentPathIndex]
    // and checks proximity. Let's simplify: FollowPath handles current node logic.
    // This function might just peek at the current target node's position.
    if (m_CurrentPath.empty() || m_CurrentPathIndex < 0 || m_CurrentPathIndex >= m_CurrentPath.size()) {
        return false;
    }
    if (m_pKnowledgeBase && m_pKnowledgeBase->navMesh) {
        const NavMeshGraph::NavAreaNodePlaceholder* areaNode = m_pKnowledgeBase->navMesh->GetNode(m_CurrentPath[m_CurrentPathIndex]);
        if (areaNode) {
            nextPos = areaNode->center;
            // delete areaNode; // BAD
            return true;
        }
    }
    return false;
}


bool CFFBaseAI::AttackTarget(CBaseEntity* pTarget, UserCmd* pCmd) {
    if (!pTarget || (m_pBotPlayer && !m_pBotPlayer->IsAlive()) ) {
        m_pCurrentTarget = nullptr;
        return false;
    }
    // Conceptual: if (!m_pBotPlayer->HasLOS(pTarget)) return false;

    m_pCurrentTarget = pTarget;
    AimAt(pTarget->GetWorldSpaceCenter(), pCmd);
    pCmd->buttons |= 1; // IN_ATTACK (assuming 1 is IN_ATTACK from engine defs)
    // std::cout << "Attacking target!" << std::endl;
    return true;
}

bool CFFBaseAI::UseAbility(int abilitySlot, CBaseEntity* pTarget, UserCmd* pCmd) {
    std::cout << "CFFBaseAI: Using ability slot " << abilitySlot << " (Placeholder)" << std::endl;
    // Example: pCmd->buttons |= (1 << (abilitySlot + 4)); // e.g. IN_ATTACK2 if slot 0 -> button 5 (IN_ATTACK2)
    return false;
}

bool CFFBaseAI::IsTargetInRange(CBaseEntity* pTarget, float range) const {
    if (!pTarget || !m_pBotPlayer) return false;
    Vector currentPos = m_pBotPlayer->GetPosition();
    Vector targetPos = pTarget->GetPosition();
    float distSq = (currentPos.x - targetPos.x)*(currentPos.x - targetPos.x) + (currentPos.y - targetPos.y)*(currentPos.y - targetPos.y) + (currentPos.z - targetPos.z)*(currentPos.z - targetPos.z);
    return distSq < (range * range);
}

bool CFFBaseAI::IsFacingTarget(CBaseEntity* pTarget, float fovDegrees) const {
    if (!pTarget || !m_pBotPlayer) return false;
    // Conceptual - requires proper QAngle, AngleVectors, Vector math from SDK
    // Vector botForward;
    // AngleVectors(m_pBotPlayer->GetViewAngles(), &botForward, nullptr, nullptr);
    // Vector dirToTarget = (pTarget->GetPosition() - m_pBotPlayer->GetPosition());
    // dirToTarget.z = 0; dirToTarget.NormalizeInPlace(); // Normalize for dot product, ignore Z for 2D FOV
    // botForward.z = 0; botForward.NormalizeInPlace();
    // return botForward.Dot(dirToTarget) > std::cos(fovDegrees * 0.5f * 3.14159265f / 180.0f);
    return true; // Placeholder
}

void CFFBaseAI::AimAt(const Vector& targetPos, UserCmd* pCmd) {
    if (!m_pBotPlayer) return;
    Vector eyePos = m_pBotPlayer->GetEyePosition();
    Vector aimDir = targetPos; // Simple subtraction assuming Vector has operator-
    aimDir.x -= eyePos.x;
    aimDir.y -= eyePos.y;
    aimDir.z -= eyePos.z;

    // Normalize (conceptual)
    float len = std::sqrt(aimDir.x*aimDir.x + aimDir.y*aimDir.y + aimDir.z*aimDir.z);
    if (len > 0) { aimDir.x /= len; aimDir.y /= len; aimDir.z /= len; }

    // VectorToAngles (conceptual)
    float yaw = std::atan2(aimDir.y, aimDir.x) * (180.0f / 3.14159265f);
    float pitch = std::atan2(-aimDir.z, std::sqrt(aimDir.x*aimDir.x + aimDir.y*aimDir.y)) * (180.0f / 3.14159265f);

    pCmd->viewangles.x = pitch;
    pCmd->viewangles.y = yaw;
    pCmd->viewangles.z = 0; // Roll
    // std::cout << "Aiming at (" << targetPos.x << ", " << targetPos.y << ", " << targetPos.z << ") -> Angles (" << pitch << "," << yaw << ")" << std::endl;
}

bool CFFBaseAI::FindPath(const Vector& startPos, const Vector& endPos, std::vector<unsigned int>& outPath) {
    if (!m_pKnowledgeBase || !m_pKnowledgeBase->navMesh) {
        std::cerr << "FindPath: No NavMesh available in KnowledgeBase!" << std::endl;
        return false;
    }
    // Conceptual: This would call a proper AStarPathfinder instance.
    // AStarPathfinder pathfinder(m_pKnowledgeBase->navMesh);
    // unsigned int startAreaId = m_pKnowledgeBase->navMesh->GetNavArea(startPos); // GetNavArea is conceptual
    // unsigned int endAreaId = m_pKnowledgeBase->navMesh->GetNavArea(endPos);
    // if (startAreaId == 0 || endAreaId == 0) { // Assuming 0 is an invalid area ID placeholder
    //     std::cerr << "FindPath: Invalid start or end area." << std::endl;
    //     return false;
    // }
    // outPath = pathfinder.FindPath(startAreaId, endAreaId); // This is the actual call

    // Simplified placeholder path for testing structure
    // std::cout << "CFFBaseAI::FindPath called from (" << startPos.x << ") to (" << endPos.x << ")" << std::endl;
    outPath.clear();
    outPath.push_back(1);
    outPath.push_back(2);
    outPath.push_back(3); // Represents IDs of nav areas
    outPath.push_back(4); // Represents IDs of nav areas
    // std::cout << " Found dummy path with " << outPath.size() << " nodes." << std::endl;
    return !outPath.empty();
}
