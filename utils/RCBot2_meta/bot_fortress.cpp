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
#include "bot_plugin_meta.h"

#include "bot.h"
#include "bot_cvars.h"

#include "ndebugoverlay.h"

#include "bot_fortress.h"
#include "bot_buttons.h"
#include "bot_globals.h"
#include "bot_profile.h"
#include "bot_schedule.h"
#include "bot_task.h"
#include "bot_waypoint.h"
#include "bot_navigator.h"
#include "bot_mods.h"
#include "bot_visibles.h"
#include "bot_weapons.h"
#include "bot_waypoint_locations.h"
#include "in_buttons.h"
#include "bot_utility.h"
#include "bot_configfile.h"
#include "bot_getprop.h"
#include "bot_mtrand.h"
#include "bot_wpt_dist.h"
#include "bot_squads.h"
//#include "bot_hooks.h"

//caxanga334: SDK 2013 contains macros for std::min and std::max which causes errors when compiling
#if SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_BMS
#include "valve_minmax_off.h"
#endif

extern IVDebugOverlay *debugoverlay;

#define TF2_SPY_CLOAK_BELIEF 40
#define TF2_HWGUY_REV_BELIEF 60
//extern float g_fBotUtilityPerturb [TF_CLASS_MAX][BOT_UTIL_MAX];

// Payload stuff by   The_Shadow

//#include "vstdlib/random.h" // for random functions

void CBroadcastOvertime ::execute (CBot*pBot)
{
	pBot->updateCondition(CONDITION_CHANGED);
	pBot->updateCondition(CONDITION_PUSH);
}
void CBroadcastCapturedPoint :: execute ( CBot *pBot )
{
	((CBotTF2*)pBot)->pointCaptured(m_iPoint,m_iTeam,m_szName);
}

CBroadcastCapturedPoint :: CBroadcastCapturedPoint ( int iPoint, int iTeam, const char *szName )
{
	m_iPoint = iPoint;
	m_iTeam = iTeam;
	m_szName = CStrings::getString(szName);
}

void CBroadcastSpySap :: execute ( CBot *pBot )
{
	if ( CTeamFortress2Mod::getTeam(m_pSpy) != pBot->getTeam() )
	{
		if ( pBot->isVisible(m_pSpy) )
			((CBotTF2*)pBot)->foundSpy(m_pSpy,CTeamFortress2Mod::getSpyDisguise(m_pSpy));
	}
}
// special delivery
void CBroadcastFlagReturned :: execute ( CBot*pBot )
{
	//if ( pBot->getTeam() == m_iTeam )
		//((CBotTF2*)pBot)->flagReturned_SD(m_vOrigin);
	//else
	//	((CBotTF2*)pBot)->teamFlagDropped(m_vOrigin);	
}

void CBroadcastFlagDropped :: execute ( CBot *pBot )
{
	if ( pBot->getTeam() == m_iTeam )
		((CBotTF2*)pBot)->flagDropped(m_vOrigin);
	else
		((CBotTF2*)pBot)->teamFlagDropped(m_vOrigin);
}
// flag picked up
void CBotTF2FunctionEnemyAtIntel :: execute (CBot *pBot)
{
	if ( m_iType == EVENT_CAPPOINT )
	{
		if ( CTeamFortress2Mod::m_ObjectiveResource.GetOwningTeam(m_iCapIndex) != pBot->getTeam() )
			return;
	}

	pBot->updateCondition(CONDITION_PUSH);

	if ( pBot->getTeam() != m_iTeam )
	{
		((CBotTF2*)pBot)->enemyAtIntel(m_vPos,m_iType,m_iCapIndex);
	}
	else
		((CBotTF2*)pBot)->teamFlagPickup();
}

void CBotTF2 :: hearVoiceCommand ( edict_t *pPlayer, byte cmd )
{
	switch ( cmd )
	{
	case TF_VC_SPY:
		// someone shouted spy, HACK the bot to think they saw a spy here too
		// for spy checking purposes
		if ( isVisible(pPlayer) )
		{
			m_vLastSeeSpy = CBotGlobals::entityOrigin(pPlayer);
			m_fSeeSpyTime = engine->Time() + randomFloat(3.0f,6.0f);
			//m_pPrevSpy = pPlayer; // HACK
		}
		break;
	// somebody shouted "MEDIC!"
	case TF_VC_MEDIC:
		medicCalled(pPlayer);
		break;
	case TF_VC_SENTRYHERE: // hear 'put sentry here' 
		// if I'm carrying a sentry just drop it here
		if ( getClass() == TF_CLASS_ENGINEER )
		{
			if ( m_bIsCarryingObj && m_bIsCarryingSentry )
			{
				if ( isVisible(pPlayer) && (distanceFrom(pPlayer) < 512) )
				{
					if ( randomInt(0,100) > 75 )
						addVoiceCommand(TF_VC_YES);

					primaryAttack();

					m_pSchedules->removeSchedule(SCHED_TF2_ENGI_MOVE_BUILDING);
				}
				else if ( randomInt(0,100) > 75 )
					addVoiceCommand(TF_VC_NO);
			}
		}
		break;
	case TF_VC_HELP:
		// add utility can find player
		if ( isVisible(pPlayer) )
		{
			if ( !m_pSchedules->isCurrentSchedule(SCHED_GOTO_ORIGIN) )
			{
				m_pSchedules->removeSchedule(SCHED_GOTO_ORIGIN);

				m_pSchedules->addFront(new CBotGotoOriginSched(pPlayer));
			}
		}
		break;
	case TF_VC_GOGOGO:
		// if bot is nesting, or waiting for something, it will go
		if ( distanceFrom(pPlayer) > 512 )
			return;

		updateCondition(CONDITION_PUSH);

		if ( randomFloat(0,1.0) > 0.75f )
			m_nextVoicecmd = TF_VC_YES;

		// don't break // flow down to uber if medic
	case TF_VC_ACTIVATEUBER:
		if ( CTeamFortress2Mod::hasRoundStarted() && (getClass() == TF_CLASS_MEDIC)  )
		{
			if ( m_pHeal == pPlayer )
			{
				if ( !CTeamFortress2Mod::isFlagCarrier(pPlayer) )
					secondaryAttack();
				else if ( randomFloat(0,1.0) > 0.5f )
					m_nextVoicecmd = TF_VC_NO;
			}
		}
		break;
	case TF_VC_MOVEUP:

		if ( distanceFrom(pPlayer) > 1000 )
			return;

		updateCondition(CONDITION_PUSH);

		if ( randomFloat(0,1.0) > 0.75f )
			m_nextVoicecmd = TF_VC_YES;

		break;
	default:
		break;
	}
}

void CBroadcastFlagCaptured :: execute ( CBot *pBot )
{
	if ( pBot->getTeam() == m_iTeam )
		((CBotTF2*)pBot)->flagReset();
	else
		((CBotTF2*)pBot)->teamFlagReset();
}

void CBroadcastRoundStart :: execute ( CBot *pBot )
{
	((CBotTF2*)pBot)->roundReset(m_bFullReset);
}

CBotFortress :: CBotFortress()
{ 
	CBot(); 

	m_iLastFailSentryWpt = -1;
	m_iLastFailTeleExitWpt = -1;

	// remember prev spy disguised in game while playing
	m_iPrevSpyDisguise = (TF_Class)0;

	m_fSentryPlaceTime = 0;
	m_iSentryKills = 0;
	m_fSnipeAttackTime = 0;
	m_pAmmo = NULL;
	m_pHealthkit = NULL;
	m_pFlag = NULL; 
	m_pHeal = NULL; 
	m_fCallMedic = 0; 
	m_fTauntTime = 0; 
	m_fLastKnownFlagTime = 0.0f; 
	m_bHasFlag = false; 
	m_pSentryGun = NULL; 
	m_pDispenser = NULL; 
	m_pTeleExit = NULL; 
	m_pTeleEntrance = NULL; 
	m_pNearestDisp = NULL;
	m_pNearestEnemySentry = NULL;
	m_pNearestEnemyTeleporter = NULL;
	m_pNearestEnemyDisp = NULL;
	m_pNearestPipeGren = NULL;
	m_pPrevSpy = NULL;
	m_fSeeSpyTime = 0.0f;
	m_bEntranceVectorValid = false;
	m_pLastCalledMedic = NULL;
	m_fLastCalledMedicTime = 0.0f;
	m_bIsBeingHealed = false;
	m_bCanBeUbered = false;
}

void CBotFortress :: checkDependantEntities ()
{
	CBot::checkDependantEntities();
}

void CBotFortress :: init (bool bVarInit)
{
	CBot::init(bVarInit);

	m_bCheckClass = false;
	m_bHasFlag = false;
	m_iClass = TF_CLASS_MAX; // important

}

void CBotFortress :: setup ()
{
	CBot::setup();
}

bool CBotFortress::someoneCalledMedic()
{
	return (getClass()==TF_CLASS_MEDIC) && 
			(m_pLastCalledMedic.get() != NULL) && 
			((m_fLastCalledMedicTime+30.0f)>engine->Time());
}

bool CBotTF2 :: sentryRecentlyHadEnemy ()
{
	return (m_fLastSentryEnemyTime + 15.0f) > engine->Time();
}

bool CBotFortress :: startGame()
{
	int team = m_pPlayerInfo->GetTeamIndex();
	
	// For FF, m_iClass will be set by FF specific logic, or needs a mapping if TF_Class enums are used internally
	// For now, we assume chooseClass/selectClass in CBotFF will handle m_iDesiredClass appropriately.
	// If CBotFF uses TF_Class internally, it needs to map its 1-10 system to TF_Class values.
	// m_iClass = (TF_Class)CClassInterface::getTF2Class(m_pEdict); // This line might be TF2 specific

	if ( (team != TF2_TEAM_BLUE) && (team != TF2_TEAM_RED) ) // These team defines might need to be FF specific
	{
		selectTeam();
	}
	else if ( m_iDesiredClass == -1 ) // invalid class
	{
		chooseClass(); // Should call CBotFF's version
	}
	// This condition might need adjustment if CBotFF.m_iClass is not directly comparable to m_iDesiredClass (if one is TF_enum and other is 1-10)
	// For now, assuming m_iDesiredClass (1-10 for FF) is the primary driver and selectClass handles it.
	// The actual class check (m_iClass != m_iDesiredClass) might need to be more sophisticated
	// if m_iClass is populated by a generic game function vs. FF specific one.
	else if ( (m_iDesiredClass > 0 && CClassInterface::getTF2Class(m_pEdict) != m_iDesiredClass ) || CClassInterface::getTF2Class(m_pEdict) == TF_CLASS_MAX ) // HACKY: Using getTF2Class for now
	{
		// TODO: MVM check might not apply to FF
		// if ( CTeamFortress2Mod::isMapType(TF_MAP_MVM) && CTeamFortress2Mod::hasRoundStarted() )
		//	return true;

		selectClass(); // Should call CBotFF's version
	}
	else
		return true;

	return false;
}

