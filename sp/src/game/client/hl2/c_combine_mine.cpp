#include "cbase.h"
#include "c_ai_basenpc.h"
#include "beam_shared.h"
#include "engine\ivdebugoverlay.h"
#include "iviewrender_beams.h"
#include "dlight.h"
#include "iefx.h"

#define LIGHT_SCALE 1.0f

class C_BounceBomb : public C_AI_BaseNPC
{
	DECLARE_CLASS( C_BounceBomb, C_AI_BaseNPC );
public:
	DECLARE_CLIENTCLASS();
	C_BounceBomb( void ) {}
	void 	Simulate( void );

private:
	float m_flLightColor[3];
};

IMPLEMENT_CLIENTCLASS_DT( C_BounceBomb, DT_BounceBomb, CBounceBomb )
	RecvPropArray( RecvPropFloat( RECVINFO( m_flLightColor[0] ) ), m_flLightColor ),
END_RECV_TABLE()

void C_BounceBomb::Simulate( void )
{
	extern ConVar hl2r_dynamic_light_level;
	if ( hl2r_dynamic_light_level.GetInt() != 0 ) // VFX + Entities
		return;

	if ( m_flLightColor[0] <= 0 && m_flLightColor[1] <= 0 && m_flLightColor[2] <= 0 )
		return;

/*	Vector mins = GetAbsOrigin();
	if ( !engine->IsBoxVisible( mins, mins ) )
		return;*/

	Vector up;
	GetVectors( NULL, NULL, &up );

	dlight_t* dl = effects->CL_AllocDlight(index);
	dl->origin = GetAbsOrigin() + (up * 20);
	dl->color.r = m_flLightColor[0] * LIGHT_SCALE;
	dl->color.g = m_flLightColor[1] * LIGHT_SCALE;
	dl->color.b = m_flLightColor[2] * LIGHT_SCALE;
	dl->radius = 64;
	dl->die = gpGlobals->curtime + 0.1;
	dl->decay = 512;
	dl->style = 1;
}