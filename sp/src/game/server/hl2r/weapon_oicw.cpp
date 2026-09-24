#include "cbase.h"
#include "basehlcombatweapon.h"
#include "npcevent.h"
#include "basecombatcharacter.h"
#include "ai_basenpc.h"
#include "player.h"
#include "game.h"
#include "in_buttons.h"
#include "grenade_ar2.h"
#include "ai_memory.h"
#include "soundent.h"
#include "rumble_shared.h"
#include "gamestats.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar   sk_plr_dmg_smg1_grenade;	
extern ConVar hl2r_new_screenshake_effects;
extern ConVar hl2r_new_viewpunch_effects;

ConVar sk_oicw_grenade_speed( "sk_oicw_grenade_speed", "1000.0" );
ConVar sk_oicw_grenade_timer( "sk_oicw_grenade_timer", "0.5" );

class CWeaponOICW : public CHLSelectFireMachineGun
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS( CWeaponOICW, CHLSelectFireMachineGun );

	CWeaponOICW();

	DECLARE_SERVERCLASS();
	
	void	Precache( void );
	void	AddViewKick( void );
	void	SecondaryAttack( void );

#ifndef USE_NEW_PROFICIENCY_SYSTEM
	int		GetMinBurst() { return 4; }
	int		GetMaxBurst() { return 5; }
	float	GetMinRestTime( void ) { return 0.2f; }
	float	GetMaxRestTime( void ) { return 0.4f; }
#endif
	float 	GetBurstCycleRate( void ){ return 0.2; }
	int		GetBurstSize( void ){ return 3.0; }
	float	GetFireRate( void ) { return 0.04f; }	// 13.3hz

	virtual void Equip( CBaseCombatCharacter *pOwner );
	bool	Reload( void );
	int		CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	int 	WeaponRangeAttack2Condition( float flDot, float flDist );
	Activity	GetPrimaryAttackActivity( void );

	virtual const Vector& GetBulletSpread( void )
	{
		static const Vector cone = VECTOR_CONE_2DEGREES;
		return cone;
	}
#ifndef USE_NEW_PROFICIENCY_SYSTEM
	const WeaponProficiencyInfo_t *GetProficiencyValues();
#endif

	void FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir );
	void Operator_ForceNPCFire( CBaseCombatCharacter  *pOperator, bool bSecondary );
	void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );

	DECLARE_ACTTABLE();

protected:

	Vector	m_vecTossVelocity;
	float	m_flNextGrenadeCheck;
};

IMPLEMENT_SERVERCLASS_ST(CWeaponOICW, DT_WeaponOICW)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_oicw, CWeaponOICW );
PRECACHE_WEAPON_REGISTER(weapon_oicw);

BEGIN_DATADESC( CWeaponOICW )

	DEFINE_FIELD( m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( m_flNextGrenadeCheck, FIELD_TIME ),

END_DATADESC()

acttable_t	CWeaponOICW::m_acttable[] = 
{
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_AR2,			true },
	{ ACT_RELOAD,					ACT_RELOAD_SMG1,				true },		// FIXME: hook to AR2 unique
	{ ACT_IDLE,						ACT_IDLE_SMG1,					true },		// FIXME: hook to AR2 unique
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_SMG1,			true },		// FIXME: hook to AR2 unique

	{ ACT_WALK,						ACT_WALK_RIFLE,					true },

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
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_AR2,	false },
	{ ACT_COVER_LOW,				ACT_COVER_SMG1_LOW,				false },		// FIXME: hook to AR2 unique
	{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_AR2_LOW,			false },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_SMG1_LOW,		true },		// FIXME: hook to AR2 unique
	{ ACT_RELOAD_LOW,				ACT_RELOAD_SMG1_LOW,			false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_SMG1,		true },
//	{ ACT_RANGE_ATTACK2, ACT_RANGE_ATTACK_AR2_GRENADE, true },
};

IMPLEMENT_ACTTABLE(CWeaponOICW);