void CBotFortress ::pickedUpFlag()
{ 
	m_bHasFlag = true; 
	// clear tasks
	m_pSchedules->freeMemory();
}

void CBotFortress :: checkHealingValid ()
{
	if ( m_pHeal )
	{
		if ( !CBotGlobals::entityIsValid(m_pHeal) || !CBotGlobals::entityIsAlive(m_pHeal) )
		{
			m_pHeal = NULL;
			removeCondition(CONDITION_SEE_HEAL);
		}
		else if ( !isVisible(m_pHeal) )
		{
			m_pHeal = NULL;
			removeCondition(CONDITION_SEE_HEAL);
		}
		else if ( getHealFactor(m_pHeal) == 0.0f )
		{
			m_pHeal = NULL;
			removeCondition(CONDITION_SEE_HEAL);
		}
	}
	else
		removeCondition(CONDITION_SEE_HEAL);
}

float CBotFortress :: getHealFactor ( edict_t *pPlayer )
{
	// Factors are 
	// 1. health
	// 2. max health
	// 3. ubercharge
	// 4. player class
	// 5. etc
	float fFactor = 0.0f;
	float fLastCalledMedic = 0.0f;
	bool bHeavyClass = false;
	edict_t *pMedigun = CTeamFortress2Mod::getMediGun(m_pEdict); // FF Specific: Check if Medigun equivalent exists
	float fHealthPercent;
	Vector vVel = Vector(0,0,0);
	int iHighestScore = CTeamFortress2Mod::getHighestScore(); // FF Specific: Score system?
	// adds extra factor to players who have recently shouted MEDIC!
	if (!CBotGlobals::isPlayer(pPlayer))
	{
		// FF Specific: MVM revive marker may not apply
		// if (CTeamFortress2Mod::isMapType(TF_MAP_MVM))
		// {
		// 	if (strcmp(pPlayer->GetClassName(), "entity_revive_marker") == 0)
		// 	{
		// 		float fDistance = distanceFrom(pPlayer);

		// 		if (fDistance < 0.1)
		// 			return 1000;
		// 		// in case of divide by zero
		// 		return 200.0f / fDistance;
		// 	}
		// }
		return 0;
	}
	CClassInterface::getVelocity(pPlayer,&vVel);

	IPlayerInfo *p = playerinfomanager->GetPlayerInfo(pPlayer);
	TF_Class iclass = (TF_Class)CClassInterface::getTF2Class(pPlayer); // FF Specific: Map FF class to TF_Class or use FF class system
	
	if ( !CBotGlobals::entityIsAlive(pPlayer) || !p || p->IsDead() || p->IsObserver() || !p->IsConnected() )
		return 0.0f;

	if ( CClassInterface::getTF2NumHealers(pPlayer) > 1 ) // FF Specific: Num healers
		return 0.0f;

	fHealthPercent = (p->GetHealth()/p->GetMaxHealth());

	switch ( iclass ) // FF Specific: Adapt class checks
	{
	case TF_CLASS_MEDIC:
		if ( fHealthPercent >= 1.0f )
			return 0.0f;
		fFactor = 0.1f;
		break;
	case TF_CLASS_DEMOMAN:
	case TF_CLASS_HWGUY:
	case TF_CLASS_SOLDIER:
	case TF_CLASS_PYRO:
		{
			bHeavyClass = true;
			fFactor += 1.0f;
			if ( pMedigun ) // FF Specific: Medigun equivalent and UberCharge
			{
				fFactor += (float)(CClassInterface::getUberChargeLevel(pMedigun))/100;
				if ( CTeamFortress2Mod::TF2_IsPlayerInvuln(m_pEdict) )
					fFactor += (1.0f - ((float)(CClassInterface::getUberChargeLevel(pMedigun))/100));
			}
		}
	case TF_CLASS_SPY: // FF Specific: Spy disguise/cloak logic
		if ( iclass == TF_CLASS_SPY )
		{
			int iClass,iTeam,iIndex,iHealth;
			if ( CClassInterface::getTF2SpyDisguised(pPlayer,&iClass,&iTeam,&iIndex,&iHealth) )
			{
				if ( iTeam != m_iTeam )
					return 0.0f;
			}
			if ( CTeamFortress2Mod::TF2_IsPlayerCloaked(pPlayer) )
				return 0.0f;
		}
	default:
		if ( !bHeavyClass && pMedigun )
		{
			fFactor += (0.1f - ((float)(CClassInterface::getUberChargeLevel(pMedigun))/1000));
			if ( (m_StatsCanUse.stats.m_iTeamMatesVisible == 1) && (fHealthPercent >= 1.0f) )
				return 0.0f;
		}
		if ( bHeavyClass )
		{
			IPlayerInfo *player_p; // Renamed to avoid conflict
			player_p = playerinfomanager->GetPlayerInfo(pPlayer);
			if ( player_p && (player_p->GetLastUserCommand().buttons & IN_ATTACK) )
					fFactor += 1.0f;
		}
		fFactor += 1.0f - fHealthPercent;
		if ( CTeamFortress2Mod::TF2_IsPlayerOnFire(pPlayer) ) // FF Specific: OnFire state
			fFactor += 2.0f;
		fFactor += (vVel.Length() / 1000);
		fFactor += ((float)p->GetMaxHealth())/200;
		if ( iHighestScore == 0 ) iHighestScore = 1;
		fFactor += (((float)CClassInterface::getTF2Score(pPlayer))/iHighestScore)/2; // FF Score
		if ( (fLastCalledMedic = m_fCallMedicTime[ENTINDEX(pPlayer)-1]) > 0 )
			fFactor += MAX(0.0f,1.0f-((engine->Time() - fLastCalledMedic)/5));
		if ( ((m_fLastCalledMedicTime + 5.0f) > engine->Time()) && ( m_pLastCalledMedic == pPlayer ) )
			fFactor += 0.5f;
		if (CTeamFortress2Mod::isFlagCarrier(pPlayer) || CTeamFortress2Mod::isCapping(pPlayer) ) // FF Flag/Cap logic
			fFactor *= 1.5f;
	}
	return fFactor;
}

bool CBotFortress :: setVisible ( edict_t *pEntity, bool bVisible )
{
	bool bValid = CBot::setVisible(pEntity,bVisible);

	// FF Specific: Adapt TF2_WEAPON_MEDIGUN, TF_CLASS_SPY checks, CTeamFortress2Mod calls
	if ( m_iClass == TF_CLASS_MEDIC ) // Assuming TF_CLASS_MEDIC maps to FF Medic
	{
		if ( bValid && bVisible )
		{
			if (CBotGlobals::isPlayer(pEntity) )
			{
				// CBotWeapon *pMedigun = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_MEDIGUN)); // FF Medigun
				// bool bIsSpy = CClassInterface::getTF2Class(pEntity)==TF_CLASS_SPY; // FF Spy
				// ... (rest of TF2 medic logic needs FF adaptation) ...
			}
			else
			{
				// MVM logic likely not applicable to FF
			}
		}
		else if ( m_pHeal == pEntity )
		{
			m_pHeal = NULL;
			removeCondition(CONDITION_SEE_HEAL);
		}
	}

	if ( bValid && bVisible )
	{
		// FF Building checks (Sentry, Dispenser, Teleporter)
		// CTeamFortress2Mod::isSentry, isTeleporter, isDispenser need FF versions or careful use
		// CTeamFortress2Mod::isHurtfulPipeGrenade needs FF version
	}
	else 
	{
		// Nullify pointers if entities become invisible
	}

	return bValid;
}

void CBotFortress :: medicCalled(edict_t *pPlayer )
{
	bool bGoto = true;

	if ( pPlayer == m_pEdict ) return;
	if ( m_iClass != TF_CLASS_MEDIC ) return; // FF Medic
	if ( distanceFrom(pPlayer) > 1024 ) return;

	// FF Spy and team check
	if ( (CBotGlobals::getTeam(pPlayer) == getTeam()) || (CClassInterface::getTF2Class(pPlayer) == TF_CLASS_SPY) && thinkSpyIsEnemy(pPlayer,CTeamFortress2Mod::getSpyDisguise(pPlayer)) )
	{
		m_pLastCalledMedic = pPlayer;
		m_fLastCalledMedicTime = engine->Time();
		m_fCallMedicTime[ENTINDEX(pPlayer)-1] = m_fLastCalledMedicTime;
		if ( m_pHeal == pPlayer ) return;
		if (m_pHeal && CBotGlobals::isPlayer(m_pHeal))
		{
			if ( CClassInterface::getPlayerHealth(pPlayer) >= CClassInterface::getPlayerHealth(m_pHeal) )
				bGoto = false;
		}
		if ( bGoto ) m_pHeal = pPlayer;
		m_pLastHeal = m_pHeal;
	}
}

void CBotFortress ::waitBackstab ()
{
	m_fBackstabTime = engine->Time() + randomFloat(5.0f,10.0f);
	m_pLastEnemy = NULL;
}

bool CBotFortress :: isAlive ()
{
	return !m_pPlayerInfo->IsDead()&&!m_pPlayerInfo->IsObserver();
}

void CBotFortress :: seeFriendlyHurtEnemy ( edict_t *pTeammate, edict_t *pEnemy, CWeapon *pWeapon )
{
	// FF Spy check
	if ( CBotGlobals::isPlayer(pEnemy) && (CClassInterface::getTF2Class(pEnemy) == TF_CLASS_SPY) )
	{
		m_fSpyList[ENTINDEX(pEnemy)-1] = engine->Time();
	}
}

void CBotFortress :: shot ( edict_t *pEnemy )
{
	seeFriendlyHurtEnemy(m_pEdict,pEnemy,NULL);
}

void CBotFortress :: killed ( edict_t *pVictim, char *weapon )
{
	CBot::killed(pVictim,weapon);
	return;
}

void CBotFortress :: died ( edict_t *pKiller, const char *pszWeapon )
{
	CBot::died(pKiller,pszWeapon);
	// FF Spy check
	if ( CBotGlobals::isPlayer(pKiller) && (CClassInterface::getTF2Class(pKiller) == TF_CLASS_SPY) )
		foundSpy(pKiller,(TF_Class)0); // FF: getSpyDisguise might need adaptation

	droppedFlag(); // FF flag logic

	if (randomInt(0, 1))
		m_pButtons->attack();
	else
		m_pButtons->letGo(IN_ATTACK);

	m_bCheckClass = true;
}

void CBotTF2 :: buildingDestroyed ( int iType, edict_t *pAttacker, edict_t *pEdict )
{
	// This is TF2 specific, will need adaptation for FF if it has similar engineer buildings
	// For now, this function body would be largely commented or removed for CBotFF if called from a shared path.
	// If CBotFF has its own building system, it would implement its own version.
}

