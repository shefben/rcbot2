#include "FFBaseAI.h"
#include "ObjectivePlanner.h" // For CObjectivePlanner and SubTask (assuming SubTask is in BotTasks.h included by ObjectivePlanner.h)
#include "BotTasks.h"         // Explicitly include for SubTask definition
#include "FFStateStructs.h"   // For Vector, ClassConfigInfo etc.

// Conceptual includes - these would be actual engine/game SDK headers
#include <iostream> // For placeholder logging
#include <cmath>    // For fabs, atan2, sqrt, M_PI etc.

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// --- Conceptual Placeholder Types (if not properly included from SDK) ---
// These should ideally be defined once in a common "conceptual_sdk.h" or similar
// if they are used across multiple cpp files without real SDK headers.
// For this task, assume they are sufficiently defined for compilation.

#ifndef CUSERCMD_CONCEPTUAL_DEF_FFBASEAI // Prevent redefinition if included elsewhere
#define CUSERCMD_CONCEPTUAL_DEF_FFBASEAI
struct CUserCmd {
    int buttons = 0;
    float forwardmove = 0.0f;
    float sidemove = 0.0f;
    Vector viewangles;
    int weaponselect = 0;
    // Add other fields like impulse, mousedx, mousedy if your AI logic uses them.
};
#endif

#ifndef CBASEENTITY_CONCEPTUAL_DEF_FFBASEAI
#define CBASEENTITY_CONCEPTUAL_DEF_FFBASEAI
class CBaseEntity {
public:
    virtual ~CBaseEntity() {}
    Vector GetPosition() const { return Vector(100,100,0); } // Dummy position
    Vector GetWorldSpaceCenter() const { return Vector(100,100,32); } // Dummy center
    Vector GetAbsVelocity() const { return Vector(0,0,0); } // Dummy velocity
    bool IsAlive() const { return true; }
    int GetTeamNumber() const { return 0; } // 0 for neutral/world
    // virtual std::string GetClassName() const { return "unknown_entity"; }
    // virtual bool IsPlayer() const { return false; }
    // virtual bool IsBuilding() const { return false; }
};
#endif

#ifndef CFFPLAYER_CONCEPTUAL_DEF_FFBASEAI
#define CFFPLAYER_CONCEPTUAL_DEF_FFBASEAI
class CFFPlayer { // Conceptual wrapper for the bot's game entity
public:
    CFFPlayer(edict_t* ed) : m_pEdict(ed) {} // edict_t would be engine specific
    edict_t* GetEdict() const { return m_pEdict; }
    bool IsAlive() const { return true; }
    Vector GetPosition() const { return Vector(10,10,10); } // Dummy position
    Vector GetEyePosition() const { return Vector(10,10,74); } // Dummy eye position
    Vector GetViewAngles() const { return Vector(0,0,0); } // Dummy view angles (pitch, yaw, roll)
    void SetViewAngles(const Vector& ang) { /* Sets bot's view */ }
    int GetTeamNumber() const { return 1; } // Dummy team
    int GetFlags() const { return (1<<0); } // FL_ONGROUND
    // ... other methods to get health, ammo, active weapon, velocity etc.
private:
    edict_t* m_pEdict;
};
#endif

// Conceptual NavMesh related constants and classes (minimal for FindPath placeholder)
#define INVALID_NAV_AREA_ID 0
class NavMeshGraph {
public:
    struct NavAreaDef { unsigned int id; Vector center; };
    const NavAreaDef* GetNavAreaByID(unsigned int id) const {
        // static NavAreaDef dummyArea; dummyArea.id = id; dummyArea.center = Vector(id*10.f, id*10.f, 0); return &dummyArea;
        if (id > 0 && id < 5) return new NavAreaDef{id, Vector(id*100.f, id*50.f, 0)}; // LEAKS, for demo only
        return nullptr;
    }
    unsigned int GetNearestNavAreaID(const Vector& pos) const { return 1; } // Dummy
};
// --- End Conceptual Placeholders ---


CFFBaseAI::CFFBaseAI(CFFPlayer* pBotPlayer, CObjectivePlanner* pPlanner,
                   const BotKnowledgeBase* pKnowledgeBase, const ClassConfigInfo* pClassConfig)
    : m_pBotPlayer(pBotPlayer),
      m_pObjectivePlanner(pPlanner),
      m_pKnowledgeBase(pKnowledgeBase),
      m_pClassConfig(pClassConfig),
      m_pCurrentTarget(nullptr),
      m_CurrentPathIndex(-1) {
    if (!m_pObjectivePlanner) {
        std::cerr << "CFFBaseAI Error: CObjectivePlanner is NULL!" << std::endl;
        // Handle error, perhaps throw or set an invalid state
    }
     if (!m_pBotPlayer) { // This is critical for most operations
        std::cerr << "CFFBaseAI Error: CFFPlayer is NULL!" << std::endl;
    }
    // std::cout << "CFFBaseAI Initialized." << std::endl;
}

