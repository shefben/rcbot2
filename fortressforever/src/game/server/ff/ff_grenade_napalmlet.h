#ifndef FF_GRENADE_NAPALMLET_H
#define FF_GRENADE_NAPALMLET_H

#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "ff_grenade_base.h"

#include "ff_utils.h"
#include "EntityFlame.h"

#define NAPALMLET_MODEL "models/gibs/AGIBS.mdl"

#ifdef CLIENT_DLL
	#define CFFGrenadeNapalmlet C_FFGrenadeNapalmlet
#endif

#include "ff_player.h"

//=============================================================================
//
// Class CFFGrenadeNapalmlet
//
//=============================================================================
class CFFGrenadeNapalmlet : public CBaseAnimating
{
public:
	DECLARE_CLASS( CFFGrenadeNapalmlet, CBaseAnimating );
	void Precache();

	CFFGrenadeNapalmlet( void ){m_flBurnTime = gpGlobals->curtime + 5.0f;}
	void UpdateOnRemove( void );

	void Spawn();
	void SetBurnTime( float burnTime ){ m_flBurnTime = gpGlobals->curtime + burnTime; if (m_pFlame) m_pFlame->SetLifetime( m_flBurnTime );}
	
	void ResolveFlyCollisionCustom( trace_t &trace, Vector &vecVelocity );
	void FlameThink(void);
	DECLARE_DATADESC();
private:
	float m_flBurnTime;
	CEntityFlame *m_pFlame;
	int CalculateBonusBurnDamage(int burnLevel);
};

#endif