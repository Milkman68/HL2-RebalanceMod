//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "basehlcombatweapon_shared.h"
#include "basecombatcharacter.h"
#include "ai_basenpc.h"
#include "player.h"
#include "gamerules.h"
#include "in_buttons.h"
#include "soundent.h"
#include "game.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "IEffects.h"
#include "te_effect_dispatch.h"
#include "Sprite.h"
#include "SpriteTrail.h"
#include "beam_shared.h"
#include "rumble_shared.h"
#include "gamestats.h"
#include "decals.h"
#include "func_break.h"
#include "soundenvelope.h"
#include "npc_combine.h"

#ifdef PORTAL
	#include "portal_util_shared.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//#define BOLT_MODEL			"models/crossbow_bolt.mdl"
#define BOLT_MODEL	"models/weapons/w_missile_closed.mdl"

extern ConVar sk_plr_dmg_crossbow;
extern ConVar sk_plr_dmg_crossbow_charged;

extern ConVar sk_npc_dmg_crossbow;

//ConVar sk_npc_head_crossbow("sk_npc_head_crossbow", "3" );

ConVar sk_crossbow_air_velocity("sk_crossbow_air_velocity", "2500" );
ConVar sk_crossbow_water_velocity("sk_crossbow_water_velocity", "1500" );

ConVar sk_crossbow_gravity("sk_crossbow_gravity", "0.2" );

void TE_StickyBolt( IRecipientFilter& filter, float delay,	Vector vecDirection, const Vector *origin );

#define	BOLT_SKIN_NORMAL	0
#define BOLT_SKIN_GLOW		1

#define CHARGE_TIME 2.0f

//-----------------------------------------------------------------------------
// Crossbow Bolt
//-----------------------------------------------------------------------------
class CCrossbowBolt : public CBaseCombatCharacter
{
	DECLARE_CLASS( CCrossbowBolt, CBaseCombatCharacter );

public:
	CCrossbowBolt() { };
	~CCrossbowBolt();

	Class_T Classify( void ) { return CLASS_NONE; }

public:
	void Spawn( void );
	void Precache( void );
	void BubbleThink( void );
	void BoltTouch( CBaseEntity *pOther );
	bool CreateVPhysics( void );
	unsigned int PhysicsSolidMaskForEntity() const;
	static CCrossbowBolt *BoltCreate( const Vector &vecOrigin, const QAngle &angAngles, CBasePlayer *pentOwner = NULL, float flDamage = NULL );

protected:

	bool	CreateSprites( void );

	CHandle<CSprite>		m_pGlowSprite;
	float					m_flDamage;
	//CHandle<CSpriteTrail>	m_pGlowTrail;

	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();
};
LINK_ENTITY_TO_CLASS( crossbow_bolt, CCrossbowBolt );

BEGIN_DATADESC( CCrossbowBolt )
	// Function Pointers
	DEFINE_FUNCTION( BubbleThink ),
	DEFINE_FUNCTION( BoltTouch ),

	// These are recreated on reload, they don't need storage
	DEFINE_FIELD( m_pGlowSprite, FIELD_EHANDLE ),
	DEFINE_FIELD( m_flDamage, FIELD_FLOAT ),
	//DEFINE_FIELD( m_pGlowTrail, FIELD_EHANDLE ),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CCrossbowBolt, DT_CrossbowBolt )
END_SEND_TABLE()