//=========================================================
CWeaponOICW::CWeaponOICW( )
{
	m_fMinRange1		= 0;
	m_fMaxRange1		= 1000;
	
	m_iFireMode = FIREMODE_3RNDBURST;
	m_bAltFiresUnderwater = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponOICW::Precache( void )
{
	UTIL_PrecacheOther("grenade_ar2");
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Give this weapon longer range when wielded by an ally NPC.
//-----------------------------------------------------------------------------
void CWeaponOICW::Equip( CBaseCombatCharacter *pOwner )
{
	if( pOwner->Classify() == CLASS_PLAYER_ALLY )
	{
		m_fMaxRange1 = 3000;
	}
	else
	{
		m_fMaxRange1 = 1000;
	}

	BaseClass::Equip( pOwner );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponOICW::FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir )
{
	// FIXME: use the returned number of bullets to account for >10hz firerate
	WeaponSoundRealtime( SINGLE_NPC );

	CSoundEnt::InsertSound( SOUND_COMBAT|SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy() );
	pOperator->FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED,
		MAX_TRACE_LENGTH, m_iPrimaryAmmoType[INDEX_BASE], 1, entindex(), 0 );

	pOperator->DoMuzzleFlash();
	m_iClip1 = m_iClip1 - 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponOICW::Operator_ForceNPCFire( CBaseCombatCharacter *pOperator, bool bSecondary )
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
void CWeaponOICW::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
	case EVENT_WEAPON_SMG1:
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

	case EVENT_WEAPON_AR2_ALTFIRE:
	{
		CAI_BaseNPC *npc = pOperator->MyNPCPointer();

		Vector vecShootOrigin, vecShootDir;
		vecShootOrigin = pOperator->Weapon_ShootPosition();

		Vector vecThrow = m_vecTossVelocity;

		//If on the rare case the vector is 0 0 0, cancel for avoid launching the grenade without speed
		//This should be on WeaponRangeAttack2Condition(), but for some unknown reason return CASE_NONE
		//doesn't stop the launch
		if (vecThrow == Vector(0, 0, 0)){
			break;
		}

		CGrenadeAR2 *pGrenade = (CGrenadeAR2*)Create("grenade_ar2", vecShootOrigin, vec3_angle, npc);
		pGrenade->SetAbsVelocity( vecThrow );
		pGrenade->SetLocalAngularVelocity(RandomAngle(-400, 400)); //tumble in air
		pGrenade->SetMoveType(MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE);
		pGrenade->SetThrower(GetOwner());
		pGrenade->SetGravity(0.5); // lower gravity since grenade is aerodynamic and engine doesn't know it.
		pGrenade->SetDamage(sk_plr_dmg_smg1_grenade.GetFloat());
	}
	break;

	default:
		BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Activity
//-----------------------------------------------------------------------------
Activity CWeaponOICW::GetPrimaryAttackActivity( void )
{
	if ( m_nShotsFired < 2 )
		return ACT_VM_PRIMARYATTACK;

	if ( m_nShotsFired < 3 )
		return ACT_VM_RECOIL1;
	
	if ( m_nShotsFired < 4 )
		return ACT_VM_RECOIL2;

	return ACT_VM_RECOIL3;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CWeaponOICW::Reload( void )
{
	bool fRet;
	float fCacheTime = m_flNextSecondaryAttack;

	fRet = DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );
	if ( fRet )
	{
		// Undo whatever the reload process has done to our secondary
		// attack timer. We allow you to interrupt reloading to fire
		// a grenade.
		m_flNextSecondaryAttack = GetOwner()->m_flNextAttack = fCacheTime;

		WeaponSound( RELOAD );
	}

	return fRet;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponOICW::AddViewKick( void )
{
	#define	MAX_VERTICAL_KICK	0.75f	//Degrees
	#define	SLIDE_LIMIT			10.0f	//Seconds
	
	//Get the view kick
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if (!pPlayer)
		return;
	
	DoMachineGunKick( pPlayer, MAX_VERTICAL_KICK, m_fFireDuration + 1.0f, SLIDE_LIMIT );

	float flShakeScale = RemapValClamped( m_fFireDuration, 0.0f, 15.0f, 5.0f, 10.0f );

	if ( hl2r_new_screenshake_effects.GetBool() )
		UTIL_ScreenShake( pPlayer->GetAbsOrigin(), flShakeScale, 150.0, 0.25, 128, SHAKE_START );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponOICW::SecondaryAttack( void )
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	QAngle viewPunch = QAngle( -5, random->RandomFloat(-2.0, 2.0), 0 );

	if ( hl2r_new_viewpunch_effects.GetBool() )
		pPlayer->ViewPunch( viewPunch );

	//Must have ammo
	if ( ( pPlayer->GetAmmoCount( m_iSecondaryAmmoType ) <= 0 ) || ( pPlayer->GetWaterLevel() == 3 ) )
	{
		SendWeaponAnim( ACT_VM_DRYFIRE );
		BaseClass::WeaponSound( EMPTY );
		m_flNextSecondaryAttack = gpGlobals->curtime + 0.5f;
		return;
	}

	if( m_bInReload )
		m_bInReload = false;

	// MUST call sound before removing a round from the clip of a CMachineGun
	BaseClass::WeaponSound( WPN_DOUBLE );

	pPlayer->RumbleEffect( RUMBLE_357, 0, RUMBLE_FLAGS_NONE );

	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	Vector	vecThrow;
	// Don't autoaim on grenade tosses
	AngleVectors( pPlayer->EyeAngles() + pPlayer->GetPunchAngle(), &vecThrow );
	VectorScale( vecThrow, sk_oicw_grenade_speed.GetFloat(), vecThrow );
	
	//Create the grenade
	QAngle angles;
	VectorAngles( vecThrow, angles );
	CGrenadeAR2 *pGrenade = (CGrenadeAR2*)Create( "grenade_ar2", vecSrc, angles, pPlayer );
	pGrenade->SetAbsVelocity( vecThrow );
	pGrenade->m_flDetonateTime = sk_oicw_grenade_timer.GetFloat();	// Makes the grenade explode on a delay.

	pGrenade->SetLocalAngularVelocity( RandomAngle( -400, 400 ) );
	pGrenade->SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE ); 
	pGrenade->SetThrower( GetOwner() );
	pGrenade->SetDamage( sk_plr_dmg_smg1_grenade.GetFloat() );

	SendWeaponAnim( ACT_VM_SECONDARYATTACK );

	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), 1000, 0.2, GetOwner(), SOUNDENT_CHANNEL_WEAPON );

	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	// Decrease ammo
	pPlayer->RemoveAmmo( 1, m_iSecondaryAmmoType );

	// Can shoot again immediately
	m_flNextPrimaryAttack = gpGlobals->curtime + 0.5f;

	// Can blow up after a short delay (so have time to release mouse button)
	m_flNextSecondaryAttack = gpGlobals->curtime + 1.0f;

	// Register a muzzleflash for the AI.
	pPlayer->SetMuzzleFlashTime( gpGlobals->curtime + 0.5 );	

	m_iSecondaryAttacks++;
	gamestats->Event_WeaponFired( pPlayer, false, GetClassname() );
}