CFFBaseAI::~CFFBaseAI() {
    // std::cout << "CFFBaseAI Destroyed." << std::endl;
}

void CFFBaseAI::Update(CUserCmd* pCmd) {
    if (!m_pObjectivePlanner || !m_pBotPlayer || !m_pBotPlayer->IsAlive()) {
        if(pCmd) { pCmd->buttons = 0; pCmd->forwardmove = 0; pCmd->sidemove = 0; } // Ensure no stale commands
        return;
    }

    // 1. Let planner manage HighLevelTask and its subtask queue
    m_pObjectivePlanner->EvaluateAndSelectTask();

    // 2. Get current subtask from the planner's HLT
    const SubTask* pCurrentSubTask = m_pObjectivePlanner->GetCurrentSubTask();

    if (pCurrentSubTask && !pCurrentSubTask->isCompleted) {
        // std::cout << "CFFBaseAI: Executing SubTask type " << static_cast<int>(pCurrentSubTask->type) << std::endl;
        bool subTaskIsStillOngoing = ExecuteSubTask(pCurrentSubTask, pCmd);

        if (!subTaskIsStillOngoing) { // Task finished or failed in this frame
            if (pCurrentSubTask->isCompleted) { // Flag should be set by ExecuteSubTask or its callees
                m_pObjectivePlanner->OnSubTaskCompleted();
            } else { // Task failed or was aborted without explicit success
                m_pObjectivePlanner->OnSubTaskFailed();
            }
        }
    } else if (pCurrentSubTask && pCurrentSubTask->isCompleted) {
         m_pObjectivePlanner->OnSubTaskCompleted(); // Ensure planner advances if task completed previously
    } else {
        // No current subtask (either HLT is done, or no HLT was selected, or HLT has no subtasks)
        // Bot can perform idle behavior here.
        // Example: Look around, or if m_pCurrentTarget is set from a recent event, consider it.
        if (m_pCurrentTarget && m_pCurrentTarget->IsAlive()) {
            // This might be too aggressive; depends on how m_pCurrentTarget is managed
            // AttackTarget(m_pCurrentTarget, pCmd);
        } else {
            // AimAt(m_pBotPlayer->GetPosition() + Vector(100,0,0), pCmd); // Look forward
            if(pCmd) pCmd->buttons = 0; // Clear buttons if idle
        }
    }
}

