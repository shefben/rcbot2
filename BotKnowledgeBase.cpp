#include "BotKnowledgeBase.h"
#include "FFLuaBridge.h"
#include "EngineInterfaces.h"
#include "FFEngineerAI.h" // For BuildingType_FF (if not moved to a more general header)
#include <algorithm>
#include <iostream>

// --- Conceptual: Engine NavMesh Interface & Defines ---
// (Assuming INavMesh_Conceptual and related defines are placeholders if g_pEngineNavMeshInterface is used)
// extern INavMesh_Conceptual* g_pEngineNavMeshInterface;
// --- End Conceptual ---


BotKnowledgeBase::BotKnowledgeBase() : m_CurrentGameMode(GameModeType_KB::UNKNOWN) {
    // std::cout << "BotKnowledgeBase: Initialized." << std::endl;
}

void BotKnowledgeBase::CreatePlaceholderNavMesh() { /* ... (same as Task 16, Step 1) ... */ }

bool BotKnowledgeBase::LoadGlobalClassConfigs(lua_State* L, const std::vector<std::string>& classConfigTableNames) { /* ... (same as Task 16, Step 1) ... */ return true;}
bool BotKnowledgeBase::LoadNavMesh(const char* mapName) { /* ... (same as Task 16, Step 1, calls CreatePlaceholderNavMesh if needed) ... */ CreatePlaceholderNavMesh(); return true;}
bool BotKnowledgeBase::LoadMapObjectiveData(lua_State* L, const char* mapName, const std::vector<std::string>& cpTableNames) { /* ... (same as Task 16, Step 1) ... */ return true;}
void BotKnowledgeBase::UpdateControlPointState(int cpId, int newOwnerTeam, float newCaptureProgress, bool newIsLocked) { /* ... (same as Task 16, Step 1) ... */ }
void BotKnowledgeBase::UpdateTrackedEntities(const std::vector<TrackedEntityInfo>& currentTrackedEntities, int botTeamId) { /* ... (same as Task 16, Step 1) ... */ }
void BotKnowledgeBase::ClearDynamicMapData() { /* ... (same as Task 16, Step 1, ensure m_TrackedBuildings.clear() is added) ... */
    m_ControlPoints.clear(); m_PayloadPaths.clear();
    m_TrackedEnemies.clear(); m_TrackedAllies.clear();
    // m_TrackedEntityMap.clear(); // If using map
    m_TrackedBuildings.clear(); // Added
    m_NavGraph.Clear();
    m_CurrentGameMode = GameModeType_KB::UNKNOWN;
}
void BotKnowledgeBase::ClearAllData() { ClearDynamicMapData(); m_ClassConfigs.clear(); }

const ControlPointInfo* BotKnowledgeBase::GetControlPoint(int cpId) const { /* ... */ return nullptr;}
const ControlPointInfo* BotKnowledgeBase::GetControlPointByLuaName(const std::string& luaName) const { /* ... */ return nullptr;}
const ClassConfigInfo* BotKnowledgeBase::GetClassConfigByName(const std::string& className) const { /* ... */ return nullptr;}
const ClassConfigInfo* BotKnowledgeBase::GetClassConfigById(int classId) const { /* ... */ return nullptr;}
const TrackedEntityInfo* BotKnowledgeBase::GetTrackedEntity(edict_t* pEdict) const { /* ... */ return nullptr;}
const TrackedEntityInfo* BotKnowledgeBase::GetTrackedEntityById(int entityId) const { /* ... */ return nullptr;}


// --- Building Information Management ---

void BotKnowledgeBase::UpdateOrAddBuilding(const BuildingInfo& newBuildingInfo) {
    if (newBuildingInfo.pEdict == nullptr && newBuildingInfo.uniqueId == -1) {
        // std::cerr << "BotKnowledgeBase Error: UpdateOrAddBuilding called with invalid BuildingInfo (no edict or ID)." << std::endl;
        return;
    }

    // Try to find by edict first (more reliable if edicts are reused but currently valid)
    // or by uniqueId if edicts can become stale but ID is persistent from game events.
    auto it = std::find_if(m_TrackedBuildings.begin(), m_TrackedBuildings.end(),
        [&](const BuildingInfo& b) {
            if (newBuildingInfo.pEdict && b.pEdict) return b.pEdict == newBuildingInfo.pEdict;
            if (newBuildingInfo.uniqueId != -1) return b.uniqueId == newBuildingInfo.uniqueId;
            return false;
        });

    if (it != m_TrackedBuildings.end()) {
        // Update existing building
        // std::cout << "KB: Updating building ID " << it->uniqueId << " Type " << static_cast<int>(it->type) << std::endl;
        it->type = newBuildingInfo.type; // Type should generally not change, but refresh if possible
        it->level = newBuildingInfo.level;
        it->health = newBuildingInfo.health;
        it->maxHealth = newBuildingInfo.maxHealth; // Max health can change with level
        it->builderPlayerId = newBuildingInfo.builderPlayerId; // Builder might change if repossessed in some games
        it->teamId = newBuildingInfo.teamId;
        it->position = newBuildingInfo.position;
        it->angles = newBuildingInfo.angles;
        it->isSapped = newBuildingInfo.isSapped;
        it->isBuildingInProgress = newBuildingInfo.isBuildingInProgress;
        it->isUpgrading = newBuildingInfo.isUpgrading;
        if (newBuildingInfo.pEdict) it->pEdict = newBuildingInfo.pEdict; // Update edict if it changed for same uniqueId (unlikely)
    } else {
        // Add new building
        // std::cout << "KB: Adding new building ID " << newBuildingInfo.uniqueId << " Type " << static_cast<int>(newBuildingInfo.type) << std::endl;
        m_TrackedBuildings.push_back(newBuildingInfo);
    }
}

