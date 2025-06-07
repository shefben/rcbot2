#include "BotKnowledgeBase.h"
#include "FFLuaBridge.h"
#include "EngineInterfaces.h" // For g_pEngineNavMeshInterface (conceptual)
#include <algorithm>
#include <iostream>

// --- Conceptual: Engine NavMesh Interface & Defines (would be from SDK) ---
// Assume INavMesh is a global g_pEngineNavMeshInterface (defined in EngineInterfaces.cpp, extern in .h)
// extern INavMesh* g_pEngineNavMeshInterface; // Conceptual

// Conceptual Nav flags from engine (replace with actual values)
#define NAV_MESH_CROUCH_CONCEPTUAL (1 << 1)  // Example bit for crouch
#define NAV_MESH_JUMP_CONCEPTUAL   (1 << 2)  // Example bit for jump
#define NAV_MESH_DANGER_CONCEPTUAL (1 << 5)  // Example bit for danger

/* Conceptual structure of engine's NavArea and Connection
struct NavConnect_Conceptual {
    unsigned int connectedAreaID;
    unsigned int type; // Bitflags indicating JUMP, CROUCH, etc.
    bool isTwoWay;
    // Vector fromPos, toPos; // Optional specific connection points
};
struct NavConnectList_Conceptual {
    std::vector<NavConnect_Conceptual> connections; // Simplified
    const NavConnect_Conceptual& operator[](size_t i) const { return connections[i]; }
    size_t Count() const { return connections.size(); }
};
struct CNavArea_Conceptual {
    unsigned int GetID() const { return 0; }
    const Vector& GetCenter() const { static Vector v; return v; } // Assume Vector is compatible
    unsigned int GetAttributes() const { return 0; }
    bool IsBlocked(int teamID = 0) const { return false; } // teamID if applicable
    const NavConnectList_Conceptual& GetConnections() const { static NavConnectList_Conceptual l; return l; }
    // May also have methods like:
    // Vector GetRandomPoint() const;
    // float GetSizeX() const, GetSizeY() const, GetSizeZ() const;
};
struct INavMesh_Conceptual { // Placeholder for a generic engine nav mesh interface
    virtual ~INavMesh_Conceptual() {}
    virtual bool IsLoaded() const = 0;
    virtual unsigned int GetNavAreaCount() const = 0;
    virtual CNavArea_Conceptual* GetNavAreaByIndex(unsigned int index) const = 0; // Or by ID, or iterator
};
INavMesh_Conceptual* g_pEngineNavMeshInterface = nullptr; // Definition for the extern
*/
// --- End Conceptual ---


// --- BotKnowledgeBase Implementation ---

BotKnowledgeBase::BotKnowledgeBase() : m_CurrentGameMode(GameModeType_KB::UNKNOWN) {
    // std::cout << "BotKnowledgeBase: Initialized." << std::endl;
}

void BotKnowledgeBase::CreatePlaceholderNavMesh() { // Private helper method
    m_NavGraph.Clear();
    NavAreaNode node1(1); node1.center = Vector(0,0,0);     node1.attributes = NavAttribute::NONE;
    NavAreaNode node2(2); node2.center = Vector(200,0,0);   node2.attributes = NavAttribute::NONE;
    NavAreaNode node3(3); node3.center = Vector(200,200,0); node3.attributes = NavAttribute::CROUCH;
    NavAreaNode node4(4); node4.center = Vector(0,200,0);   node4.attributes = NavAttribute::NONE;
    NavAreaNode node5(5); node5.center = Vector(0,-200,0);  node5.attributes = NavAttribute::DANGER;
    NavAreaNode node6(6); node6.center = Vector(400,0,50);  node6.attributes = NavAttribute::JUMP;

    m_NavGraph.AddNode(node1); m_NavGraph.AddNode(node2);
    m_NavGraph.AddNode(node3); m_NavGraph.AddNode(node4);
    m_NavGraph.AddNode(node5); m_NavGraph.AddNode(node6);

    m_NavGraph.AddConnection(1, 2, NavConnectionType::WALK, 1.0f, true);
    m_NavGraph.AddConnection(2, 3, NavConnectionType::WALK, 1.0f, true);
    m_NavGraph.AddConnection(3, 4, NavConnectionType::CROUCH_WALK, 1.2f, true);
    m_NavGraph.AddConnection(4, 1, NavConnectionType::WALK, 1.0f, true);
    m_NavGraph.AddConnection(1, 5, NavConnectionType::WALK, 1.0f, true);
    m_NavGraph.AddConnection(2, 6, NavConnectionType::JUMP_ONEWAY, 1.8f, false);
    // std::cout << "BotKnowledgeBase: Created placeholder NavMesh with " << m_NavGraph.nodes.size() << " nodes." << std::endl;
}


