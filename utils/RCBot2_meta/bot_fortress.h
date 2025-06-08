/*
 *    part of https://rcbot2.svn.sourceforge.net/svnroot/rcbot2
 *
 *    This file is part of RCBot.
 *
 *    RCBot by Paul Murphy adapted from Botman's HPB Bot 2 template.
 *
 *    RCBot is free software; you can redistribute it and/or modify it
 *    under the terms of the GNU General Public License as published by the
 *    Free Software Foundation; either version 2 of the License, or (at
 *    your option) any later version.
 *
 *    RCBot is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with RCBot; if not, write to the Free Software Foundation,
 *    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    In addition, as a special exception, the author gives permission to
 *    link the code of this program with the Half-Life Game Engine ("HL
 *    Engine") and Modified Game Libraries ("MODs") developed by Valve,
 *    L.L.C ("Valve").  You must obey the GNU General Public License in all
 *    respects for all of the code used other than the HL Engine and MODs
 *    from Valve.  If you modify this file, you may extend this exception
 *    to your version of the file, but you are not obligated to do so.  If
 *    you do not wish to do so, delete this exception statement from your
 *    version.
 *
 */
#ifndef __BOT_FORTRESS_H__
#define __BOT_FORTRESS_H__

#include "bot_utility.h"

//#include <stack>

#define TF2_ROCKETSPEED   1100
#define TF2_GRENADESPEED  1065 // TF2 wiki
#define TF2_MAX_SENTRYGUN_RANGE 1024
#define TF2_STICKYGRENADE_MAX_DISTANCE 1600

class CBotWeapon;
class CWaypoint;
class CBotUtility;

#define TF2_SLOT_PRMRY 0 // primary
#define TF2_SLOT_SCNDR 1 // secondary
#define TF2_SLOT_MELEE 2
#define TF2_SLOT_PDA 3
#define TF2_SLOT_PDA2 4
#define TF2_SLOT_HAT 5
#define TF2_SLOT_MISC 6
#define TF2_SLOT_ACTION 7
#define TF2_SLOT_MAX 8

#define TF2_TEAM_BLUE 3
#define TF2_TEAM_RED 2

#define RESIST_BULLETS 0
#define RESIST_EXPLO 1
#define RESIST_FIRE 2

#define TF2_SENTRY_LEVEL1_HEALTH 150
#define TF2_SENTRY_LEVEL2_HEALTH 180
#define TF2_SENTRY_LEVEL3_HEALTH 216

#define TF2_DISPENSER_LEVEL1_HEALTH 150
#define TF2_DISPENSER_LEVEL2_HEALTH 180
#define TF2_DISPENSER_LEVEL3_HEALTH 216

typedef enum
{
    TF_VC_MEDIC = 0,
	TF_VC_INCOMING = 1,
	TF_VC_HELP = 2,
    TF_VC_THANKS = 4,
	TF_VC_SPY = 5,
	TF_VC_BATTLECRY = 6,
    TF_VC_GOGOGO = 8,
    TF_VC_SENTRYAHEAD = 9,
	TF_VC_CHEERS = 10,
    TF_VC_MOVEUP = 12,
    TF_VC_TELEPORTERHERE =13,
    TF_VC_JEERS = 14,
    TF_VC_GOLEFT = 16,
	TF_VC_DISPENSERHERE = 17,
	TF_VC_POSITIVE = 18,
    TF_VC_GORIGHT = 20,
    TF_VC_SENTRYHERE = 21,
    TF_VC_NEGATIVE = 22,
    TF_VC_YES = 24,
    TF_VC_ACTIVATEUBER = 25,
    TF_VC_NICESHOT = 26,
    TF_VC_NO = 28,
    TF_VC_UBERREADY = 29,
    TF_VC_GOODJOB = 30,
	TF_VC_INVALID = 31
}eTFVoiceCMD;


typedef enum
{
	TF_TRAP_TYPE_NONE,
	TF_TRAP_TYPE_WPT,
	TF_TRAP_TYPE_POINT,
	TF_TRAP_TYPE_FLAG,
	TF_TRAP_TYPE_PL,
	TF_TRAP_TYPE_ENEMY
}eDemoTrapType;

typedef enum
{
	TF_CLASS_CIVILIAN = 0,
	TF_CLASS_SCOUT,
	TF_CLASS_SNIPER,
	TF_CLASS_SOLDIER,
	TF_CLASS_DEMOMAN,
	TF_CLASS_MEDIC,
	TF_CLASS_HWGUY,
	TF_CLASS_PYRO,
	TF_CLASS_SPY,
	TF_CLASS_ENGINEER,
	TF_CLASS_MAX
}TF_Class;

