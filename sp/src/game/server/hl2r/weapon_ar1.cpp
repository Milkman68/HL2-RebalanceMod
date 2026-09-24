#include "cbase.h"
#include "basehlcombatweapon.h"
#include "npcevent.h"
#include "basecombatcharacter.h"
#include "ai_basenpc.h"
#include "player.h"
#include "game.h"
#include "in_buttons.h"
#include "ai_memory.h"
#include "soundent.h"
#include "rumble_shared.h"
#include "gamestats.h"

// memdbgon must be the last include file in a .cpp file!!!
//#include "tier0/memdbgon.h";

extern ConVar hl2r_new_screenshake_effects;

class CWeaponAR1 : public CHLSelectFireMachineGun
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS( CWeaponAR1, CHLSelectFireMachineGun );

	CWeaponAR1();

	DECLARE_SERVERCLASS();
	
	void	BurstThink( void );
	void	AddViewKick( void );
	void	SecondaryAttack( void );

	float	GetFireRate( void ) { return 0.07f; }
	float 	GetBurstCycleRate(void) { return 0.0f; }
	int		GetBurstSize(void) { return 2; }

	int		CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	const char* GetTracerType(void) { return "AR2Tracer"; }


	virtual const Vector& GetBulletSpread( void )
	{
		static const Vector cone = VECTOR_CONE_3DEGREES;
		return cone;
	}
	
	void FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir );
	void Operator_ForceNPCFire( CBaseCombatCharacter  *pOperator, bool bSecondary );
	void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );

	DECLARE_ACTTABLE();

private:
	int	m_iShotCount;
};

IMPLEMENT_SERVERCLASS_ST(CWeaponAR1, DT_WeaponAR1)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_ar1, CWeaponAR1 );
PRECACHE_WEAPON_REGISTER(weapon_ar1);

BEGIN_DATADESC( CWeaponAR1 )
END_DATADESC()

acttable_t	CWeaponAR1::m_acttable[] = 
{
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_SMG1,			true },
	{ ACT_RELOAD,					ACT_RELOAD_SMG1,				true },
	{ ACT_IDLE,						ACT_IDLE_SMG1,					true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_SMG1,			true },

	{ ACT_WALK,						ACT_WALK_RIFLE,					true },
	{ ACT_WALK_AIM,					ACT_WALK_AIM_RIFLE,				true  },
	
// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED,				ACT_IDLE_SMG1_RELAXED,			false },//never aims
	{ ACT_IDLE_STIMULATED,			ACT_IDLE_SMG1_STIMULATED,		false },
	{ ACT_IDLE_AGITATED,			ACT_IDLE_ANGRY_SMG1,			false },//always aims

	{ ACT_WALK_RELAXED,				ACT_WALK_RIFLE_RELAXED,			false },//never aims
	{ ACT_WALK_STIMULATED,			ACT_WALK_RIFLE_STIMULATED,		false },
	{ ACT_WALK_AGITATED,			ACT_WALK_AIM_RIFLE,				false },//always aims

	{ ACT_RUN_RELAXED,				ACT_RUN_RIFLE_RELAXED,			false },//never aims
	{ ACT_RUN_STIMULATED,			ACT_RUN_RIFLE_STIMULATED,		false },
	{ ACT_RUN_AGITATED,				ACT_RUN_AIM_RIFLE,				false },//always aims

// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_SMG1_RELAXED,			false },//never aims	
	{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_AIM_RIFLE_STIMULATED,	false },
	{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_SMG1,			false },//always aims

	{ ACT_WALK_AIM_RELAXED,			ACT_WALK_RIFLE_RELAXED,			false },//never aims
	{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_RIFLE_STIMULATED,	false },
	{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_RIFLE,				false },//always aims

	{ ACT_RUN_AIM_RELAXED,			ACT_RUN_RIFLE_RELAXED,			false },//never aims
	{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_RIFLE_STIMULATED,	false },
	{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_RIFLE,				false },//always aims
//End readiness activities

	{ ACT_WALK_AIM,					ACT_WALK_AIM_RIFLE,				true },
	{ ACT_WALK_CROUCH,				ACT_WALK_CROUCH_RIFLE,			true },
	{ ACT_WALK_CROUCH_AIM,			ACT_WALK_CROUCH_AIM_RIFLE,		true },
	{ ACT_RUN,						ACT_RUN_RIFLE,					true },
	{ ACT_RUN_AIM,					ACT_RUN_AIM_RIFLE,				true },
	{ ACT_RUN_CROUCH,				ACT_RUN_CROUCH_RIFLE,			true },
	{ ACT_RUN_CROUCH_AIM,			ACT_RUN_CROUCH_AIM_RIFLE,		true },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_SMG1,	true },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_SMG1_LOW,		true },
	{ ACT_COVER_LOW,				ACT_COVER_SMG1_LOW,				false },
	{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_SMG1_LOW,			false },
	{ ACT_RELOAD_LOW,				ACT_RELOAD_SMG1_LOW,			false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_SMG1,		true },
};

IMPLEMENT_ACTTABLE(CWeaponAR1);

