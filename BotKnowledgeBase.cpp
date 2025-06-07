#include "BotKnowledgeBase.h"
#include "FFLuaBridge.h"
#include <algorithm>
#include <iostream>

// --- BotKnowledgeBase Implementation ---

BotKnowledgeBase::BotKnowledgeBase() : m_CurrentGameMode(GameModeType_KB::UNKNOWN) {
    // std::cout << "BotKnowledgeBase: Initialized." << std::endl;
}

// Destructor: m_NavGraph is an object member, its own destructor will be called.
// Other std::vector members will also clean up themselves.
// No explicit cleanup needed here unless KB directly manages raw pointers.
// BotKnowledgeBase::~BotKnowledgeBase() {}

bool BotKnowledgeBase::LoadGlobalClassConfigs(lua_State* L, const std::vector<std::string>& classConfigTableNames) {
    if (!L) {
        std::cerr << "BotKnowledgeBase Error: Lua state is null. Cannot load class configs." << std::endl;
        return false;
    }
    m_ClassConfigs.clear();
    bool allSuccess = true;

    if (classConfigTableNames.empty()) {
        std::cout << "BotKnowledgeBase: No class config table names provided. Class configs will be empty." << std::endl;
        return true; // Not necessarily a failure, could be intentional for some tests
    }

    // std::cout << "BotKnowledgeBase: Loading " << classConfigTableNames.size() << " global class configs from Lua..." << std::endl;
    for (const std::string& tableName : classConfigTableNames) {
        ClassConfigInfo tempConfig;
        if (FFLuaBridge::PopulateClassConfigInfo(L, tableName.c_str(), tempConfig)) {
            m_ClassConfigs.push_back(tempConfig);
        } else {
            std::cerr << "BotKnowledgeBase Warning: Failed to populate ClassConfigInfo from Lua table '" << tableName << "'." << std::endl;
            allSuccess = false;
        }
    }
    // std::cout << "BotKnowledgeBase: Finished loading global class configs. Loaded: " << m_ClassConfigs.size() << std::endl;
    return allSuccess;
}

