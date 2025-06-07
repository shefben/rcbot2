#include "cbase.h"
#include "ff_weapon_base.h"
#include "ff_weapon_jumpgun.h"
#include "ff_fx_shared.h"
#include "in_buttons.h"
#include "beam_flags.h"

#ifdef CLIENT_DLL
	#include "soundenvelope.h"
	#include "c_ff_player.h"
	#include "beamdraw.h"
	#include "c_te_effect_dispatch.h"

	extern void FormatViewModelAttachment( Vector &vOrigin, bool bInverse );
#else
	#include "omnibot_interface.h"
	#include "ff_player.h"
	#include "ff_entity_system.h"
	#include "te_effect_dispatch.h"
#endif

//ConVar ffdev_jumpgun_allowunchargedshot("ffdev_jumpgun_allowunchargedshot", "0", FCVAR_REPLICATED | FCVAR_CHEAT);
#define JUMPGUN_ALLOWUNCHARGEDSHOT 0 //ffdev_jumpgun_allowunchargedshot.GetBool()

//ConVar ffdev_jumpgun_verticalpush("ffdev_jumpgun_verticalpush", "500", FCVAR_REPLICATED | FCVAR_CHEAT);
#define JUMPGUN_VERTICALPUSH 500 //ffdev_jumpgun_verticalpush.GetFloat()

//ConVar ffdev_jumpgun_horizontalpush("ffdev_jumpgun_horizontalpush", "450", FCVAR_REPLICATED | FCVAR_CHEAT);
#define JUMPGUN_HORIZONTALPUSH 450 //ffdev_jumpgun_horizontalpush.GetFloat()

//ConVar ffdev_jumpgun_horizontalsetvelocity("ffdev_jumpgun_horizontalsetvelocity", "1", FCVAR_REPLICATED | FCVAR_CHEAT);
#define JUMPGUN_HORIZONTALSETVELOCITY 1//ffdev_jumpgun_horizontalsetvelocity.GetBool()

//ConVar ffdev_jumpgun_verticalsetvelocity("ffdev_jumpgun_verticalsetvelocity", "1", FCVAR_REPLICATED | FCVAR_CHEAT);
#define JUMPGUN_VERTICALSETVELOCITY 1//ffdev_jumpgun_verticalsetvelocity.GetBool()

//effect vars

//ConVar ffdev_jumpgun_fx_radius("ffdev_jumpgun_fx_radius", "128", FCVAR_REPLICATED /* | FCVAR_CHEAT */);
#define JUMPGUN_EFFECT_RADIUS 128 //ffdev_jumpgun_fx_radius.GetFloat()

//ConVar ffdev_jumpgun_fx_lifetime("ffdev_jumpgun_fx_lifetime", ".3", FCVAR_REPLICATED /* | FCVAR_CHEAT */);
#define JUMPGUN_EFFECT_LIFETIME 0.3 // ffdev_jumpgun_fx_lifetime.GetFloat()

//ConVar ffdev_jumpgun_fx_width("ffdev_jumpgun_fx_width", "16", FCVAR_REPLICATED /* | FCVAR_CHEAT */);
#define JUMPGUN_EFFECT_WIDTH 16 //ffdev_jumpgun_fx_width.GetFloat()

//ConVar ffdev_jumpgun_fx_spread("ffdev_jumpgun_fx_spread", "0", FCVAR_REPLICATED /* | FCVAR_CHEAT */);
#define JUMPGUN_EFFECT_SPREAD 0 //ffdev_jumpgun_fx_spread.GetFloat()

//ConVar ffdev_jumpgun_fx_amplitude("ffdev_jumpgun_fx_amplitude", "10", FCVAR_REPLICATED /* | FCVAR_CHEAT */);
#define JUMPGUN_EFFECT_AMPLITUDE 10 //ffdev_jumpgun_fx_amplitude.GetFloat()

