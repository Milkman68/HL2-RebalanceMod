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
#include "effect_dispatch_data.h"
#include "te_effect_dispatch.h"
#include "rumble_shared.h"
#include "particle_parse.h"
#include "soundenvelope.h"
#include "ammodef.h"

// memdbgon must be the last include file in a .cpp file!!!
//#include "tier0/memdbgon.h";

//#define CHARGE_TIME 2.0f
#define AMMO_DRAIN_AMOUNT 3.0f
#define AMMO_DRAIN_AMOUNT_ALT 6.0f

extern ConVar hl2r_bullet_tracer_freq;

class CWeaponCShotgun : public CBaseHLCombatWeapon
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS( CWeaponCShotgun, CBaseHLCombatWeapon );

	CWeaponCShotgun();

	DECLARE_SERVERCLASS();
	
	void	Precache( void );

	void	PrimaryAttack( void );
	void	SecondaryAttack( void );

/*	void	ItemPostFrame( void );
	void	ItemPreFrame( void );
	void	ItemBusyFrame( void );*/

	void	AddViewKick( void );
	virtual const Vector& GetBulletSpread( void )
	{		
		// Handle NPCs first
		static Vector cone;
		cone = VECTOR_CONE_6DEGREES;
		return cone;
	}

	float		GetFireRate( void ) { return 0.5f; }
	int			CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }

	const char *GetTracerType( void ) { return "AR2Tracer"; }
	
	void FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir );
	void Operator_ForceNPCFire( CBaseCombatCharacter  *pOperator, bool bSecondary );
	void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );

	DECLARE_ACTTABLE();

	
private:/*
	void 	UpdateCharge( void );
	void 	UpdateLoopingSounds( void );
	CSoundPatch *GetChargeSound( void );

private:
	enum eChargeState
	{
		CS_NO_CHARGE = 0,
		CS_CHARGING_UP,
		CS_CHARGING_DOWN,
		CS_DONE_CHARGING,
	};
	eChargeState		m_ChargeState;

	float				m_flChargeTime;
	CSoundPatch			*m_sndCharge;		// Charging sounds*/

	bool m_bAltFiring;
};

IMPLEMENT_SERVERCLASS_ST(CWeaponCShotgun, DT_WeaponCShotgun)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_cshotgun, CWeaponCShotgun );
PRECACHE_WEAPON_REGISTER(weapon_cshotgun);

BEGIN_DATADESC( CWeaponCShotgun )

	DEFINE_FIELD(m_bAltFiring, FIELD_BOOLEAN),
//	DEFINE_FIELD(m_ChargeState, FIELD_INTEGER),
//	DEFINE_FIELD(m_flChargeTime, FIELD_TIME),
//	DEFINE_SOUNDPATCH(m_sndCharge),

END_DATADESC()

acttable_t	CWeaponCShotgun::m_acttable[] = 
{
	{ ACT_IDLE,						ACT_IDLE_SMG1,					true },	// FIXME: hook to shotgun unique

	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_SHOTGUN,			true },
	{ ACT_RELOAD,					ACT_RELOAD_SHOTGUN,					false },
	{ ACT_WALK,						ACT_WALK_RIFLE,						true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_SHOTGUN,				true },

// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED,				ACT_IDLE_SHOTGUN_RELAXED,		false },//never aims
	{ ACT_IDLE_STIMULATED,			ACT_IDLE_SHOTGUN_STIMULATED,	false },
	{ ACT_IDLE_AGITATED,			ACT_IDLE_SHOTGUN_AGITATED,		false },//always aims

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

	{ ACT_WALK_AIM,					ACT_WALK_AIM_SHOTGUN,				true },
	{ ACT_WALK_CROUCH,				ACT_WALK_CROUCH_RIFLE,				true },
	{ ACT_WALK_CROUCH_AIM,			ACT_WALK_CROUCH_AIM_RIFLE,			true },
	{ ACT_RUN,						ACT_RUN_RIFLE,						true },
	{ ACT_RUN_AIM,					ACT_RUN_AIM_SHOTGUN,				true },
	{ ACT_RUN_CROUCH,				ACT_RUN_CROUCH_RIFLE,				true },
	{ ACT_RUN_CROUCH_AIM,			ACT_RUN_CROUCH_AIM_RIFLE,			true },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_SHOTGUN,	true },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_SHOTGUN_LOW,		true },
	{ ACT_RELOAD_LOW,				ACT_RELOAD_SHOTGUN_LOW,				false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_SHOTGUN,			false },
};


IMPLEMENT_ACTTABLE(CWeaponCShotgun);