bool BotKnowledgeBase::LoadNavMesh(const char* mapName) {
    m_NavGraph.Clear();
    // std::cout << "BotKnowledgeBase: Loading NavMesh for map: " << (mapName ? mapName : "N/A") << std::endl;

    // Conceptual: Interface with the engine's navmesh system.
    // Example: INavMesh* pEngineNavMesh = g_pEngineInterfaces->NavMeshSystem->GetNavMeshForMap(mapName);
    // if (pEngineNavMesh && pEngineNavMesh->IsLoaded()) {
    //     int areaCount = pEngineNavMesh->GetNavAreaCount();
    //     for (int i = 0; i < areaCount; ++i) {
    //         // CNavAreaHandle handle = pEngineNavMesh->GetNavAreaHandleByIndex(i); // Conceptual
    //         // const CNavArea* pEngineArea = handle.GetNavArea();
    //         // if (pEngineArea) {
    //         //     NavAreaNode botNavNode(pEngineArea->GetID());
    //         //     botNavNode.center = pEngineArea->GetCenter();
    //         //     botNavNode.attributes = NavAttribute::NONE;
    //         //     if (pEngineArea->HasAttribute(ENGINE_NAV_CROUCH)) botNavNode.attributes |= NavAttribute::CROUCH;
    //         //     if (pEngineArea->HasAttribute(ENGINE_NAV_JUMP))   botNavNode.attributes |= NavAttribute::JUMP;
    //         //     if (pEngineArea->HasAttribute(ENGINE_NAV_DANGER)) botNavNode.attributes |= NavAttribute::DANGER;
    //         //     // botNavNode.isBlocked = pEngineArea->IsBlocked(m_pBotOwner->GetTeamNumber()); // Example
    //         //     m_NavGraph.AddNode(botNavNode);
    //         // }
    //     }
    //     // After adding all nodes, iterate again to build connections
    //     // for (int i = 0; i < areaCount; ++i) { /* ... Get pEngineArea ... */
    //     //     NavAreaNode* pBotNode = m_NavGraph.GetNodeMutable(pEngineArea->GetID());
    //     //     if (pBotNode) {
    //     //         for (const EngineNavConnection& engConn : pEngineArea->GetConnections()) { // Conceptual
    //     //             NavConnectionType botConnType = ConvertEngineConnectionTypeToBot(engConn.type);
    //     //             m_NavGraph.AddConnection(pBotNode->id, engConn.targetAreaID, botConnType,
    //     //                                      engConn.costMultiplier, engConn.isTwoWay);
    //     //         }
    //     //     }
    //     // }
    //     // std::cout << "BotKnowledgeBase: Loaded " << m_NavGraph.nodes.size() << " areas from engine NavMesh." << std::endl;
    //     return true;
    // } else {
    //     std::cerr << "BotKnowledgeBase Warning: Engine NavMesh not loaded or interface not available for map "
    //               << (mapName ? mapName : "N/A") << ". Using placeholder navmesh." << std::endl;
    // }

    // --- Placeholder NavMesh Loading for continued development ---
    if (m_NavGraph.nodes.empty()) { // Only load placeholder if "real" loading above is commented out or failed
        NavAreaNode node1(1); node1.center = Vector(0,0,0);     node1.attributes = NavAttribute::NONE;
        NavAreaNode node2(2); node2.center = Vector(200,0,0);   node2.attributes = NavAttribute::NONE;
        NavAreaNode node3(3); node3.center = Vector(200,200,0); node3.attributes = NavAttribute::CROUCH;
        NavAreaNode node4(4); node4.center = Vector(0,200,0);   node4.attributes = NavAttribute::NONE;
        NavAreaNode node5(5); node5.center = Vector(0,-200,0);  node5.attributes = NavAttribute::DANGER;
        NavAreaNode node6(6); node6.center = Vector(400,0,50);  node6.attributes = NavAttribute::JUMP; // Area reachable by jump

        m_NavGraph.AddNode(node1); m_NavGraph.AddNode(node2);
        m_NavGraph.AddNode(node3); m_NavGraph.AddNode(node4);
        m_NavGraph.AddNode(node5); m_NavGraph.AddNode(node6);

        m_NavGraph.AddConnection(1, 2, NavConnectionType::WALK, 1.0f, true);
        m_NavGraph.AddConnection(2, 3, NavConnectionType::WALK, 1.0f, true); // Path leads into crouch area
        m_NavGraph.AddConnection(3, 4, NavConnectionType::CROUCH_WALK, 1.2f, true); // Connection itself requires crouch
        m_NavGraph.AddConnection(4, 1, NavConnectionType::WALK, 1.0f, true);
        m_NavGraph.AddConnection(1, 5, NavConnectionType::WALK, 1.0f, true); // Path leads into danger area
        m_NavGraph.AddConnection(2, 6, NavConnectionType::JUMP_ONEWAY, 1.8f, false); // Jump from node 2 to node 6

        // std::cout << "BotKnowledgeBase: Loaded placeholder NavMesh with " << m_NavGraph.nodes.size() << " nodes." << std::endl;
    }
    return true;
}

bool BotKnowledgeBase::LoadMapObjectiveData(lua_State* L, const char* mapName, const std::vector<std::string>& cpTableNames) {
    if (!L) {
        std::cerr << "BotKnowledgeBase Error: Lua state is null. Cannot load map objective data." << std::endl;
        return false;
    }
    m_ControlPoints.clear();
    m_PayloadPaths.clear();
    // std::cout << "BotKnowledgeBase: Loading objective data for map: " << (mapName ? mapName : "N/A") << std::endl;

    if (!cpTableNames.empty()) {
        SetCurrentGameMode(GameModeType_KB::CONTROL_POINT);
        // std::cout << "BotKnowledgeBase: Game mode set to Control Point. Loading " << cpTableNames.size() << " CPs from Lua..." << std::endl;
        bool allSuccess = true;
        for (const std::string& tableName : cpTableNames) {
            ControlPointInfo tempCP;
            if (FFLuaBridge::PopulateControlPointInfo(L, tableName.c_str(), tempCP)) {
                m_ControlPoints.push_back(tempCP);
            } else {
                std::cerr << "BotKnowledgeBase Warning: Failed to populate ControlPointInfo from Lua table '" << tableName << "'." << std::endl;
                allSuccess = false;
            }
        }
        // std::cout << "BotKnowledgeBase: Finished loading CPs. Loaded: " << m_ControlPoints.size() << std::endl;
        return allSuccess;
    } else {
        // std::cout << "BotKnowledgeBase: No Control Point Lua table names provided for map " << (mapName ? mapName : "N/A") << ". No CPs loaded." << std::endl;
        SetCurrentGameMode(GameModeType_KB::UNKNOWN);
    }
    return false;
}

