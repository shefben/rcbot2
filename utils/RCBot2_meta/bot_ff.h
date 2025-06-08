// bot_ff.h
#ifndef __BOT_FF_H__
#define __BOT_FF_H__

#include "bot_fortress.h" // For CBotFortress base class

class CBotWeapon;
class CWaypoint;
class CBotUtility;

#define FF_ENGIBUILD_MANCANNON 1

class CBotFF : public CBotFortress {
public:
	CBotFF();
	virtual ~CBotFF();
	virtual void modThink ();
	virtual bool isEnemy ( edict_t *pEdict,bool bCheckWeapons = true );
	virtual void getTasks(unsigned int iIgnore = 0);
	virtual bool executeAction(CBotUtility *util);
	virtual void handleWeapons();
	virtual bool handleAttack(CBotWeapon *pWeapon, edict_t *pEnemy);
	virtual void modAim(edict_t *pEntity, Vector &v_origin, Vector *v_desired_offset, Vector &v_size, float fDist, float fDist2D);
	virtual void chooseClass();
	virtual void selectClass();
	virtual bool isZoomed();
	float m_fGrenadePrimeStartTime;
	bool  m_bIsPrimingGrenade;
	edict_t* m_pGrenadeTargetEnt;
	Vector   m_vGrenadeTargetPos;
	float m_fPrimeDuration;
	edict_t* m_pVIP;
	bool     m_bIsVIP;
	static bool s_IsHuntedModeForTesting;
    MyEHandle m_hBuiltMancannon;
    float m_fNextMancannonBuildTime;
    bool m_bHasActivePipes;
    Vector m_vLastPipeTrapLocation;
    int m_iPipesToLay;
    float m_fNextPipeLayTime;
    bool m_bWantsToAirblast;
    float m_fNextAirblastTime;
};
class CTaskFFPrimeGrenade : public CBotTask {
public:
	CTaskFFPrimeGrenade(const Vector &vTargetPos, float fDuration);
	virtual void init(CBot* pBot);
	virtual void execute(CBot* pBot);
	virtual bool isTaskComplete(CBot* pBot);
	virtual const char *getTaskName();
private:
	Vector m_vTargetPos;
	float m_fPrimeDuration;
	float m_fPrimeStartTime;
};
class CTaskFFThrowGrenade : public CBotTask {
public:
	CTaskFFThrowGrenade(const Vector &vTargetPos, CBotWeapon* pGrenadeWeapon);
	virtual void execute(CBot* pBot);
	virtual bool isTaskComplete(CBot* pBot);
	virtual const char *getTaskName();
private:
	Vector m_vTargetPos;
	CBotWeapon* m_pGrenadeWeapon;
};
class CTaskFFExecuteConcJump : public CBotTask {
public:
	CTaskFFExecuteConcJump();
	virtual void execute(CBot* pBot);
	virtual bool isTaskComplete(CBot* pBot);
	virtual const char *getTaskName();
};
class CTaskFFEngineerBuild : public CBotTask {
public:
    CTaskFFEngineerBuild(int buildableId, const Vector& buildPos);
    virtual void execute(CBot* pBot);
    virtual bool isTaskComplete(CBot* pBot);
    virtual const char* getTaskName();
private:
    int m_buildableId;
    Vector m_vBuildPos;
    bool m_bCommandSent;
};
class CTaskFFDemoLaySinglePipe : public CBotTask {
public:
    CTaskFFDemoLaySinglePipe(const Vector& vTargetPos);
    virtual void execute(CBot* pBot);
    virtual bool isTaskComplete(CBot* pBot);
    virtual const char* getTaskName();
private:
    Vector m_vTargetPos;
    bool m_bFired;
};
class CTaskFFDemoDetonatePipes : public CBotTask {
public:
    CTaskFFDemoDetonatePipes();
    virtual void execute(CBot* pBot);
    virtual bool isTaskComplete(CBot* pBot);
    virtual const char* getTaskName();
};
class CTaskFFMedicAimAndHeal : public CBotTask {
public:
    CTaskFFMedicAimAndHeal(edict_t* pTarget);
    virtual void init(CBot* pBot);
    virtual void execute(CBot* pBot);
    virtual bool isTaskComplete(CBot* pBot);
    virtual const char* getTaskName();
private:
    MyEHandle m_hTarget;
};
class CTaskFFSnipeAttackSequence : public CBotTask {
public:
    CTaskFFSnipeAttackSequence(edict_t* pTarget);
    virtual void init(CBot* pBot);
    virtual void execute(CBot* pBot);
    virtual bool isTaskComplete(CBot* pBot);
    virtual const char* getTaskName();
private:
    MyEHandle m_hTarget;
    int m_iState;
    float m_fNextActionTime;
};
class CTaskFFPyroAirblast : public CBotTask {
public:
    CTaskFFPyroAirblast(edict_t* pTarget = NULL);
    virtual void init(CBot* pBot);
    virtual void execute(CBot* pBot);
    virtual bool isTaskComplete(CBot* pBot);
    virtual const char* getTaskName();
private:
    MyEHandle m_hTargetEntity;
};
class CSchedFFPrimeThrowGrenade : public CBotSchedule {
public:
	CSchedFFPrimeThrowGrenade(const Vector &vTargetPos, CBotWeapon* pGrenadeWeapon, float fPrimeTime = 1.0f);
	virtual const char *getScheduleName();
};
class CSchedFFConcJumpSelf : public CBotSchedule {
public:
	CSchedFFConcJumpSelf(CBotFF* pBot, CBotWeapon* pConcGrenade);
	virtual const char *getScheduleName();
};
class CSchedFFBuildMancannon : public CBotSchedule {
public:
    CSchedFFBuildMancannon(CBotFF* pBot, CWaypoint* pBuildSpot);
    virtual const char* getScheduleName();
};
class CSchedFFDemoLayPipeTrap : public CBotSchedule {
public:
    CSchedFFDemoLayPipeTrap(CBotFF* pBot, CWaypoint* pTrapSpotWpt, int numPipes = 3);
    virtual const char* getScheduleName();
};
class CSchedFFDemoDetonatePipes : public CBotSchedule {
public:
    CSchedFFDemoDetonatePipes(CBotFF* pBot);
    virtual const char* getScheduleName();
};
class CSchedFFMedicHealTeammate : public CBotSchedule {
public:
    CSchedFFMedicHealTeammate(CBotFF* pBot, edict_t* pTargetTeammate);
    virtual const char* getScheduleName();
};
class CSchedFFSnipe : public CBotSchedule {
public:
    CSchedFFSnipe(CBotFF* pBot, edict_t* pTarget);
    virtual const char* getScheduleName();
};
class CSchedFFPyroAirblast : public CBotSchedule {
public:
    CSchedFFPyroAirblast(CBotFF* pBot, edict_t* pTarget = NULL);
    virtual const char* getScheduleName();
};
#endif // __BOT_FF_H__