typedef enum
{
	ENGI_DISP = 0,
	ENGI_TELE,
	ENGI_SENTRY,
	ENGI_SAPPER,
	ENGI_EXIT,
	ENGI_ENTRANCE,
}eEngiBuild;

typedef enum
{
	ENGI_BUILD,
	ENGI_DESTROY
}eEngiCmd;

class CBotTF2FunctionEnemyAtIntel : public IBotFunction
{
public:
	CBotTF2FunctionEnemyAtIntel( int iTeam, Vector vPos, int type, edict_t *pPlayer = NULL, int capindex = -1 ){m_iTeam = iTeam;m_vPos = vPos;m_iType = type; m_pPlayer = pPlayer; m_iCapIndex = capindex; }

	void execute (CBot *pBot);
private:
	int m_iTeam;
	Vector m_vPos;
	int m_iType;
	edict_t *m_pPlayer;
	int m_iCapIndex;
};

class CBroadcastSpySap : public IBotFunction
{
public:
	CBroadcastSpySap (edict_t *pSpy) { m_pSpy = pSpy; }
	void execute ( CBot *pBot );

private:
	edict_t *m_pSpy;
};

class CBroadcastOvertime : public IBotFunction
{
public:
	CBroadcastOvertime () {};
	void execute (CBot *pBot);
};

class CBroadcastFlagReturned : public IBotFunction
{
public:
	CBroadcastFlagReturned (int iTeam) { m_iTeam = iTeam; }
	void execute ( CBot *pBot );

private:
	int m_iTeam;
};

class CBroadcastFlagDropped : public IBotFunction
{
public:
	CBroadcastFlagDropped (int iTeam, Vector origin) { m_iTeam = iTeam; m_vOrigin = origin; }
	void execute ( CBot *pBot );

private:
	Vector m_vOrigin;
	int m_iTeam;
};

class CBroadcastFlagCaptured : public IBotFunction
{
public:
	CBroadcastFlagCaptured(int iTeam) { m_iTeam = iTeam; }

	void execute ( CBot *pBot );
private:
	int m_iTeam;
};

class CBroadcastRoundStart : public IBotFunction
{
public:
	CBroadcastRoundStart ( bool bFullReset ) { m_bFullReset = bFullReset; }
	void execute ( CBot *pBot );
private:
	bool m_bFullReset;
};

class CBroadcastCapturedPoint : public IBotFunction
{
public:
	CBroadcastCapturedPoint ( int iPoint, int iTeam, const char *szName );

	void execute ( CBot *pBot );
private:
	int m_iPoint;
	int m_iTeam;
	const char *m_szName;
};

#define EVENT_FLAG_PICKUP 0
#define EVENT_CAPPOINT    1


class CBotFortress : public CBot
{
public:	

	CBotFortress();