bool CFFBaseAI::ExecuteSubTask(const SubTask* pSubTask, CUserCmd* pCmd) {
    if (!pSubTask || !m_pBotPlayer) return false; // No task or no bot player wrapper

    bool isOngoing = true;

    switch (pSubTask->type) {
        case SubTaskType::MOVE_TO_POSITION:
        case SubTaskType::MOVE_TO_ENTITY: {
            Vector targetPos = (pSubTask->type == SubTaskType::MOVE_TO_ENTITY && pSubTask->pTargetEntity) ?
                               pSubTask->pTargetEntity->GetPosition() : pSubTask->targetPosition;

            bool targetPositionChangedSignificantly = true; // Assume changed if path is empty
            if (!m_CurrentPath_NavAreaIDs.empty()) {
                float dx = m_vCurrentMoveToTarget.x - targetPos.x;
                float dy = m_vCurrentMoveToTarget.y - targetPos.y;
                float dz = m_vCurrentMoveToTarget.z - targetPos.z;
                targetPositionChangedSignificantly = (dx*dx + dy*dy + dz*dz > 25.0f*25.0f); // More than 25 units change
            }

            if (m_CurrentPath_NavAreaIDs.empty() || targetPositionChangedSignificantly) {
                if (!MoveTo(targetPos, pCmd)) { // This calls FindPath and sets m_vCurrentMoveToTarget
                    isOngoing = false; // Pathfinding failed
                    break;
                }
            }
            isOngoing = FollowPath(pCmd);
            if (!isOngoing) { // Path finished
                ((SubTask*)pSubTask)->isCompleted = true;
            }
            break;
        }
        case SubTaskType::ATTACK_TARGET:
            if (pSubTask->pTargetEntity && pSubTask->pTargetEntity->IsAlive()) {
                isOngoing = AttackTarget(pSubTask->pTargetEntity, pCmd); // Derived class implements this
                if (!pSubTask->pTargetEntity->IsAlive()) { // If target died during this frame's attack logic
                    isOngoing = false; ((SubTask*)pSubTask)->isCompleted = true;
                }
            } else {
                isOngoing = false; // No target or target dead
            }
            break;

        case SubTaskType::CAPTURE_OBJECTIVE: { // Example for a generic capture
            float captureRadiusSq = 64.0f * 64.0f; // Example capture radius
            if ((m_pBotPlayer->GetPosition().x - pSubTask->targetPosition.x) * (m_pBotPlayer->GetPosition().x - pSubTask->targetPosition.x) +
                (m_pBotPlayer->GetPosition().y - pSubTask->targetPosition.y) * (m_pBotPlayer->GetPosition().y - pSubTask->targetPosition.y) < captureRadiusSq) {
                // Bot is on the point. What it does depends on game rules.
                // For now, just consider being on point as "action performed".
                // Real capture might take time or specific interactions.
                AimAt(m_pBotPlayer->GetPosition() + Vector(100,0,0), pCmd); // Look forward while capturing
                // Check game state for actual capture completion.
                // if (IsObjectiveCaptured(pSubTask->pTargetEntity)) { ((SubTask*)pSubTask)->isCompleted = true; isOngoing = false;}
                ((SubTask*)pSubTask)->isCompleted = true; isOngoing = false; // TEMP: assume instant capture for subtask
            } else {
                // Not on point yet, try to move there. This logic is similar to MOVE_TO_POSITION.
                if (m_CurrentPath_NavAreaIDs.empty() || m_vCurrentMoveToTarget.x != pSubTask->targetPosition.x) { // Simplified check
                    if (!MoveTo(pSubTask->targetPosition, pCmd)) { isOngoing = false; break;}
                }
                isOngoing = FollowPath(pCmd);
                if (!isOngoing) { ((SubTask*)pSubTask)->isCompleted = true; } // Path ended, assume at point
            }
            break;
        }
        // Cases for USE_ABILITY_ON_TARGET, USE_ABILITY_AT_POSITION etc. would call UseAbility()
        // case SubTaskType::USE_ABILITY_ON_TARGET:
        //     isOngoing = UseAbility(pSubTask->abilitySlot, pSubTask->pTargetEntity, pCmd);
        //     if (!isOngoing) ((SubTask*)pSubTask)->isCompleted = true; // Assuming ability use is instant for subtask
        //     break;

        default:
            std::cout << "CFFBaseAI: ExecuteSubTask - Unknown/unhandled SubTaskType: " << static_cast<int>(pSubTask->type) << std::endl;
            isOngoing = false; // Mark as not ongoing to prevent getting stuck
            break;
    }
    return isOngoing;
}

bool CFFBaseAI::MoveTo(const Vector& targetPos, CUserCmd* pCmd) {
    ClearCurrentPath();
    m_vCurrentMoveToTarget = targetPos; // Store the ultimate destination
    m_CurrentPath_NavAreaIDs = FindPath(targetPos); // FindPath takes target, assumes start is bot's current pos

    if (!m_CurrentPath_NavAreaIDs.empty()) {
        m_CurrentPathIndex = 0;
        // std::cout << "CFFBaseAI: Path found to (" << targetPos.x << "," << targetPos.y << "). Nodes: " << m_CurrentPath_NavAreaIDs.size() << std::endl;
        return true; // Path found, FollowPath will take over
    }
    // std::cerr << "CFFBaseAI: MoveTo failed - Pathfinding failed to (" << targetPos.x << "," << targetPos.y << ")" << std::endl;
    return false; // Pathfinding failed
}

bool CFFBaseAI::FollowPath(CUserCmd* pCmd) {
    if (m_CurrentPath_NavAreaIDs.empty() || m_CurrentPathIndex < 0) {
        return false; // No path to follow or path already finished conceptually
    }
    if (m_CurrentPathIndex >= m_CurrentPath_NavAreaIDs.size()) { // Explicitly beyond the last node
        ClearCurrentPath();
        return false; // Path finished
    }

    Vector nextNodeWorldPos;
    if (!GetNextPathPosition(nextNodeWorldPos)) { // This advances index if current node is reached
        ClearCurrentPath(); // Path is now exhausted
        return false;
    }

    AimAt(nextNodeWorldPos, pCmd);

    // Simple forward movement
    // More complex logic would adjust speed based on distance, path curvature, etc.
    pCmd->forwardmove = 400; // Conceptual forward movement speed
    pCmd->sidemove = 0;

    return true; // Path following ongoing
}

