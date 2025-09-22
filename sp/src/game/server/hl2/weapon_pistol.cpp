//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Pistol - hand gun
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "basehlcombatweapon.h"
#include "basecombatcharacter.h"
#include "ai_basenpc.h"
#include "player.h"
#include "gamerules.h"
#include "in_buttons.h"
#include "soundent.h"
#include "game.h"
#include "vstdlib/random.h"
#include "gamestats.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	PISTOL_FASTEST_REFIRE_TIME		0.1f
#define	PISTOL_FASTEST_DRY_REFIRE_TIME	0.22f

#define	PISTOL_ACCURACY_SHOT_PENALTY_PRIMARY	0.3f
#define	PISTOL_ACCURACY_SHOT_PENALTY_SECONDARY	0.5f

#define	PISTOL_ACCURACY_DECAY_TIME_PRIMARY		0.15f
#define	PISTOL_ACCURACY_DECAY_TIME_SECONDARY	0.1f

#define	PISTOL_ACCURACY_MAXIMUM_PENALTY_TIME	10.0f

//ConVar	pistol_use_new_accuracy( "pistol_use_new_accuracy", "1" );
ConVar sk_pistol_can_secondary_attack("sk_pistol_can_secondary_attack", "1" );
extern ConVar hl2r_bullet_tracer_freq;

//-----------------------------------------------------------------------------
// CWeaponPistol
//-----------------------------------------------------------------------------
class CWeaponPistol : public CBaseHLCombatWeapon
{
	DECLARE_DATADESC();

public:
	DECLARE_CLASS( CWeaponPistol, CBaseHLCombatWeapon );

	CWeaponPistol(void);

	DECLARE_SERVERCLASS();

	void	Precache( void );
	void	ItemPostFrame( void );
	void	ItemPreFrame( void );
	void	ItemBusyFrame( void );
	void	PrimaryAttack( void );
	void	SecondaryAttack( void );
	bool	Reload();
	void	AddViewKick( void );
	void	DryFire( void );
	void	Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );

	void	UpdatePenaltyTime( void );

	int		CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	Activity	GetPrimaryAttackActivity( void );

	virtual const Vector& GetBulletSpread( void )
	{		
		// Handle NPCs first
		static Vector npcCone;
		npcCone = VECTOR_CONE_3DEGREES;
		
		if ( GetOwner() && GetOwner()->IsNPC() )
			return npcCone;
			
		static Vector cone;
		cone = VECTOR_CONE_05DEGREES;
		return cone;
	}
	
	virtual int	GetMinBurst(){ return 2;}

	virtual int	GetMaxBurst(){ return 4;}
	
	virtual float	GetMinRestTime(){ return 0.6; };
	virtual float	GetMaxRestTime(){ return 1.0; };

	virtual float GetFireRate( void ) 
	{
		if( GetOwner() && GetOwner()->IsNPC() && !FClassnameIs( GetOwner(), "npc_combine_s" ) )
		{
			return 0.4;
		}
		return 0.17f; 
	}

	DECLARE_ACTTABLE();

private:
	float	m_flSoonestPrimaryAttack;
	float	m_flLastAttackTime;

	float	m_flAccuracyPenalty;
	float	m_flAccuracyPenaltyDecayTime;

//	int		m_nNumShotsFired;
	int		m_nFireMode;
};


IMPLEMENT_SERVERCLASS_ST(CWeaponPistol, DT_WeaponPistol)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_pistol, CWeaponPistol );
PRECACHE_WEAPON_REGISTER( weapon_pistol );

BEGIN_DATADESC( CWeaponPistol )

	DEFINE_FIELD( m_flSoonestPrimaryAttack, FIELD_TIME ),
	DEFINE_FIELD( m_flLastAttackTime,		FIELD_TIME ),
	DEFINE_FIELD( m_flAccuracyPenalty,		FIELD_FLOAT ),
	DEFINE_FIELD( m_flAccuracyPenaltyDecayTime,		FIELD_FLOAT ),
//	DEFINE_FIELD( m_nNumShotsFired,			FIELD_INTEGER ),
	DEFINE_FIELD( m_nFireMode,			FIELD_INTEGER ),

END_DATADESC()

acttable_t	CWeaponPistol::m_acttable[] = 
{
	{ ACT_IDLE,						ACT_IDLE_PISTOL,				true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_PISTOL,			true },
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_PISTOL,		true },
	{ ACT_RELOAD,					ACT_RELOAD_PISTOL,				true },
	{ ACT_WALK_AIM,					ACT_WALK_AIM_PISTOL,			true },
	{ ACT_RUN_AIM,					ACT_RUN_AIM_PISTOL,				true },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_PISTOL,true },
	{ ACT_RELOAD_LOW,				ACT_RELOAD_PISTOL_LOW,			false },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_PISTOL_LOW,	false },
	{ ACT_COVER_LOW,				ACT_COVER_PISTOL_LOW,			false },
	{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_PISTOL_LOW,		false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_PISTOL,		false },
	{ ACT_WALK,						ACT_WALK_PISTOL,				false },
	{ ACT_RUN,						ACT_RUN_PISTOL,					false },
};