	virtual void enemyLost (edict_t *pEnemy);
	virtual void updateConditions();
	virtual void shot ( edict_t *pEnemy );
	virtual int engiBuildObject ( int *iState, eEngiBuild iObject, float *fTime, int *iTries );
	virtual float getEnemyFactor ( edict_t *pEnemy ) { return CBot::getEnemyFactor(pEnemy); }
	virtual void checkDependantEntities();
	int getMetal ();
	virtual void modAim ( edict_t *pEntity, Vector &v_origin, Vector *v_desired_offset, Vector &v_size, float fDist, float fDist2D )
	{
		CBot::modAim(pEntity,v_origin,v_desired_offset,v_size,fDist,fDist2D);
	}
	virtual void touchedWpt ( CWaypoint *pWaypoint, int iNextWaypoint = -1, int iPrevWaypoint = -1 ) { CBot::touchedWpt(pWaypoint); }
	inline edict_t *getHealingEntity () { return m_pHeal; }
	inline void clearHealingEntity () { m_pHeal = NULL; }
	virtual unsigned int maxEntityIndex ( ) { return gpGlobals->maxEntities; }
	virtual void init (bool bVarInit=false);
	virtual void foundSpy (edict_t *pEdict, TF_Class iDisguise );
	virtual void getTasks ( unsigned int iIgnore = 0 ) { CBot :: getTasks(iIgnore); }
	virtual void died ( edict_t *pKiller, const char *pszWeapon );
	virtual void killed ( edict_t *pVictim, char *weapon );
	virtual void modThink ();
	bool isBuilding ( edict_t *pBuilding );
	float getHealFactor ( edict_t *pPlayer );
	virtual bool wantToFollowEnemy ();
	virtual void checkBuildingsValid (bool bForce = false) {};
	virtual void checkHealingValid ();
	virtual edict_t *findEngineerBuiltObject ( eEngiBuild iBuilding, int index ) { return NULL; }
	virtual void engineerBuild ( eEngiBuild iBuilding, eEngiCmd iEngiCmd ) {};
	virtual void spyDisguise ( int iTeam, int iClass ) {};
	virtual bool lookAfterBuildings (float *fTime) { return false; }
	inline void nextLookAfterSentryTime ( float fTime ) { m_fLookAfterSentryTime = fTime; }
	inline edict_t *getSentry () { return m_pSentryGun; }
	virtual bool hasEngineerBuilt ( eEngiBuild iBuilding ) {return false;}
	virtual void engiBuildSuccess ( eEngiBuild iObject, int index ) {};
	virtual bool healPlayer ( edict_t *pPlayer, edict_t *pPrevPlayer ) { return false; }
	virtual bool upgradeBuilding ( edict_t *pBuilding, bool removesapper = false) {return false;}
	virtual bool isCloaked () { return false; }
	virtual bool isDisguised () { return false; }
	virtual CBotWeapon *getCurrentWeapon()
	{
		return CBot::getCurrentWeapon();
	}
	virtual bool handleAttack(CBotWeapon *pWeapon, edict_t *pEnemy) { return CBot::handleAttack(pWeapon, pEnemy); }
	void resetAttackingEnemy() { m_pAttackingEnemy = NULL; }
	virtual bool setVisible ( edict_t *pEntity, bool bVisible );
	virtual void setClass ( TF_Class _class );
	inline edict_t *seeFlag ( bool reset = false ) { if ( reset ) { m_pFlag = NULL; } return m_pFlag; }
	virtual bool canAvoid ( edict_t *pEntity );
	virtual bool isEnemy ( edict_t *pEdict,bool bCheckWeapons = true );
	virtual bool startGame ();
	virtual void spawnInit ();
	bool isTF () { return true; }
	virtual bool isTF2 () { return false; }
	virtual bool hurt ( edict_t *pAttacker, int iHealthNow, bool bDontHide  = false )
	{
		return CBot::hurt(pAttacker,iHealthNow,bDontHide);
	}
	void chooseClass ();
	virtual TF_Class getClass () { return TF_CLASS_CIVILIAN; }
	virtual void updateClass () { };
	virtual void currentlyDead ();
	virtual void onInventoryApplication (){}
	void pickedUpFlag ();
	inline bool hasFlag () { return m_bHasFlag; }
	inline void droppedFlag () { m_bHasFlag = false; }
	void medicCalled ( edict_t *pPlayer );
	bool isAlive ();
	void enemyDown (edict_t *pEnemy) 
	{ 
		CBot::enemyDown(pEnemy);

		if ( pEnemy == m_pPrevSpy )
		{
			m_pPrevSpy = NULL;
			m_fSeeSpyTime = 0.0f;
		}
	}
	bool isTeleporterUseful ( edict_t *pTele );
	bool waitForFlag ( Vector *vOrigin, float *fWait, bool bFindFlag );
	void flagDropped ( Vector vOrigin );
	void teamFlagDropped ( Vector vOrigin );
	void teamFlagPickup ();
	virtual bool wantToListenToPlayer ( edict_t *pPlayer, int iWeaponID = -1 ) { return true; }
	virtual bool wantToListenToPlayerFootsteps ( edict_t *pPlayer ) { return true; }
	virtual bool wantToInvestigateSound () { return true; }
	inline void flagReset () { m_fLastKnownFlagTime = 0.0f; }
	inline void teamFlagReset () { m_fLastKnownTeamFlagTime = 0.0f; }
	virtual bool canGotoWaypoint ( Vector vPrevWaypoint, CWaypoint *pWaypoint, CWaypoint *pPrev = NULL )
	{
		return CBot::canGotoWaypoint(vPrevWaypoint,pWaypoint,pPrev);
	}
	virtual void setup ();
	virtual bool needHealth();
	virtual bool needAmmo ();
	void waitBackstab ();
	void wantToDisguise ( bool bSet );
	virtual bool select_CWeapon ( CWeapon *pWeapon ) { return CBot::select_CWeapon(pWeapon); }
	virtual bool selectBotWeapon ( CBotWeapon *pBotWeapon ) { return CBot::selectBotWeapon(pBotWeapon); }
	virtual bool getIgnoreBox ( Vector *vLoc, float *fSize );
	virtual void enemyFound (edict_t *pEnemy){CBot::enemyFound(pEnemy); }
	bool wantToNest ();
	bool overrideAmmoTypes () { return false; }
	bool wantToCloak();
	bool wantToUnCloak();
	bool someoneCalledMedic ();
	void waitCloak ();
	void detectedAsSpy ( edict_t *pDetector, bool bDisguiseComprimised );
	virtual edict_t *getVisibleSpecial ();
	inline bool isBeingHealed () { return m_bIsBeingHealed; }
	virtual void handleWeapons () { CBot::handleWeapons(); }
	virtual void seeFriendlyDie ( edict_t *pDied, edict_t *pKiller, CWeapon *pWeapon ) { CBot::seeFriendlyDie(pDied,pKiller,pWeapon); }
	virtual void seeFriendlyKill ( edict_t *pTeamMate, edict_t *pDied, CWeapon *pWeapon ) { CBot::seeFriendlyKill(pTeamMate,pDied,pWeapon); }
	virtual void voiceCommand ( int cmd ) { };
	virtual void seeFriendlyHurtEnemy ( edict_t *pTeammate, edict_t *pEnemy, CWeapon *pWeapon );
	bool incomingRocket ( float fRange );
	virtual void hearPlayerAttack( edict_t *pAttacker, int iWeaponID ) { CBot::hearPlayerAttack(pAttacker,iWeaponID); }
protected:
	virtual void selectTeam ();
	virtual void selectClass ();
	virtual void callMedic ();
	static bool isClassOnTeam ( int iClass, int iTeam );
	int getSpyDisguiseClass ( int iTeam );
	virtual bool thinkSpyIsEnemy ( edict_t *pEdict, TF_Class iDisguise );
	virtual bool checkStuck ( void ) { return CBot::checkStuck(); }

