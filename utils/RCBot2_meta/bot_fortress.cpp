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