//ConVar ffdev_jumpgun_fx_r("ffdev_jumpgun_fx_r", "255", FCVAR_REPLICATED /* | FCVAR_CHEAT */);
#define JUMPGUN_EFFECT_R 255 //ffdev_jumpgun_fx_r.GetInt()

//ConVar ffdev_jumpgun_fx_g("ffdev_jumpgun_fx_g", "255", FCVAR_REPLICATED /* | FCVAR_CHEAT */);
#define JUMPGUN_EFFECT_G 255 //ffdev_jumpgun_fx_g.GetInt()

//ConVar ffdev_jumpgun_fx_b("ffdev_jumpgun_fx_b", "255", FCVAR_REPLICATED /* | FCVAR_CHEAT */);
#define JUMPGUN_EFFECT_B 255 //ffdev_jumpgun_fx_b.GetInt()

//ConVar ffdev_jumpgun_fx_alpha("ffdev_jumpgun_fx_alpha", "100", FCVAR_REPLICATED /* | FCVAR_CHEAT */);
#define JUMPGUN_EFFECT_ALPHA 100 //ffdev_jumpgun_fx_alpha.GetInt()

//ConVar ffdev_jumpgun_fx_speed("ffdev_jumpgun_fx_speed", "0", FCVAR_REPLICATED /* | FCVAR_CHEAT */);
#define JUMPGUN_EFFECT_SPEED 0 //ffdev_jumpgun_fx_speed.GetFloat()

//ConVar ffdev_jumpgun_fx_offset_x("ffdev_jumpgun_fx_offset_x", "0", FCVAR_REPLICATED /* | FCVAR_CHEAT */);
#define JUMPGUN_EFFECT_X_OFFSET 0 //ffdev_jumpgun_fx_offset_x.GetFloat()

//ConVar ffdev_jumpgun_fx_offset_y("ffdev_jumpgun_fx_offset_y", "0", FCVAR_REPLICATED /* | FCVAR_CHEAT */);
#define JUMPGUN_EFFECT_Y_OFFSET 0 //ffdev_jumpgun_fx_offset_y.GetFloat()

//ConVar ffdev_jumpgun_fx_offset_z("ffdev_jumpgun_fx_offset_z", "-32", FCVAR_REPLICATED /* | FCVAR_CHEAT */);
#define JUMPGUN_EFFECT_Z_OFFSET -32 //ffdev_jumpgun_fx_offset_z.GetFloat()

//=============================================================================
// CFFWeaponJumpgun tables
//=============================================================================

IMPLEMENT_NETWORKCLASS_ALIASED( FFWeaponJumpgun, DT_FFWeaponJumpgun )

BEGIN_NETWORK_TABLE(CFFWeaponJumpgun, DT_FFWeaponJumpgun )
#ifdef GAME_DLL
	SendPropTime( SENDINFO( m_flTotalChargeTime ) ), 
	SendPropTime( SENDINFO( m_flClampedChargeTime ) ), 
#else
	RecvPropTime( RECVINFO( m_flTotalChargeTime ) ), 
	RecvPropTime( RECVINFO( m_flClampedChargeTime ) ), 
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CFFWeaponJumpgun )
	DEFINE_PRED_FIELD_TOL( m_flTotalChargeTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ), 
	DEFINE_PRED_FIELD_TOL( m_flClampedChargeTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ), 
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( ff_weapon_jumpgun, CFFWeaponJumpgun );
PRECACHE_WEAPON_REGISTER( ff_weapon_jumpgun );

//=============================================================================
// CFFWeaponJumpgun implementation
//=============================================================================