	float m_fCallMedic;
	float m_fTauntTime;
	float m_fTaunting;
	float m_fDefendTime;
	float m_fHealFactor;
	MyEHandle m_pHeal;
	MyEHandle m_pLastHeal;
	MyEHandle m_pSentryGun;
	MyEHandle m_pDispenser;
	MyEHandle m_pTeleEntrance;
	MyEHandle m_pTeleExit;
	MyEHandle m_pAmmo;
	MyEHandle m_pHealthkit;
	MyEHandle m_pNearestDisp;
	MyEHandle m_pNearestEnemySentry;
	MyEHandle m_pNearestAllySentry;
	MyEHandle m_pNearestEnemyTeleporter;
	MyEHandle m_pNearestEnemyDisp;
	MyEHandle m_pNearestTeleEntrance;
	MyEHandle m_pNearestPipeGren;
	MyEHandle m_pAttackingEnemy;
	MyEHandle m_pFlag;
	MyEHandle m_pPrevSpy;
	float m_fFrenzyTime;
	float m_fSpyCloakTime;
	float m_fSpyUncloakTime;
	float m_fSeeSpyTime;
	float m_fLastSeeSpyTime;
	float m_fSpyDisguiseTime;
	float m_fLastSaySpy;
	float m_fPickupTime;
	float m_fLookAfterSentryTime;
	TF_Class m_iPrevSpyDisguise;
	Vector m_vLastSeeSpy;
	Vector m_vLastKnownFlagPoint;
	Vector m_vLastKnownTeamFlagPoint;
	Vector m_vTeleportEntrance;
	bool m_bEntranceVectorValid;
	Vector m_vSentryGun;
	bool m_bSentryGunVectorValid;
	Vector m_vDispenser;
	bool m_bDispenserVectorValid;
	Vector m_vTeleportExit;
	bool m_bTeleportExitVectorValid;
	float m_fLastKnownFlagTime;
	float m_fLastKnownTeamFlagTime;
	float m_fBackstabTime;
	TF_Class m_iClass; // This might need to be an FF-specific enum or int if TF_Class is too TF2-centric
	float m_fUpdateClass;
	float m_fUseTeleporterTime;
	bool m_bHasFlag;	
	float m_fSnipeAttackTime;
	float m_fChangeClassTime;
	bool m_bCheckClass;
	MyEHandle m_pLastCalledMedic;
	CBotLastSee m_pLastSeeMedic;
	float m_fLastCalledMedicTime;
	bool m_bIsBeingHealed;
	float m_fMedicUpdatePosTime;
	Vector m_vMedicPosition;
	bool m_bCanBeUbered;
	float m_fCheckHealTime;
	float m_fClassDisguiseFitness[10];
	float m_fClassDisguiseTime[10];
	float m_fDisguiseTime;
	unsigned short m_iDisguiseClass;
	float m_fSentryPlaceTime;
	unsigned int m_iSentryKills;
	float m_fTeleporterEntPlacedTime;
	float m_fTeleporterExtPlacedTime;
	unsigned m_iTeleportedPlayers;
	float m_fSpyList[MAX_PLAYERS];
	int m_iTeam;
	float m_fWaitTurnSentry;
	float m_fCallMedicTime[MAX_PLAYERS];
	int m_iLastFailSentryWpt;
	int m_iLastFailTeleExitWpt;
	MyEHandle m_pHealer;
	float m_fHealingMoveTime;
	MyEHandle m_pLastEnemySentry;
	MyEHandle m_NearestEnemyRocket;
	MyEHandle m_NearestEnemyGrenade;
	float m_fLastSentryEnemyTime;
};