void BotKnowledgeBase::RemoveBuilding(edict_t* pEdict) {
    if (!pEdict) return;
    m_TrackedBuildings.erase(
        std::remove_if(m_TrackedBuildings.begin(), m_TrackedBuildings.end(),
                       [pEdict](const BuildingInfo& b) { return b.pEdict == pEdict; }),
        m_TrackedBuildings.end()
    );
}

void BotKnowledgeBase::UpdateBuildingHealth(edict_t* pEdict, int newHealth, bool isStillBuilding) {
    BuildingInfo* building = const_cast<BuildingInfo*>(GetBuildingInfo(pEdict));
    if (building) {
        building->health = newHealth;
        building->isBuildingInProgress = isStillBuilding;
        if (newHealth <= 0) {
            building->isSapped = false; // No longer sapped if destroyed
            // Optionally mark as DESTROYED status or remove (RemoveBuilding would be better)
        }
    }
}
void BotKnowledgeBase::UpdateBuildingSapped(edict_t* pEdict, bool isSappedStatus) {
    BuildingInfo* building = const_cast<BuildingInfo*>(GetBuildingInfo(pEdict));
    if (building) {
        building->isSapped = isSappedStatus;
    }
}
void BotKnowledgeBase::UpdateBuildingLevel(edict_t* pEdict, int newLevel) {
    BuildingInfo* building = const_cast<BuildingInfo*>(GetBuildingInfo(pEdict));
    if (building) {
        building->level = newLevel;
        // Conceptual: building->maxHealth = GetMaxHealthForBuilding(building->type, newLevel);
    }
}


const BuildingInfo* BotKnowledgeBase::GetBuildingInfo(edict_t* pEdict) const {
    if (!pEdict) return nullptr;
    for (const auto& building : m_TrackedBuildings) {
        if (building.pEdict == pEdict) {
            return &building;
        }
    }
    return nullptr;
}
const BuildingInfo* BotKnowledgeBase::GetBuildingInfoByUniqueId(int uniqueId) const {
    if (uniqueId == -1) return nullptr;
     for (const auto& building : m_TrackedBuildings) {
        if (building.uniqueId == uniqueId) {
            return &building;
        }
    }
    return nullptr;
}

std::vector<const BuildingInfo*> BotKnowledgeBase::GetFriendlyBuildings(int botTeamId) const {
    std::vector<const BuildingInfo*> friendlyBuildings;
    for (const auto& building : m_TrackedBuildings) {
        if (building.teamId == botTeamId && building.health > 0) {
            friendlyBuildings.push_back(&building);
        }
    }
    return friendlyBuildings;
}

std::vector<const BuildingInfo*> BotKnowledgeBase::GetFriendlyBuildingsOfType(BuildingType_FF type, int botTeamId) const {
    std::vector<const BuildingInfo*> friendlyTypedBuildings;
    for (const auto& building : m_TrackedBuildings) {
        if (building.teamId == botTeamId && building.type == type && building.health > 0) {
            friendlyTypedBuildings.push_back(&building);
        }
    }
    return friendlyTypedBuildings;
}

std::vector<const BuildingInfo*> BotKnowledgeBase::GetEnemyBuildingsInArea(const Vector& areaCenter, float radius, int botTeamId) const {
    std::vector<const BuildingInfo*> enemyBuildings;
    float radiusSq = radius * radius;
    for (const auto& building : m_TrackedBuildings) {
        if (building.teamId != 0 && building.teamId != botTeamId && building.health > 0) {
            if ((building.position.x - areaCenter.x)*(building.position.x - areaCenter.x) +
                (building.position.y - areaCenter.y)*(building.position.y - areaCenter.y) < radiusSq) { // Simple 2D check for now
                enemyBuildings.push_back(&building);
            }
        }
    }
    return enemyBuildings;
}

std::vector<const BuildingInfo*> BotKnowledgeBase::GetOwnBuildings(int botPlayerId) const {
    std::vector<const BuildingInfo*> ownBuildings;
    for (const auto& building : m_TrackedBuildings) {
        if (building.builderPlayerId == botPlayerId && building.health > 0) {
            ownBuildings.push_back(&building);
        }
    }
    return ownBuildings;
}


// --- Ensure previous methods are present ---
// NavMeshGraph::FindNearestNodeID implementation should be in NavSystem.cpp
// unsigned int NavMeshGraph::FindNearestNodeID(const Vector& pos, float maxDist) const { /* ... */ return 0; }
// GameModeType_KB BotKnowledgeBase::GetCurrentGameMode() const { return m_CurrentGameMode; }
// void BotKnowledgeBase::SetCurrentGameMode(GameModeType_KB mode) { m_CurrentGameMode = mode; }
// float BotKnowledgeBase::GetAreaThreat(unsigned int navAreaId) const { return 0.0f; }
// bool BotKnowledgeBase::IsAreaNearObjective(unsigned int navAreaId, const HighLevelTask* currentTask) const { return false; }
// bool BotKnowledgeBase::IsPathToObjective(unsigned int navAreaId) const { return false; }
// bool BotKnowledgeBase::IsTeamWinning() const { return false; }