//----------------------------------------------------------------------------
// Purpose: Constructor
//----------------------------------------------------------------------------
CFFWeaponJumpgun::CFFWeaponJumpgun( void )
{
	m_nRevSound = SPECIAL1;

#ifdef GAME_DLL

	// -1 means we are not charging
	m_flStartTime = m_flLastUpdate = -1.0f;
	m_flTotalChargeTime = m_flClampedChargeTime = 0.0f;

	m_flRevSoundNextUpdate = 0.0f;

#else

	m_iAttachment1 = m_iAttachment2 = -1;

	m_flTotalChargeTimeBuffered = m_flClampedChargeTimeBuffered = 0.0f;
	m_flChargeTimeBufferedNextUpdate = 0.0f;

	//m_colorMuzzleDLight.r = g_uchRailColors[0][0];
	//m_colorMuzzleDLight.g = g_uchRailColors[0][1];
	//m_colorMuzzleDLight.b = g_uchRailColors[0][2];

#endif
}

//----------------------------------------------------------------------------
// Purpose: 
//----------------------------------------------------------------------------
void CFFWeaponJumpgun::UpdateOnRemove( void )
{
#ifdef GAME_DLL

	m_flStartTime = m_flLastUpdate = -1.0f;
	m_flTotalChargeTime = m_flClampedChargeTime = 0.0f;

	m_flNextPrimaryAttack = (JUMPGUN_ALLOWUNCHARGEDSHOT) ? (gpGlobals->curtime + 0.2f) : (gpGlobals->curtime + JUMPGUN_CHARGEUPTIME);

#else

	m_flTotalChargeTimeBuffered = m_flClampedChargeTimeBuffered = 0.0f;
	m_flChargeTimeBufferedNextUpdate = 0.0f;

#endif

	BaseClass::UpdateOnRemove();
}

//----------------------------------------------------------------------------
// Purpose: Deploy
//----------------------------------------------------------------------------
bool CFFWeaponJumpgun::Deploy( void )
{
#ifdef GAME_DLL

	//Get this player's last weapon
	CFFWeaponBase* pLastWeapon = ToFFPlayer(GetOwnerEntity())->GetLastFFWeapon();

	//If the weapon is valid
	if( pLastWeapon != NULL )
	{
		//If the last weapon was a jumppad, dont reset the jumpgun values
		if( pLastWeapon->GetWeaponID() == FF_WEAPON_DEPLOYMANCANNON )
		{
			//The last weapon was a jumpad
			DevMsg("Last weapon was a jumpad.  Do nothing\n");
			//Dont reset anything?
		}
		//If this was the players last weapon
		else if( pLastWeapon == this )
		{
			//Means this was the weapon deployed when the player built a jumppad with right click
			DevMsg("Jumpgun was the last weapon.  Do nothing\n");
			//Dont reset anything?
		}
		//The last weapon was not a jumppad, so reset normally
		else
		{
			DevMsg("Last weapon was valid but not a jumppad.  Deploy jumpgun normally\n");

			m_flStartTime = -1.0f;
			m_flLastUpdate = -1.0f;
			m_flTotalChargeTime = 0.0f;
			m_flClampedChargeTime = 0.0f;

			m_flNextPrimaryAttack = (JUMPGUN_ALLOWUNCHARGEDSHOT) ? (gpGlobals->curtime + 0.2f) : (gpGlobals->curtime + JUMPGUN_CHARGEUPTIME);
		}
	}
	//This is the first time the player pulls out a weapon
	else
	{
		DevMsg("Last weapon was NULL.  Deploy charged jumpgun\n");

		m_flStartTime = gpGlobals->curtime - JUMPGUN_CHARGEUPTIME;
		m_flLastUpdate = gpGlobals->curtime - JUMPGUN_CHARGEUPTIME;
		m_flTotalChargeTime = JUMPGUN_CHARGEUPTIME;
		m_flClampedChargeTime = JUMPGUN_CHARGEUPTIME;

		m_flNextPrimaryAttack = gpGlobals->curtime;
	}

	//Reset the last weapon to this jumpgun
	ToFFPlayer(GetOwnerEntity())->SetLastFFWeapon(this);

#endif

	return BaseClass::Deploy();
}