class CBotTF2 : public CBotFortress
{
public:
	CBotTF2();
	virtual CBotWeapon *getCurrentWeapon();
	void onInventoryApplication();
	void MannVsMachineWaveComplete();
	void MannVsMachineAlarmTriggered (Vector vLoc);
	bool sentryRecentlyHadEnemy ();
	void highFivePlayer ( edict_t *pPlayer, float fYaw );
	virtual bool hurt ( edict_t *pAttacker, int iHealthNow, bool bDontHide  = false );
	void updateAttackDefendPoints ();
	void updateAttackPoints ();
	void updateDefendPoints ();
	void enemyFound (edict_t *pEnemy);
	void enemyAtIntel ( Vector vPos, int type = EVENT_FLAG_PICKUP, int iArea = -1 );
	bool isTF2 () { return true; }
	void checkDependantEntities ();
	virtual bool wantToListenToPlayerAttack ( edict_t *pPlayer, int iWeaponID = -1 );
	virtual bool wantToListenToPlayerFootsteps ( edict_t *pPlayer );
	bool wantToInvestigateSound ();
	void getDefendArea ( std::vector<int> *m_iAreas );
	void getAttackArea ( std::vector <int> *m_iAreas );
	int getCurrentAttackArea () { return m_iCurrentAttackArea; }
	int getCurrentDefendArea () { return m_iCurrentDefendArea; }
	void pointsUpdated ( );
	eBotFuncState rocketJump(int *iState,float *fTime);
	virtual bool wantToFollowEnemy ();
	void resetCloakTime () { m_fSpyCloakTime = 0.0f; }
	float getEnemyFactor ( edict_t *pEnemy );
	void foundSpy (edict_t *pEdict, TF_Class iDisguise);
	void touchedWpt ( CWaypoint *pWaypoint, int iNextWaypoint = -1, int iPrevWaypoint = -1 );
	bool handleAttack ( CBotWeapon *pWeapon, edict_t *pEnemy );
	void engiBuildSuccess ( eEngiBuild iObject, int index );
	bool lookAfterBuildings (float *fTime);
	void spawnInit ();
	bool setVisible ( edict_t *pEntity, bool bVisible );
	virtual void modAim ( edict_t *pEntity, Vector &v_origin, 
		Vector *v_desired_offset, Vector &v_size,
		float fDist, float fDist2D);
	void modThink ();
	bool isCloaked ();
	bool executeAction ( CBotUtility *util );
	void setClass ( TF_Class _class );
	bool isDisguised ();
	void checkBuildingsValid (bool bForce = false);
	edict_t *findEngineerBuiltObject ( eEngiBuild iBuilding, int index );
	bool isEnemy ( edict_t *pEdict,bool bCheckWeapons = true );
	bool isTF () { return true; }
	void taunt ( bool bOverride = false );
	void callMedic ();
	void roundReset (bool bFullReset);
	void pointCaptured ( int iPoint, int iTeam, const char *szPointName );
	void engineerBuild ( eEngiBuild iBuilding, eEngiCmd iEngiCmd );
	void spyDisguise ( int iTeam, int iClass );
	bool hasEngineerBuilt ( eEngiBuild iBuilding );
	void getTasks ( unsigned int iIgnore = 0 );
	void died ( edict_t *pKiller, const char *pszWeapon );
	void killed ( edict_t *pVictim, char *weapon );
	void capturedFlag ();
	void pointCaptured ();
	void waitRemoveSap ();
	void roundWon ( int iTeam, bool bFullRound );
	void changeClass ();
	virtual bool needAmmo();
	void buildingDestroyed ( int iType, edict_t *pAttacker, edict_t *pEdict );
	TF_Class getClass ();
	void updateClass ();
	bool healPlayer ( );
	bool upgradeBuilding ( edict_t *pBuilding, bool removesapper = false );
	void setup ();
	void buildingSapped ( eEngiBuild building, edict_t *pSapper, edict_t *pSpy );
	void sapperDestroyed ( edict_t *pSapper );
	bool canGotoWaypoint ( Vector vPrevWaypoint, CWaypoint *pWaypoint, CWaypoint *pPrev = NULL );
	bool deployStickies ( eDemoTrapType type, Vector vStand, Vector vLocation, Vector vSpread, Vector *vPoint, int *iState, int *iStickyNum, bool *bFail, float *fTime, int wptindex );
	void detonateStickies (bool isJumping = false);
	void setStickyTrapType ( Vector vLocation, eDemoTrapType iTrapType ) { m_vStickyLocation = vLocation; m_iTrapType = iTrapType; }
	bool canDeployStickies ();
	bool thinkSpyIsEnemy ( edict_t *pEdict, TF_Class iDisguise );
	void seeFriendlyDie ( edict_t *pDied, edict_t *pKiller, CWeapon *pWeapon );
	void seeFriendlyKill ( edict_t *pTeamMate, edict_t *pDied, CWeapon *pWeapon );
	void voiceCommand ( int cmd );
	void handleWeapons ( void ) ;
	virtual bool select_CWeapon ( CWeapon *pWeapon );
	virtual bool selectBotWeapon ( CBotWeapon *pBotWeapon );
	void checkStuckonSpy ( void );
	bool checkStuck ( void );
	void init (bool bVarInit=false);
	bool checkAttackPoint ( void );
	bool canAvoid ( edict_t *pEntity );
	void hearVoiceCommand ( edict_t *pPlayer, byte cmd );
	void checkBeingHealed ( );
	void spyCloak ();
	void spyUnCloak ();
	void healedPlayer ( edict_t *pPlayer, float fAmount );
	void teleportedPlayer ( void );
	inline bool isCarrying () { return m_bIsCarryingObj; }
	void updateCarrying ();
	inline void resetCarryTime () { m_fCarryTime = engine->Time(); }
	void MvM_Upgrade ();
private:
	float m_fDoubleJumpTime;
	float m_fSpySapTime;
	int m_iCurrentDefendArea;
	int m_iCurrentAttackArea;
	MyEHandle m_pDefendPayloadBomb;
	MyEHandle m_pPushPayloadBomb;
	MyEHandle m_pRedPayloadBomb;
	MyEHandle m_pBluePayloadBomb;
	eDemoTrapType m_iTrapType;
	int m_iTrapCPIndex;
	Vector m_vStickyLocation;
	float m_fRemoveSapTime;
	float m_fRevMiniGunTime;
	float m_fNextRevMiniGunTime;
	float m_fRevMiniGunBelief;
	float m_fCloakBelief;
	MyEHandle m_pCloakedSpy;
	float m_fAttackPointTime;
	 float m_prevSentryHealth;
	 float m_prevDispHealth;
	 float m_prevTeleExtHealth;
	 float m_prevTeleEntHealth;
	 float m_fDispenserHealAmount;
	 float m_fDispenserPlaceTime;
	 int m_iSentryArea;
	 int m_iDispenserArea;
	 int m_iTeleEntranceArea;
	 int m_iTeleExitArea;
	 eTFVoiceCMD m_nextVoicecmd;
	bool m_bIsCarryingTeleExit;
	bool m_bIsCarryingSentry;
	bool m_bIsCarryingDisp;
	bool m_bIsCarryingTeleEnt;
	bool m_bIsCarryingObj;
	float m_fCarryTime;
	float m_fCheckNextCarrying;
	void *m_pVTable;
	float m_fUseBuffItemTime;
	int m_iDesiredResistType;
};


#endif
