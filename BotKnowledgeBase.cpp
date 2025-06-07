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

void BotKnowledgeBase::UpdateTrackedProjectiles(const std::vector<ReflectableProjectileInfo>& currentProjectiles) {
    m_TrackedProjectiles = currentProjectiles; // Simple overwrite for now
    // More advanced logic could involve merging, tracking disappearance, etc.
}

void BotKnowledgeBase::ClearDynamicMapData() { /* ... (same as Task 16, Step 1, ensure m_TrackedBuildings.clear() is added) ... */
    m_ControlPoints.clear(); m_PayloadPaths.clear();
    m_TrackedEnemies.clear(); m_TrackedAllies.clear();
    m_TrackedProjectiles.clear(); // Clear projectiles as well
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
const std::vector<ReflectableProjectileInfo>& BotKnowledgeBase::GetTrackedProjectiles() const {
    return m_TrackedProjectiles;
}


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

// --- New implementations for Perception Data ---

// Conceptual: Assume this global or a member is set reflecting the bot's current team.
// This is needed to correctly categorize players.
// This would ideally be passed into UpdateTrackedPlayers_Conceptual or be a member of BotKnowledgeBase
// if it's a shared KB, or obtained from the bot instance if it's a per-bot KB.
static int g_bots_own_team_id_conceptual = 2; // Example: Bot is on RED team.

void BotKnowledgeBase::UpdateTrackedPlayers_Conceptual(const std::vector<TrackedEntityInfo>& perceivedPlayers) {
    m_TrackedEnemies.clear();
    m_TrackedAllies.clear();

    // A more robust system would get the bot's actual team ID rather than using a static global.
    // For instance, if this KB instance is tied to a specific bot, or if the team ID is passed in.
    // For now, we use the conceptual global g_bots_own_team_id_conceptual.

    for (const auto& playerInfo : perceivedPlayers) {
        if (playerInfo.team == 0) { // Skip neutral or unassigned players for these lists
            continue;
        }

        // Use the actual team for hostility check, not displayedTeam for spies
        if (playerInfo.team == g_bots_own_team_id_conceptual) {
            m_TrackedAllies.push_back(playerInfo);
        } else {
            m_TrackedEnemies.push_back(playerInfo);
        }
    }
    // More advanced: Update existing entries if possible, to preserve history or smooth data.
    // This simple version just rebuilds the lists each time.
}

void BotKnowledgeBase::UpdateTrackedBuildings_Conceptual(const std::vector<BuildingInfo>& perceivedBuildings) {
    // Simplistic update: Overwrite the entire list.
    // This means buildings that disappear from perception are immediately gone from KB.
    // A more robust system might:
    // 1. Mark all existing m_TrackedBuildings as "not seen this frame".
    // 2. Iterate perceivedBuildings:
    //    - If building exists in m_TrackedBuildings, update it and mark as "seen".
    //    - If new, add to m_TrackedBuildings and mark as "seen".
    // 3. Remove any buildings from m_TrackedBuildings still marked "not seen this frame".
    // For now, keeping it simple:
    m_TrackedBuildings = perceivedBuildings;
}