//----------------------------------------------------------------------------
// Purpose: Holster
//----------------------------------------------------------------------------
bool CFFWeaponJumpgun::Holster( CBaseCombatWeapon *pSwitchingTo )
{
#ifdef GAME_DLL
	//Only set this weapon to the last if something is calling Holster(NULL) on the jumpgun
	if( pSwitchingTo == NULL )
	{
		DevMsg("Holstering jumpgun - Setting last weapon to this jumpgun\n");

		//Reset the last weapon to this jumpgun
		ToFFPlayer(GetOwnerEntity())->SetLastFFWeapon(this);
	}
	//this means the player is switching to a valid weapon other then this
	else
	{
		DevMsg("Holstering jumpgun - Setting new last weapon\n");

		//Reset the last weapon to this jumpgun
		ToFFPlayer(GetOwnerEntity())->SetLastFFWeapon((CFFWeaponBase*)pSwitchingTo);

		//Only reset the values if the weapon being switched to is NOT a mancannon
		if( ((CFFWeaponBase*)pSwitchingTo)->GetWeaponID() != FF_WEAPON_DEPLOYMANCANNON )
		{
			m_flStartTime = -1.0f;
			m_flLastUpdate = -1.0f;
			m_flTotalChargeTime = 0.0f;
			m_flClampedChargeTime = 0.0f;

			m_flNextPrimaryAttack = (JUMPGUN_ALLOWUNCHARGEDSHOT) ? (gpGlobals->curtime + 0.2f) : (gpGlobals->curtime + JUMPGUN_CHARGEUPTIME);
		}
	}
#endif

	return BaseClass::Holster( pSwitchingTo );
}

//----------------------------------------------------------------------------
// Purpose: Precache
//----------------------------------------------------------------------------
void CFFWeaponJumpgun::Precache( void )
{
	PrecacheScriptSound( "railgun.single_shot" );		// SINGLE
	m_iShockwaveTexture = PrecacheModel( "sprites/lgtning.vmt" );	

	BaseClass::Precache();
}