void CBotFortress ::wantToDisguise(bool bSet)
{
	// FF Spy disguise logic
	// if ( rcbot_tf2_debug_spies_cloakdisguise.GetBool() ) // This cvar might be TF2 specific
	// {
	// 	if ( bSet )
	// 		m_fSpyDisguiseTime = 0.0f;
	// 	else
	// 		m_fSpyDisguiseTime = engine->Time() + 2.0f;
	// }
	// else
	// 	m_fSpyDisguiseTime = engine->Time() + 10.0f;
}

void CBotFortress :: detectedAsSpy( edict_t *pDetector, bool bDisguiseComprimised )
{
	// FF Spy detected logic
}

void CBotFortress :: spawnInit ()
{
	CBot::spawnInit();
	m_fLastSentryEnemyTime = 0.0f;
	m_fHealFactor = 0.0f;
	m_pHealthkit = MyEHandle(NULL);
	m_pFlag = MyEHandle(NULL);
	m_pNearestDisp = MyEHandle(NULL);
	m_pAmmo = MyEHandle(NULL);
	m_pHeal = MyEHandle(NULL);
	m_pNearestPipeGren = MyEHandle(NULL);
	memset(m_fCallMedicTime,0,sizeof(float)*MAX_PLAYERS);
	m_fWaitTurnSentry = 0.0f;
	m_pLastSeeMedic.reset();
	memset(m_fSpyList,0,sizeof(float)*MAX_PLAYERS);
	m_fTaunting = 0.0f;
	m_fMedicUpdatePosTime = 0.0f;
	m_pLastHeal = NULL;
	m_fDisguiseTime = 0.0f;
	m_pNearestEnemyTeleporter = NULL;
	m_pNearestTeleEntrance = NULL;
	m_fBackstabTime = 0.0f;
	m_fPickupTime = 0.0f;
	m_fDefendTime = 0.0f;
	m_fLookAfterSentryTime = 0.0f;
	m_fSnipeAttackTime = 0.0f;
	m_fSpyCloakTime = 0.0f;
	m_fSpyUncloakTime = 0.0f;
	m_fLastSaySpy = 0.0f;
	m_fSpyDisguiseTime = 0.0f;
	m_pHeal = NULL;
	m_pNearestDisp = NULL;
	m_pNearestEnemySentry = NULL;
	m_pNearestAllySentry = NULL;
	m_bHasFlag = false; 
	m_bSentryGunVectorValid = false;
	m_bDispenserVectorValid = false;
	m_bTeleportExitVectorValid = false;
}

bool CBotFortress :: isBuilding ( edict_t *pBuilding )
{
	// FF Engineer building check
	return (pBuilding == m_pSentryGun.get()) || (pBuilding == m_pDispenser.get());
}

int CBotFortress :: engiBuildObject (int *iState, eEngiBuild iObject, float *fTime, int *iTries )
{
	// FF Engineer build logic
	return 0; // Placeholder
}

void CBotFortress :: setClass ( TF_Class _class )
{
	m_iClass = _class; // This might need adjustment if FF class IDs are different from TF_Class enum
}

bool CBotFortress :: thinkSpyIsEnemy ( edict_t *pEdict, TF_Class iDisguise )
{
	// FF Spy detection logic
	return ( (m_fSeeSpyTime > engine->Time()) &&
		(m_pPrevSpy == pEdict) &&
		((m_iPrevSpyDisguise == iDisguise)||((engine->Time()-m_fLastSeeSpyTime)<3.0f)) );
}

bool CBotTF2 ::thinkSpyIsEnemy(edict_t *pEdict, TF_Class iDisguise)
{
	// TF2 specific, CBotFF should have its own or rely on CBotFortress's if general enough
	return CBotFortress::thinkSpyIsEnemy(pEdict,iDisguise) || 
		(m_pCloakedSpy && (m_pCloakedSpy == pEdict) && !CTeamFortress2Mod::TF2_IsPlayerCloaked(m_pCloakedSpy));
}

bool CBotFortress :: isEnemy ( edict_t *pEdict,bool bCheckWeapons )
{
	if ( pEdict == m_pEdict ) return false;
	if ( !ENTINDEX(pEdict) || (ENTINDEX(pEdict) > CBotGlobals::maxClients()) ) return false;
	if ( CBotGlobals::getTeam(pEdict) == getTeam() ) return false;
	return true;	
}

bool CBotFortress :: needAmmo () { return false; } // FF specific implementation needed

bool CBotFortress :: needHealth ()
{
	// FF specific (e.g. no uber, different on-fire conditions)
	return !m_bIsBeingHealed && ((getHealthPercent() < 0.7) /* || FF_IsPlayerOnFire(m_pEdict) */);
}

bool CBotTF2 :: needAmmo() { /* TF2 specific */ return false; }

void CBotFortress :: currentlyDead ()
{
	CBot::currentlyDead();
	m_fUpdateClass = engine->Time() + 0.1f;
}

void CBotFortress :: modThink ()
{
	// FF specific class update
	// m_iClass = (TF_Class)CClassInterface::getFFClass(m_pEdict); // Example
	m_iTeam = getTeam();

	if ( needHealth() ) updateCondition(CONDITION_NEED_HEALTH);
	else removeCondition(CONDITION_NEED_HEALTH);
	if ( needAmmo() ) updateCondition(CONDITION_NEED_AMMO);
	else removeCondition(CONDITION_NEED_AMMO);

	// FF specific conditions (e.g. no uber/kritz)
	// if ( CTeamFortress2Mod::TF2_IsPlayerInvuln(m_pEdict) || CTeamFortress2Mod::TF2_IsPlayerKrits(m_pEdict) )
	//	updateCondition(CONDITION_PUSH);

	if ( m_fCallMedic < engine->Time() && getHealthPercent() < 0.5 )
	{
		m_fCallMedic = engine->Time() + randomFloat(10.0f,30.0f);
		callMedic();
	}
	// FF teleporter logic
	// ...
	checkHealingValid();
	// FF specific logic for reloading, pipe grenade avoidance etc.
}

bool CBotFortress :: isTeleporterUseful ( edict_t *pTele ) { return false; } // FF specific

void CBotFortress :: selectTeam ()
{
	char buffer[32];
	int team = randomInt(1,2); // Assuming FF teams are 1 and 2 like TF2's Red/Blue often map to. Adjust if not.
	sprintf(buffer,"jointeam %d",team); // May need FF specific command
	helpers->ClientCommand(m_pEdict,buffer);
}

void CBotFortress :: selectClass ()
{
	// This will be overridden by CBotFF::selectClass
}

bool CBotFortress :: waitForFlag ( Vector *vOrigin, float *fWait, bool bFindFlag ) { return true; } // FF specific

void CBotFortress :: foundSpy (edict_t *pEdict,TF_Class iDisguise)  // TF_Class might need to be FF_Class
{
	m_pPrevSpy = pEdict;
	m_fSeeSpyTime = engine->Time() + randomFloat(9.0f,18.0f);
	m_vLastSeeSpy = CBotGlobals::entityOrigin(pEdict);
	m_fLastSeeSpyTime = engine->Time();
	// if ( iDisguise && (m_iPrevSpyDisguise != iDisguise) ) // FF_Class
	//	m_iPrevSpyDisguise = iDisguise;
};

