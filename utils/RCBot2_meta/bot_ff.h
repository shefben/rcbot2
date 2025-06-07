// bot_ff.h
#ifndef __BOT_FF_H__
#define __BOT_FF_H__

#include "bot_fortress.h" // For CBotFortress base class

class CBotWeapon;
class CWaypoint;
class CBotUtility;

// Placeholder defines for FF specific enums
// IMPORTANT: These MUST be replaced with actual enum values from bot_const.h / bot_schedule.h
#define BOT_UTIL_FF_USE_GRENADE_STD 10001
#define BOT_UTIL_FF_USE_GRENADE_CONC 10002
#define BOT_UTIL_FF_USE_GRENADE_NAIL 10003
#define BOT_UTIL_FF_USE_GRENADE_MIRV 10004
#define BOT_UTIL_FF_USE_GRENADE_EMP 10005
#define BOT_UTIL_FF_USE_GRENADE_GAS 10006
#define BOT_UTIL_FF_USE_GRENADE_CALTROP 10007
#define BOT_UTIL_FF_CONC_JUMP_MOBILITY 10008
#define BOT_UTIL_FF_HUNTED_VIP_ESCAPE 10009
#define BOT_UTIL_FF_HUNTED_KILL_VIP 10010
#define BOT_UTIL_FF_HUNTED_PROTECT_VIP 10011
#define BOT_UTIL_FF_ENGRI_BUILD_MANCANNON 10012
#define BOT_UTIL_FF_DEMO_LAY_PIPE_TRAP 10013
#define BOT_UTIL_FF_SPY_USE_TRANQ 10014
#define BOT_UTIL_FF_SCOUT_USE_CALTROPS 10015
#define BOT_UTIL_FF_PYRO_AIRBLAST 10016
#define BOT_UTIL_FF_PYRO_USE_IC 10017
#define BOT_UTIL_FF_DEMO_DETONATE_PIPES 10018
#ifndef BOT_UTIL_ATTACK
#define BOT_UTIL_ATTACK 70
#endif
#ifndef BOT_UTIL_ROAM
#define BOT_UTIL_ROAM 21
#endif
#ifndef BOT_UTIL_MEDIC_HEAL
#define BOT_UTIL_MEDIC_HEAL 34
#endif
#ifndef BOT_UTIL_SNIPE
#define BOT_UTIL_SNIPE 20
#endif

#define SCHED_FF_PRIME_THROW_GRENADE 10201
#define SCHED_FF_CONC_JUMP_SELF 10202
#define SCHED_FF_HUNTED_ESCAPE 10203
#define SCHED_FF_HUNTED_GUARD_VIP 10204
#define SCHED_FF_ENGRI_BUILD_MANCANNON 10205
#define SCHED_FF_DEMO_LAY_PIPE_TRAP 10206
#define SCHED_FF_MEDIC_HEAL_TEAMMATE 10207
#define SCHED_FF_SNIPE_ATTACK 10208
#define SCHED_FF_DEMO_DETONATE_PIPES 10209
#define SCHED_FF_PYRO_AIRBLAST_DEFEND 10210
#define SCHED_FF_PYRO_EXTINGUISH_TEAMMATE 10211
#ifndef SCHED_HEAL
#define SCHED_HEAL 11
#endif
#ifndef SCHED_SNIPE
#define SCHED_SNIPE 10
#endif

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