//----------------------------------------------------------------------------
// Purpose: Fire a jumpgun
//----------------------------------------------------------------------------
void CFFWeaponJumpgun::Fire( void )
{
	CFFPlayer *pPlayer = GetPlayerOwner();

	if (!pPlayer)
		return;

	pPlayer->m_flTrueAimTime = gpGlobals->curtime;

	Vector vecForward, vecRight, vecUp;
	pPlayer->EyeVectors( &vecForward, &vecRight, &vecUp);
	VectorNormalizeFast( vecForward );
	VectorNormalizeFast( vecRight );
	
	// get only the direction the player is looking (ignore any z)
	Vector horizPush = CrossProduct(Vector( 0.0f, 0.0f, 1.0f ), vecRight);
	horizPush *= JUMPGUN_HORIZONTALPUSH;

	Vector vecSrc = pPlayer->Weapon_ShootPosition();

	float flPercent = m_flClampedChargeTime / JUMPGUN_CHARGEUPTIME;

	// Push them
	if (!JUMPGUN_VERTICALSETVELOCITY && !JUMPGUN_HORIZONTALSETVELOCITY)
		pPlayer->ApplyAbsVelocityImpulse(Vector(horizPush.x, horizPush.y, JUMPGUN_VERTICALPUSH) * flPercent);
	else if (JUMPGUN_VERTICALSETVELOCITY && JUMPGUN_HORIZONTALSETVELOCITY)
	    pPlayer->SetAbsVelocity(Vector(horizPush.x, horizPush.y, JUMPGUN_VERTICALPUSH) * flPercent);
	else
	{
		if (JUMPGUN_VERTICALSETVELOCITY)
		{
			Vector vecVelocity = pPlayer->GetAbsVelocity();
			pPlayer->SetAbsVelocity(Vector(vecVelocity.x, vecVelocity.y, JUMPGUN_VERTICALPUSH * flPercent));
		}
		else
		{
			pPlayer->ApplyAbsVelocityImpulse(Vector(0, 0, JUMPGUN_VERTICALPUSH) * flPercent);
		}
		
		if (JUMPGUN_HORIZONTALSETVELOCITY)
		{
			Vector vecVelocity = pPlayer->GetAbsVelocity();
			pPlayer->SetAbsVelocity(Vector(horizPush.x * flPercent, horizPush.y * flPercent, vecVelocity.z));
		}
		else
		{
			pPlayer->ApplyAbsVelocityImpulse(Vector(horizPush.x, horizPush.y, 0) * flPercent);
		}
	}

	if (m_bMuzzleFlash)
		pPlayer->DoMuzzleFlash();

	SendWeaponAnim( GetPrimaryAttackActivity() );

	// MUST call sound before removing a round from the clip of a CMachineGun
	WeaponSound(SINGLE);

	// Player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_flNextPrimaryAttack = (JUMPGUN_ALLOWUNCHARGEDSHOT) ? (gpGlobals->curtime + 0.2f) : (gpGlobals->curtime + JUMPGUN_CHARGEUPTIME);

	// reset these variables
#ifdef GAME_DLL
	m_flStartTime = m_flLastUpdate = -1.0f;
#endif
	m_flTotalChargeTime = m_flClampedChargeTime = 0.0f;

	// effect
	CBroadcastRecipientFilter filter;
	te->BeamRingPoint( 
		filter, 
		0,										//delay
		pPlayer->GetAbsOrigin() + Vector(JUMPGUN_EFFECT_X_OFFSET, JUMPGUN_EFFECT_Y_OFFSET, JUMPGUN_EFFECT_Z_OFFSET + ((pPlayer->GetFlags() & FL_DUCKING) ? 16.0f : 0.0f)),					//origin
		1.0f,									//start radius
		flPercent * JUMPGUN_EFFECT_RADIUS,		//end radius
		m_iShockwaveTexture,					//texture
		0,										//halo index
		0,										//start frame
		2,										//framerate
		JUMPGUN_EFFECT_LIFETIME,				//life
		JUMPGUN_EFFECT_WIDTH,					//width
		JUMPGUN_EFFECT_SPREAD,					//spread
		JUMPGUN_EFFECT_AMPLITUDE,				//amplitude
		JUMPGUN_EFFECT_R,						//r
		JUMPGUN_EFFECT_G,						//g
		JUMPGUN_EFFECT_B,						//b
		JUMPGUN_EFFECT_ALPHA,					//a
		JUMPGUN_EFFECT_SPEED,					//speed
		FBEAM_FADEOUT
	);
}

