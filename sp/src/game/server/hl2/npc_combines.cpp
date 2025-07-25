//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This is the soldier version of the combine, analogous to the HL1 grunt.
//
//=============================================================================//

#include "cbase.h"
#include "ai_hull.h"
#include "ai_motor.h"
#include "npc_combines.h"
#include "bitstring.h"
#include "engine/IEngineSound.h"
#include "soundent.h"
#include "ndebugoverlay.h"
#include "npcevent.h"
#include "hl2/hl2_player.h"
#include "game.h"
#include "ammodef.h"
#include "explode.h"
#include "ai_memory.h"
#include "Sprite.h"
#include "soundenvelope.h"
#include "weapon_physcannon.h"
#include "hl2_gamerules.h"
#include "gameweaponmanager.h"
#include "vehicle_base.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar	sk_combine_s_health( "sk_combine_s_health","0");
ConVar	sk_combine_s_kick( "sk_combine_s_kick","0");

ConVar sk_combine_guard_health( "sk_combine_guard_health", "0");
ConVar sk_combine_guard_kick( "sk_combine_guard_kick", "0");
 
// Whether or not the combine guard should spawn health on death
ConVar combine_guard_spawn_health( "combine_guard_spawn_health", "1" );

extern ConVar sk_plr_dmg_buckshot;	
extern ConVar sk_plr_num_shotgun_pellets;

//Whether or not the combine should spawn health on death
ConVar	combine_spawn_health( "combine_spawn_health", "1" );

extern ConVar hl2r_random_weapons;
extern ConVar hl2r_enemy_promotion;

LINK_ENTITY_TO_CLASS( npc_combine_s, CNPC_CombineS );


#define AE_SOLDIER_BLOCK_PHYSICS		20 // trying to block an incoming physics object