bool BotKnowledgeBase::LoadNavMesh(const char* mapName) {
    m_NavGraph.Clear();
    // std::cout << "BotKnowledgeBase: Attempting to load NavMesh for map: " << (mapName ? mapName : "N/A") << std::endl;

    // Conceptual: Interface with the engine's navmesh system.
    // Assume g_pEngineNavMeshInterface is initialized by CRCBotPlugin::Load()
    // if (!g_pEngineNavMeshInterface || !g_pEngineNavMeshInterface->IsLoaded()) {
    //     std::cerr << "BotKnowledgeBase Warning: Engine NavMesh not available or not loaded for map "
    //               << (mapName ? mapName : "N/A") << ". Using placeholder." << std::endl;
    //     CreatePlaceholderNavMesh();
    //     return true; // Still return true as placeholder is a valid fallback for now
    // }

    // unsigned int areaCount = g_pEngineNavMeshInterface->GetNavAreaCount();
    // if (areaCount == 0) {
    //     std::cout << "BotKnowledgeBase: Engine NavMesh reports 0 areas. Using placeholder." << std::endl;
    //     CreatePlaceholderNavMesh();
    //     return true;
    // }
    // std::cout << "BotKnowledgeBase: Loading " << areaCount << " nav areas from engine..." << std::endl;

    // // First pass: Add all nodes
    // for (unsigned int i = 0; i < areaCount; ++i) {
    //     CNavArea_Conceptual* pEngineArea = g_pEngineNavMeshInterface->GetNavAreaByIndex(i); // Conceptual
    //     if (pEngineArea) {
    //         NavAreaNode botNavNode(pEngineArea->GetID());
    //         botNavNode.center = pEngineArea->GetCenter();

    //         botNavNode.attributes = NavAttribute::NONE; // Reset before setting
    //         unsigned int engineAttrs = pEngineArea->GetAttributes();
    //         if (engineAttrs & NAV_MESH_CROUCH_CONCEPTUAL) botNavNode.attributes |= NavAttribute::CROUCH;
    //         if (engineAttrs & NAV_MESH_JUMP_CONCEPTUAL)   botNavNode.attributes |= NavAttribute::JUMP;
    //         if (engineAttrs & NAV_MESH_DANGER_CONCEPTUAL) botNavNode.attributes |= NavAttribute::DANGER;
    //         // botNavNode.isBlocked = pEngineArea->IsBlocked(TEAM_ANY_CONCEPTUAL); // Conceptual

    //         m_NavGraph.AddNode(botNavNode);
    //     }
    // }

    // // Second pass: Add connections
    // for (unsigned int i = 0; i < areaCount; ++i) {
    //     CNavArea_Conceptual* pEngineArea = g_pEngineNavMeshInterface->GetNavAreaByIndex(i);
    //     if (pEngineArea) {
    //         unsigned int fromId = pEngineArea->GetID();
    //         const NavConnectList_Conceptual& connections = pEngineArea->GetConnections();
    //         for (size_t j = 0; j < connections.Count(); ++j) {
    //             const NavConnect_Conceptual& engConn = connections[j];
    //             unsigned int toId = engConn.connectedAreaID;

    //             NavConnectionType type = NavConnectionType::WALK; // Default
    //             if (engConn.type & NAV_MESH_JUMP_CONCEPTUAL)   type = NavConnectionType::JUMP_ONEWAY; // Or JUMP_TWOWAY if info available
    //             else if (engConn.type & NAV_MESH_CROUCH_CONCEPTUAL) type = NavConnectionType::CROUCH_WALK;
    //             // ... map other engine connection types to NavConnectionType ...

    //             float costMultiplier = 1.0f; // Default, could be higher for jumps etc.
    //             // if (type == NavConnectionType::JUMP_ONEWAY) costMultiplier = 1.5f;

    //             // AddConnection handles twoWay internally if specified
    //             m_NavGraph.AddConnection(fromId, toId, type, costMultiplier, engConn.isTwoWay /*, engConn.fromPos, engConn.toPos */);
    //         }
    //     }
    // }
    // std::cout << "BotKnowledgeBase: Finished loading " << m_NavGraph.nodes.size() << " areas from engine NavMesh." << std::endl;

    // Fallback to placeholder if engine loading is disabled or fails
    if (m_NavGraph.nodes.empty()) {
        CreatePlaceholderNavMesh();
    }
    return true;
}


// ... (rest of BotKnowledgeBase.cpp methods: LoadGlobalClassConfigs, LoadMapObjectiveData, Updates, Clears, Accessors)
// Ensure these methods are present from previous step (Task 12, Step 3)