//----------------------------------------------------------------------------
// Purpose: Handle all the chargeup stuff here
//----------------------------------------------------------------------------
void CFFWeaponJumpgun::ItemPostFrame( void )
{

	CFFPlayer *pPlayer = GetPlayerOwner();

	if (!pPlayer)
		return;

#ifdef GAME_DLL
	// Not currently charging, but wanting to start it up
	if (m_flStartTime == -1.0f)
	{
		m_flStartTime = m_flLastUpdate = gpGlobals->curtime;
		m_flTotalChargeTime = m_flClampedChargeTime = 0.0f;
	}

	else
	{
		m_flTotalChargeTime += gpGlobals->curtime - m_flLastUpdate;
		m_flLastUpdate = gpGlobals->curtime;

		m_flClampedChargeTime = clamp(gpGlobals->curtime - m_flStartTime, 0, JUMPGUN_CHARGEUPTIME);

	}
#endif

    if ((pPlayer->m_nButtons & IN_ATTACK) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
	{
		CANCEL_IF_BUILDING();
		CANCEL_IF_CLOAKED();
		Fire();
	}

#ifdef CLIENT_DLL
	// create a little buffer so some client stuff can be more smooth
	if (m_flChargeTimeBufferedNextUpdate <= gpGlobals->curtime)
	{
		m_flChargeTimeBufferedNextUpdate = gpGlobals->curtime + JUMPGUN_CHARGETIMEBUFFERED_UPDATEINTERVAL;
		m_flTotalChargeTimeBuffered = m_flTotalChargeTime;
		m_flClampedChargeTimeBuffered = m_flClampedChargeTime;
	}
#endif
}

//----------------------------------------------------------------------------
// Purpose: Get charge
//----------------------------------------------------------------------------
float CFFWeaponJumpgun::GetClampedCharge( void )
{
	return m_flClampedChargeTime;
}

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: This takes place after the viewmodel is drawn. We use this to
//			create the glowing glob of stuff inside the model and the faint
//			glow at the barrel.
//-----------------------------------------------------------------------------
void CFFWeaponJumpgun::ViewModelDrawn( C_BaseViewModel *pBaseViewModel )
{
	// Not charging at all or even much, so no need to draw shit
	if (m_flClampedChargeTimeBuffered <= 0.0)
		return;

	// We'll get these done and out of the way
	if (m_iAttachment1 == -1 || m_iAttachment2 == -1)
	{
		m_iAttachment1 = pBaseViewModel->LookupAttachment("railgunFX1");
		m_iAttachment2 = pBaseViewModel->LookupAttachment("railgunFX2");
	}

	Vector vecStart, vecEnd, vecMuzzle;
	QAngle tmpAngle;

	pBaseViewModel->GetAttachment(m_iAttachment1, vecStart, tmpAngle);
	pBaseViewModel->GetAttachment(m_iAttachment2, vecEnd, tmpAngle);
	pBaseViewModel->GetAttachment(1, vecMuzzle, tmpAngle);

	::FormatViewModelAttachment(vecStart, true);
	::FormatViewModelAttachment(vecEnd, true);
	::FormatViewModelAttachment(vecMuzzle, true);

	float flPercent = clamp( m_flClampedChargeTimeBuffered / JUMPGUN_CHARGEUPTIME, 0.0f, 1.0f);
	flPercent = sqrtf( flPercent );

	// Haha, clean this up pronto!
	Vector vecControl = (vecStart + vecEnd) * 0.5f + Vector(random->RandomFloat(-flPercent, flPercent), random->RandomFloat(-flPercent, flPercent), random->RandomFloat(-flPercent, flPercent));

	float flScrollOffset = gpGlobals->curtime - (int) gpGlobals->curtime;

	CMatRenderContextPtr pMatRenderContext(g_pMaterialSystem);

	IMaterial *pMat = materials->FindMaterial("sprites/physbeam", TEXTURE_GROUP_CLIENT_EFFECTS);
	pMatRenderContext->Bind(pMat);

	float effectScale = flPercent == 1.0f ? 5.0f : 2.0f;

	float JUMPGUN_BARRELCOLOR_R = 0.4f;
	float JUMPGUN_BARRELCOLOR_G = 0.7f;
	float JUMPGUN_BARRELCOLOR_B = 0.2f;
	DrawBeamQuadratic(vecStart, vecControl, vecEnd, effectScale * flPercent, Vector(JUMPGUN_BARRELCOLOR_R, JUMPGUN_BARRELCOLOR_G, JUMPGUN_BARRELCOLOR_B), flScrollOffset);

	float colour[3] = { JUMPGUN_BARRELCOLOR_R, JUMPGUN_BARRELCOLOR_G, JUMPGUN_BARRELCOLOR_B };

	pMat = materials->FindMaterial("effects/stunstick", TEXTURE_GROUP_CLIENT_EFFECTS);
	pMatRenderContext->Bind(pMat);
	
	DrawHalo(pMat, vecMuzzle, 0.58f * effectScale, colour);
}
#endif