bool CBotTF2 :: hurt ( edict_t *pAttacker, int iHealthNow, bool bDontHide ) { return CBotFortress::hurt(pAttacker, iHealthNow, bDontHide); }
void CBotTF2 :: spawnInit() { CBotFortress::spawnInit(); }
bool CBotTF2 ::checkAttackPoint() { return false; }
void CBotTF2 :: setClass ( TF_Class _class ) { CBotFortress::setClass(_class); }
void CBotTF2 :: highFivePlayer ( edict_t *pPlayer, float fYaw ) {}
void CBotTF2 :: taunt ( bool bOverride ) {}
void CBotTF2::healedPlayer(edict_t *pPlayer, float fAmount) {}
void CBotTF2 :: engineerBuild ( eEngiBuild iBuilding, eEngiCmd iEngiCmd ) {}
void CBotTF2 :: updateCarrying () {}
void CBotTF2 :: checkBuildingsValid (bool bForce) {}
edict_t *CBotTF2 :: findEngineerBuiltObject ( eEngiBuild iBuilding, int index ) { return NULL;}
void CBotTF2 :: died ( edict_t *pKiller, const char *pszWeapon  ) { CBotFortress::died(pKiller, pszWeapon); }
void CBotTF2 :: killed ( edict_t *pVictim, char *weapon ) { CBotFortress::killed(pVictim, weapon); }
void CBotTF2 :: capturedFlag () {}
void CBotTF2 :: spyDisguise ( int iTeam, int iClass ) {}
bool CBotTF2 :: isCloaked () { return false; }
bool CBotTF2 :: isDisguised () { return false; }
void CBotTF2 :: updateClass () {}
TF_Class CBotTF2 :: getClass () { return CBotFortress::getClass(); }
void CBotTF2 :: setup () { CBotFortress::setup(); }
void CBotTF2 :: seeFriendlyKill ( edict_t *pTeamMate, edict_t *pDied, CWeapon *pWeapon ) {}
void CBotTF2 :: seeFriendlyDie ( edict_t *pDied, edict_t *pKiller, CWeapon *pWeapon ) { CBotFortress::seeFriendlyDie(pDied, pKiller, pWeapon); }
void CBotTF2 :: engiBuildSuccess ( eEngiBuild iBuilding, int index ) {}
bool CBotTF2 :: hasEngineerBuilt ( eEngiBuild iBuilding ) { return false; }
void CBotFortress :: flagDropped ( Vector vOrigin ) {}
void CBotFortress :: teamFlagDropped ( Vector vOrigin ) {}
void CBotFortress :: teamFlagPickup () {}
void CBotFortress :: callMedic () { helpers->ClientCommand (m_pEdict,"voicemenu 0 0"); /* Default "MEDIC!" in many Source mods */ }
bool CBotTF2 :: canGotoWaypoint (Vector vPrevWaypoint, CWaypoint *pWaypoint, CWaypoint *pPrev) { return CBotFortress::canGotoWaypoint(vPrevWaypoint, pWaypoint, pPrev); }
void CBotTF2 :: callMedic () { CBotFortress::callMedic(); }
void CBotFortress ::waitCloak() {}
bool CBotFortress:: wantToCloak() { return false; } // FF specific
bool CBotFortress:: wantToUnCloak () { return false; } // FF specific
void CBotTF2 :: spyUnCloak () {}
void CBotTF2 ::spyCloak() {}
void CBotFortress::chooseClass() {} // Will be implemented by CBotFF
void CBotFortress::updateConditions() { CBot::updateConditions(); }
void CBotTF2 :: onInventoryApplication () {}
void CBotTF2::modThink() { CBotFortress::modThink(); }
bool CBotTF2 :: canDeployStickies () { return false; }
bool CBotTF2::deployStickies(eDemoTrapType type, Vector vStand, Vector vLocation, Vector vSpread, Vector *vPoint, int *iState, int *iStickyNum, bool *bFail, float *fTime, int wptindex) { return true; }
void CBotTF2::detonateStickies(bool isJumping) {}
bool CBotTF2::lookAfterBuildings ( float *fTime ) {return false;}
bool CBotTF2 :: select_CWeapon ( CWeapon *pWeapon ) { return CBotFortress::select_CWeapon(pWeapon); }
bool CBotTF2 :: selectBotWeapon ( CBotWeapon *pBotWeapon ) { return CBotFortress::selectBotWeapon(pBotWeapon); }
bool CBotTF2 :: executeAction ( CBotUtility *util ) { return CBotFortress::executeAction(util); }
void CBotTF2 :: touchedWpt ( CWaypoint *pWaypoint , int iNextWaypoint, int iPrevWaypoint ) { CBotFortress::touchedWpt(pWaypoint, iNextWaypoint, iPrevWaypoint); }
void CBotTF2 :: modAim ( edict_t *pEntity, Vector &v_origin, Vector *v_desired_offset, Vector &v_size, float fDist, float fDist2D ) { CBotFortress::modAim(pEntity, v_origin, v_desired_offset, v_size, fDist, fDist2D); }
eBotFuncState CBotTF2 :: rocketJump(int *iState,float *fTime) { return BOT_FUNC_FAILED; }
bool CBotTF2 :: handleAttack ( CBotWeapon *pWeapon, edict_t *pEnemy ) { return CBotFortress::handleAttack(pWeapon, pEnemy); }
bool CBotFortress :: wantToFollowEnemy () { return CBot::wantToFollowEnemy(); }
void CBotTF2 ::voiceCommand ( int cmd ) {}
bool CBotTF2 ::checkStuck(void) { return CBotFortress::checkStuck(); }
void CBotTF2 :: foundSpy (edict_t *pEdict,TF_Class iDisguise) { CBotFortress::foundSpy(pEdict, iDisguise); }
int CBotFortress :: getSpyDisguiseClass ( int iTeam ) { return randomInt(1,9); } // Basic random for FF
bool CBotFortress :: incomingRocket ( float fRange ) { return false; } // FF specific
void CBotFortress :: enemyLost(edict_t *pEnemy) { CBot::enemyLost(pEnemy); }
bool CBotTF2 :: setVisible ( edict_t *pEntity, bool bVisible ) { return CBotFortress::setVisible(pEntity, bVisible); }
void CBotTF2 :: checkBeingHealed () {}
bool CBotTF2::healPlayer() {return false;}
void CBotTF2::roundWon(int iTeam, bool bFullRound ) {}
void CBotTF2::waitRemoveSap () {}
void CBotTF2::roundReset(bool bFullReset) {}
void CBotTF2::updateAttackDefendPoints() {}
void CBotTF2 :: pointsUpdated() {}
void CBotTF2::updateAttackPoints() {}
void CBotTF2::updateDefendPoints() {}
void CBotTF2::getDefendArea ( std::vector<int> *m_iAreas ) {}
void CBotTF2::getAttackArea ( std::vector <int> *m_iAreas ) {}
void CBotTF2::pointCaptured(int iPoint, int iTeam, const char *szPointName) {}
bool CBotTF2 :: isEnemy ( edict_t *pEdict,bool bCheckWeapons ) { return CBotFortress::isEnemy(pEdict, bCheckWeapons); }
void CBotTF2 :: getTasks ( unsigned int iIgnore ) { CBotFortress::getTasks(iIgnore); }
void CBotTF2::handleWeapons() { CBotFortress::handleWeapons(); }
CBotTF2::CBotTF2() { CBotFortress(); }
void CBotTF2 ::init(bool bVarInit) { CBotFortress::init(bVarInit); }


/////////////////////////////////////////////////////////////////////////
// FORTRESS FOREVER

// Placeholder defines
#define BOT_UTIL_FF_USE_GRENADE_STD 1001
#define BOT_UTIL_FF_USE_GRENADE_CONC 1002
#define BOT_UTIL_FF_CONC_JUMP_MOBILITY 1008
#define BOT_UTIL_FF_HUNTED_VIP_ESCAPE 1009
#define BOT_UTIL_FF_HUNTED_KILL_VIP 1010
#define BOT_UTIL_FF_HUNTED_PROTECT_VIP 1011
#define BOT_UTIL_FF_ENGRI_BUILD_MANCANNON 1012
#define BOT_UTIL_FF_DEMO_LAY_PIPE_TRAP 1013
#define BOT_UTIL_FF_SPY_USE_TRANQ 1014
#define BOT_UTIL_FF_SCOUT_USE_CALTROPS 1015


#define SCHED_FF_PRIME_THROW_GRENADE 2001
#define SCHED_FF_CONC_JUMP_SELF 2002
#define SCHED_FF_HUNTED_ESCAPE 2003
#define SCHED_FF_HUNTED_GUARD_VIP 2004
#define SCHED_FF_ENGRI_BUILD_MANCANNON 2005
#define SCHED_FF_DEMO_LAY_PIPE_TRAP 2006
#define SCHED_FF_MEDIC_HEAL_TEAMMATE 2007
// SCHED_SNIPE is 10 from base, BOT_UTIL_SNIPE is 20 from base


// FF Engineer Buildable Defines (local for now)
#define FF_ENGIBUILD_MANCANNON 1
// Potentially others:
// #define FF_ENGIBUILD_DISPENSER 0
// #define FF_ENGIBUILD_DETPACK 2


// Initialize static member for Hunted mode testing
bool CBotFF::s_IsHuntedModeForTesting = false; // Set to true via debugger or temp code to test Hunted logic


// --- CTaskFFPrimeGrenade ---
CTaskFFPrimeGrenade::CTaskFFPrimeGrenade(const Vector &vTargetPos, float fDuration)
	: m_vTargetPos(vTargetPos), m_fPrimeDuration(fDuration), m_fPrimeStartTime(0.0f)
{
	setTaskName("CTaskFFPrimeGrenade");
}

void CTaskFFPrimeGrenade::execute(CBot* pBot)
{
	CBotFF* pFFBot = static_cast<CBotFF*>(pBot);
	if (!pFFBot) {
		setTaskStatus(TASK_FAILED);
		return;
	}

	if (m_fPrimeStartTime == 0.0f)
	{
		m_fPrimeStartTime = engine->Time();
		pFFBot->m_bIsPrimingGrenade = true;
		pFFBot->m_fGrenadePrimeStartTime = m_fPrimeStartTime;
		pFFBot->m_fPrimeDuration = m_fPrimeDuration;
		pFFBot->m_vGrenadeTargetPos = m_vTargetPos;
	}

	pFFBot->setLookAt(m_vTargetPos);
	pFFBot->setLookAtTask(LOOK_VECTOR_PRIORITY);
	pFFBot->m_pButtons->hold(IN_ATTACK);

	if ((engine->Time() - m_fPrimeStartTime) >= m_fPrimeDuration)
	{
		setTaskStatus(TASK_COMPLETE);
	}
	else
	{
		setTaskStatus(TASK_CONTINUE);
	}
}

bool CTaskFFPrimeGrenade::isTaskComplete(CBot* pBot)
{
	CBotFF* pFFBot = static_cast<CBotFF*>(pBot);
	if (!pFFBot || !pFFBot->m_bIsPrimingGrenade) return true;
	return (engine->Time() - m_fPrimeStartTime) >= m_fPrimeDuration;
}
const char* CTaskFFPrimeGrenade::getTaskName() { return "CTaskFFPrimeGrenade"; }


// --- CTaskFFThrowGrenade ---
CTaskFFThrowGrenade::CTaskFFThrowGrenade(const Vector &vTargetPos, CBotWeapon* pGrenadeWeapon)
	: m_vTargetPos(vTargetPos), m_pGrenadeWeapon(pGrenadeWeapon)
{
	setTaskName("CTaskFFThrowGrenade");
}

void CTaskFFThrowGrenade::execute(CBot* pBot)
{
	CBotFF* pFFBot = static_cast<CBotFF*>(pBot);
	if (!pFFBot) {
		setTaskStatus(TASK_FAILED);
		return;
	}

	pFFBot->setLookAt(m_vTargetPos);
	pFFBot->setLookAtTask(LOOK_VECTOR_PRIORITY);
	pFFBot->m_pButtons->letGo(IN_ATTACK);
	pFFBot->m_bIsPrimingGrenade = false;
	
	setTaskStatus(TASK_COMPLETE);
}
bool CTaskFFThrowGrenade::isTaskComplete(CBot* pBot) { return true; }
const char* CTaskFFThrowGrenade::getTaskName() { return "CTaskFFThrowGrenade"; }


// --- CTaskFFExecuteConcJump ---
CTaskFFExecuteConcJump::CTaskFFExecuteConcJump()
{
	setTaskName("CTaskFFExecuteConcJump");
	m_bTaskComplete = false;
}

void CTaskFFExecuteConcJump::execute(CBot* pBot)
{
	CBotFF* pFFBot = static_cast<CBotFF*>(pBot);
	if (!pFFBot) {
		setTaskStatus(TASK_FAILED);
		m_bTaskComplete = true;
		return;
	}
	pFFBot->m_pButtons->tap(IN_JUMP);
	pFFBot->m_pButtons->letGo(IN_ATTACK);
	pFFBot->m_bIsPrimingGrenade = false;
	setTaskStatus(TASK_COMPLETE);
	m_bTaskComplete = true;
}

bool CTaskFFExecuteConcJump::isTaskComplete(CBot* pBot) { return m_bTaskComplete; }
const char* CTaskFFExecuteConcJump::getTaskName() { return "CTaskFFExecuteConcJump"; }

// --- CTaskFFEngineerBuild ---
CTaskFFEngineerBuild::CTaskFFEngineerBuild(int buildableId, const Vector& buildPos) :
  m_buildableId(buildableId), m_vBuildPos(buildPos), m_bCommandSent(false)
{
    setTaskName("CTaskFFEngineerBuild");
    m_bTaskComplete = false;
}

void CTaskFFEngineerBuild::execute(CBot* pBot) {
    CBotFF* pFFBot = static_cast<CBotFF*>(pBot);
    if (!pFFBot) {
        setTaskStatus(TASK_FAILED);
        m_bTaskComplete = true;
        return;
    }

    if (m_bCommandSent) {
        setTaskStatus(TASK_COMPLETE);
        m_bTaskComplete = true;
        return;
    }

    pFFBot->setLookAt(m_vBuildPos);
    pFFBot->setLookAtTask(LOOK_VECTOR_PRIORITY);

    const char* cmd = "";
    if (m_buildableId == FF_ENGIBUILD_MANCANNON) {
        cmd = "build mancannon";
    }


    if (cmd[0] != '\0') {
        if (helpers && pFFBot->getEdict()) {
            helpers->ClientCommand(pFFBot->getEdict(), cmd);
        }
        pFFBot->m_fNextMancannonBuildTime = engine->Time() + 30.0f;
        m_bCommandSent = true;
        setTaskStatus(TASK_COMPLETE);
        m_bTaskComplete = true;
    } else {
        setTaskStatus(TASK_FAILED);
        m_bTaskComplete = true;
    }
}