// Gets world position of the nav area ID at m_CurrentPathIndex.
// Advances m_CurrentPathIndex if current waypoint is reached.
// Returns true if a valid next position is available, false if path end or error.
bool CFFBaseAI::GetNextPathPosition(Vector& outNextPos) {
    if (m_CurrentPath_NavAreaIDs.empty() || m_CurrentPathIndex < 0 || m_CurrentPathIndex >= m_CurrentPath_NavAreaIDs.size()) {
        return false;
    }

    unsigned int currentNodeNavId = m_CurrentPath_NavAreaIDs[m_CurrentPathIndex];
    const NavMeshGraph::NavAreaDef* pCurrentNodeDef = nullptr; // Using NavAreaDef from placeholder
    if (m_pKnowledgeBase && m_pKnowledgeBase->navMesh) {
        pCurrentNodeDef = m_pKnowledgeBase->navMesh->GetNavAreaByID(currentNodeNavId);
    }

    if (!pCurrentNodeDef) {
        std::cerr << "GetNextPathPosition: Failed to get NavAreaDef for ID " << currentNodeNavId << std::endl;
        ClearCurrentPath(); // Path is invalid
        return false;
    }
    outNextPos = pCurrentNodeDef->center;
    // delete pCurrentNodeDef; // If GetNavAreaByID new's it (bad design, fix in placeholder)

    // Check if bot is close enough to this current node to advance
    const float BOT_REACHED_NODE_DISTANCE_SQ = 50.0f * 50.0f; // Example: 50 units squared
    Vector botPos = m_pBotPlayer ? m_pBotPlayer->GetPosition() : Vector(0,0,0);
    float dx = botPos.x - outNextPos.x;
    float dy = botPos.y - outNextPos.y;
    // float dz = botPos.z - outNextPos.z; // Often ignore Z for 2D "reached" check on nav areas
    if ((dx*dx + dy*dy) < BOT_REACHED_NODE_DISTANCE_SQ) {
        m_CurrentPathIndex++; // Advance to next node
        if (m_CurrentPathIndex >= m_CurrentPath_NavAreaIDs.size()) {
            // std::cout << "GetNextPathPosition: Reached end of path." << std::endl;
            return false; // Reached end of current path
        }
        // Get the new current node's position for this frame's movement goal
        currentNodeNavId = m_CurrentPath_NavAreaIDs[m_CurrentPathIndex];
        if (m_pKnowledgeBase && m_pKnowledgeBase->navMesh) {
            pCurrentNodeDef = m_pKnowledgeBase->navMesh->GetNavAreaByID(currentNodeNavId);
        }
        if (!pCurrentNodeDef) {
            std::cerr << "GetNextPathPosition: Failed to get NavAreaDef for advanced ID " << currentNodeNavId << std::endl;
            ClearCurrentPath(); return false;
        }
        outNextPos = pCurrentNodeDef->center;
        // delete pCurrentNodeDef; // LEAK
    }
    return true; // Valid position to move towards
}


std::vector<unsigned int> CFFBaseAI::FindPath(const Vector& targetPos) {
    if (!m_pKnowledgeBase || !m_pKnowledgeBase->navMesh || !m_pBotPlayer) {
        std::cerr << "CFFBaseAI::FindPath: Missing NavMesh or BotPlayer!" << std::endl;
        return {};
    }
    Vector startPos = m_pBotPlayer->GetPosition();

    // Conceptual placeholder for A* pathfinding call
    // This would involve:
    // 1. Getting startNavId = m_pKnowledgeBase->navMesh->GetNearestNavAreaID(startPos);
    // 2. Getting endNavId = m_pKnowledgeBase->navMesh->GetNearestNavAreaID(targetPos);
    // 3. Instantiating AStarPathfinder pathfinder(m_pKnowledgeBase->navMesh);
    // 4. return pathfinder.FindPath(startNavId, endNavId);

    // std::cout << "CFFBaseAI::FindPath (Placeholder) from (" << startPos.x << ") to (" << targetPos.x << ")" << std::endl;
    // Dummy path for testing structure:
    std::vector<unsigned int> dummyPath;
    unsigned int startArea = m_pKnowledgeBase->navMesh->GetNearestNavAreaID(startPos);
    unsigned int endArea   = m_pKnowledgeBase->navMesh->GetNearestNavAreaID(targetPos);
    if (startArea != INVALID_NAV_AREA_ID && endArea != INVALID_NAV_AREA_ID) {
        // Create a simple path for testing, assuming areas 1,2,3,4 exist
        if (startArea <=4 && endArea <=4 && startArea != endArea) { // very dummy
             for(unsigned int i=startArea; i<=endArea; ++i) dummyPath.push_back(i);
        } else if (startArea == endArea && startArea != INVALID_NAV_AREA_ID) {
            dummyPath.push_back(startArea); // Path to current area
        }
    }
    if (dummyPath.empty()) { /* std::cout << " (dummy path is empty) " << std::endl; */ }
    return dummyPath;
}

