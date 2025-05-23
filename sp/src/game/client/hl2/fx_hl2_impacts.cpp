//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Game-specific impact effect hooks
//
//=============================================================================//
#include "cbase.h"
#include "fx_impact.h"
#include "fx.h"
#include "decals.h"
#include "fx_quad.h"
#include "fx_sparks.h"
#include "dlight.h"
#include "iefx.h"
#include "flashlighteffect.h"

#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar hl2r_projected_muzzleflash;
extern ConVar hl2r_dynamic_light_level;

//-----------------------------------------------------------------------------
// Purpose: Handle jeep impacts
//-----------------------------------------------------------------------------
void ImpactJeepCallback( const CEffectData &data )
{
	trace_t tr;
	Vector vecOrigin, vecStart, vecShotDir;
	int iMaterial, iDamageType, iHitbox;
	short nSurfaceProp;
	C_BaseEntity *pEntity = ParseImpactData( data, &vecOrigin, &vecStart, &vecShotDir, nSurfaceProp, iMaterial, iDamageType, iHitbox );

	if ( !pEntity )
	{
		// This happens for impacts that occur on an object that's then destroyed.
		// Clear out the fraction so it uses the server's data
		tr.fraction = 1.0;
		PlayImpactSound( pEntity, tr, vecOrigin, nSurfaceProp );
		return;
	}

	// If we hit, perform our custom effects and play the sound
	if ( Impact( vecOrigin, vecStart, iMaterial, iDamageType, iHitbox, pEntity, tr ) )
	{
		// Check for custom effects based on the Decal index
		PerformCustomEffects( vecOrigin, tr, vecShotDir, iMaterial, 2 );
	}

	PlayImpactSound( pEntity, tr, vecOrigin, nSurfaceProp );
}

DECLARE_CLIENT_EFFECT( "ImpactJeep", ImpactJeepCallback );


//-----------------------------------------------------------------------------
// Purpose: Handle gauss impacts
//-----------------------------------------------------------------------------
void ImpactGaussCallback( const CEffectData &data )
{
	trace_t tr;
	Vector vecOrigin, vecStart, vecShotDir;
	int iMaterial, iDamageType, iHitbox;
	short nSurfaceProp;
	C_BaseEntity *pEntity = ParseImpactData( data, &vecOrigin, &vecStart, &vecShotDir, nSurfaceProp, iMaterial, iDamageType, iHitbox );

	if ( !pEntity )
	{
		// This happens for impacts that occur on an object that's then destroyed.
		// Clear out the fraction so it uses the server's data
		tr.fraction = 1.0;
		PlayImpactSound( pEntity, tr, vecOrigin, nSurfaceProp );
		return;
	}

	// If we hit, perform our custom effects and play the sound
	if ( Impact( vecOrigin, vecStart, iMaterial, iDamageType, iHitbox, pEntity, tr ) )
	{
		// Check for custom effects based on the Decal index
		PerformCustomEffects( vecOrigin, tr, vecShotDir, iMaterial, 2 );
	}

	PlayImpactSound( pEntity, tr, vecOrigin, nSurfaceProp );
}

DECLARE_CLIENT_EFFECT( "ImpactGauss", ImpactGaussCallback );

//-----------------------------------------------------------------------------
// Purpose: Handle weapon impacts
//-----------------------------------------------------------------------------
void ImpactCallback( const CEffectData &data )
{
	VPROF_BUDGET( "ImpactCallback", VPROF_BUDGETGROUP_PARTICLE_RENDERING );

	trace_t tr;
	Vector vecOrigin, vecStart, vecShotDir;
	int iMaterial, iDamageType, iHitbox;
	short nSurfaceProp;
	C_BaseEntity *pEntity = ParseImpactData( data, &vecOrigin, &vecStart, &vecShotDir, nSurfaceProp, iMaterial, iDamageType, iHitbox );

	if ( !pEntity )
	{
		// This happens for impacts that occur on an object that's then destroyed.
		// Clear out the fraction so it uses the server's data
		tr.fraction = 1.0;
		PlayImpactSound( pEntity, tr, vecOrigin, nSurfaceProp );
		return;
	}

	// If we hit, perform our custom effects and play the sound
	if ( Impact( vecOrigin, vecStart, iMaterial, iDamageType, iHitbox, pEntity, tr ) )
	{
		// Check for custom effects based on the Decal index
		PerformCustomEffects( vecOrigin, tr, vecShotDir, iMaterial, 1.0 );
	}

	PlayImpactSound( pEntity, tr, vecOrigin, nSurfaceProp );
}