bool CTaskFFEngineerBuild::isTaskComplete(CBot* pBot) {
    return m_bTaskComplete;
}
const char* CTaskFFEngineerBuild::getTaskName() { return "CTaskFFEngineerBuild"; }

// --- CTaskFFDemoLaySinglePipe ---
CTaskFFDemoLaySinglePipe::CTaskFFDemoLaySinglePipe(const Vector& vTargetPos) : m_vTargetPos(vTargetPos), m_bFired(false) {
    setTaskName("CTaskFFDemoLaySinglePipe");
    m_bTaskComplete = false;
}
void CTaskFFDemoLaySinglePipe::execute(CBot* pBot) {
    CBotFF* pFFBot = static_cast<CBotFF*>(pBot);
    if (!pFFBot) {
        setTaskStatus(TASK_FAILED);
        m_bTaskComplete = true;
        return;
    }
    if (m_bFired) {
        setTaskStatus(TASK_COMPLETE);
        m_bTaskComplete = true;
        return;
    }
    pFFBot->setLookAt(m_vTargetPos);
    pFFBot->setLookAtTask(LOOK_VECTOR_PRIORITY);
    pFFBot->getButtons()->tap(IN_ATTACK);
    pFFBot->m_fNextPipeLayTime = engine->Time() + 0.5f;
    m_bFired = true;
    setTaskStatus(TASK_COMPLETE);
    m_bTaskComplete = true;
}
bool CTaskFFDemoLaySinglePipe::isTaskComplete(CBot* pBot) { return m_bTaskComplete; }
const char* CTaskFFDemoLaySinglePipe::getTaskName() { return "CTaskFFDemoLaySinglePipe"; }

// --- CTaskFFMedicAimAndHeal ---
CTaskFFMedicAimAndHeal::CTaskFFMedicAimAndHeal(edict_t* pTarget) : m_hTarget(NULL) {
    setTaskName("CTaskFFMedicAimAndHeal");
    m_bTaskComplete = false;
    if (pTarget) {
       m_hTarget.Set(pTarget);
    }
}

void CTaskFFMedicAimAndHeal::init(CBot* pBot) {
    CBotTask::init(pBot);
    if (!m_hTarget.Get()) {
       CBotFF* pFFBot = static_cast<CBotFF*>(pBot);
       if (pFFBot && pFFBot->m_pHeal.Get()) {
           m_hTarget.Set(pFFBot->m_pHeal.Get());
       }
    }
     if (!m_hTarget.Get() && m_pTaskDataTargetEdict) {
        m_hTarget.Set(m_pTaskDataTargetEdict);
    }
}

void CTaskFFMedicAimAndHeal::execute(CBot* pBot) {
    CBotFF* pFFBot = static_cast<CBotFF*>(pBot);
    if (!pFFBot) {
        setTaskStatus(TASK_FAILED);
        m_bTaskComplete = true;
        return;
    }

    edict_t* pTarget = m_hTarget.Get();

    if (pTarget && CBotGlobals::entityIsValid(pTarget) && CBotGlobals::entityIsAlive(pTarget) &&
        pFFBot->getTeam() == CBotGlobals::getTeam(pTarget) &&
        CClassInterface::getPlayerHealth(pTarget) < CClassInterface::getPlayerMaxHealth(pTarget) &&
        pFFBot->isVisible(pTarget) && pFFBot->distanceFrom(pTarget) < 800.0f)
    {
        pFFBot->setLookAt(pTarget);
        pFFBot->setLookAtTask(LOOK_EDICT_PRIORITY);
        pFFBot->m_pButtons->hold(IN_ATTACK);
        setTaskStatus(TASK_CONTINUE);
    } else {
        pFFBot->m_pButtons->letGo(IN_ATTACK);
        setTaskStatus(TASK_COMPLETE);
        m_bTaskComplete = true;
    }
}
bool CTaskFFMedicAimAndHeal::isTaskComplete(CBot* pBot) { return m_bTaskComplete; }
const char* CTaskFFMedicAimAndHeal::getTaskName() { return "CTaskFFMedicAimAndHeal"; }

// --- CTaskFFSnipeAttackSequence ---
CTaskFFSnipeAttackSequence::CTaskFFSnipeAttackSequence(edict_t* pTarget) : m_hTarget(pTarget), m_iState(0), m_fNextActionTime(0.0f) {
    setTaskName("CTaskFFSnipeAttackSequence");
    m_bTaskComplete = false;
}

void CTaskFFSnipeAttackSequence::init(CBot* pBot) {
    CBotTask::init(pBot);
    if (m_pTaskDataTargetEdict) m_hTarget.Set(m_pTaskDataTargetEdict);
    else if (!m_hTarget.Get()) m_hTarget.Set(pBot->getEnemy());

    m_iState = 0;
    m_fNextActionTime = 0.0f;
}

void CTaskFFSnipeAttackSequence::execute(CBot* pBot) {
    CBotFF* pFFBot = static_cast<CBotFF*>(pBot);
    if (!pFFBot) { setTaskStatus(TASK_FAILED); m_bTaskComplete = true; return; }

    edict_t* pTarget = m_hTarget.Get();
    if (!pTarget || !CBotGlobals::entityIsAlive(pTarget) || !pFFBot->isVisible(pTarget)) {
        if (pFFBot->isZoomed()) { pFFBot->getButtons()->tap(IN_ATTACK2); }
        setTaskStatus(TASK_COMPLETE); m_bTaskComplete = true; return;
    }

    pFFBot->setLookAt(pTarget);
    pFFBot->setLookAtTask(LOOK_ENEMY_PRECISE);

    if (engine->Time() < m_fNextActionTime) {
        setTaskStatus(TASK_CONTINUE);
        return;
    }

    switch (m_iState) {
        case 0: // Zoom In
            if (!pFFBot->isZoomed()) {
                pFFBot->getButtons()->tap(IN_ATTACK2);
            }
            m_fNextActionTime = engine->Time() + 0.5f;
            m_iState++;
            setTaskStatus(TASK_CONTINUE);
            break;
        case 1: // Fire
            pFFBot->getButtons()->tap(IN_ATTACK);
            m_fNextActionTime = engine->Time() + 0.75f;
            m_iState++;
            setTaskStatus(TASK_CONTINUE);
            break;
        case 2: // Zoom Out
            if (pFFBot->isZoomed()) {
                pFFBot->getButtons()->tap(IN_ATTACK2);
            }
            setTaskStatus(TASK_COMPLETE);
            m_bTaskComplete = true;
            break;
        default:
            setTaskStatus(TASK_FAILED);
            m_bTaskComplete = true;
            break;
    }
}
bool CTaskFFSnipeAttackSequence::isTaskComplete(CBot* pBot) { return m_bTaskComplete; }
const char* CTaskFFSnipeAttackSequence::getTaskName() { return "CTaskFFSnipeAttackSequence"; }


// --- CSchedFFPrimeThrowGrenade ---
CSchedFFPrimeThrowGrenade::CSchedFFPrimeThrowGrenade(const Vector &vTargetPos, CBotWeapon* pGrenadeWeapon, float fPrimeTime)
{
	setID(SCHED_FF_PRIME_THROW_GRENADE);
	setScheduleName("CSchedFFPrimeThrowGrenade");
	if (pGrenadeWeapon && pGrenadeWeapon->getWeaponInfo())
	{
		addTask(new CSelectWeaponTask(pGrenadeWeapon->getWeaponInfo()));
		addTask(new CTaskFFPrimeGrenade(vTargetPos, fPrimeTime));
		addTask(new CTaskFFThrowGrenade(vTargetPos, pGrenadeWeapon));
	} else {
		failSchedule();
	}
}
const char* CSchedFFPrimeThrowGrenade::getScheduleName() { return "CSchedFFPrimeThrowGrenade"; }

// --- CSchedFFConcJumpSelf ---
CSchedFFConcJumpSelf::CSchedFFConcJumpSelf(CBotFF* pBot, CBotWeapon* pConcGrenade)
{
	setID(SCHED_FF_CONC_JUMP_SELF);
	setScheduleName("CSchedFFConcJumpSelf");
	if (pBot && pConcGrenade && pConcGrenade->getWeaponInfo())
	{
		Vector selfTargetPos = pBot->getOrigin();
		addTask(new CSelectWeaponTask(pConcGrenade->getWeaponInfo()));
		addTask(new CTaskFFPrimeGrenade(selfTargetPos, 0.25f));
		addTask(new CTaskFFExecuteConcJump());
	} else {
		failSchedule();
	}
}
const char* CSchedFFConcJumpSelf::getScheduleName() { return "CSchedFFConcJumpSelf"; }

// --- CSchedFFBuildMancannon ---
CSchedFFBuildMancannon::CSchedFFBuildMancannon(CBotFF* pBot, CWaypoint* pBuildSpot) {
    setID(SCHED_FF_ENGRI_BUILD_MANCANNON);
    setScheduleName("CSchedFFBuildMancannon");

    if (!pBuildSpot || !pBot) {
        failSchedule();
        return;
    }
    addTask(new CMoveToPointTask(pBuildSpot->getOrigin(), CWaypointLocations::REACHABLE_RANGE - 20.0f));
    addTask(new CTaskFFEngineerBuild(FF_ENGIBUILD_MANCANNON, pBuildSpot->getOrigin()));
}
const char* CSchedFFBuildMancannon::getScheduleName() { return "CSchedFFBuildMancannon"; }

// --- CSchedFFDemoLayPipeTrap ---
CSchedFFDemoLayPipeTrap::CSchedFFDemoLayPipeTrap(CBotFF* pBot, CWaypoint* pTrapSpotWpt, int numPipes) {
    setID(SCHED_FF_DEMO_LAY_PIPE_TRAP);
    setScheduleName("CSchedFFDemoLayPipeTrap");

    if (!pBot || !pTrapSpotWpt || numPipes <= 0) {
        failSchedule();
        return;
    }

    CBotWeapon* pPipeLauncher = pBot->getWeapons()->getWeaponByName("weapon_ff_pipebomblauncher");
    if (!pPipeLauncher || !pPipeLauncher->hasWeapon() || pPipeLauncher->getAmmo(pBot) < numPipes) {
        failSchedule();
        return;
    }

    addTask(new CSelectWeaponTask(pPipeLauncher->getWeaponInfo()));
    addTask(new CMoveToPointTask(pTrapSpotWpt->getOrigin(), CWaypointLocations::REACHABLE_RANGE - 20.0f));

    for (int i = 0; i < numPipes; ++i) {
        Vector targetPos = pTrapSpotWpt->getOrigin() + Vector(randomFloat(-30, 30), randomFloat(-30, 30), 10);
        addTask(new CTaskFFDemoLaySinglePipe(targetPos));
        if (i < numPipes - 1) {
            addTask(new CBotTaskWait(0.6f));
        }
    }
}
const char* CSchedFFDemoLayPipeTrap::getScheduleName() { return "CSchedFFDemoLayPipeTrap"; }