bool CFFBaseAI::IsTargetInRange(CBaseEntity* pTarget, float range) const {
    if (!pTarget || !m_pBotPlayer) return false;
    Vector botPos = m_pBotPlayer->GetPosition();
    Vector targetPos = pTarget->GetPosition();
    float distSq = (botPos.x - targetPos.x) * (botPos.x - targetPos.x) +
                   (botPos.y - targetPos.y) * (botPos.y - targetPos.y) +
                   (botPos.z - targetPos.z) * (botPos.z - targetPos.z);
    return distSq < (range * range);
}

bool CFFBaseAI::IsFacingTarget(CBaseEntity* pTarget, float fovDegrees) const {
    if (!pTarget || !m_pBotPlayer) return false;
    // Conceptual - requires proper Vector math and AngleVectors from SDK
    Vector botEyePos = m_pBotPlayer->GetEyePosition();
    Vector botViewAngles = m_pBotPlayer->GetViewAngles(); // Assuming this returns a Vector(pitch,yaw,roll)

    Vector vecToTarget = pTarget->GetWorldSpaceCenter(); // Get target center
    vecToTarget.x -= botEyePos.x; vecToTarget.y -= botEyePos.y; vecToTarget.z -= botEyePos.z;

    float targetDist = vecToTarget.Length();
    if (targetDist > 0) {
        vecToTarget.x /= targetDist; vecToTarget.y /= targetDist; vecToTarget.z /= targetDist; // Normalize
    } else {
        return true; // Target is at eye origin
    }

    // Convert bot's view yaw to a 2D forward vector (ignoring pitch for simplicity in FOV check)
    float yawRad = botViewAngles.y * (M_PI / 180.0f);
    Vector botForward2D(std::cos(yawRad), std::sin(yawRad), 0);

    // Project vecToTarget to 2D plane (optional, or do 3D FOV)
    Vector vecToTarget2D = vecToTarget;
    vecToTarget2D.z = 0;
    float len2D = vecToTarget2D.Length();
    if (len2D > 0) { vecToTarget2D.x /= len2D; vecToTarget2D.y /= len2D; }
    else { return true; } // Target directly above/below

    float dotProduct = botForward2D.x * vecToTarget2D.x + botForward2D.y * vecToTarget2D.y;
    float fovRadians = fovDegrees * (M_PI / 180.0f);

    return dotProduct > std::cos(fovRadians * 0.5f);
}

void CFFBaseAI::AimAt(const Vector& targetPos, CUserCmd* pCmd) {
    if (!m_pBotPlayer || !pCmd) return;
    Vector eyePos = m_pBotPlayer->GetEyePosition();
    Vector aimDir = targetPos;
    aimDir.x -= eyePos.x; aimDir.y -= eyePos.y; aimDir.z -= eyePos.z;

    if (aimDir.x == 0.f && aimDir.y == 0.f && aimDir.z == 0.f) {
        // No direction, use current view angles or default forward
        pCmd->viewangles = m_pBotPlayer->GetViewAngles(); // Keep current if no change
        return;
    }

    float yaw = std::atan2(aimDir.y, aimDir.x) * (180.0f / M_PI);
    float pitch = std::atan2(-aimDir.z, std::sqrt(aimDir.x*aimDir.x + aimDir.y*aimDir.y)) * (180.0f / M_PI);

    pCmd->viewangles.x = pitch;
    pCmd->viewangles.y = yaw;
    pCmd->viewangles.z = 0; // Roll
    // std::cout << "AimAt: Target (" << targetPos.x << "," << targetPos.y << "," << targetPos.z << ") -> Angles P=" << pitch << " Y=" << yaw << std::endl;
}

void CFFBaseAI::ClearCurrentPath() {
    m_CurrentPath_NavAreaIDs.clear();
    m_CurrentPathIndex = -1;
    m_vCurrentMoveToTarget = Vector(0,0,0); // Or some invalid marker
}
