#ifndef __BOT_TASK_FF_H__
#define __BOT_TASK_FF_H__

#include "bot_task.h"

class CBotTaskFFMedicHeal : public CBotTask
{
public:
    CBotTaskFFMedicHeal() {}
    bool IsComplete() const { return m_iState == STATE_COMPLETE; }
};

class CBotTaskFFEngineerBuild : public CBotTask
{
public:
    CBotTaskFFEngineerBuild() {}
    bool IsComplete() const { return m_iState == STATE_COMPLETE; }
};

class CBotTaskFFCaptureFlag : public CBotTask
{
public:
    explicit CBotTaskFFCaptureFlag(int enemyTeam);
    void execute(CBot *pBot, CBotSchedule *pSchedule);
    bool IsComplete() const { return m_iState == STATE_COMPLETE; }
private:
    int m_iEnemyTeam;
};

class CBotTaskFFDefendFlag : public CBotTask
{
public:
    explicit CBotTaskFFDefendFlag(int team);
    void execute(CBot *pBot, CBotSchedule *pSchedule);
    bool IsComplete() const { return m_iState == STATE_COMPLETE; }
private:
    int m_iTeam;
};

#endif