// --- CSchedFFMedicHealTeammate ---
CSchedFFMedicHealTeammate::CSchedFFMedicHealTeammate(CBotFF* pBot, edict_t* pTargetTeammate) {
    setID(SCHED_HEAL);
    setScheduleName("CSchedFFMedicHealTeammate");

    if (!pBot || !pTargetTeammate) {
        failSchedule();
        return;
    }

    CBotWeapon* pMedkit = pBot->getWeapons()->getWeaponByName("weapon_ff_medkit");
    if (!pMedkit || !pMedkit->hasWeapon() || !pMedkit->getWeaponInfo()) {
        failSchedule();
        return;
    }

    addTask(new CSelectWeaponTask(pMedkit->getWeaponInfo()));
    addTask(new CTaskFFMedicAimAndHeal(pTargetTeammate));
}
const char* CSchedFFMedicHealTeammate::getScheduleName() { return "CSchedFFMedicHealTeammate"; }

// --- CSchedFFSnipe ---
CSchedFFSnipe::CSchedFFSnipe(CBotFF* pBot, edict_t* pTarget) {
    setID(SCHED_SNIPE);
    setScheduleName("CSchedFFSnipe");

    if (!pBot || !pTarget) { failSchedule(); return; }

    CBotWeapon* pSniperRifle = pBot->getWeapons()->getWeaponByName("weapon_ff_sniperrifle");
    if (!pSniperRifle || !pSniperRifle->hasWeapon() || !pSniperRifle->getWeaponInfo()) {
        failSchedule(); return;
    }
    addTask(new CSelectWeaponTask(pSniperRifle->getWeaponInfo()));
    addTask(new CTaskFFSnipeAttackSequence(pTarget));
}
const char* CSchedFFSnipe::getScheduleName() { return "CSchedFFSnipe"; }


// CBotFF Method Implementations
CBotFF::CBotFF()
	: CBotFortress(),
	  m_fGrenadePrimeStartTime(0.0f),
	  m_bIsPrimingGrenade(false),
	  m_pGrenadeTargetEnt(NULL),
	  m_fPrimeDuration(1.0f),
      m_pVIP(NULL),
      m_bIsVIP(false),
      m_fNextMancannonBuildTime(0.0f),
      m_iPipesToLay(0),
      m_fNextPipeLayTime(0.0f)
	{
		m_vGrenadeTargetPos = Vector(0,0,0);
        m_hBuiltMancannon.Set(NULL);
        m_vCurrentPipeTrapLocation = Vector(0,0,0);
	}


void CBotFF::chooseClass()
{
	const int forcedClass = rcbot_force_class.GetInt();
	if (forcedClass >= 1 && forcedClass <= 10) {
		m_iDesiredClass = forcedClass;
	} else {
		m_iDesiredClass = randomInt(1, 10);
	}
}

void CBotFF::selectClass()
{
	const char* cmd = "";
	if (m_iDesiredClass < 1 || m_iDesiredClass > 10) {
		chooseClass();
	}
	switch (m_iDesiredClass) {
		case 1:  cmd = "joinclass scout"; break;
		case 2:  cmd = "joinclass sniper"; break;
		case 3:  cmd = "joinclass soldier"; break;
		case 4:  cmd = "joinclass demoman"; break;
		case 5:  cmd = "joinclass medic"; break;
		case 6:  cmd = "joinclass hwguy"; break;
		case 7:  cmd = "joinclass pyro"; break;
		case 8:  cmd = "joinclass spy"; break;
		case 9:  cmd = "joinclass engineer"; break;
		case 10: cmd = "joinclass civilian"; break;
		default: cmd = "joinclass scout"; break;
	}
	if (helpers && m_pEdict) { helpers->ClientCommand(m_pEdict, cmd); }
	if (engine) { m_fChangeClassTime = engine->Time() + randomFloat(bot_min_cc_time.GetFloat(), bot_max_cc_time.GetFloat()); }
}

void CBotFF::modThink ()
{
	CBotFortress :: modThink();
	if (s_IsHuntedModeForTesting) {
		TF_Class currentClass = (TF_Class)CClassInterface::getTF2Class(m_pEdict);
		if (currentClass == TF_CLASS_CIVILIAN) {
			m_bIsVIP = true;
		} else {
			m_bIsVIP = false;
		}
	} else {
		m_bIsVIP = false;
	}
    // Reset ideal move speed to class default, can be modified by other logic later
    m_fIdealMoveSpeed = CClassInterface::getMaxSpeed(m_pEdict); // Or FF specific getMaxSpeed

    CBotWeapon* currentWeapon = getCurrentWeapon();
     if (getClass() == TF_CLASS_SNIPER && isZoomed()) // TF_CLASS_SNIPER should map to FF Sniper
     {
         m_fIdealMoveSpeed *= 0.5f; // Example: Snipers move slower when zoomed
     }
     else if (getClass() == TF_CLASS_HWGUY && currentWeapon &&
         currentWeapon->getWeaponInfo() &&
         strcmp(currentWeapon->getWeaponInfo()->getWeaponName(), "weapon_ff_assaultcannon") == 0 &&
         (m_pButtons->holdingButton(IN_ATTACK) || (m_pEnemy && isVisible(m_pEnemy) && wantToShoot())) )
     {
         m_fIdealMoveSpeed *= 0.4f;
     }
}

bool CBotFF::isEnemy ( edict_t *pEdict,bool bCheckWeapons )
{
	if ( pEdict == m_pEdict ) return false;
	if ( !CBotGlobals::entityIsValid(pEdict) || !CBotGlobals::entityIsAlive(pEdict) ) return false;
	if ( CBotGlobals::getTeam(pEdict) == getTeam() && CBotGlobals::getTeam(pEdict) != 0 ) return false;
	return true;
}

bool CBotFF::isZoomed() {
    CBotWeapon* pWep = getCurrentWeapon();
    if (pWep && pWep->getWeaponInfo() && strcmp(pWep->getWeaponInfo()->getWeaponName(), "weapon_ff_sniperrifle") == 0) {
        if (m_pPlayerInfo) return m_fFov < m_pPlayerInfo->GetDefaultFOV();
    }
    return false;
}

void CBotFF::modAim(edict_t *pEntity, Vector &v_origin, Vector *v_desired_offset, Vector &v_size, float fDist, float fDist2D)
{
	CBot::modAim(pEntity, v_origin, v_desired_offset, v_size, fDist, fDist2D);

	CBotWeapon *pWp = getCurrentWeapon();
	static float fTime;

	if (m_bIsPrimingGrenade && m_pSchedules && m_pSchedules->isCurrentSchedule(SCHED_FF_CONC_JUMP_SELF))
	{
		Vector aimDir = Vector(0, 0, -1);
		if (getNavigator()->hasNextPoint() && getNavigator()->getGoalOrigin() != m_vEmpty) {
			Vector nextNav = getNavigator()->getNextPoint();
			Vector moveDir = (nextNav - getOrigin()).Normalized();
			aimDir = Vector(moveDir.x * 0.2f, moveDir.y * 0.2f, -0.9f).Normalized();
		}
		Vector botEyePos = getEyePosition();
		*v_desired_offset = (botEyePos + aimDir * 100.0f) - v_origin;

		setLookAtTask(LOOK_VECTOR_PRECISE);
		m_bIncreaseSensitivity = true;
		return;
	}

	if (pWp)
	{
		float projSpeed = pWp->getProjectileSpeed();
		if (projSpeed <= 0.001f) projSpeed = 1000.0f;

		bool isArcingFFGrenade = pWp->isGrenade();

		if (pWp->weaponFiresProjectile() || isArcingFFGrenade)
		{
			CClient *pClient = CClients::get(pEntity);
			Vector vVelocity = Vector(0,0,0);

			if (CClassInterface::getVelocity(pEntity, &vVelocity))
			{
				if (pClient && (vVelocity.IsZero()))
					vVelocity = pClient->getVelocity();
			}
			else if (pClient)
				vVelocity = pClient->getVelocity();

			bool use2DdistForTime = isArcingFFGrenade;
			const char* currentWeaponClassname = pWp->getWeaponInfo() ? pWp->getWeaponInfo()->getWeaponName() : "";
            if (strcmp(currentWeaponClassname, "weapon_ff_grenadelauncher") == 0 ||
                strcmp(currentWeaponClassname, "weapon_ff_pipebomblauncher") == 0 ||
                strcmp(currentWeaponClassname, "weapon_ff_mirv") == 0) {
                use2DdistForTime = true;
            }

			if (use2DdistForTime)
				fTime = fDist2D / projSpeed;
			else
				fTime = fDist / projSpeed;

			float skillFactor = m_pProfile ? m_pProfile->m_fAimSkill : 1.0f;
			*v_desired_offset = *v_desired_offset + ((vVelocity * fTime) * skillFactor);

			if (sv_gravity.IsValid() && (pWp->weaponFiresProjectile() || isArcingFFGrenade))
			{
				float gravityEffect = 0.5f * sv_gravity.GetFloat() * fTime * fTime;
				if (isArcingFFGrenade) {
                     v_desired_offset->z += gravityEffect * skillFactor;
                     float arcFactor = fDist2D * 0.25f;
                     if (arcFactor > 120.0f) arcFactor = 120.0f;
                     v_desired_offset->z += arcFactor * skillFactor;
				} else if (use2DdistForTime) {
					v_desired_offset->z += gravityEffect * skillFactor;
                }
			}

			if (pWp->isExplosive() && !isArcingFFGrenade && !use2DdistForTime && CBotGlobals::isPlayer(pEntity) && hasSomeConditions(CONDITION_SEE_ENEMY_GROUND))
			{
                 v_desired_offset->z -= (v_size.z * 0.4f * skillFactor);
			}
		}
	}
}