DECLARE_CLIENT_EFFECT( "Impact", ImpactCallback );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
//			&normal - 
//			scale - 
//-----------------------------------------------------------------------------
void FX_AirboatGunImpact( const Vector &origin, const Vector &normal, float scale )
{
#ifdef _XBOX

	Vector offset = origin + ( normal * 1.0f );

	CSmartPtr<CTrailParticles> sparkEmitter = CTrailParticles::Create( "FX_MetalSpark 1" );

	if ( sparkEmitter == NULL )
		return;

	//Setup our information
	sparkEmitter->SetSortOrigin( offset );
	sparkEmitter->SetFlag( bitsPARTICLE_TRAIL_VELOCITY_DAMPEN );
	sparkEmitter->SetVelocityDampen( 8.0f );
	sparkEmitter->SetGravity( 800.0f );
	sparkEmitter->SetCollisionDamped( 0.25f );
	sparkEmitter->GetBinding().SetBBox( offset - Vector( 32, 32, 32 ), offset + Vector( 32, 32, 32 ) );

	int	numSparks = random->RandomInt( 4, 8 );

	TrailParticle	*pParticle;
	PMaterialHandle	hMaterial = sparkEmitter->GetPMaterial( "effects/spark" );
	Vector			dir;

	float	length	= 0.1f;

	//Dump out sparks
	for ( int i = 0; i < numSparks; i++ )
	{
		pParticle = (TrailParticle *) sparkEmitter->AddParticle( sizeof(TrailParticle), hMaterial, offset );

		if ( pParticle == NULL )
			return;

		pParticle->m_flLifetime	= 0.0f;
		pParticle->m_flDieTime	= random->RandomFloat( 0.05f, 0.1f );

		float	spreadOfs = random->RandomFloat( 0.0f, 2.0f );

		dir[0] = normal[0] + random->RandomFloat( -(0.5f*spreadOfs), (0.5f*spreadOfs) );
		dir[1] = normal[1] + random->RandomFloat( -(0.5f*spreadOfs), (0.5f*spreadOfs) );
		dir[2] = normal[2] + random->RandomFloat( -(0.5f*spreadOfs), (0.5f*spreadOfs) );

		VectorNormalize( dir );

		pParticle->m_flWidth		= random->RandomFloat( 1.0f, 4.0f );
		pParticle->m_flLength		= random->RandomFloat( length*0.25f, length );

		pParticle->m_vecVelocity	= dir * random->RandomFloat( (128.0f*(2.0f-spreadOfs)), (512.0f*(2.0f-spreadOfs)) );

		Color32Init( pParticle->m_color, 255, 255, 255, 255 );
	}

#else

	// Normal metal spark
	FX_MetalSpark( origin, normal, normal, (int) scale );

#endif // _XBOX

	// Add a quad to highlite the hit point
	FX_AddQuad( origin, 
				normal, 
				random->RandomFloat( 16, 32 ),
				random->RandomFloat( 32, 48 ),
				0.75f, 
				1.0f,
				0.0f,
				0.4f,
				random->RandomInt( 0, 360 ), 
				0,
				Vector( 1.0f, 1.0f, 1.0f ), 
				0.05f, 
				"effects/combinemuzzle2_nocull",
				(FXQUAD_BIAS_SCALE|FXQUAD_BIAS_ALPHA) );
}

