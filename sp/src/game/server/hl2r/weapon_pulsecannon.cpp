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
#include "beam_shared.h"
#include "ammodef.h"

// memdbgon must be the last include file in a .cpp file!!!
//#include "tier0/memdbgon.h";

extern ConVar r_mirrored;

static const char *s_pUpdateBeamThinkContext = "UpdateBeamThinkContext";
#define COMBINE_CANNON_BEAM "effects/blueblacklargebeam.vmt"

class CWeaponPulseCannon : public CBaseHLCombatWeapon
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS( CWeaponPulseCannon, CBaseHLCombatWeapon );

	CWeaponPulseCannon();

	DECLARE_SERVERCLASS();
	
	void	Precache( void );
	void	PrimaryAttack( void );
	bool	HasPrimaryAmmo( void );

	void	AddViewKick( void );
	void	MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType );

	// Don't need these anymore.
	/*
	int		GetMinBurst() { return 4; }
	int		GetMaxBurst() { return 5; }
	float	GetMinRestTime( void ) { return 0.2f; }
	float	GetMaxRestTime( void ) { return 0.4f; }

	const WeaponProficiencyInfo_t *GetProficiencyValues();
	*/
	virtual const Vector& GetBulletSpread( void )
	{		
		// Handle NPCs first
		static Vector npcCone;
		npcCone = VECTOR_CONE_1DEGREES;
		
		if ( GetOwner() && GetOwner()->IsNPC() )
			return npcCone;
			
		static Vector cone;
		cone = VECTOR_CONE_PRECALCULATED;
		return cone;
	}

	float		GetFireRate( void ) { return 0.75f; }
	int			CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	
	void FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir );
	void Operator_ForceNPCFire( CBaseCombatCharacter  *pOperator, bool bSecondary );
	void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );

	DECLARE_ACTTABLE();
};

IMPLEMENT_SERVERCLASS_ST(CWeaponPulseCannon, DT_WeaponPulseCannon)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_pulsecannon, CWeaponPulseCannon );
PRECACHE_WEAPON_REGISTER(weapon_pulsecannon);

BEGIN_DATADESC( CWeaponPulseCannon )
END_DATADESC()

acttable_t	CWeaponPulseCannon::m_acttable[] = 
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

IMPLEMENT_ACTTABLE(CWeaponPulseCannon);

//=========================================================
CWeaponPulseCannon::CWeaponPulseCannon( )
{
	m_fMinRange1		= 0;
	m_fMaxRange1		= 1000;
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPulseCannon::Precache( void )
{
	PrecacheParticleSystem( "Weapon_Combine_Ion_Cannon" );
	BaseClass::Precache();
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#define AMMO_DRAIN_AMOUNT 20.0f

void CWeaponPulseCannon::PrimaryAttack( void )
{
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

	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	FireBulletsInfo_t info;
	info.m_vecSrc	 = pPlayer->Weapon_ShootPosition( );
	info.m_vecDirShooting = pPlayer->GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );
	info.m_iShots = 1;
	info.m_flDistance = MAX_TRACE_LENGTH;
	info.m_iAmmoType = m_iPrimaryAmmoType[INDEX_BASE];
	info.m_iTracerFreq = 1;					// No tracers.
	info.m_vecSpread = GetBulletSpread();

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

	pPlayer->FireBullets( info );

	if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType[INDEX_CARRY]) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0); 
	}

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
void CWeaponPulseCannon::AddViewKick()
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if (!pPlayer)
		return;

	//Add our view kick in
	QAngle viewPunch = QAngle( -2, random->RandomFloat( -0.6, 0.6 ), 0 );
	pPlayer->ViewPunch( viewPunch );
	UTIL_ScreenShake( pPlayer->Weapon_ShootPosition(), 10, 60, 0.3, 120.0f, SHAKE_START, false );
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponPulseCannon::HasPrimaryAmmo( void )
{
	if ( GetOwner() && !GetOwner()->IsPlayer() )
		return true;

	return BaseClass::HasPrimaryAmmo();
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPulseCannon::MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType )
{
	CBaseEntity *pOwner = GetOwner();
	if ( pOwner == NULL )
	{
		BaseClass::MakeTracer( vecTracerSrc, tr, iTracerType );
		return;
	}

	Vector vNewSrc = vecTracerSrc;
	if (pOwner->IsPlayer() && r_mirrored.GetBool() ) 
	{
		Vector ovForward, ovRight, ovUp;
		pOwner->GetVectors(&ovForward, &ovRight, &ovUp);
		vNewSrc -= ovRight * 11;
	}

	Vector vAttachment;
	Vector vForward, vRight;

	GetAttachment(1, vAttachment, &vForward, &vRight);
	Vector offset = (vForward * -3) + (vRight * -3);

	// Send the railgun effect
	DispatchParticleEffect( "Weapon_Combine_Ion_Cannon", vNewSrc + offset, tr.endpos, vec3_angle, NULL );

//	trace_t *pNewTrace = const_cast<trace_t*>( &tr );
//	UTIL_DecalTrace( pNewTrace, "RedGlowFade" );
//	UTIL_ImpactTrace( pNewTrace, GetAmmoDef()->Index("GaussEnergy"), "ImpactJeep" );
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPulseCannon::FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir )
{
	// FIXME: use the returned number of bullets to account for >10hz firerate
	WeaponSound(SINGLE_NPC);

	CSoundEnt::InsertSound( SOUND_COMBAT|SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy() );
	pOperator->FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED,
		MAX_TRACE_LENGTH, m_iPrimaryAmmoType[INDEX_BASE], 1, entindex(), 0 );

	pOperator->DoMuzzleFlash();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPulseCannon::Operator_ForceNPCFire( CBaseCombatCharacter *pOperator, bool bSecondary )
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
void CWeaponPulseCannon::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
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