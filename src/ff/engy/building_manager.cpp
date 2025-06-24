#include "building_manager.h"

CFFBuildingManager g_FFBuildingManager;

CFFBuildingManager::CFFBuildingManager()
{
    Reset();
}

void CFFBuildingManager::Reset()
{
    m_iSentryLevel = 0;
}

void CFFBuildingManager::BuildDispenser()
{
    // placeholder
}

int CFFBuildingManager::BuildSentry()
{
    if (m_iSentryLevel < 3)
        ++m_iSentryLevel;
    return m_iSentryLevel;
}

void CFFBuildingManager::BuildJumpPad()
{
    // placeholder
}

int CFFBuildingManager::GetSentryLevel() const
{
    return m_iSentryLevel;
}
