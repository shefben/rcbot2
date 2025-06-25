#ifndef FF_BUILDING_MANAGER_H
#define FF_BUILDING_MANAGER_H

class CFFBuildingManager
{
public:
    CFFBuildingManager();

    void Reset();
    void BuildDispenser();
    int  BuildSentry();
    void BuildJumpPad();

    int GetSentryLevel() const;

private:
    int m_iSentryLevel;
};

extern CFFBuildingManager g_FFBuildingManager;

#endif // FF_BUILDING_MANAGER_H
