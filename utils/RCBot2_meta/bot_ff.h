// bot_ff.h
#ifndef __BOT_FF_H__
#define __BOT_FF_H__

#include "bot_fortress.h" // For CBotFortress base class

class CBotWeapon;
class CWaypoint;
class CBotUtility;

// NOTE: FF specific utility and schedule enums are now formally defined in
// bot_utility.h (as part of eBotAction) and bot_schedule.h (as part of eBotSchedule).
// The old #define placeholders have been removed from this file.

// Fallback defines for base game utilities/schedules might still be needed if they
// are not universally available or if FF uses a different set.
#ifndef BOT_UTIL_ATTACK
#define BOT_UTIL_ATTACK 70 // Example value, confirm from base
#endif
#ifndef BOT_UTIL_ROAM
#define BOT_UTIL_ROAM 21 // Example value, confirm from base
#endif
#ifndef BOT_UTIL_MEDIC_HEAL
#define BOT_UTIL_MEDIC_HEAL 34 // Example value, confirm from base
#endif
#ifndef BOT_UTIL_SNIPE
#define BOT_UTIL_SNIPE 20 // Example value, confirm from base
#endif

#ifndef SCHED_HEAL
#define SCHED_HEAL 11 // Example value, confirm from base
#endif
#ifndef SCHED_SNIPE
#define SCHED_SNIPE 10 // Example value, confirm from base
#endif

// This specific buildable ID should be moved to a game-specific const file (e.g. game_ff.h or similar)
// or used as a direct integer literal if it maps to an engine 'build' command ID.
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
    bool m_bWantsToAirblast; // For Pyro airblast
    float m_fNextAirblastTime; // Cooldown for Pyro airblast
    MyEHandle m_pNearestArmorItem;
    float m_fNextArmorCheckTime;
    CHandle<CBaseEntity> m_hBuiltSentryGun;
    float m_fNextSentryBuildTime;

private: // FF Specific Helper
    const CFFPlayerClassInfo* GetClassGameData() const;
    // Scout specific logic helpers
    void getScoutTasks(unsigned int iIgnore);
    bool executeScoutAction(CBotUtility *util);
    // Demoman specific logic helpers
    void getDemomanTasks(unsigned int iIgnore);
    bool executeDemomanAction(CBotUtility *util);
    // Engineer specific logic helpers
    void getEngineerTasks(unsigned int iIgnore);
    bool executeEngineerAction(CBotUtility *util);
    // Pyro specific logic helpers
    void getPyroTasks(unsigned int iIgnore);
    bool executePyroAction(CBotUtility *util);
    void pyroModThink();
    // HWGuy specific logic helpers
    void getHWGuyTasks(unsigned int iIgnore);
    void hwguyModThink();
    // Medic specific logic helpers
    bool executeMedicAction(CBotUtility *util);
    // Sniper specific logic helpers
    bool executeSniperAction(CBotUtility *util);
    // Spy specific logic helpers
    void getSpyTasks(unsigned int iIgnore);
    bool executeSpyAction(CBotUtility *util);

public: // CClassInterface Overrides
    virtual int GetMaxHP() const override;
    virtual int GetMaxAP() const override;
    virtual float GetMaxSpeed() const override;
    virtual weapon_t GetWeaponByIndex(int index) const override;
    virtual weapon_t GetGrenade1WeaponID() const override;
    virtual weapon_t GetGrenade2WeaponID() const override;
    virtual int GetMaxGren1() const override;
    virtual int GetMaxGren2() const override;
    virtual int GetInitialGren1() const override;
    virtual int GetInitialGren2() const override;
    virtual int GetMaxAmmo(int ammoIndex) const override;

    // Dynamic State Accessors
    virtual int GetCurrentHP() const override;
    virtual int GetCurrentAP() const override;
    virtual float GetCurrentSpeed() const override;
    virtual int GetAmmoCount(int ammoIndex) const override;
    virtual int GetGrenade1Count() const override;
    virtual int GetGrenade2Count() const override;
    virtual bool IsPlayerCloaked() const;
    virtual int GetPlayerJetpackFuel() const;
    virtual bool IsPlayerBuilding() const;
    virtual bool IsPlayerPrimingGrenade() const;
};

// Hunted Mode Tasks & Schedules
class CTaskFFFindEscapeRoute : public CBotTask {
public:
    CTaskFFFindEscapeRoute();
    virtual void init(CBot* pBot) override;
    virtual void execute(CBot* pBot) override;
    virtual bool isTaskComplete(CBot* pBot) override;
    virtual const char* getTaskName() override;
private:
    bool m_bRouteFound;
};

class CSchedFFHuntedVIPEscape : public CBotSchedule {
public:
    CSchedFFHuntedVIPEscape(CBotFF* pBot);
    virtual const char* getScheduleName() override;
};

class CTaskFFStayNearVIP : public CBotTask {
public:
    CTaskFFStayNearVIP(float fFollowDistance = 400.0f);
    virtual void init(CBot* pBot) override;
    virtual void execute(CBot* pBot) override;
    virtual bool isTaskComplete(CBot* pBot) override;
    virtual const char* getTaskName() override;
private:
    float m_fFollowDistance;
    float m_fNextRepathTime;
};

class CSchedFFHuntedProtectVIP : public CBotSchedule {
public:
    CSchedFFHuntedProtectVIP(CBotFF* pBot);
    virtual const char* getScheduleName() override;
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