extern Activity ACT_WALK_EASY;
extern Activity ACT_WALK_MARCH;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineS::Spawn( void )
{
/* 	if( FStrEq(STRING(gpGlobals->mapname), "d3_breen_01") )
	{
		SetModelName( MAKE_STRING( "models/combine_super_soldier.mdl" ) );
	} */
	
	const char *pModelName = STRING( GetModelName() );
	bAlreadyElite = false;
	
	// Don't do this if we're already an elite.
	if ( Q_stricmp( pModelName, "models/combine_super_soldier.mdl" ) )
	{
		if ( hl2r_enemy_promotion.GetBool() )
			DoPromotion();
	}
	else
	{
		bAlreadyElite = true;
	}
	
	Precache();
	SetModel( STRING( GetModelName() ) );
	
	HandleSpawnEquipment();

	if( IsElite() || FStrEq(STRING(m_spawnEquipment), "weapon_rpg") )
	{
		// Stronger, tougher.
		SetHealth( sk_combine_guard_health.GetFloat() );
		SetMaxHealth( sk_combine_guard_health.GetFloat() );
		SetKickDamage( sk_combine_guard_kick.GetFloat() );
	}
	else
	{
		SetHealth( sk_combine_s_health.GetFloat() );
		SetMaxHealth( sk_combine_s_health.GetFloat() );
		SetKickDamage( sk_combine_s_kick.GetFloat() );
	}

	CapabilitiesAdd( bits_CAP_ANIMATEDFACE );
	CapabilitiesAdd( bits_CAP_MOVE_SHOOT );
	CapabilitiesAdd( bits_CAP_DOORS_GROUP );

	BaseClass::Spawn();
//bookmark
#if HL2_EPISODIC
	if (m_iUseMarch && !HasSpawnFlags(SF_NPC_START_EFFICIENT))
	{
		Msg( "Soldier %s is set to use march anim, but is not an efficient AI. The blended march anim can only be used for dead-ahead walks!\n", GetDebugName() );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_CombineS::Precache()
{
	const char *pModelName = STRING( GetModelName() );

	if( !Q_stricmp( pModelName, "models/combine_super_soldier.mdl" ) )
	{
		m_fIsElite = true;
	}
	else
	{
		m_fIsElite = false;
	}
	
	if( !Q_stricmp( pModelName, "models/combine_soldier_prisonguard.mdl" ) )
	{
		m_fIsGuard = true;
	}
	else
	{
		m_fIsGuard = false;
	}

	if( !GetModelName() )
	{
		SetModelName( MAKE_STRING( "models/combine_soldier.mdl" ) );
	}

	PrecacheModel( STRING( GetModelName() ) );

	UTIL_PrecacheOther( "item_healthvial" );
	UTIL_PrecacheOther( "weapon_frag" );
	UTIL_PrecacheOther( "item_ammo_ar2_altfire" );
	UTIL_PrecacheOther( "item_ammo_smg1_grenade" );

	BaseClass::Precache();
}


void CNPC_CombineS::DeathSound( const CTakeDamageInfo &info )
{
	// NOTE: The response system deals with this at the moment
	if ( GetFlags() & FL_DISSOLVING )
		return;

	GetSentences()->Speak( "COMBINE_DIE", SENTENCE_PRIORITY_INVALID, SENTENCE_CRITERIA_ALWAYS ); 
}


//-----------------------------------------------------------------------------
// Purpose: Soldiers use CAN_RANGE_ATTACK2 to indicate whether they can throw
//			a grenade. Because they check only every half-second or so, this
//			condition must persist until it is updated again by the code
//			that determines whether a grenade can be thrown, so prevent the 
//			base class from clearing it out. (sjb)
//-----------------------------------------------------------------------------
void CNPC_CombineS::ClearAttackConditions( )
{
	bool fCanRangeAttack2 = HasCondition( COND_CAN_RANGE_ATTACK2 );

	// Call the base class.
	BaseClass::ClearAttackConditions();

	if( fCanRangeAttack2 )
	{
		// We don't allow the base class to clear this condition because we
		// don't sense for it every frame.
		SetCondition( COND_CAN_RANGE_ATTACK2 );
	}
}

void CNPC_CombineS::PrescheduleThink( void )
{
	/*//FIXME: This doesn't need to be in here, it's all debug info
	if( HasCondition( COND_HEAR_PHYSICS_DANGER ) )
	{
		// Don't react unless we see the item!!
		CSound *pSound = NULL;

		pSound = GetLoudestSoundOfType( SOUND_PHYSICS_DANGER );

		if( pSound )
		{
			if( FInViewCone( pSound->GetSoundReactOrigin() ) )
			{
				DevMsg( "OH CRAP!\n" );
				NDebugOverlay::Line( EyePosition(), pSound->GetSoundReactOrigin(), 0, 0, 255, false, 2.0f );
			}
		}
	}
	*/

	BaseClass::PrescheduleThink();
}

//-----------------------------------------------------------------------------
// Purpose: Allows for modification of the interrupt mask for the current schedule.
//			In the most cases the base implementation should be called first.
//-----------------------------------------------------------------------------
void CNPC_CombineS::BuildScheduleTestBits( void )
{
	//Interrupt any schedule with physics danger (as long as I'm not moving or already trying to block)
	if ( m_flGroundSpeed == 0.0 && !IsCurSchedule( SCHED_FLINCH_PHYSICS ) )
	{
		SetCustomInterruptCondition( COND_HEAR_PHYSICS_DANGER );
	}

	BaseClass::BuildScheduleTestBits();
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_CombineS::SelectSchedule ( void )
{
	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
float CNPC_CombineS::GetHitgroupDamageMultiplier( int iHitGroup, const CTakeDamageInfo &info )
{
	switch( iHitGroup )
	{
	case HITGROUP_HEAD:
		{
			// Soldiers take double headshot damage
			return 2.0f;
		}
	}

	return BaseClass::GetHitgroupDamageMultiplier( iHitGroup, info );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_CombineS::HandleAnimEvent( animevent_t *pEvent )
{
	switch( pEvent->event )
	{
	case AE_SOLDIER_BLOCK_PHYSICS:
		DevMsg( "BLOCKING!\n" );
		m_fIsBlocking = true;
		break;

	default:
		BaseClass::HandleAnimEvent( pEvent );
		break;
	}
}

void CNPC_CombineS::OnChangeActivity( Activity eNewActivity )
{
	// Any new sequence stops us blocking.
	m_fIsBlocking = false;

	BaseClass::OnChangeActivity( eNewActivity );

#if HL2_EPISODIC
	// Give each trooper a varied look for his march. Done here because if you do it earlier (eg Spawn, StartTask), the
	// pose param gets overwritten.
	if (m_iUseMarch)
	{
		SetPoseParameter("casual", RandomFloat());
	}
#endif
}

void CNPC_CombineS::OnListened()
{
	BaseClass::OnListened();

	if ( HasCondition( COND_HEAR_DANGER ) && HasCondition( COND_HEAR_PHYSICS_DANGER ) )
	{
		if ( HasInterruptCondition( COND_HEAR_DANGER ) )
		{
			ClearCondition( COND_HEAR_PHYSICS_DANGER );
		}
	}

	// debugging to find missed schedules
#if 0
	if ( HasCondition( COND_HEAR_DANGER ) && !HasInterruptCondition( COND_HEAR_DANGER ) )
	{
		DevMsg("Ignore danger in %s\n", GetCurSchedule()->GetName() );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
void CNPC_CombineS::Event_Killed( const CTakeDamageInfo &info )
{
	// Don't bother if we've been told not to, or the player has a megaphyscannon
	if ( combine_spawn_health.GetBool() == false || PlayerHasMegaPhysCannon() )
	{
		BaseClass::Event_Killed( info );
		return;
	}

	CBasePlayer *pPlayer = ToBasePlayer( info.GetAttacker() );

	if ( !pPlayer )
	{
		CPropVehicleDriveable *pVehicle = dynamic_cast<CPropVehicleDriveable *>( info.GetAttacker() ) ;
		if ( pVehicle && pVehicle->GetDriver() && pVehicle->GetDriver()->IsPlayer() )
		{
			pPlayer = assert_cast<CBasePlayer *>( pVehicle->GetDriver() );
		}
	}

	if ( pPlayer != NULL )
	{
		// Elites drop alt-fire ammo, so long as they weren't killed by dissolving.
		if( IsElite() && GetActiveWeapon() && GetActiveWeapon()->UsesSecondaryAmmo() )
		{
#ifdef HL2_EPISODIC
			if ( HasSpawnFlags( SF_COMBINE_NO_AR2DROP ) == false )
#endif
			{
				CBaseEntity *pItem;
				if ( FClassnameIs(GetActiveWeapon(), "weapon_smg1"))
				{
					pItem = DropItem( "item_ammo_smg1_grenade", WorldSpaceCenter()+RandomVector(-4,4), RandomAngle(0,360) );
				}
				else
				{
					pItem = DropItem( "item_ammo_ar2_altfire", WorldSpaceCenter()+RandomVector(-4,4), RandomAngle(0,360) );
				}

				if ( pItem )
				{
					IPhysicsObject *pObj = pItem->VPhysicsGetObject();

					if ( pObj )
					{
						Vector			vel		= RandomVector( -64.0f, 64.0f );
						AngularImpulse	angImp	= RandomAngularImpulse( -300.0f, 300.0f );

						vel[2] = 0.0f;
						pObj->AddVelocity( &vel, &angImp );
					}

					if( info.GetDamageType() & DMG_DISSOLVE )
					{
						CBaseAnimating *pAnimating = dynamic_cast<CBaseAnimating*>(pItem);

						if( pAnimating )
						{
							pAnimating->Dissolve( NULL, gpGlobals->curtime, false, ENTITY_DISSOLVE_NORMAL );
						}
					}
					else
					{
						WeaponManager_AddManaged( pItem );
					}
				}
			}
		}

		CHalfLife2 *pHL2GameRules = static_cast<CHalfLife2 *>(g_pGameRules);

		float flCurrentHealth = pPlayer->GetHealth();

		// Attempt to drop health
		if ( pHL2GameRules->NPC_ShouldDropHealth( pPlayer ) )
		{
			if ( random->RandomInt( 0, 100 ) < 25 )
			{
				DropItem( "item_healthvial", WorldSpaceCenter()+RandomVector(-4,4), RandomAngle(0,360) );
			}
			else
			{
				DropItem( "item_battery", WorldSpaceCenter()+RandomVector(-4,4), RandomAngle(0,360) );
			}
			pHL2GameRules->NPC_DroppedHealth();
		}
		else if( flCurrentHealth <= 15 )
		{
			DropItem( "item_healthvial", WorldSpaceCenter()+RandomVector(-4,4), RandomAngle(0,360) );
		}
		
		if ( HasSpawnFlags( SF_COMBINE_NO_GRENADEDROP ) == false )
		{
			// Attempt to drop a grenade
			if ( pHL2GameRules->NPC_ShouldDropGrenade( pPlayer ) )
			{
				DropItem( "weapon_frag", WorldSpaceCenter()+RandomVector(-4,4), RandomAngle(0,360) );
				pHL2GameRules->NPC_DroppedGrenade();
			}
		}
	}

	BaseClass::Event_Killed( info );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_CombineS::IsLightDamage( const CTakeDamageInfo &info )
{
	return BaseClass::IsLightDamage( info );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_CombineS::IsHeavyDamage( const CTakeDamageInfo &info )
{
	if ( m_nRecentDamage > RECENT_DAMAGE_THRESHOLD && m_flRecentDamageTime != FLT_MAX )
	{
		if ( random->RandomInt(0,1) == 1 )
		{
			// This doesn't feel clean, as most responses go in the ai file, but its convinient so its going here.
			GetSentences()->Speak( "COMBINE_TAUNT", SENTENCE_PRIORITY_HIGH, SENTENCE_CRITERIA_NORMAL ); 

			m_nRecentDamage = 0;
			m_flRecentDamageTime = 0;
		}
		else
		{
			m_flRecentDamageTime = FLT_MAX;
			return true;
		}
	}
	
	return false;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_CombineS::HandleSpawnEquipment( void )
{
	if ( hl2r_random_weapons.GetBool() )
	{
		// Normal elites use AR2's, whose alt-fire is used
		// in some scripted sequences.
		if ( bAlreadyElite )
			return;
		
		// Elites.
		if ( IsElite() )
		{
			switch ( random->RandomInt( 0, 4 ) )
			{
			case 0:
				m_spawnEquipment = MAKE_STRING( "weapon_smg1" );
				break;
			case 1:
				m_spawnEquipment = MAKE_STRING( "weapon_ar2" );
				break;
			case 2:
				m_spawnEquipment = MAKE_STRING( "weapon_pistol" );
				break;
			case 3:
				m_spawnEquipment = MAKE_STRING( "weapon_shotgun" );
				break;
			case 4:
				m_spawnEquipment = MAKE_STRING( "weapon_357" );
				break;
			}
		}
		
		// Shotgunners.
		else if ( FStrEq(STRING(m_spawnEquipment), "weapon_shotgun") )
		{
			if ( random->RandomInt( 0, 1 ) == 0 )
				m_spawnEquipment = MAKE_STRING( "weapon_357" );
			else
				m_spawnEquipment = MAKE_STRING( "weapon_shotgun" );
		}
		
		// Standard prison guards.
		else if ( IsPrisonGuard() )
		{
			if ( random->RandomInt( 0, 1 ) == 0 )
				m_spawnEquipment = MAKE_STRING( "weapon_pistol" );
			else
				m_spawnEquipment = MAKE_STRING( "weapon_smg1" );
		}
		
		// Standard soldiers
		else
		{
			switch ( random->RandomInt( 0, 2 ) )
			{
			case 0:
				m_spawnEquipment = MAKE_STRING( "weapon_smg1" );
				break;
			case 1:
				m_spawnEquipment = MAKE_STRING( "weapon_ar2" );
				break;
			case 2:
				m_spawnEquipment = MAKE_STRING( "weapon_pistol" );
				break;
			}
		}
	}
	else if( IsPrisonGuard() )
	{
		// Prison shotgunners use Revolvers!
		if ( FStrEq(STRING(m_spawnEquipment), "weapon_shotgun") )
		{
			m_spawnEquipment = MAKE_STRING( "weapon_357" );
		}
		
		else if ( FStrEq(STRING(m_spawnEquipment), "weapon_ar2") )
		{
			m_spawnEquipment = MAKE_STRING( "weapon_pistol" );
		}	
		else if ( FStrEq(STRING(m_spawnEquipment), "weapon_smg1") )
		{
			// Use these as introductory maps
			if ( FStrEq(STRING(gpGlobals->mapname), "d2_coast_12") || FStrEq(STRING(gpGlobals->mapname), "d2_prison_01") ) 
			{
				m_spawnEquipment = MAKE_STRING( "weapon_pistol" );
			}
		}	
	}
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_CombineS::DoPromotion( void )
{
	SetModelName( MAKE_STRING( "models/combine_super_soldier.mdl" ) );
	return;
	
	// Our model name
	const char *pModelName = STRING( GetModelName() );
	
 	// We have to do this because our weapon is only equipped after we're spawned.
	bool bHasShotgun = ( FStrEq(STRING(m_spawnEquipment), "weapon_shotgun") || FStrEq(STRING(m_spawnEquipment), "weapon_357") );
	
	if ( !bHasShotgun )
	{
		if ( !Q_stricmp( pModelName, "models/combine_soldier_prisonguard.mdl" )  )
		{
			if ( FStrEq(STRING(m_spawnEquipment), "weapon_smg1") )
			{ 
				SetModelName( MAKE_STRING( "models/combine_super_soldier.mdl" ) );
				if ( random->RandomInt(0, 1) < 1 )
				{
					m_spawnEquipment = MAKE_STRING( "weapon_ar2" );
				}
			}
		}
		else 
		{
			if ( FStrEq(STRING(m_spawnEquipment), "weapon_ar2") )
			{
				SetModelName( MAKE_STRING( "models/combine_super_soldier.mdl" ) );
				if ( random->RandomInt(0, 1) < 1 )
				{
					m_spawnEquipment = MAKE_STRING( "weapon_smg1" );
				}
			}
		}
	}
}
#if HL2_EPISODIC
//-----------------------------------------------------------------------------
// Purpose: Translate base class activities into combot activites
//-----------------------------------------------------------------------------
Activity CNPC_CombineS::NPC_TranslateActivity( Activity eNewActivity )
{
	// If the special ep2_outland_05 "use march" flag is set, use the more casual marching anim.
	if ( m_iUseMarch && eNewActivity == ACT_WALK )
	{
		eNewActivity = ACT_WALK_MARCH;
	}

	return BaseClass::NPC_TranslateActivity( eNewActivity );
}
//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CNPC_CombineS )

	DEFINE_KEYFIELD( m_iUseMarch, FIELD_INTEGER, "usemarch" ),

END_DATADESC()
#endif