IMPLEMENT_ACTTABLE( CWeaponPistol );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponPistol::CWeaponPistol( void )
{
	m_flSoonestPrimaryAttack = gpGlobals->curtime;
	m_flAccuracyPenalty = 0.0f;

	m_fMinRange1		= 0;
	m_fMaxRange1		= 1000;

	m_bFiresUnderwater	= true;
	m_bAltFiresUnderwater = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPistol::Precache( void )
{
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeaponPistol::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
		case EVENT_WEAPON_PISTOL_FIRE:
		{
			Vector vecShootOrigin, vecShootDir;
			vecShootOrigin = pOperator->Weapon_ShootPosition();

			CAI_BaseNPC *npc = pOperator->MyNPCPointer();
			ASSERT( npc != NULL );

			vecShootDir = npc->GetActualShootTrajectory( vecShootOrigin );

			CSoundEnt::InsertSound( SOUND_COMBAT|SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_PISTOL, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy() );

			WeaponSound( SINGLE_NPC );
			pOperator->FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 1 );
			pOperator->DoMuzzleFlash();
			m_iClip1 = m_iClip1 - 1;
		}
		break;
		default:
			BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponPistol::DryFire( void )
{
	WeaponSound( EMPTY );
	SendWeaponAnim( ACT_VM_DRYFIRE );
	
	m_flSoonestPrimaryAttack	= gpGlobals->curtime + PISTOL_FASTEST_DRY_REFIRE_TIME;

	m_flNextPrimaryAttack		= gpGlobals->curtime + SequenceDuration();
	m_flNextSecondaryAttack		= gpGlobals->curtime + SequenceDuration();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponPistol::PrimaryAttack( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
	    return;
	
	if (( pOwner->m_nButtons & IN_ATTACK ) && ( pOwner->m_nButtons & IN_ATTACK2 ))
		return;

	m_flLastAttackTime = gpGlobals->curtime;
	m_flSoonestPrimaryAttack = gpGlobals->curtime + PISTOL_FASTEST_REFIRE_TIME;
	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), SOUNDENT_VOLUME_PISTOL, 0.1, GetOwner() );

	m_nFireMode = FIREMODE_SEMI;

	BaseClass::PrimaryAttack();
	m_flNextSecondaryAttack = gpGlobals->curtime + GetFireRate();

	// Add an accuracy penalty which can move past our maximum penalty time if we're really spastic
	m_flAccuracyPenalty += (PISTOL_ACCURACY_SHOT_PENALTY_PRIMARY * 0.75);
	m_flAccuracyPenaltyDecayTime = gpGlobals->curtime + PISTOL_ACCURACY_DECAY_TIME_PRIMARY;

	m_iPrimaryAttacks++;
	gamestats->Event_WeaponFired( pOwner, true, GetClassname() );
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPistol::SecondaryAttack( void )
{
	if ( !sk_pistol_can_secondary_attack.GetBool() )
		return;
	
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner == NULL )
	    return;
	
	if (( pOwner->m_nButtons & IN_ATTACK ) && ( pOwner->m_nButtons & IN_ATTACK2 ))
		return;

	m_flNextSecondaryAttack = gpGlobals->curtime + 0.1;
	m_flNextPrimaryAttack = gpGlobals->curtime + 0.1;

	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), SOUNDENT_VOLUME_PISTOL, 0.1, GetOwner() );

	m_nFireMode = FIREMODE_FULLAUTO;

	if ( !m_iClip1 ) 
		return;

	pOwner->DoMuzzleFlash();

	SendWeaponAnim( GetPrimaryAttackActivity() );

	// player "shoot" animation
	pOwner->SetAnimation( PLAYER_ATTACK1 );

	FireBulletsInfo_t info;
	info.m_vecSrc	 = pOwner->Weapon_ShootPosition( );
	info.m_vecDirShooting = pOwner->GetAutoaimVector( AUTOAIM_SCALE_DEFAULT ); 
	info.m_iShots	= 1;
	info.m_flDistance = MAX_TRACE_LENGTH;
	info.m_iAmmoType = m_iPrimaryAmmoType;
	info.m_iTracerFreq = hl2r_bullet_tracer_freq.GetInt();
	info.m_vecSpread = pOwner->GetAttackSpread( this );

	WeaponSound(SINGLE, 0);
	m_flNextPrimaryAttack = m_flNextPrimaryAttack + GetFireRate();

	m_iClip1 -= 1;

	// Fire the bullets
	pOwner->FireBullets( info );

	if (!m_iClip1 && pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pOwner->SetSuitUpdate("!HEV_AMO0", FALSE, 0); 
	}

	//Add our view kick in
	AddViewKick();

	// Add an accuracy penalty which can move past our maximum penalty time if we're really spastic
	m_flAccuracyPenalty += (PISTOL_ACCURACY_SHOT_PENALTY_PRIMARY * 0.75);
	m_flAccuracyPenaltyDecayTime = gpGlobals->curtime + PISTOL_ACCURACY_DECAY_TIME_PRIMARY;

	m_iPrimaryAttacks++;
	gamestats->Event_WeaponFired( pOwner, true, GetClassname() );
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponPistol::Reload()
{
	m_flAccuracyPenalty = 0.0f;
	return BaseClass::Reload();
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPistol::UpdatePenaltyTime( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;
	
	if ( pOwner->m_nButtons & IN_ATTACK )
		return;
	
	if ( pOwner->m_nButtons & IN_ATTACK2 )
		return;
	
	// Check our penalty time decay
	if ( m_flAccuracyPenaltyDecayTime < gpGlobals->curtime )
	{
		m_flAccuracyPenalty -= (gpGlobals->frametime * 20);
		m_flAccuracyPenalty = clamp( m_flAccuracyPenalty, 0.0f, PISTOL_ACCURACY_MAXIMUM_PENALTY_TIME );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPistol::ItemPreFrame( void )
{
	UpdatePenaltyTime();

	BaseClass::ItemPreFrame();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPistol::ItemBusyFrame( void )
{
	UpdatePenaltyTime();

	BaseClass::ItemBusyFrame();
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponPistol::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
	{
		BaseClass::ItemPostFrame();
		return;
	}

	if ( ( ( pOwner->m_nButtons & IN_ATTACK ) || ( pOwner->m_nButtons & IN_ATTACK2 ) ) 
	&& m_flNextPrimaryAttack < gpGlobals->curtime && m_iClip1 <= 0 && !m_bInReload )
	{
		if (!m_bFireOnEmpty)
			DryFire();

		HandleFireOnEmpty();
	}

	BaseClass::ItemPostFrame();

	if ( m_bInReload )
		return;
	
	//Allow a refire as fast as the player can click
	if ( ( ( pOwner->m_nButtons & IN_ATTACK ) == false ) && m_iClip1 > 0 && ( m_flSoonestPrimaryAttack < gpGlobals->curtime ) )
	{
		m_flNextPrimaryAttack = gpGlobals->curtime - 0.1f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
Activity CWeaponPistol::GetPrimaryAttackActivity( void )
{
/*	if ( m_nNumShotsFired < 1 )
		return ACT_VM_PRIMARYATTACK;

	if ( m_nNumShotsFired < 5 )
		return ACT_VM_RECOIL1;

	if ( m_nNumShotsFired < 10 )
		return ACT_VM_RECOIL2;
*/
	return ACT_VM_RECOIL3;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPistol::AddViewKick( void )
{
	CBasePlayer *pPlayer  = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	pPlayer->ViewPunchScale(0.75);

	QAngle	viewPunch;
	if ( m_nFireMode == FIREMODE_SEMI )
	{
		viewPunch.x = -0.37f;
		viewPunch.y = random->RandomFloat( -0.1f,  0.1f );
		viewPunch.z = 0.0f;
	
  		//Find how far into our accuracy degradation we are
		float limit = PISTOL_ACCURACY_MAXIMUM_PENALTY_TIME;
		float scale = 7.5;
	
		float duration = MIN( m_flAccuracyPenalty, limit );
		float kickPerc = duration / (limit / scale);
	
		viewPunch.x *= (1.0f + kickPerc);
		viewPunch.y *= (1.0f + ( kickPerc * 0.5 ));
	}
	else
	{
		viewPunch.x = -0.6f;
		viewPunch.y = random->RandomFloat( -0.1f,  0.1f );
		viewPunch.z = 0.0f;
	
  		//Find how far into our accuracy degradation we are
		float limit = PISTOL_ACCURACY_MAXIMUM_PENALTY_TIME;
		float scale = 10;
	
		float duration = MIN( m_flAccuracyPenalty, limit );
		float kickPerc = duration / (limit / scale);
	
		viewPunch.x *= (1.0f + kickPerc);
		viewPunch.y *= (1.0f + ( kickPerc * 2 ));
	}

	//Add it to the view punch
	pPlayer->ViewPunch( viewPunch );
}
