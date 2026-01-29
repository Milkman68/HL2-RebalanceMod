//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#include "dlight.h"
#include "iefx.h"

class C_Missile : public C_BaseCombatCharacter
{
public:
	DECLARE_CLASS( C_Missile, C_BaseCombatCharacter );
	DECLARE_CLIENTCLASS();
 	DECLARE_DATADESC();

					C_Missile();
	virtual			~C_Missile();

private:
	void Simulate( void );
};

IMPLEMENT_CLIENTCLASS_DT(C_Missile, DT_Missile, CMissile)
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Save/restore
//-----------------------------------------------------------------------------
BEGIN_DATADESC( C_Missile )
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
C_Missile::C_Missile()
{
}
C_Missile::~C_Missile()
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_Missile::Simulate( void )
{
	if ( gpGlobals->frametime <= 0.0f )
		return;

	// The rocket becomes solid when activated.
	if ( GetSolidFlags() == FSOLID_NOT_SOLID )
		return;
	
	extern ConVar hl2r_dynamic_light_level;
	if ( hl2r_dynamic_light_level.GetInt() != 0 ) // VFX + Entities
		return;

	dlight_t* dl = effects->CL_AllocDlight(index);
	dl->origin = GetAbsOrigin();
	dl->color.r = 255;
	dl->color.g = 170;
	dl->color.b = 120;
	dl->color.exponent = 1;
	dl->radius = random->RandomFloat(96, 192);
	dl->decay = dl->radius / 0.1;
	dl->die = gpGlobals->curtime + 0.1;
	dl->style = 1;
}