//-----------------------------------------------------------------------------
// Purpose: Handle weapon impacts from the airboat gun shooting (cheaper versions)
//-----------------------------------------------------------------------------
void ImpactAirboatGunCallback( const CEffectData &data )
{
	VPROF_BUDGET( "ImpactAirboatGunCallback", VPROF_BUDGETGROUP_PARTICLE_RENDERING );

	trace_t tr;
	Vector vecOrigin, vecStart, vecShotDir;
	int iMaterial, iDamageType, iHitbox;
	short nSurfaceProp;
	C_BaseEntity *pEntity = ParseImpactData( data, &vecOrigin, &vecStart, &vecShotDir, nSurfaceProp, iMaterial, iDamageType, iHitbox );

	if ( !pEntity )
	{
		// This happens for impacts that occur on an object that's then destroyed.
		// Clear out the fraction so it uses the server's data
		tr.fraction = 1.0;
		PlayImpactSound( pEntity, tr, vecOrigin, nSurfaceProp );
		return;
	}

#if !defined( _XBOX )
	// If we hit, perform our custom effects and play the sound. Don't create decals
	if ( Impact( vecOrigin, vecStart, iMaterial, iDamageType, iHitbox, pEntity, tr, IMPACT_NODECAL | IMPACT_REPORT_RAGDOLL_IMPACTS ) )
	{
		FX_AirboatGunImpact( vecOrigin, tr.plane.normal, 2 );
	}
#else
	FX_AirboatGunImpact( vecOrigin, tr.plane.normal, 1 );
#endif
}

DECLARE_CLIENT_EFFECT( "AirboatGunImpact", ImpactAirboatGunCallback );


//-----------------------------------------------------------------------------
// Purpose: Handle weapon impacts from the helicopter shooting (cheaper versions)
//-----------------------------------------------------------------------------
void ImpactHelicopterCallback( const CEffectData &data )
{
	VPROF_BUDGET( "ImpactHelicopterCallback", VPROF_BUDGETGROUP_PARTICLE_RENDERING );

	trace_t tr;
	Vector vecOrigin, vecStart, vecShotDir;
	int iMaterial, iDamageType, iHitbox;
	short nSurfaceProp;
	C_BaseEntity *pEntity = ParseImpactData( data, &vecOrigin, &vecStart, &vecShotDir, nSurfaceProp, iMaterial, iDamageType, iHitbox );

	if ( !pEntity )
	{
		// This happens for impacts that occur on an object that's then destroyed.
		// Clear out the fraction so it uses the server's data
		tr.fraction = 1.0;
		PlayImpactSound( pEntity, tr, vecOrigin, nSurfaceProp );
		return;
	}

	// If we hit, perform our custom effects and play the sound. Don't create decals
	if ( Impact( vecOrigin, vecStart, iMaterial, iDamageType, iHitbox, pEntity, tr, IMPACT_NODECAL | IMPACT_REPORT_RAGDOLL_IMPACTS ) )
	{
		FX_AirboatGunImpact( vecOrigin, tr.plane.normal, IsXbox() ? 1 : 2 );

		// Only do metal + computer custom effects
		if ( (iMaterial == CHAR_TEX_METAL) || (iMaterial == CHAR_TEX_COMPUTER) )
		{
			PerformCustomEffects( vecOrigin, tr, vecShotDir, iMaterial, 1.0, FLAGS_CUSTIOM_EFFECTS_NOFLECKS );
		}
	}

	PlayImpactSound( pEntity, tr, vecOrigin, nSurfaceProp );
}

DECLARE_CLIENT_EFFECT( "HelicopterImpact", ImpactHelicopterCallback );