bool BotKnowledgeBase::LoadGlobalClassConfigs(lua_State* L, const std::vector<std::string>& classConfigTableNames) {
    if (!L) { std::cerr << "BotKnowledgeBase Error: Lua state is null. Cannot load class configs." << std::endl; return false; }
    m_ClassConfigs.clear();
    bool allSuccess = true;
    if (classConfigTableNames.empty()) { /*std::cout << "BotKnowledgeBase: No class config table names provided." << std::endl;*/ return true; }
    for (const std::string& tableName : classConfigTableNames) {
        ClassConfigInfo tempConfig;
        if (FFLuaBridge::PopulateClassConfigInfo(L, tableName.c_str(), tempConfig)) {
            m_ClassConfigs.push_back(tempConfig);
        } else { allSuccess = false; }
    }
    return allSuccess;
}

bool BotKnowledgeBase::LoadMapObjectiveData(lua_State* L, const char* mapName, const std::vector<std::string>& cpTableNames) {
    if (!L) { std::cerr << "BotKnowledgeBase Error: Lua state is null. Cannot load map objective data." << std::endl; return false; }
    m_ControlPoints.clear(); m_PayloadPaths.clear();
    if (!cpTableNames.empty()) {
        SetCurrentGameMode(GameModeType_KB::CONTROL_POINT);
        bool allSuccess = true;
        for (const std::string& tableName : cpTableNames) {
            ControlPointInfo tempCP;
            if (FFLuaBridge::PopulateControlPointInfo(L, tableName.c_str(), tempCP)) {
                m_ControlPoints.push_back(tempCP);
            } else { allSuccess = false; }
        }
        return allSuccess;
    } else { SetCurrentGameMode(GameModeType_KB::UNKNOWN); }
    return false;
}

void BotKnowledgeBase::UpdateControlPointState(int cpId, int newOwnerTeam, float newCaptureProgress, bool newIsLocked) {
    auto it = std::find_if(m_ControlPoints.begin(), m_ControlPoints.end(),
                           [cpId](const ControlPointInfo& cp){ return cp.id == cpId; });
    if (it != m_ControlPoints.end()) { it->ownerTeam = newOwnerTeam; it->captureProgress = newCaptureProgress; it->isLocked = newIsLocked; }
}

void BotKnowledgeBase::UpdateTrackedEntities(const std::vector<TrackedEntityInfo>& currentTrackedEntities, int botTeamId) {
    m_TrackedEnemies.clear(); m_TrackedAllies.clear(); m_TrackedEntityMap.clear();
    for (const TrackedEntityInfo& entityFromPerception : currentTrackedEntities) {
        if (entityFromPerception.team == botTeamId) { m_TrackedAllies.push_back(entityFromPerception); }
        else if (entityFromPerception.team != 0 && entityFromPerception.team != botTeamId) { m_TrackedEnemies.push_back(entityFromPerception); }
    }
}

void BotKnowledgeBase::ClearDynamicMapData() {
    m_ControlPoints.clear(); m_PayloadPaths.clear(); m_TrackedEnemies.clear(); m_TrackedAllies.clear();
    m_TrackedEntityMap.clear(); m_NavGraph.Clear(); m_CurrentGameMode = GameModeType_KB::UNKNOWN;
}

void BotKnowledgeBase::ClearAllData() { ClearDynamicMapData(); m_ClassConfigs.clear(); }

const ControlPointInfo* BotKnowledgeBase::GetControlPoint(int cpId) const { /* ... */ return nullptr;}
const ControlPointInfo* BotKnowledgeBase::GetControlPointByLuaName(const std::string& luaName) const { /* ... */ return nullptr;}
const ClassConfigInfo* BotKnowledgeBase::GetClassConfigByName(const std::string& className) const { /* ... */ return nullptr;}
const ClassConfigInfo* BotKnowledgeBase::GetClassConfigById(int classId) const { /* ... */ return nullptr;}
const TrackedEntityInfo* BotKnowledgeBase::GetTrackedEntity(edict_t* pEdict) const { /* ... */ return nullptr;}
const TrackedEntityInfo* BotKnowledgeBase::GetTrackedEntityById(int entityId) const { /* ... */ return nullptr;}

// NavMeshGraph::FindNearestNodeID implementation was moved to NavSystem.cpp in previous task.
// Make sure it's correctly defined there.
// unsigned int NavMeshGraph::FindNearestNodeID(const Vector& pos, float maxDist) const { /* ... */ return 0; }

// Placeholder for GetCurrentGameMode accessor if needed by other parts directly
// GameModeType_KB BotKnowledgeBase::GetCurrentGameMode() const { return m_CurrentGameMode; }
// void BotKnowledgeBase::SetCurrentGameMode(GameModeType_KB mode) { m_CurrentGameMode = mode; }

// Conceptual methods for A* dynamic costs
// float BotKnowledgeBase::GetAreaThreat(unsigned int navAreaId) const { return 0.0f; }
// bool BotKnowledgeBase::IsAreaNearObjective(unsigned int navAreaId, const HighLevelTask* currentTask) const { return false; }
// bool BotKnowledgeBase::IsPathToObjective(unsigned int navAreaId) const { return false; }
// bool BotKnowledgeBase::IsTeamWinning() const { return false; }