//=========================================================
CWeaponCShotgun::CWeaponCShotgun( )
{
	m_fMinRange1		= 0;
	m_fMaxRange1		= 1000;
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCShotgun::Precache( void )
{
	PrecacheScriptSound("Weapon_CShotgun.ChargeLoop");
	BaseClass::Precache();
}
/*
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCShotgun::ItemPreFrame( void )
{
	UpdateCharge();
	BaseClass::ItemPreFrame();
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCShotgun::ItemBusyFrame( void )
{
	UpdateCharge();
	BaseClass::ItemBusyFrame();
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCShotgun::ItemPostFrame( void )
{
	UpdateCharge();
	BaseClass::ItemPostFrame();
}
*/
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCShotgun::PrimaryAttack( void )
{
	m_bAltFiring = false;

//	if ( m_ChargeState != CS_DONE_CHARGING )
//		return;

	// If my clip is empty (and I use clips) start reload
	if ( UsesClipsForAmmo1() && !m_iClip1 ) 
	{
		Reload();
		return;
	}

	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if (!pPlayer)
		return;

	pPlayer->DoMuzzleFlash();

	SendWeaponAnim( GetPrimaryAttackActivity() );

	WeaponSound(SINGLE);

	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
	m_flNextSecondaryAttack = gpGlobals->curtime + GetFireRate();

	int iAmmoDrain = 0;
	if ( UsesClipsForAmmo1() )
	{
		iAmmoDrain = MIN( AMMO_DRAIN_AMOUNT, m_iClip1 );
		m_iClip1 -= iAmmoDrain;
	}
	else
	{
		iAmmoDrain = MIN( AMMO_DRAIN_AMOUNT, pPlayer->GetAmmoCount( m_iPrimaryAmmoType[INDEX_CARRY] ) );
		pPlayer->RemoveAmmo( iAmmoDrain, m_iPrimaryAmmoType[INDEX_CARRY] );
	}

	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	FireBulletsInfo_t info;
	info.m_vecSrc	 = pPlayer->Weapon_ShootPosition( );
	info.m_vecDirShooting = pPlayer->GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );
	info.m_iShots = iAmmoDrain;
	info.m_flDistance = MAX_TRACE_LENGTH;
	info.m_iAmmoType = m_iPrimaryAmmoType[INDEX_BASE];
	info.m_iTracerFreq = hl2r_bullet_tracer_freq.GetInt();
	info.m_vecSpread = GetBulletSpread();

	pPlayer->FireBullets( info );

	AddViewKick();
	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), SOUNDENT_VOLUME_SHOTGUN, 0.2, GetOwner() );

	if (pPlayer->GetAmmoCount(m_iPrimaryAmmoType[INDEX_CARRY]) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0); 
	}

	m_iPrimaryAttacks++;
	gamestats->Event_WeaponFired( pPlayer, true, GetClassname() );
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCShotgun::SecondaryAttack( void )
{
	m_bAltFiring = true;

	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if (!pPlayer)
		return;

	bool bPrimaryAttackInstead = false;
	if ( UsesClipsForAmmo1() )
	{
		if ( m_iClip1 <= AMMO_DRAIN_AMOUNT )
			bPrimaryAttackInstead = true;
	}
	else
	{
		if ( pPlayer->GetAmmoCount( m_iPrimaryAmmoType[INDEX_CARRY] ) <= AMMO_DRAIN_AMOUNT )
			bPrimaryAttackInstead = true;
	}

	if ( bPrimaryAttackInstead )
	{
		PrimaryAttack();
		return;
	}

	// If my clip is empty (and I use clips) start reload
	if ( UsesClipsForAmmo1() && !m_iClip1 ) 
	{
		Reload();
		return;
	}

	pPlayer->DoMuzzleFlash();

	SendWeaponAnim( GetPrimaryAttackActivity() );

	WeaponSound(SINGLE);

	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate() * 2.25f;
	m_flNextSecondaryAttack = gpGlobals->curtime + GetFireRate() * 2.25f;

	int iAmmoDrain = 0;
	if ( UsesClipsForAmmo1() )
	{
		iAmmoDrain = MIN( AMMO_DRAIN_AMOUNT_ALT, m_iClip1 );
		m_iClip1 -= iAmmoDrain;
	}
	else
	{
		iAmmoDrain = MIN( AMMO_DRAIN_AMOUNT_ALT, pPlayer->GetAmmoCount( m_iPrimaryAmmoType[INDEX_CARRY] ) );
		pPlayer->RemoveAmmo( iAmmoDrain, m_iPrimaryAmmoType[INDEX_CARRY] );
	}

	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	FireBulletsInfo_t info;
	info.m_vecSrc	 = pPlayer->Weapon_ShootPosition( );
	info.m_vecDirShooting = pPlayer->GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );
	info.m_iShots = iAmmoDrain;
	info.m_flDistance = MAX_TRACE_LENGTH;
	info.m_iAmmoType = m_iPrimaryAmmoType[INDEX_BASE];
	info.m_iTracerFreq = hl2r_bullet_tracer_freq.GetInt();
	info.m_vecSpread = GetBulletSpread();

	pPlayer->FireBullets( info );

	AddViewKick();
	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), SOUNDENT_VOLUME_SHOTGUN, 0.2, GetOwner() );

	if (pPlayer->GetAmmoCount(m_iPrimaryAmmoType[INDEX_CARRY]) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0); 
	}

	m_iPrimaryAttacks++;
	gamestats->Event_WeaponFired( pPlayer, true, GetClassname() );
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCShotgun::AddViewKick()
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if (!pPlayer)
		return;

	QAngle viewPunch = QAngle( -3, random->RandomFloat( -1.8, 1.8 ), 0 );

	if ( m_bAltFiring )
		viewPunch *= 2.0f;

	//Add our view kick in
	pPlayer->ViewPunch( viewPunch );
	UTIL_ScreenShake( pPlayer->Weapon_ShootPosition(), 10, 60, 0.3, 120.0f, SHAKE_START, false );
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCShotgun::FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir )
{
	// FIXME: use the returned number of bullets to account for >10hz firerate
	WeaponSound(SINGLE_NPC);

	CSoundEnt::InsertSound( SOUND_COMBAT|SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy() );
	pOperator->FireBullets( AMMO_DRAIN_AMOUNT, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED,
		MAX_TRACE_LENGTH, m_iPrimaryAmmoType[INDEX_BASE], 1, entindex(), 0 );

	pOperator->DoMuzzleFlash();

	m_iClip1 -= AMMO_DRAIN_AMOUNT;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCShotgun::Operator_ForceNPCFire( CBaseCombatCharacter *pOperator, bool bSecondary )
{
	Vector vecShootOrigin, vecShootDir;
	QAngle	angShootDir;
	GetAttachment( LookupAttachment( "muzzle" ), vecShootOrigin, angShootDir );
	AngleVectors( angShootDir, &vecShootDir );
	FireNPCPrimaryAttack( pOperator, vecShootOrigin, vecShootDir );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCShotgun::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
	case EVENT_WEAPON_AR2:
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
		default:
			BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
			break;
	};
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
/*
void CWeaponCShotgun::UpdateCharge( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
//	if ( !pPlayer )
		return;

	if ( ( m_ChargeState != CS_CHARGING_DOWN && pPlayer->m_nButtons & IN_ATTACK && !m_bInReload ) )
	{
		m_flChargeTime = MIN( m_flChargeTime + gpGlobals->frametime, CHARGE_TIME );
		m_ChargeState = CS_CHARGING_UP;
	}
	else if ( m_flChargeTime > 0.0f )
	{
		m_flChargeTime = MAX( m_flChargeTime - gpGlobals->frametime * 3, 0.0f );
		m_ChargeState = CS_CHARGING_DOWN;
	}

	UpdateLoopingSounds();

	if ( m_flChargeTime == CHARGE_TIME )
		m_ChargeState = CS_DONE_CHARGING;

	if ( m_flChargeTime == 0.0f )
		m_ChargeState = CS_NO_CHARGE;
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCShotgun::UpdateLoopingSounds( void )
{
	if ( !GetChargeSound() )
		return;
	
	// Play the charging sounds!
	if ( m_flChargeTime > 0.0f )
	{
		// Makes the sound pitch reflect how damage is calculated.
		float ChargePerc = RemapValClamped( m_flChargeTime, 0.0f, CHARGE_TIME, 0.0f, 1.0f );
		ChargePerc = MAX(ChargePerc, 0.6f);
		
		(CSoundEnvelopeController::GetController()).SoundChangePitch( GetChargeSound(), 250 * ChargePerc, 0.1f );
		(CSoundEnvelopeController::GetController()).SoundChangeVolume( GetChargeSound(), 1.5, 0.1f );
		return;
	}
	
	// Fade it out if desired
	(CSoundEnvelopeController::GetController()).SoundFadeOut( GetChargeSound(), 0.1f );
	m_sndCharge = NULL;
}
//-----------------------------------------------------------------------------
// Purpose: 
// Output : CSoundPatch
//-----------------------------------------------------------------------------
CSoundPatch *CWeaponCShotgun::GetChargeSound( void )
{
	if ( m_sndCharge == NULL )
	{
		CPASAttenuationFilter filter( this );
		m_sndCharge = (CSoundEnvelopeController::GetController()).SoundCreate( filter, entindex(), CHAN_STATIC, "Weapon_Crossbow.ChargeLoop", ATTN_NORM );
		(CSoundEnvelopeController::GetController()).Play( GetChargeSound(), 0.0f, 50 );
	}

	return m_sndCharge;
}
*/