#define	COMBINE_MIN_GRENADE_CLEAR_DIST 512

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flDot - 
//			flDist - 
// Output : int
//-----------------------------------------------------------------------------
int CWeaponOICW::WeaponRangeAttack2Condition( float flDot, float flDist )
{
	CAI_BaseNPC *npcOwner = GetOwner()->MyNPCPointer();
	CBaseEntity *pEnemy = npcOwner->GetEnemy();

	if (!pEnemy)
		return COND_NONE;
	// --------------------------------------
	//  Get target vector
	// --------------------------------------
	// toss it to where you last saw them
	Vector vecTarget = npcOwner->GetEnemyLKP() + pEnemy->GetViewOffset();

	// ---------------------------------------------------------------------
	// Check that throw is legal and clear
	// ---------------------------------------------------------------------
	Vector vecToss = VecCheckThrow( this, npcOwner->GetLocalOrigin() + Vector(0,0,60), vecTarget, sk_oicw_grenade_speed.GetFloat(), 0.5 );
	if ( vecToss != vec3_origin )
	{
		m_vecTossVelocity = vecToss;
		return COND_CAN_RANGE_ATTACK2;
	}
	else
	{
		return COND_NONE;
	}
}

//-----------------------------------------------------------------------------
#ifndef USE_NEW_PROFICIENCY_SYSTEM
const WeaponProficiencyInfo_t *CWeaponOICW::GetProficiencyValues()
{
	static WeaponProficiencyInfo_t proficiencyTable[] =
	{
		{ 7.0,		0.75	},
		{ 5.00,		0.75	},
		{ 6.0/3.0,  0.75	},
		{ 5.0/3.0,	0.75	},
		{ 1.00,		1.0		},
	};

	COMPILE_TIME_ASSERT( ARRAYSIZE(proficiencyTable) == WEAPON_PROFICIENCY_PERFECT + 1);

	return proficiencyTable;
}
#endif