CCrossbowBolt *CCrossbowBolt::BoltCreate( const Vector &vecOrigin, const QAngle &angAngles, CBasePlayer *pentOwner, float flDamage )
{
	// Create a new entity with CCrossbowBolt private data
	CCrossbowBolt *pBolt = (CCrossbowBolt *)CreateEntityByName( "crossbow_bolt" );
	UTIL_SetOrigin( pBolt, vecOrigin );
	pBolt->SetAbsAngles( angAngles );
	pBolt->Spawn();
	pBolt->SetOwnerEntity( pentOwner );
	pBolt->m_flDamage = flDamage != NULL ? flDamage : sk_plr_dmg_crossbow.GetFloat();

	return pBolt;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCrossbowBolt::~CCrossbowBolt( void )
{
	if ( m_pGlowSprite )
	{
		UTIL_Remove( m_pGlowSprite );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CCrossbowBolt::CreateVPhysics( void )
{
	// Create the object in the physics system
	VPhysicsInitNormal( SOLID_BBOX, FSOLID_NOT_STANDABLE, false );

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
unsigned int CCrossbowBolt::PhysicsSolidMaskForEntity() const
{
	return ( BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_HITBOX ) & ~CONTENTS_GRATE;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CCrossbowBolt::CreateSprites( void )
{
	// Start up the eye glow
	m_pGlowSprite = CSprite::SpriteCreate( "sprites/light_glow02_noz.vmt", GetLocalOrigin(), false );

	if ( m_pGlowSprite != NULL )
	{
		m_pGlowSprite->FollowEntity( this );
		m_pGlowSprite->SetTransparency( kRenderGlow, 255, 255, 255, 128, kRenderFxNoDissipation );
		m_pGlowSprite->SetScale( 0.2f );
		m_pGlowSprite->TurnOff();
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCrossbowBolt::Spawn( void )
{
	Precache( );

	SetModel( "models/crossbow_bolt.mdl" );
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM );
	UTIL_SetSize( this, -Vector(0.3f,0.3f,0.3f), Vector(0.3f,0.3f,0.3f) );
	SetSolid( SOLID_BBOX );
	SetGravity( sk_crossbow_gravity.GetFloat() );
	
	// Make sure we're updated if we're underwater
	UpdateWaterState();

	SetTouch( &CCrossbowBolt::BoltTouch );

	SetThink( &CCrossbowBolt::BubbleThink );
	SetNextThink( gpGlobals->curtime + 0.1f );
	
	CreateSprites();

	// Make us glow until we've hit the wall
	m_nSkin = BOLT_SKIN_GLOW;
}


void CCrossbowBolt::Precache( void )
{
	PrecacheModel( BOLT_MODEL );

	// This is used by C_TEStickyBolt, despte being different from above!!!
	PrecacheModel( "models/crossbow_bolt.mdl" );

	PrecacheModel( "sprites/light_glow02_noz.vmt" );
	
	PrecacheScriptSound("Weapon_Crossbow.ChargeLoop");
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CCrossbowBolt::BoltTouch( CBaseEntity *pOther )
{
	if ( pOther->IsSolidFlagSet(FSOLID_VOLUME_CONTENTS | FSOLID_TRIGGER) )
	{
		// Some NPCs are triggers that can take damage (like antlion grubs). We should hit them.
		if ( ( pOther->m_takedamage == DAMAGE_NO ) || ( pOther->m_takedamage == DAMAGE_EVENTS_ONLY ) )
			return;
	}

	if ( pOther->m_takedamage != DAMAGE_NO )
	{
		trace_t	tr, tr2;
		tr = BaseClass::GetTouchTrace();
		Vector	vecNormalizedVel = GetAbsVelocity();

		ClearMultiDamage();
		VectorNormalize( vecNormalizedVel );

#if defined(HL2_EPISODIC)
		//!!!HACKHACK - specific hack for ep2_outland_10 to allow crossbow bolts to pass through her bounding box when she's crouched in front of the player
		// (the player thinks they have clear line of sight because Alyx is crouching, but her BBOx is still full-height and blocks crossbow bolts.
		if( GetOwnerEntity() && GetOwnerEntity()->IsPlayer() && pOther->Classify() == CLASS_PLAYER_ALLY_VITAL && FStrEq(STRING(gpGlobals->mapname), "ep2_outland_10") )
		{
			// Change the owner to stop further collisions with Alyx. We do this by making her the owner.
			// The player won't get credit for this kill but at least the bolt won't magically disappear!
			SetOwnerEntity( pOther );
			return;
		}
#endif//HL2_EPISODIC
		
		// MODIFIED MAPBASE CODE BELOW!!
/* 		CBaseAnimating *pOtherAnimating = pOther->GetBaseAnimating();
		if (pOtherAnimating && pOtherAnimating->GetModelPtr() && pOtherAnimating->GetModelPtr()->numbones() > 1)
		{
			// Iterate through all bones.
			int iClosestBone = -1;
			float flCurDistSqr = Square(128.0f);
			matrix3x4_t bonetoworld;
			Vector vecBonePos;
			for (int i = 0; i < pOtherAnimating->GetModelPtr()->numbones(); i++)
			{
				pOtherAnimating->GetBoneTransform( i, bonetoworld );
				MatrixPosition( bonetoworld, vecBonePos );

				float flDist = vecBonePos.DistToSqr(GetLocalOrigin());
				if (flDist < flCurDistSqr)
				{
					iClosestBone = i;
					flCurDistSqr = flDist;
				}
			}
			
			// Does the target even have bones?
			if (iClosestBone != -1)
			{
 				// Compare the hit bone with the head bone. If we have a hit, deal headshot damage.
				bool iHead = 
				iClosestBone == pOtherAnimating->LookupBone( "ValveBiped.Bip01_Head1" ) ||
				iClosestBone == pOtherAnimating->LookupBone( "ValveBiped.Bip01_Spine4" ) ||
				iClosestBone == pOtherAnimating->LookupBone( "ValveBiped.Bip01_Neck1" );
				
				// (For Metropolice)
				bool iHeadHack = iClosestBone == pOtherAnimating->LookupBone( "ValveBiped.forward" );
				
				bool iHeadZombie = 
				
				// So. Many. Bones.
				
				// PoisonZombie.
				( pOtherAnimating->LookupBone( "ValveBiped.HC_BodyCube" ) == iClosestBone	) ||
				
				// FastZombite.
				( pOtherAnimating->LookupBone( "ValveBiped.HC_HeadCube" ) 	== iClosestBone) ||
				( pOtherAnimating->LookupBone( "ValveBiped.HC_BodyCube" ) 	== iClosestBone) ||
				( pOtherAnimating->LookupBone( "ValveBiped.HC_BoneLB" ) 	== iClosestBone) ||
				( pOtherAnimating->LookupBone( "ValveBiped.HC_BoneRB" ) 	== iClosestBone) ||
				( pOtherAnimating->LookupBone( "ValveBiped.HC_BoneLF" ) 	== iClosestBone) ||
				( pOtherAnimating->LookupBone( "ValveBiped.HC_BoneRF" ) 	== iClosestBone) ||
				
				// ClassicZombie. (I have no idea if this works with Zombines.)
				( pOtherAnimating->LookupBone( "ValveBiped.HC_Body_Bone" )	 	== iClosestBone) ||
				( pOtherAnimating->LookupBone( "ValveBiped.HC_Head_Bone" ) 		== iClosestBone) ||
				( pOtherAnimating->LookupBone( "ValveBiped.HC_ThighL_Bone" ) 	== iClosestBone) ||
				( pOtherAnimating->LookupBone( "ValveBiped.HC_ThighR_Bone" ) 	== iClosestBone) ||
				( pOtherAnimating->LookupBone( "ValveBiped.HC_UpperArmL_Bone" ) == iClosestBone) ||
				( pOtherAnimating->LookupBone( "ValveBiped.HC_UpperArmR_Bone" ) == iClosestBone);
				
				
				
				if ( iHead || iHeadHack || iHeadZombie )
				{
					m_flDamage *= sk_npc_head_crossbow.GetFloat();
				}
				
				//DevMsg("iClosestBone (%i)\n", iClosestBone );
				
				tr.physicsbone = pOtherAnimating->GetPhysicsBone(iClosestBone);
			}
		} */

		if( GetOwnerEntity() && GetOwnerEntity()->IsPlayer() && pOther->IsNPC() )
		{
			// Multiply damage by our charge percentage.
			CTakeDamageInfo	dmgInfo( this, GetOwnerEntity(), m_flDamage, DMG_BULLET | DMG_NEVERGIB );
			
			dmgInfo.AdjustPlayerDamageInflictedForSkillLevel();
			CalculateMeleeDamageForce( &dmgInfo, vecNormalizedVel, tr.endpos, 0.7f );
			dmgInfo.SetDamagePosition( tr.endpos );
			pOther->DispatchTraceAttack( dmgInfo, vecNormalizedVel, &tr );
			
			// Only ignite at full charge.
			if( m_flDamage == sk_plr_dmg_crossbow_charged.GetFloat() )
			{
				CBaseAnimating *pAnim;
				pAnim = dynamic_cast<CBaseAnimating*>(pOther);
					
				if ( pAnim )
					pAnim->Ignite( 30.0f );
			}

			CBasePlayer *pPlayer = ToBasePlayer( GetOwnerEntity() );
			if ( pPlayer )
			{
				gamestats->Event_WeaponHit( pPlayer, true, "weapon_crossbow", dmgInfo );
			}

		}
		else
		{
			CTakeDamageInfo	dmgInfo( this, GetOwnerEntity(), sk_plr_dmg_crossbow.GetFloat(), DMG_BULLET | DMG_NEVERGIB );
			CalculateMeleeDamageForce( &dmgInfo, vecNormalizedVel, tr.endpos, 0.7f );
			dmgInfo.SetDamagePosition( tr.endpos );
			pOther->DispatchTraceAttack( dmgInfo, vecNormalizedVel, &tr );
		}

		ApplyMultiDamage();

		//Adrian: keep going through the glass.
		if ( pOther->GetCollisionGroup() == COLLISION_GROUP_BREAKABLE_GLASS )
			 return;
		 
		if ( FClassnameIs(pOther, "func_breakable") )
		{
			CBreakable* pOtherEntity = static_cast<CBreakable*>( pOther );
			if ( pOtherEntity->GetMaterialType() == matGlass )
				return;
		}

		if ( !pOther->IsAlive() )
		{
			// We killed it! 
			const surfacedata_t *pdata = physprops->GetSurfaceData( tr.surface.surfaceProps );
			if ( pdata->game.material == CHAR_TEX_GLASS )
			{
				return;
			}
		}

		SetAbsVelocity( Vector( 0, 0, 0 ) );

		// play body "thwack" sound
		EmitSound( "Weapon_Crossbow.BoltHitBody" );

		Vector vForward;

		AngleVectors( GetAbsAngles(), &vForward );
		VectorNormalize ( vForward );

		UTIL_TraceLine( GetAbsOrigin(),	GetAbsOrigin() + vForward * 128, MASK_BLOCKLOS, pOther, COLLISION_GROUP_NONE, &tr2 );

		if ( tr2.fraction != 1.0f )
		{
//			NDebugOverlay::Box( tr2.endpos, Vector( -16, -16, -16 ), Vector( 16, 16, 16 ), 0, 255, 0, 0, 10 );
//			NDebugOverlay::Box( GetAbsOrigin(), Vector( -16, -16, -16 ), Vector( 16, 16, 16 ), 0, 0, 255, 0, 10 );

			if ( tr2.m_pEnt == NULL || ( tr2.m_pEnt && tr2.m_pEnt->GetMoveType() == MOVETYPE_NONE ) )
			{
				CEffectData	data;

				data.m_vOrigin = tr2.endpos;
				data.m_vNormal = vForward;
				data.m_nEntIndex = tr2.fraction != 1.0f;
			
				DispatchEffect( "BoltImpact", data );
			}
		}
		
		SetTouch( NULL );
		SetThink( NULL );

		if ( !g_pGameRules->IsMultiplayer() )
		{
			UTIL_Remove( this );
		}
	}
	else
	{
		trace_t	tr;
		tr = BaseClass::GetTouchTrace();

		// See if we struck the world
		if ( pOther->GetMoveType() == MOVETYPE_NONE && !( tr.surface.flags & SURF_SKY ) )
		{
			EmitSound( "Weapon_Crossbow.BoltHitWorld" );

			// if what we hit is static architecture, can stay around for a while.
			Vector vecDir = GetAbsVelocity();
			float speed = VectorNormalize( vecDir );

			// See if we should reflect off this surface
			float hitDot = DotProduct( tr.plane.normal, -vecDir );
			
			if ( ( hitDot < 0.5f ) && ( speed > 100 ) )
			{
				Vector vReflection = 2.0f * tr.plane.normal * hitDot + vecDir;
				
				QAngle reflectAngles;

				VectorAngles( vReflection, reflectAngles );

				SetLocalAngles( reflectAngles );

				SetAbsVelocity( vReflection * speed * 0.75f );

				// Start to sink faster
				SetGravity( 1.0f );
			}
			else
			{
				SetThink( &CCrossbowBolt::SUB_Remove );
				SetNextThink( gpGlobals->curtime + 2.0f );
				
				//FIXME: We actually want to stick (with hierarchy) to what we've hit
				SetMoveType( MOVETYPE_NONE );
			
				Vector vForward;

				AngleVectors( GetAbsAngles(), &vForward );
				VectorNormalize ( vForward );

				CEffectData	data;

				data.m_vOrigin = tr.endpos;
				data.m_vNormal = vForward;
				data.m_nEntIndex = 0;
			
				DispatchEffect( "BoltImpact", data );
				
				UTIL_ImpactTrace( &tr, DMG_BULLET );

				AddEffects( EF_NODRAW );
				SetTouch( NULL );
				SetThink( &CCrossbowBolt::SUB_Remove );
				SetNextThink( gpGlobals->curtime + 2.0f );

				if ( m_pGlowSprite != NULL )
				{
					m_pGlowSprite->TurnOn();
					m_pGlowSprite->FadeAndDie( 3.0f );
				}
			}
			
			// Shoot some sparks
			if ( UTIL_PointContents( GetAbsOrigin() ) != CONTENTS_WATER)
			{
				g_pEffects->Sparks( GetAbsOrigin() );
			}
		}
		else
		{
			// Put a mark unless we've hit the sky
			if ( ( tr.surface.flags & SURF_SKY ) == false )
			{
				UTIL_ImpactTrace( &tr, DMG_BULLET );
			}

			UTIL_Remove( this );
		}
	}

	if ( g_pGameRules->IsMultiplayer() )
	{
//		SetThink( &CCrossbowBolt::ExplodeThink );
//		SetNextThink( gpGlobals->curtime + 0.1f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCrossbowBolt::BubbleThink( void )
{
	QAngle angNewAngles;

	VectorAngles( GetAbsVelocity(), angNewAngles );
	SetAbsAngles( angNewAngles );

	SetNextThink( gpGlobals->curtime + 0.1f );

	// Make danger sounds out in front of me, to scare snipers back into their hole
	CSoundEnt::InsertSound( SOUND_DANGER_SNIPERONLY, GetAbsOrigin() + GetAbsVelocity() * 0.2, 120.0f, 0.5f, this, SOUNDENT_CHANNEL_REPEATED_DANGER );

	if ( GetWaterLevel()  == 0 )
		return;

	UTIL_BubbleTrail( GetAbsOrigin() - GetAbsVelocity() * 0.1f, GetAbsOrigin(), 5 );
}


//-----------------------------------------------------------------------------
// CWeaponCrossbow
//-----------------------------------------------------------------------------

class CWeaponCrossbow : public CBaseHLCombatWeapon
{
	DECLARE_CLASS( CWeaponCrossbow, CBaseHLCombatWeapon );
public:

	CWeaponCrossbow( void );
	
	virtual void	Precache( void );
	virtual void	PrimaryAttack( void );
	virtual void	SecondaryAttack( void );
	virtual bool	Deploy( void );
	virtual void	Drop( const Vector &vecVelocity );
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	virtual bool	Reload( void );
	virtual void	ItemPostFrame( void );
	virtual void	ItemBusyFrame( void );
	virtual void	Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	virtual bool	SendWeaponAnim( int iActivity );
	virtual bool	IsWeaponZoomed() { return m_bInZoom; }
	
	bool	ShouldDisplayHUDHint() { return true; }

	CNetworkVar( float, m_flCharge );
	
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

private:
	
	void	StopEffects( void );
	void	SetSkin( int skinNum );
	void	CheckZoomToggle( void );
	void	FireBolt( void );
	void	ToggleZoom( void );
	void    ReloadAnimSpeed( void );
	void    DoChargedFire( void );
	void 	SetLoopingSounds( bool bState );
	float	GetActivityAnimSpeed( Activity ideal );
	CSoundPatch *GetChargeSound( void );
	
	// Various states for the crossbow's charger
	enum ChargerState_t
	{
		CHARGER_STATE_START_LOAD,
		CHARGER_STATE_START_CHARGE,
		CHARGER_STATE_READY,
		CHARGER_STATE_DISCHARGE,
		CHARGER_STATE_OFF,
	};

	void	CreateChargerEffects( void );
	void	SetChargerState( ChargerState_t state, bool bRepeat );
	void	DoLoadEffect( void );

private:
	
	// Charger effects
	ChargerState_t		m_nChargeState;
	CHandle<CSprite>	m_hChargerSprite;

	bool				m_bInZoom;
	bool				m_bMustReload;
	bool				m_bIsCharging;
	float				m_flChargeTime;
	
	CSoundPatch			*m_sndCharge;		// Charging sounds
};

LINK_ENTITY_TO_CLASS( weapon_crossbow, CWeaponCrossbow );

PRECACHE_WEAPON_REGISTER( weapon_crossbow );

IMPLEMENT_SERVERCLASS_ST( CWeaponCrossbow, DT_WeaponCrossbow )
SendPropFloat( SENDINFO( m_flCharge ) ),
END_SEND_TABLE()

BEGIN_DATADESC( CWeaponCrossbow )

	DEFINE_FIELD( m_bInZoom,		FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bMustReload,	FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bIsCharging,	FIELD_BOOLEAN ),
	DEFINE_FIELD( m_nChargeState,	FIELD_INTEGER ),
	DEFINE_FIELD( m_flCharge, 		FIELD_FLOAT ),
	DEFINE_FIELD( m_flChargeTime, 	FIELD_TIME ),
	DEFINE_FIELD( m_hChargerSprite,	FIELD_EHANDLE ),
	DEFINE_SOUNDPATCH( m_sndCharge ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponCrossbow::CWeaponCrossbow( void )
{
	m_bReloadsSingly	= true;
	m_bFiresUnderwater	= true;
	m_bAltFiresUnderwater = true;
	m_bInZoom			= false;
	m_bMustReload		= false;
	m_bIsCharging		= false;
}

#define	CROSSBOW_GLOW_SPRITE	"sprites/light_glow02_noz.vmt"
#define	CROSSBOW_GLOW_SPRITE2	"sprites/blueflare1.vmt"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::Precache( void )
{
	UTIL_PrecacheOther( "crossbow_bolt" );

	PrecacheScriptSound( "Weapon_Crossbow.BoltHitBody" );
	PrecacheScriptSound( "Weapon_Crossbow.BoltHitWorld" );
	PrecacheScriptSound( "Weapon_Crossbow.BoltSkewer" );

	PrecacheModel( CROSSBOW_GLOW_SPRITE );
	PrecacheModel( CROSSBOW_GLOW_SPRITE2 );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponCrossbow::PrimaryAttack( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
    if ( !pPlayer )
		return;
	
	if ( m_bIsCharging )
		return;
	
	// Start charging, if we can.
	m_bIsCharging = true;
	m_flChargeTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponCrossbow::SecondaryAttack( void )
{
	//NOTENOTE: The zooming is handled by the post/busy frames
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponCrossbow::Reload( void )
{
	if ( BaseClass::Reload() )
	{
		m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration( ACT_VM_RELOAD ) + 1.75;
		m_bMustReload = false;
		
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::CheckZoomToggle( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	
	if ( pPlayer->m_afButtonPressed & IN_ATTACK2 )
	{
		ToggleZoom();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::ItemBusyFrame( void )
{
	// Allow zoom toggling even when we're reloading
	CheckZoomToggle();
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::ItemPostFrame( void )//bookmark
{
	// Allow zoom toggling
	CheckZoomToggle();

	if ( m_bMustReload && HasWeaponIdleTimeElapsed())
	{
		Reload();
	}
	
	DoChargedFire();

	BaseClass::ItemPostFrame();
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::FireBolt( void )
{
	if ( m_iClip1 <= 0 )
	{
		if ( !m_bFireOnEmpty )
		{
			Reload();
		}
		else
		{
			WeaponSound( EMPTY );
			m_flNextPrimaryAttack = 0.15;
		}

		return;
	}

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	pOwner->RumbleEffect( RUMBLE_357, 0, RUMBLE_FLAG_RESTART );

	Vector vecAiming	= pOwner->GetAutoaimVector( 0 );
	Vector vecSrc		= pOwner->Weapon_ShootPosition();

	QAngle angAiming;
	VectorAngles( vecAiming, angAiming );

#if defined(HL2_EPISODIC)
	// !!!HACK - the other piece of the Alyx crossbow bolt hack for Outland_10 (see ::BoltTouch() for more detail)
	if( FStrEq(STRING(gpGlobals->mapname), "ep2_outland_10") )
	{
		trace_t tr;
		UTIL_TraceLine( vecSrc, vecSrc + vecAiming * 24.0f, MASK_SOLID, pOwner, COLLISION_GROUP_NONE, &tr );

		if( tr.m_pEnt != NULL && tr.m_pEnt->Classify() == CLASS_PLAYER_ALLY_VITAL )
		{
			// If Alyx is right in front of the player, make sure the bolt starts outside of the player's BBOX, or the bolt
			// will instantly collide with the player after the owner of the bolt is switched to Alyx in ::BoltTouch(). We 
			// avoid this altogether by making it impossible for the bolt to collide with the player.
			vecSrc += vecAiming * 24.0f;
		}
	}
#endif

	float flChargeRatio = CHARGE_TIME / 3.0f;
	
	// Only start increasing damage after 1/3 of the max charge time has occurred.
	float flChargeTime = MAX( gpGlobals->curtime - ( m_flChargeTime + flChargeRatio ), 0.0f );
	
	// Lerp from default dmg to charged dmg based on our charge.
	float flDamage = RemapValClamped( flChargeTime, 0.0f, CHARGE_TIME - flChargeRatio, 
	sk_plr_dmg_crossbow.GetFloat(), 
	sk_plr_dmg_crossbow_charged.GetFloat() );

	CCrossbowBolt *pBolt = CCrossbowBolt::BoltCreate( vecSrc, angAiming, pOwner, flDamage );

	if ( pOwner->GetWaterLevel() == 3 )
	{
		pBolt->SetAbsVelocity( vecAiming * sk_crossbow_water_velocity.GetFloat() );
	}
	else
	{
		pBolt->SetAbsVelocity( vecAiming * sk_crossbow_air_velocity.GetFloat() );
	}

	m_iClip1--;
	
	QAngle viewPunch = QAngle( -2, 0, 0 );
	pOwner->ViewPunch( viewPunch );

	WeaponSound( SINGLE );
	WeaponSound( SPECIAL2 );

	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), 200, 0.2 );

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	if ( !m_iClip1 && pOwner->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
	{
		// HEV suit - indicate out of ammo condition
		pOwner->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
	}

	m_flNextPrimaryAttack = m_flNextSecondaryAttack	= gpGlobals->curtime + 0.75;

	//DoLoadEffect();
	SetChargerState( CHARGER_STATE_OFF, false );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponCrossbow::Deploy( void )
{
	if ( m_iClip1 <= 0 )//bookmark
	{
		return DefaultDeploy( (char*)GetViewModel(), (char*)GetWorldModel(), ACT_CROSSBOW_DRAW_UNLOADED, (char*)GetAnimPrefix() );
	}
	
	SetSkin( BOLT_SKIN_NORMAL );
	SetChargerState( CHARGER_STATE_OFF, false );

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSwitchingTo - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponCrossbow::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	StopEffects();
	
	m_bIsCharging = false;
	SetLoopingSounds( false );
	
	m_flCharge = 0.0;
	
	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::ToggleZoom( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	if ( m_bInZoom )
	{
		if ( pPlayer->SetFOV( this, 0, 0.2f ) )
		{
			m_bInZoom = false;
		}
	}
	else
	{
		if ( pPlayer->SetFOV( this, 20, 0.1f ) )
		{
			m_bInZoom = true;
		}
	}
}

#define	BOLT_TIP_ATTACHMENT	2

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::CreateChargerEffects( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( m_hChargerSprite != NULL )
		return;

	m_hChargerSprite = CSprite::SpriteCreate( CROSSBOW_GLOW_SPRITE, GetAbsOrigin(), false );

	if ( m_hChargerSprite )
	{
		m_hChargerSprite->SetAttachment( pOwner->GetViewModel(), BOLT_TIP_ATTACHMENT );
		m_hChargerSprite->SetTransparency( kRenderTransAdd, 255, 128, 0, 255, kRenderFxNoDissipation );
		m_hChargerSprite->SetBrightness( 0 );
		m_hChargerSprite->SetScale( 0.1f );
		m_hChargerSprite->TurnOff();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : skinNum - 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::SetSkin( int skinNum )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	CBaseViewModel *pViewModel = pOwner->GetViewModel();

	if ( pViewModel == NULL )
		return;

	pViewModel->m_nSkin = skinNum;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::DoLoadEffect( void )
{
	SetSkin( BOLT_SKIN_GLOW );

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;

	CBaseViewModel *pViewModel = pOwner->GetViewModel();

	if ( pViewModel == NULL )
		return;

	CEffectData	data;

	data.m_nEntIndex = pViewModel->entindex();
	data.m_nAttachmentIndex = 1;

	DispatchEffect( "CrossbowLoad", data );

	CSprite *pBlast = CSprite::SpriteCreate( CROSSBOW_GLOW_SPRITE2, GetAbsOrigin(), false );

	if ( pBlast )
	{
		pBlast->SetAttachment( pOwner->GetViewModel(), 1 );
		pBlast->SetTransparency( kRenderTransAdd, 255, 255, 255, 255, kRenderFxNone );
		pBlast->SetBrightness( 128 );
		pBlast->SetScale( 0.2f );
		pBlast->FadeOutFromSpawn();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : state - 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::SetChargerState( ChargerState_t state, bool bRepeat )
{
	// Make sure we're setup
	CreateChargerEffects();

	// Don't do this twice(unless told otherwise)
	if ( state == m_nChargeState && !bRepeat )
		return;

	m_nChargeState = state;

	switch( m_nChargeState )
	{
	case CHARGER_STATE_START_LOAD:
	
		WeaponSound( SPECIAL1 );
		
		// Shoot some sparks and draw a beam between the two outer points
		DoLoadEffect();
		
		break;

	case CHARGER_STATE_START_CHARGE:
		{
			if ( m_hChargerSprite == NULL )
				break;
			
			float flCharge = RemapValClamped( gpGlobals->curtime - m_flChargeTime, 0.0f, CHARGE_TIME, 0.0f, 1.0f );
			
			m_hChargerSprite->SetBrightness( 80 * flCharge, 0.5f );
			m_hChargerSprite->SetScale( 0.1f * flCharge, 0.5f );
			m_hChargerSprite->TurnOn();
		}

		break;

	case CHARGER_STATE_READY:
		{
			// Get fully charged
			if ( m_hChargerSprite == NULL )
				break;
			
			m_hChargerSprite->SetBrightness( 80, 1.0f );
			m_hChargerSprite->SetScale( 0.2f, 0.5f );
			m_hChargerSprite->TurnOn();
		}

		break;

	case CHARGER_STATE_DISCHARGE:
		{
			SetSkin( BOLT_SKIN_NORMAL );
			
			if ( m_hChargerSprite == NULL )
				break;
			
			m_hChargerSprite->SetBrightness( 0 );
			m_hChargerSprite->TurnOff();
		}

		break;

	case CHARGER_STATE_OFF:
		{
		//	SetSkin( BOLT_SKIN_NORMAL );

			if ( m_hChargerSprite == NULL )
				break;
			
			m_hChargerSprite->SetBrightness( 0 );
			m_hChargerSprite->TurnOff();
		}
		break;

	default:
		break;
	}
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::DoChargedFire( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
    if ( !pPlayer )
		return;
	
	// Handle weapon firing here, because we need to detect when IN_ATTACK is released.
	if ( pPlayer->m_afButtonReleased & IN_ATTACK && m_bIsCharging )
	{
		m_bIsCharging = false;
		SetLoopingSounds( false );
		SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration( ACT_VM_PRIMARYATTACK ) );
		
		FireBolt();
			
		// Signal a reload
		m_bMustReload = true;

		m_iPrimaryAttacks++;
		gamestats->Event_WeaponFired( pPlayer, true, GetClassname() );
		SetSkin( BOLT_SKIN_NORMAL );
	}
	
	if ( m_bMustReload )
		return;
	
	// Charge effects.
	float flCharge = 0.0f;
	if ( m_bIsCharging )
	{
		SetSkin( BOLT_SKIN_GLOW );
		flCharge = RemapValClamped( gpGlobals->curtime - m_flChargeTime, 0.0f, CHARGE_TIME, 0.0f, 25 );
		
		if ( flCharge >= 25 )
		{
			SetChargerState( CHARGER_STATE_START_LOAD, false );
		}
		else
		{
			// Update our charge sprites until at full charge
			SetChargerState( CHARGER_STATE_START_CHARGE, true );
		}
		
		// Update our sounds
		SetLoopingSounds( true );
	}
	
	m_flCharge = flCharge;
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CWeaponCrossbow::GetActivityAnimSpeed( Activity ideal )
{
	// Just makes the weapon more convenient to use.
	if ( ideal == ACT_VM_RELOAD )
		return 1.15;
	
	if ( ideal == ACT_VM_PRIMARYATTACK )
		return 2.0;
	
	return BaseClass::GetActivityAnimSpeed(ideal);
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::SetLoopingSounds( bool bState )
{
	if ( !GetChargeSound() )
		return;
	
	// Play the charging sounds!
	if ( bState )
	{
		
		// Makes the sound pitch reflect how damage is calculated.
		float flChargeRatio = CHARGE_TIME / 3.0f;
		float flChargeTime = MAX( gpGlobals->curtime - ( m_flChargeTime + flChargeRatio ), 0.0f );
		
		float ChargePerc = RemapValClamped( flChargeTime, 0.0f, CHARGE_TIME - flChargeRatio, 0.4f, 1.0f );
		
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
CSoundPatch *CWeaponCrossbow::GetChargeSound( void )
{
	if ( m_sndCharge == NULL )
	{
		CPASAttenuationFilter filter( this );
		m_sndCharge = (CSoundEnvelopeController::GetController()).SoundCreate( filter, entindex(), CHAN_STATIC, "Weapon_Crossbow.ChargeLoop", ATTN_NORM );
		(CSoundEnvelopeController::GetController()).Play( GetChargeSound(), 0.0f, 50 );
	}

	return m_sndCharge;
}
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEvent - 
//			*pOperator - 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
}

//-----------------------------------------------------------------------------
// Purpose: Set the desired activity for the weapon and its viewmodel counterpart
// Input  : iActivity - activity to play
//-----------------------------------------------------------------------------
bool CWeaponCrossbow::SendWeaponAnim( int iActivity )
{
	int newActivity = iActivity;

	// The last shot needs a non-loaded activity
	if ( ( newActivity == ACT_VM_IDLE ) && ( m_iClip1 <= 0 ) )
	{
		newActivity = ACT_VM_FIDGET;
	}

	//For now, just set the ideal activity and be done with it
	return BaseClass::SendWeaponAnim( newActivity );
}

//-----------------------------------------------------------------------------
// Purpose: Stop all zooming and special effects on the viewmodel
//-----------------------------------------------------------------------------
void CWeaponCrossbow::StopEffects( void )
{
	// Stop zooming
	if ( m_bInZoom )
	{
		ToggleZoom();
	}

	// Turn off our sprites
	SetChargerState( CHARGER_STATE_OFF, false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::Drop( const Vector &vecVelocity )
{
	StopEffects();
	BaseClass::Drop( vecVelocity );
}