void CBotFF::handleWeapons()
{
	if (m_bIsPrimingGrenade)
	{
		if (!m_pSchedules || !m_pSchedules->isCurrentSchedule(SCHED_FF_CONC_JUMP_SELF)) {
			setLookAt(m_vGrenadeTargetPos);
			setLookAtTask(LOOK_VECTOR_PRIORITY);
		}
		return;
	}

	if (m_pEnemy && !hasSomeConditions(CONDITION_ENEMY_DEAD) &&
		hasSomeConditions(CONDITION_SEE_CUR_ENEMY) && wantToShoot() &&
		isVisible(m_pEnemy) && isEnemy(m_pEnemy))
	{
		CBotWeapon *pWeapon;
		pWeapon = m_pWeapons->getBestWeapon(m_pEnemy, !hasFlag(), !hasFlag(), rcbot_melee_only.GetBool(), false, false);

		setLookAtTask(LOOK_ENEMY);
		m_pAttackingEnemy = NULL;

		if (m_bWantToChangeWeapon && (pWeapon != NULL) && (pWeapon != getCurrentWeapon()) && pWeapon->getWeaponIndex())
		{
			if (pWeapon->getWeaponInfo())
				select_CWeapon(pWeapon->getWeaponInfo());
			else
				selectWeapon(pWeapon->getWeaponIndex());
		}
		else
		{
			if (!handleAttack(pWeapon, m_pEnemy))
			{
				m_pEnemy = NULL;
				m_pOldEnemy = NULL;
				wantToShoot(false);
			}
		}
	}
}

bool CBotFF::handleAttack(CBotWeapon *pWeapon, edict_t *pEnemy)
{
	if (!pWeapon)
	{
		primaryAttack();
		m_pAttackingEnemy = pEnemy;
		return true;
	}

	float fDistance = distanceFrom(pEnemy);

	if ((fDistance > 128) && (DotProductFromOrigin(m_vAimVector) < rcbot_enemyshootfov.GetFloat()))
	{
		return true;
	}

	clearFailedWeaponSelect();

	if (pWeapon->isMelee())
	{
		setMoveTo(CBotGlobals::entityOrigin(pEnemy));
		setLookAtTask(LOOK_ENEMY);
		m_fAvoidTime = engine->Time() + 1.0f;
	}
	
	bool useSecondary = false;
	const char* weaponName = pWeapon->getWeaponInfo() ? pWeapon->getWeaponInfo()->getWeaponName() : "";
	if (strcmp(weaponName, "weapon_ff_pipebomblauncher") == 0 )
	{
		// This needs a way to count live pipes for FF, e.g., CClassInterface::FF_GetLivePipeCount(m_pEdict)
		// if (FF_GetLivePipeCount(m_pEdict) > 0 && some_condition_for_detonation) useSecondary = true;
	}


	if (pWeapon->mustHoldAttack())
	{
		if (useSecondary && pWeapon->hasSecondaryAttack())
			secondaryAttack(true);
		else
			primaryAttack(true);
	}
	else
	{
		if (useSecondary && pWeapon->hasSecondaryAttack())
			secondaryAttack();
		else
			primaryAttack();
	}

	if (pWeapon->isMelee() && (distanceFrom2D(pEnemy) < 64.0f))
	{
		Vector vEnemyOrigin = CBotGlobals::entityOrigin(pEnemy);
		if ((vEnemyOrigin.z < getOrigin().z) && (vEnemyOrigin.z > (getOrigin().z - 128)))
		{
			duck();
		}
	}

	if ((!pWeapon->isMelee() || pWeapon->isSpecial()) && pWeapon->outOfAmmo(this))
	{
		return false;
	}

	m_pAttackingEnemy = pEnemy;
	return true;
}

void CBotFF::getTasks(unsigned int iIgnore)
{
	CBot::getTasks(iIgnore);

	if (!hasSomeConditions(CONDITION_CHANGED) && !m_pSchedules->isEmpty() && !m_bIsVIP)
		return;

	removeCondition(CONDITION_CHANGED);
	CBotUtilities utils;

	m_pVIP = NULL;

	if (CBotFF::s_IsHuntedModeForTesting)
	{
		if (m_bIsVIP)
		{
			ADD_UTILITY(BOT_UTIL_FF_HUNTED_VIP_ESCAPE, true, 1.0f);
		}
		else
		{
			edict_t* foundVIP = NULL;
			for (int i = 1; i <= gpGlobals->maxClients; ++i) {
				edict_t* pPlayer = INDEXENT(i);
				if (pPlayer && pPlayer != m_pEdict && CBotGlobals::entityIsValid(pPlayer) && CBotGlobals::entityIsAlive(pPlayer)) {
					TF_Class playerFFClass = (TF_Class)CClassInterface::getTF2Class(pPlayer);
					if (playerFFClass == TF_CLASS_CIVILIAN) {
						foundVIP = pPlayer;
						if (CBotGlobals::getTeam(pPlayer) == getTeam()) {
							m_pVIP = pPlayer;
							ADD_UTILITY_TARGET(BOT_UTIL_FF_HUNTED_PROTECT_VIP, true, 0.9f, pPlayer);
						} else {
							m_pVIP = pPlayer;
							ADD_UTILITY_TARGET(BOT_UTIL_FF_HUNTED_KILL_VIP, true, 0.95f, pPlayer);
						}
						break;
					}
				}
			}
		}
	}

	if (!m_bIsVIP)
	{
        if (getClass() == TF_CLASS_ENGINEER && engine->Time() > m_fNextMancannonBuildTime && !m_bIsPrimingGrenade)
        {
            CWaypoint* pMancannonSpot = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_FF_MANCANNON_SPOT, getTeam(), 0, true, this);
            if (pMancannonSpot)
            {
                bool spotTaken = false;
                if (m_hBuiltMancannon.Get() && CBotGlobals::entityIsValid(m_hBuiltMancannon.Get())) {
                    if ( (CBotGlobals::entityOrigin(m_hBuiltMancannon.Get()) - pMancannonSpot->getOrigin()).LengthSqr() < (200.0f * 200.0f) ) {
                        spotTaken = true;
                    }
                }
                if (!spotTaken) {
                    ADD_UTILITY_DATA(BOT_UTIL_FF_ENGRI_BUILD_MANCANNON, true, 0.7f, CWaypoints::getWaypointIndex(pMancannonSpot));
                }
            }
        }

        if (getClass() == TF_CLASS_DEMOMAN && engine->Time() > m_fNextPipeLayTime && !m_bIsPrimingGrenade)
        {
            CBotWeapon* pPipeLauncher = m_pWeapons->getWeaponByName("weapon_ff_pipebomblauncher");
            if (pPipeLauncher && pPipeLauncher->hasWeapon() && pPipeLauncher->getAmmo(this) >= 3)
            {
                CWaypoint* pTrapSpot = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_FF_PIPE_TRAP_SPOT, getTeam(), 0, true, this);
                if (pTrapSpot)
                {
                    ADD_UTILITY_DATA(BOT_UTIL_FF_DEMO_LAY_PIPE_TRAP, true, 0.6f, CWaypoints::getWaypointIndex(pTrapSpot));
                }
            }
        }

        if (getClass() == TF_CLASS_SPY && !m_bIsPrimingGrenade)
        {
            CBotWeapon* pTranqGun = m_pWeapons->getWeaponByName("weapon_ff_tranqgun");
            if (pTranqGun && pTranqGun->hasWeapon() && !pTranqGun->outOfAmmo(this))
            {
                if (m_pEnemy && CBotGlobals::entityIsAlive(m_pEnemy) && isVisible(m_pEnemy))
                {
                    float utilityScore = 0.5f;
                    TF_Class enemyClass = (TF_Class)CClassInterface::getTF2Class(m_pEnemy);

                    if (enemyClass == TF_CLASS_MEDIC || enemyClass == TF_CLASS_HWGUY ||
                        enemyClass == TF_CLASS_SOLDIER || enemyClass == TF_CLASS_SCOUT || enemyClass == TF_CLASS_SPY) {
                        utilityScore += 0.25f;
                    }
                    Vector enemyVelocity;
                    if (CClassInterface::getVelocity(m_pEnemy, &enemyVelocity) && enemyVelocity.Length() > 150.0f) {
                        utilityScore += 0.1f;
                    }
                    ADD_UTILITY_WEAPON(BOT_UTIL_FF_SPY_USE_TRANQ, true, utilityScore, pTranqGun);
                }
            }
        }

        if (getClass() == TF_CLASS_MEDIC && !m_bIsPrimingGrenade)
        {
            if (m_pHeal.Get() && CBotGlobals::entityIsValid(m_pHeal.Get()) && CBotGlobals::entityIsAlive(m_pHeal.Get()) &&
                getTeam() == CBotGlobals::getTeam(m_pHeal.Get()) &&
                CClassInterface::getPlayerHealth(m_pHeal.Get()) < CClassInterface::getPlayerMaxHealth(m_pHeal.Get()))
            {
                CBotWeapon* pMedkit = m_pWeapons->getWeaponByName("weapon_ff_medkit");
                if (pMedkit && pMedkit->hasWeapon()) {
                    ADD_UTILITY_WEAPON(BOT_UTIL_MEDIC_HEAL, true, 0.98f, pMedkit);
                }
            }
        }

        if (getClass() == TF_CLASS_SCOUT && !m_bIsPrimingGrenade)
        {
            CBotWeapon* pCaltrops = m_pWeapons->getWeaponByName("weapon_ff_gren_caltrop");
            if (pCaltrops && pCaltrops->hasWeapon() && !pCaltrops->outOfAmmo(this))
            {
                bool shouldUseCaltrops = false;
                Vector vTargetPos = m_vEmpty;
                float utilityScore = 0.6f;

                if (m_pEnemy && CBotGlobals::entityIsAlive(m_pEnemy) && isVisible(m_pEnemy) && distanceFrom(m_pEnemy) < 400.0f) {
                    bool isFleeing = false;
                    if (m_pNavigator && m_pNavigator->getPath()) {
                        isFleeing = m_pNavigator->getPath()->isFlagSet(CPath::PATH_FLEE);
                    }

                    if (isFleeing || getHealthPercent() < 0.4f) {
                        shouldUseCaltrops = true;
                        Vector toEnemy = (CBotGlobals::entityOrigin(m_pEnemy) - getOrigin()).Normalized();
                        vTargetPos = getOrigin() - toEnemy * 100.0f + Vector(0,0,10);
                        utilityScore += 0.3f;
                    }
                }

                if (!shouldUseCaltrops && m_pNavigator && m_pNavigator->getCurrentWaypoint()) {
                    CWaypoint* pCurrentWpt = m_pNavigator->getCurrentWaypoint();
                    if (pCurrentWpt->hasFlag(CWaypointTypes::W_FL_FF_CALTROP_SPOT)) {
                         if (m_fLastSeeEnemyTime > 0 && engine->Time() - m_fLastSeeEnemyTime < 10.0f) {
                           shouldUseCaltrops = true;
                           vTargetPos = pCurrentWpt->getOrigin() + Vector(0,0,10);
                           utilityScore += 0.2f;
                         }
                    }
                }

                if (shouldUseCaltrops && vTargetPos != m_vEmpty) {
                    ADD_UTILITY_WEAPON_DATA_VECTOR(BOT_UTIL_FF_SCOUT_USE_CALTROPS, true, utilityScore, pCaltrops, 0, vTargetPos);
                }
            }
        }

        if (getClass() == TF_CLASS_SNIPER && !m_bIsPrimingGrenade) // Ensure sniper isn't priming a grenade
        {
            CBotWeapon* pSniperRifle = m_pWeapons->getWeaponByName("weapon_ff_sniperrifle");
            if (pSniperRifle && pSniperRifle->hasWeapon() && !pSniperRifle->outOfAmmo(this))
            {
                if (m_pEnemy && CBotGlobals::entityIsAlive(m_pEnemy) && isVisible(m_pEnemy) &&
                    distanceFrom(m_pEnemy) > 500.0f)
                {
                    ADD_UTILITY_WEAPON(BOT_UTIL_SNIPE, true, 0.85f, pSniperRifle);
                }
            }
        }


		if (m_pEnemy && CBotGlobals::entityIsAlive(m_pEnemy) && isVisible(m_pEnemy) && !m_bIsPrimingGrenade)
		{
			float enemyDist = distanceFrom(m_pEnemy);
			CBotWeapon* pGrenade = NULL;

			pGrenade = m_pWeapons->getWeaponByName("weapon_ff_gren_std");
			if (pGrenade && pGrenade->hasWeapon() && pGrenade->getAmmo(this) > 0) {
				if (enemyDist > 250 && enemyDist < 1200) {
					float utility = 0.65f;
					if (CBotGlobals::countPlayersNearOrigin(CBotGlobals::entityOrigin(m_pEnemy), 250.0f, CTeamFortress2Mod::getEnemyTeam(getTeam()), m_pEdict, true) > 1) utility += 0.2f;
					ADD_UTILITY_WEAPON(BOT_UTIL_FF_USE_GRENADE_STD, true, utility, pGrenade);
				}
			}

			pGrenade = m_pWeapons->getWeaponByName("weapon_ff_gren_conc");
			if (pGrenade && pGrenade->hasWeapon() && pGrenade->getAmmo(this) > 0) {
				if (enemyDist > 200 && enemyDist < 1000) {
					float utility = 0.60f;
					ADD_UTILITY_WEAPON(BOT_UTIL_FF_USE_GRENADE_CONC, true, utility, pGrenade);
				}
			}
		}

		CBotWeapon* pConcGrenade = m_pWeapons->getWeaponByName("weapon_ff_gren_conc");
		if (pConcGrenade && pConcGrenade->hasWeapon() && !pConcGrenade->outOfAmmo(this))
		{
			bool conc_condition_met = false;
			if (m_pNavigator && m_pNavigator->getGoalOrigin() != m_vEmpty && m_pNavigator->getGoalOrigin().z > getOrigin().z + 120.0f ) {
				conc_condition_met = true;
			}
			if (getHealthPercent() < 0.3f && m_pEnemy && CBotGlobals::entityIsAlive(m_pEnemy) && distanceFrom(m_pEnemy) < 250.0f) {
				conc_condition_met = true;
			}
			if (conc_condition_met) {
				ADD_UTILITY_WEAPON(BOT_UTIL_FF_CONC_JUMP_MOBILITY, true, 0.8f, pConcGrenade);
			}
		}
	}
	
    ADD_UTILITY(BOT_UTIL_ROAM,true,0.0001f);

	CBotUtility* nextUtil = NULL;
	bool bCheckCurrent = !m_pSchedules->isEmpty();
	utils.execute();

	while ((nextUtil = utils.nextBest()) != NULL)
	{
		if (bCheckCurrent && m_pSchedules->getCurrentSchedule() && m_CurrentUtil == nextUtil->getId()) {
			break;
		}
		if (executeAction(nextUtil)) {
			m_CurrentUtil = nextUtil->getId();
			m_fUtilTimes[m_CurrentUtil] = engine->Time() + 0.5f;
			utils.freeMemory();
			return;
		}
	}
	utils.freeMemory();
}