//-----------------------------------------------------------------------------
// Purpose: Just throwing grenade light effects here.
//-----------------------------------------------------------------------------
void Grenade_Blip( ClientEntityHandle_t hEntity, int attachmentIndex )
{
	VPROF_BUDGET( "Grenade_Blip", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	
	extern ConVar hl2r_reduced_assists;
	if ( hl2r_dynamic_light_level.GetInt() != 0 ) // VFX + Entities
		return;
		
	if ( hl2r_reduced_assists.GetBool() )
		return;

	// Grab the origin out of the transform for the attachment
	// If the client hasn't seen this entity yet, bail.
	matrix3x4_t	matAttachment;
	if ( FX_GetAttachmentTransform( hEntity, attachmentIndex, matAttachment ) )
	{
		Vector		origin;
		MatrixGetColumn( matAttachment, 3, &origin );
		
		int entityIndex = ClientEntityList().HandleToEntIndex( hEntity );
		if ( entityIndex >= 0 )
		{
			dlight_t *dl = effects->CL_AllocDlight( LIGHT_INDEX_TE_DYNAMIC + entityIndex );

			dl->origin	= origin;
			dl->color.r = 255;
			dl->color.g = 0;
			dl->color.b = 0;
			dl->color.exponent = 1;
			dl->radius	= 128;
			dl->decay	= dl->radius / 0.05f;
			dl->die		= gpGlobals->curtime + 0.1f;
			dl->flags = DLIGHT_NO_MODEL_ILLUMINATION;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void GrenadeBlipCallback( const CEffectData &data )
{
	Grenade_Blip( data.m_hEntity, data.m_nAttachmentIndex );
}

DECLARE_CLIENT_EFFECT( "GrenadeBlip", GrenadeBlipCallback );

//-----------------------------------------------------------------------------
// Purpose: A way to initialize a muzzleflash light without needing to come from CBasecombatWeapon
//-----------------------------------------------------------------------------
void MuzzleFlashLight( ClientEntityHandle_t hEntity, int attachmentIndex, int flashtype )
{
	VPROF_BUDGET( "MuzzleFlashLight", VPROF_BUDGETGROUP_PARTICLE_RENDERING );

	// Grab the origin out of the transform for the attachment
	// If the client hasn't seen this entity yet, bail.
	Vector origin;
	QAngle angles;
	
	if ( FX_GetAttachmentTransform( hEntity, attachmentIndex, &origin, &angles ) )
	{
		int entityIndex = ClientEntityList().HandleToEntIndex( hEntity );
		if ( entityIndex >= 0 )
		{
			if ( !hl2r_projected_muzzleflash.GetBool() || hl2r_dynamic_light_level.GetInt() == 2 )
			{
				dlight_t *dl;
				if ( hl2r_dynamic_light_level.GetInt() == 2 )
				{
					dl = effects->CL_AllocElight( LIGHT_INDEX_TE_DYNAMIC + entityIndex );
				}
				else
				{
					dl = effects->CL_AllocDlight( LIGHT_INDEX_TE_DYNAMIC + entityIndex );
				}
				
				dl->origin	= origin;
				
				if ( flashtype == MUZZLEFLASH_COMBINE )
				{
					dl->color.r = 255;
					dl->color.g = 255;
					dl->color.b = 168;
				}
				else
				{
					dl->color.r = 255;
					dl->color.g = 204;
					dl->color.b = 76;
				}
				
				dl->color.exponent = 1;
				dl->radius	= 196;
				dl->decay	= dl->radius / 0.05f;
				dl->die		= gpGlobals->curtime + 0.1f;
			}
			else
			{
				C_ProjMuzzleFlash *pFlash = new C_ProjMuzzleFlash();
					
				// Initialize our effect.
				if ( pFlash->InitializeAsClientEntity( NULL, RENDER_GROUP_TRANSLUCENT_ENTITY ) == false )
				{
					pFlash->Release();
					return;
				}
					
				if ( pFlash )
				{
					// Set the position and angle.
					pFlash->SetAbsOrigin( origin );
					pFlash->SetAbsAngles( angles );
						
					// Parent the light to our muzzle.
					pFlash->SetParent( ClientEntityList().GetBaseEntityFromHandle( hEntity ) );
					
					// Add random rotation.
					QAngle localangle = pFlash->GetLocalAngles();
					localangle[ ROLL ] = random->RandomInt( -180, 180 );
						
					pFlash->SetLocalAngles( localangle );
				
					// Parameters
					float r, g, b, e; // Red, Green, Blue, Intensity.
					
					if ( flashtype == MUZZLEFLASH_COMBINE )
					{
						r = 1.0f;
						g = 1.0f;
						b = 0.66f;
						e = 1.0f;
					}
					else
					{
						r = 1.0f;
						g = 0.8f;
						b = 0.3f;
						e = 1.0f;
					}
					
					pFlash->color[0] = r;
					pFlash->color[1] = g;
					pFlash->color[2] = b;
					pFlash->color[3] = e;

					pFlash->die = 0.1f;
					pFlash->holdtime = 0.0f;
					pFlash->fov = 100;
					pFlash->range = 1000;
					pFlash->clq[0] = 0.0f;
					pFlash->clq[1] = 0.0f;
					pFlash->clq[2] = 4000.0f;
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void MuzzleFlashLightCallback( const CEffectData &data )
{
	MuzzleFlashLight( data.m_hEntity, data.m_nAttachmentIndex, data.m_fFlags );
}

DECLARE_CLIENT_EFFECT( "MuzzleFlashLight", MuzzleFlashLightCallback );
