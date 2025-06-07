#include "cbase.h"
#include "ff_grenade_base.h"

#include "ff_grenade_napalmlet.h"
#include "ff_utils.h"

#include "ff_player.h"

//ConVar ffdev_nap_bonusdamage_burn1("ffdev_nap_bonusdamage_burn1", "0", FCVAR_REPLICATED | FCVAR_CHEAT);
#define NAP_BONUSDAMAGE_BURN1 0 //ffdev_nap_bonusdamage_burn1.GetInt()
//ConVar ffdev_nap_bonusdamage_burn2("ffdev_nap_bonusdamage_burn2", "1", FCVAR_REPLICATED | FCVAR_CHEAT);
#define NAP_BONUSDAMAGE_BURN2 1 //ffdev_nap_bonusdamage_burn2.GetInt()
//ConVar ffdev_nap_bonusdamage_burn3("ffdev_nap_bonusdamage_burn3", "2", FCVAR_REPLICATED | FCVAR_CHEAT);
#define NAP_BONUSDAMAGE_BURN3 2 //ffdev_nap_bonusdamage_burn3.GetInt()

//ConVar burn_standon_ng("ffdev_burn_standon_ng", "7.0", 0, "Damage you take when standing on a burning napalmlet");
//ConVar ffdev_nap_flamesize("ffdev_nap_flamesize", "30.0", 0, "Napalmlet flame size");
#define FFDEV_NAP_FLAMESIZE 30.0f //ffdev_nap_flamesize.GetFloat() // 50.0f
//ConVar nap_burn_radius("ffdev_nap_burn_radius","70.0",FCVAR_FF_FFDEV,"Burn radius of a napalmlet.");
#define NAP_BURN_RADIUS 70.0f //nap_burn_radius.GetFloat() //98.0f

//ConVar ffdev_nap_burnamount("ffdev_nap_burnamount", "10.0", 0, "Napalmlet burn increase per tick, 100 is a full burn level");
#define FFDEV_NAPALM_BURNAMOUNT 10.0f //ffdev_nap_burnamount.GetFloat() // 50.0f
//ConVar ffdev_nap_height("ffdev_nap_height", "70.0", 0, "Napalmlet maximum burn height above the ground");
#define FFDEV_NAP_HEIGHT 70.0f //ffdev_nap_height.GetFloat() // 50.0f

#define BURN_STANDON_NG 2

BEGIN_DATADESC( CFFGrenadeNapalmlet )
	DEFINE_THINKFUNC( FlameThink ),
END_DATADESC()


LINK_ENTITY_TO_CLASS( ff_grenade_napalmlet, CFFGrenadeNapalmlet );
PRECACHE_WEAPON_REGISTER( ff_grenade_napalmlet );