bool CBotFF::executeAction(CBotUtility *util)
{
    if (!util) return false;
    int id = util->getId();
    CBotWeapon* pGrenadeWeapon = NULL;
	float primeTime = 1.0f;
	Vector targetPos = m_vEmpty;

	if (id >= BOT_UTIL_FF_USE_GRENADE_STD && id <= BOT_UTIL_FF_CONC_JUMP_MOBILITY) {
        pGrenadeWeapon = util->getWeaponChoice();
        if (!pGrenadeWeapon) return false;
    } else if (id == BOT_UTIL_FF_SPY_USE_TRANQ) {
         pGrenadeWeapon = util->getWeaponChoice();
        if (!pGrenadeWeapon) return false;
    } else if (id == BOT_UTIL_FF_SCOUT_USE_CALTROPS) {
        pGrenadeWeapon = util->getWeaponChoice();
        if (!pGrenadeWeapon) return false;
        targetPos = util->getVectorData();
    }


	if (id == BOT_UTIL_FF_USE_GRENADE_STD || (id == BOT_UTIL_FF_USE_GRENADE_CONC && id != BOT_UTIL_FF_CONC_JUMP_MOBILITY) || id == BOT_UTIL_FF_SPY_USE_TRANQ ) {
		if (m_pEnemy && CBotGlobals::entityIsAlive(m_pEnemy)) {
			targetPos = CBotGlobals::entityOrigin(m_pEnemy);
		} else if (id != BOT_UTIL_FF_CONC_JUMP_MOBILITY) {
			return false;
		}
	}

    switch (id)
    {
        case BOT_UTIL_FF_USE_GRENADE_STD:
            if (targetPos != m_vEmpty && pGrenadeWeapon) {
				primeTime = 1.5f;
                m_pSchedules->add(new CSchedFFPrimeThrowGrenade(targetPos, pGrenadeWeapon, primeTime));
                return true;
            }
            break;

        case BOT_UTIL_FF_USE_GRENADE_CONC:
            if (targetPos != m_vEmpty && pGrenadeWeapon) {
				primeTime = 0.5f;
                m_pSchedules->add(new CSchedFFPrimeThrowGrenade(targetPos, pGrenadeWeapon, primeTime));
		        return true;
			} else {
                return false;
            }
            break;

		case BOT_UTIL_FF_CONC_JUMP_MOBILITY:
			if (pGrenadeWeapon)
			{
				m_pSchedules->add(new CSchedFFConcJumpSelf(this, pGrenadeWeapon));
				return true;
			}
			break;
        case BOT_UTIL_FF_ENGRI_BUILD_MANCANNON:
            {
                CWaypoint* pBuildSpot = CWaypoints::getWaypoint(util->getIntData());
                if (pBuildSpot) {
                    m_pSchedules->add(new CSchedFFBuildMancannon(this, pBuildSpot));
                    return true;
                }
            }
            break;
        case BOT_UTIL_FF_DEMO_LAY_PIPE_TRAP:
            {
                CWaypoint* pTrapSpot = CWaypoints::getWaypoint(util->getIntData());
                if (pTrapSpot) {
                    m_pSchedules->add(new CSchedFFDemoLayPipeTrap(this, pTrapSpot));
                    return true;
                }
            }
            break;
        case BOT_UTIL_FF_SPY_USE_TRANQ:
             if (pGrenadeWeapon && targetPos != m_vEmpty) {
                CBotSchedule* pSpyTranqSched = new CBotSchedule();
                if(util->getWeaponChoice() && util->getWeaponChoice()->getWeaponInfo())
                {
                    pSpyTranqSched->addTask(new CSelectWeaponTask(util->getWeaponChoice()->getWeaponInfo()));
                    pSpyTranqSched->addTask(new CBotAttackSched(m_pEnemy));
                    m_pSchedules->add(pSpyTranqSched);
                    return true;
                }
            }
            break;
        case BOT_UTIL_FF_SCOUT_USE_CALTROPS:
             if (pGrenadeWeapon && targetPos != m_vEmpty) {
                 float caltropPrimeTime = 0.1f;
                 m_pSchedules->add(new CSchedFFPrimeThrowGrenade(targetPos, pGrenadeWeapon, caltropPrimeTime));
                 return true;
             }
             break;
		case BOT_UTIL_FF_HUNTED_VIP_ESCAPE:
			{
				CWaypoint* pEscapeWpt = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_VIP_ESCAPE_POINT, getTeam());
				if (pEscapeWpt) {
					m_pSchedules->add(new CBotGotoOriginSched(pEscapeWpt->getOrigin()));
				} else {
					pEscapeWpt = CWaypoints::randomWaypointGoal(-1, getTeam(),0,false,this);
					if (pEscapeWpt) {
						m_pSchedules->add(new CBotGotoOriginSched(pEscapeWpt->getOrigin()));
					}
				}
				return true;
			}
			break;
       case BOT_UTIL_FF_HUNTED_KILL_VIP:
           if (util->getTaskEdictTarget()) {
               m_pSchedules->add(new CBotAttackSched(util->getTaskEdictTarget()));
			   return true;
           }
           break;
       case BOT_UTIL_FF_HUNTED_PROTECT_VIP:
           if (util->getTaskEdictTarget()) {
               CBotSchedule* pFollowSched = new CBotSchedule();
               pFollowSched->addTask(new CFindPathTask(util->getTaskEdictTarget()));
               pFollowSched->addTask(new CBotNest(randomFloat(200.0f, 400.0f), 15.0f));
               m_pSchedules->add(pFollowSched);
			   return true;
           }
           break;
        case BOT_UTIL_MEDIC_HEAL:
             {
                 CBotWeapon* chosenWeapon = util->getWeaponChoice();
                 if (chosenWeapon && chosenWeapon->getWeaponInfo() &&
                     strcmp(chosenWeapon->getWeaponInfo()->getWeaponName(), "weapon_ff_medkit") == 0)
                 {
                     if (m_pHeal.Get() && CBotGlobals::entityIsValid(m_pHeal.Get())) {
                         m_pSchedules->add(new CSchedFFMedicHealTeammate(this, m_pHeal.Get()));
                         return true;
                     }
                 }
             }
             break;
        case BOT_UTIL_SNIPE: // Reusing TF2's BOT_UTIL_SNIPE
             if (util->getWeaponChoice() && util->getWeaponChoice()->getWeaponInfo() &&
                 strcmp(util->getWeaponChoice()->getWeaponInfo()->getWeaponName(), "weapon_ff_sniperrifle") == 0)
             {
                 if (m_pEnemy && CBotGlobals::entityIsValid(m_pEnemy.Get())) {
                     m_pSchedules->add(new CSchedFFSnipe(this, m_pEnemy.Get()));
                     return true;
                 }
             }
             break;

        default:
            return CBot::executeAction(util);
    }
    return false;
}


// CBotTF2 methods remain below, untouched by this specific FF modification pass.
void CBotTF2::MannVsMachineWaveComplete()

[end of utils/RCBot2_meta/bot_fortress.cpp]