void BotKnowledgeBase::UpdateControlPointState(int cpId, int newOwnerTeam, float newCaptureProgress, bool newIsLocked) {
    auto it = std::find_if(m_ControlPoints.begin(), m_ControlPoints.end(),
                           [cpId](const ControlPointInfo& cp){ return cp.id == cpId; });
    if (it != m_ControlPoints.end()) {
        it->ownerTeam = newOwnerTeam;
        it->captureProgress = newCaptureProgress;
        it->isLocked = newIsLocked;
    } else {
        // std::cerr << "BotKnowledgeBase Warning: UpdateControlPointState called for unknown cpId: " << cpId << std::endl;
    }
}

void BotKnowledgeBase::UpdateTrackedEntities(const std::vector<TrackedEntityInfo>& currentTrackedEntities, int botTeamId) {
    m_TrackedEnemies.clear();
    m_TrackedAllies.clear();
    // m_TrackedEntityMap.clear(); // If using map of pointers, repopulate carefully

    for (const TrackedEntityInfo& entityFromPerception : currentTrackedEntities) {
        if (entityFromPerception.team == botTeamId) {
            m_TrackedAllies.push_back(entityFromPerception);
        } else if (entityFromPerception.team != 0 && entityFromPerception.team != botTeamId) { // Basic enemy check
            m_TrackedEnemies.push_back(entityFromPerception);
        }
    }
}

void BotKnowledgeBase::ClearDynamicMapData() {
    m_ControlPoints.clear();
    m_PayloadPaths.clear();
    m_TrackedEnemies.clear();
    m_TrackedAllies.clear();
    // m_TrackedEntityMap.clear();
    m_NavGraph.Clear();
    m_CurrentGameMode = GameModeType_KB::UNKNOWN;
}

void BotKnowledgeBase::ClearAllData() {
    ClearDynamicMapData();
    m_ClassConfigs.clear();
}

const ControlPointInfo* BotKnowledgeBase::GetControlPoint(int cpId) const {
    for (const auto& cp : m_ControlPoints) { if (cp.id == cpId) return &cp; }
    return nullptr;
}
const ControlPointInfo* BotKnowledgeBase::GetControlPointByLuaName(const std::string& luaName) const {
    for (const auto& cp : m_ControlPoints) { if (cp.luaName == luaName) return &cp; }
    return nullptr;
}
const ClassConfigInfo* BotKnowledgeBase::GetClassConfigByName(const std::string& className) const {
    for (const auto& cfg : m_ClassConfigs) { if (cfg.className == className || cfg.luaBotClassName == className) return &cfg; }
    return nullptr;
}
const ClassConfigInfo* BotKnowledgeBase::GetClassConfigById(int classId) const {
     for (const auto& cfg : m_ClassConfigs) { if (cfg.classId == classId) return &cfg; }
    return nullptr;
}
const TrackedEntityInfo* BotKnowledgeBase::GetTrackedEntity(edict_t* pEdict) const {
    for(const auto& entity : m_TrackedAllies) if(entity.pEdict == pEdict) return &entity;
    for(const auto& entity : m_TrackedEnemies) if(entity.pEdict == pEdict) return &entity;
    return nullptr;
}
const TrackedEntityInfo* BotKnowledgeBase::GetTrackedEntityById(int entityId) const {
    for(const auto& entity : m_TrackedAllies) if(entity.entityId == entityId) return &entity;
    for(const auto& entity : m_TrackedEnemies) if(entity.entityId == entityId) return &entity;
    return nullptr;
}

// --- NavMeshGraph FindNearestNodeID (Conceptual, needs to be in NavSystem.cpp if NavMeshGraph owns it) ---
// Or, BotKnowledgeBase can provide this utility if it has direct access to m_NavGraph.nodes
unsigned int NavMeshGraph::FindNearestNodeID(const Vector& pos, float maxDist) const {
    unsigned int nearestNodeId = 0; // Assuming 0 is an invalid/no node ID
    float minDistanceSq = maxDist * maxDist;

    for (const auto& pair : nodes) {
        const NavAreaNode& node = pair.second;
        float dx = node.center.x - pos.x;
        float dy = node.center.y - pos.y;
        float dz = node.center.z - pos.z; // Consider 3D distance or 2D based on game nav
        float distSq = dx*dx + dy*dy + dz*dz;

        if (distSq < minDistanceSq) {
            minDistanceSq = distSq;
            nearestNodeId = node.id;
        }
    }
    return nearestNodeId;
}