void CFFGrenadeNapalmlet::UpdateOnRemove( void )
{
	StopSound( "General.BurningFlesh" );
	StopSound( "General.BurningObject" );

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: Precache assets
//-----------------------------------------------------------------------------
void CFFGrenadeNapalmlet::Precache ( void )
{
	PrecacheModel( NAPALMLET_MODEL );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Spawn
//-----------------------------------------------------------------------------
void CFFGrenadeNapalmlet::Spawn( void )
{
	BaseClass::Spawn();

	SetModel ( NAPALMLET_MODEL );
	SetAbsAngles( QAngle( 0, 0, 0 ) );
	SetSolidFlags( FSOLID_TRIGGER | FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM );
	SetSolid( SOLID_BBOX );
	SetCollisionGroup( COLLISION_GROUP_DEBRIS );
	SetSize( Vector ( -5, -5, -5 ), Vector ( 5, 5, 5 ) );
	SetThink( &CFFGrenadeNapalmlet::FlameThink );
	SetNextThink( gpGlobals->curtime );
	SetEffects(EF_NOSHADOW);

	m_pFlame = CEntityFlame::Create( this, false );
	if (m_pFlame)
	{
		m_pFlame->SetLifetime( m_flBurnTime );
		AddFlag( FL_ONFIRE );
		SetEffectEntity( m_pFlame );
		m_pFlame->SetSize( FFDEV_NAP_FLAMESIZE );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFFGrenadeNapalmlet::ResolveFlyCollisionCustom( trace_t &trace, Vector &vecVelocity )
{
	//Assume all surfaces have the same elasticity
	float flSurfaceElasticity = 1.0;

	//Don't bounce off of players with perfect elasticity
	if( trace.m_pEnt && trace.m_pEnt->IsPlayer() )
	{
		flSurfaceElasticity = 0.3;
	}

	// if its breakable glass and we kill it, don't bounce.
	// give some damage to the glass, and if it breaks, pass 
	// through it.
	bool breakthrough = false;

	if (trace.m_pEnt && ( FClassnameIs(trace.m_pEnt, "func_breakable") || FClassnameIs(trace.m_pEnt, "func_breakable_surf") ) )
	{
		breakthrough = true;
	}

	if (breakthrough)
	{
		CTakeDamageInfo info( this, GetOwnerEntity(), 10, DMG_CLUB );
		trace.m_pEnt->DispatchTraceAttack( info, GetAbsVelocity(), &trace );

		ApplyMultiDamage();

		if( trace.m_pEnt->m_iHealth <= 0 )
		{
			// slow our flight a little bit
			Vector vel = GetAbsVelocity();

			vel *= 0.4;

			SetAbsVelocity( vel );
			return;
		}
	}

	float flTotalElasticity = GetElasticity() * flSurfaceElasticity;
	flTotalElasticity = clamp( flTotalElasticity, 0.0f, 0.9f );

	// NOTE: A backoff of 2.0f is a reflection
	Vector vecAbsVelocity;
	PhysicsClipVelocity( GetAbsVelocity(), trace.plane.normal, vecAbsVelocity, 2.0f );
	vecAbsVelocity *= flTotalElasticity;

	// Get the total velocity (player + conveyors, etc.)
	VectorAdd( vecAbsVelocity, GetBaseVelocity(), vecVelocity );
	float flSpeedSqr = DotProduct( vecVelocity, vecVelocity );

	// Stop if on ground.
	if ( trace.plane.normal.z > 0.7f )			// Floor
	{
		// Verify that we have an entity.
		CBaseEntity *pEntity = trace.m_pEnt;
		Assert( pEntity );

		SetAbsVelocity( vecAbsVelocity );

		if ( flSpeedSqr < ( 30 * 30 ) )
		{
			if ( pEntity->IsStandable() )
			{
				SetGroundEntity( pEntity );
			}

			// Reset velocities.
			SetAbsVelocity( vec3_origin );
			SetLocalAngularVelocity( vec3_angle );	
		}
		else
		{
			Vector vecDelta = GetBaseVelocity() - vecAbsVelocity;	
			Vector vecBaseDir = GetBaseVelocity();
			VectorNormalize( vecBaseDir );
			float flScale = vecDelta.Dot( vecBaseDir );

			VectorScale( vecAbsVelocity, ( 1.0f - trace.fraction ) * gpGlobals->frametime, vecVelocity ); 
			VectorMA( vecVelocity, ( 1.0f - trace.fraction ) * gpGlobals->frametime, GetBaseVelocity() * flScale, vecVelocity );
			PhysicsPushEntity( vecVelocity, &trace );
		}
	}
	else
	{
		// If we get *too* slow, we'll stick without ever coming to rest because
		// we'll get pushed down by gravity faster than we can escape from the wall.
		if ( flSpeedSqr < ( 30 * 30 ) )
		{
			// Reset velocities.
			SetAbsVelocity( vec3_origin );
			SetLocalAngularVelocity( vec3_angle );
		}
		else
		{
			SetAbsVelocity( vecAbsVelocity );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Burninate the players
//-----------------------------------------------------------------------------
void CFFGrenadeNapalmlet::FlameThink()
{
	// Remove if we've reached the end of our fuse
	if( gpGlobals->curtime > m_flBurnTime )
	{
		UTIL_Remove(this);
		return;
	}

	// Bug #0001664: Pyro napalm flames in water shouldnt exist
	if( GetWaterLevel() != 0  )
	{
		UTIL_Remove(this);
		return;
	}

	Vector	vecSrc = GetAbsOrigin();
	vecSrc.z += 1;

	CBaseEntity *pEntity = NULL;

	for( CEntitySphereQuery sphere( vecSrc, NAP_BURN_RADIUS ); ( pEntity = sphere.GetCurrentEntity() ) != NULL; sphere.NextEntity() )
	{
		if( !pEntity )
			continue;

		// Bug #0000269: Napalm through walls.
		// Mulch: if we hit water w/ the trace, abort too!
		trace_t tr;
		UTIL_TraceLine(GetAbsOrigin(), pEntity->GetAbsOrigin(), MASK_SOLID_BRUSHONLY | CONTENTS_WATER, this, COLLISION_GROUP_DEBRIS, &tr);

		if (tr.fraction < 1.0f)
			continue;

		// Bug #0000270: Napalm grenade burn radius reaches unrealisticly high.
		float height = tr.startpos.z - tr.endpos.z;
		if (height < -FFDEV_NAP_HEIGHT || height > FFDEV_NAP_HEIGHT)
			continue;

		// Don't damage if entity is more than feet deep in water
		if( pEntity->GetWaterLevel() >= 2 )
			continue;

		switch( pEntity->Classify() )
		{
			case CLASS_PLAYER:
			{
				CFFPlayer *pPlayer = ToFFPlayer( pEntity );
				if( !pPlayer )
					continue;

				if (g_pGameRules->FCanTakeDamage(pPlayer, GetOwnerEntity()))
				{
					int damage = BURN_STANDON_NG + CalculateBonusBurnDamage(pPlayer->GetBurnLevel());
					pPlayer->TakeDamage( CTakeDamageInfo( this, GetOwnerEntity(), damage, DMG_BURN ) );
					pPlayer->IncreaseBurnLevel ( FFDEV_NAPALM_BURNAMOUNT );
				}
			}
			break;
			case CLASS_SENTRYGUN:
			case CLASS_MANCANNON://Adding napalm damage to jumppad -GreenMushy
			case CLASS_DISPENSER:
			{
				if (g_pGameRules->FCanTakeDamage( pEntity, GetOwnerEntity()))
					pEntity->TakeDamage( CTakeDamageInfo( this, GetOwnerEntity(), BURN_STANDON_NG, DMG_BURN ) );
			}
			
			default:
				break;
		}
	}

	SetNextThink( gpGlobals->curtime + 0.25f );
}

//----------------------------------------------------------------------------
// Purpose: Calculate the bonus damage for the napalmlet based on the players current burn level
//----------------------------------------------------------------------------
int CFFGrenadeNapalmlet::CalculateBonusBurnDamage(int burnLevel)
{
	if (burnLevel <100)
	{
		return NAP_BONUSDAMAGE_BURN1;
	}
	if (burnLevel <200)
	{
		return NAP_BONUSDAMAGE_BURN2;
	}

	return NAP_BONUSDAMAGE_BURN3;
}