//=========================================================
CWeaponAR1::CWeaponAR1( )
{
	m_fMinRange1		= 0;
	m_fMaxRange1		= 1000;
	m_iFireMode			= FIREMODE_3RNDBURST;

	m_bAltFiresUnderwater = true;
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponAR1::FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir )
{
	// FIXME: use the returned number of bullets to account for >10hz firerate
	WeaponSoundRealtime( SINGLE_NPC );

	CSoundEnt::InsertSound( SOUND_COMBAT|SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy() );
	pOperator->FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED,
		MAX_TRACE_LENGTH, m_iPrimaryAmmoType[INDEX_BASE], 1, entindex(), 0 );

	pOperator->DoMuzzleFlash();
	m_iShotCount++;

	if (m_iShotCount % 2)
		m_iClip1 -= 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponAR1::Operator_ForceNPCFire( CBaseCombatCharacter *pOperator, bool bSecondary )
{
	// Ensure we have enough rounds in the clip
	m_iClip1++;

	Vector vecShootOrigin, vecShootDir;
	QAngle	angShootDir;
	GetAttachment( LookupAttachment( "muzzle" ), vecShootOrigin, angShootDir );
	AngleVectors( angShootDir, &vecShootDir );
	FireNPCPrimaryAttack( pOperator, vecShootOrigin, vecShootDir );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponAR1::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
	case EVENT_WEAPON_PISTOL_FIRE:
		{
			Vector vecShootOrigin, vecShootDir;
			QAngle angDiscard;

			// Support old style attachment point firing
			if ((pEvent->options == NULL) || (pEvent->options[0] == '\0') || (!pOperator->GetAttachment(pEvent->options, vecShootOrigin, angDiscard)))
			{
				vecShootOrigin = pOperator->Weapon_ShootPosition();
			}

			CAI_BaseNPC *npc = pOperator->MyNPCPointer();
			ASSERT( npc != NULL );
			vecShootDir = npc->GetActualShootTrajectory( vecShootOrigin );

			FireNPCPrimaryAttack( pOperator, vecShootOrigin, vecShootDir );
		}
		break;
	};
}
extern ConVar hl2r_bullet_tracer_freq;
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CWeaponAR1::BurstThink( void )
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if (!pPlayer)
		return;
	
	// Abort here to handle burst and auto fire modes
	if ( (UsesClipsForAmmo1() && m_iClip1 == 0) || ( !UsesClipsForAmmo1() && !pPlayer->GetAmmoCount(m_iPrimaryAmmoType[INDEX_CARRY]) ) )
	{
		m_iBurstSize = 0;
		return;
	}

	m_nShotsFired++;

	pPlayer->DoMuzzleFlash();

	// To make the firing framerate independent, we may have to fire more than one bullet here on low-framerate systems, 
	// especially if the weapon we're firing has a really fast rate of fire.
	int iBulletsToFire = 0;
	float fireRate = GetFireRate();
	float nextshottime = gpGlobals->curtime;

	// MUST call sound before removing a round from the clip of a CHLMachineGun
	while ( nextshottime <= gpGlobals->curtime )
	{
		WeaponSound(SINGLE, nextshottime);
		nextshottime = nextshottime + fireRate;

		iBulletsToFire++;
		m_iShotCount++;

		if ( m_iShotCount % 2 )	
			m_iClip1--;

		if ( m_iClip1 <= 0 )
			break;
	}

	m_flNextPrimaryAttack = m_flNextPrimaryAttack + fireRate;

	m_iPrimaryAttacks++;
	gamestats->Event_WeaponFired( pPlayer, true, GetClassname() );
	
	Vector vecSpread = pPlayer->GetAttackSpread( this );

	// Fire the bullets
	FireBulletsInfo_t info;
	info.m_iShots = iBulletsToFire;
	info.m_vecSrc = pPlayer->Weapon_ShootPosition( );
	info.m_vecDirShooting = pPlayer->GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );
	info.m_vecSpread = RandomVector( -vecSpread.x, vecSpread.x );;
	info.m_flDistance = MAX_TRACE_LENGTH;
	info.m_iAmmoType = m_iPrimaryAmmoType[INDEX_BASE];
	info.m_iTracerFreq = hl2r_bullet_tracer_freq.GetInt();
	FireBullets( info );

	//Factor in the view kick
	AddViewKick();

	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2, pPlayer );
	
	if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType[INDEX_CARRY]) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0); 
	}

	SendWeaponAnim( GetPrimaryAttackActivity() );
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	// Register a muzzleflash for the AI
	pPlayer->SetMuzzleFlashTime( gpGlobals->curtime + 0.5 );

	m_iBurstSize--;

	if( m_iBurstSize == 0 )
	{
		// The burst is over!
		SetThink(NULL);

		// idle immediately to stop the firing animation
		SetWeaponIdleTime( gpGlobals->curtime );
		return;
	}

	SetNextThink( gpGlobals->curtime + GetFireRate() );
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponAR1::AddViewKick( void )
{
	#define	MAX_VERTICAL_KICK	15.0f	//Degrees
	#define	SLIDE_LIMIT			15.0f	//Seconds
	
	//Get the view kick
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if ( pPlayer == NULL )
		return;

	DoMachineGunKick( pPlayer, MAX_VERTICAL_KICK, m_fFireDuration, SLIDE_LIMIT );

	float flShakeScale = RemapValClamped( m_fFireDuration, 0.0f, 15.0f, 3.0f, 15.0f );

	if ( hl2r_new_screenshake_effects.GetBool() )
		UTIL_ScreenShake( pPlayer->GetAbsOrigin(), flShakeScale*0.75, 150.0, 0.25, 128, SHAKE_START );
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponAR1::SecondaryAttack( void )
{

}