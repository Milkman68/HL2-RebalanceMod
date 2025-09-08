#include "cbase.h"
#include "c_ai_basenpc.h"
#include "beam_shared.h"
#include "engine\ivdebugoverlay.h"
#include "iviewrender_beams.h"
#include "dlight.h"
#include "iefx.h"
#include "beam_shared.h"

#define	LASER_BEAM_SPRITE	"effects/laser1.vmt"

#define	LASER_BEAM_RANGE		512
#define	LASER_BEAM_BRIGHTNESS	100
#define	LASER_BEAM_WIDTH		5.0f

//#define	LASER_DOT_SCALE		5.0f

//Eye states
enum eyeState_t
{
	TURRET_EYE_SEE_TARGET,			//Sees the target, bright and big
	TURRET_EYE_SEEKING_TARGET,		//Looking for a target, blinking (bright)
	TURRET_EYE_DORMANT,				//Not active
	TURRET_EYE_DEAD,				//Completely invisible
	TURRET_EYE_DISABLED,			//Turned off, must be reactivated before it'll deploy again (completely invisible)
	TURRET_EYE_ALARM,				// On side, but warning player to pick it back up
};
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class C_TurretFloor : public C_AI_BaseNPC
{
	DECLARE_CLASS( C_TurretFloor, C_AI_BaseNPC );
public:
	DECLARE_CLIENTCLASS();

	virtual void	Spawn( void );
	virtual void	ClientThink( void );

private:
	void			GetLaserVectors( Vector &start, Vector &end);
	void			GetLaserColors( float &r, float &g, float &b, float &a );

	void			UpdateLaserBeam( Vector start, Vector end, float r, float g, float b, float a);

private:

	// CNetworkVars
	int				m_iEyeAttachment;
	int				m_iLaserDotSprite;

	float			m_flLaserColor[3];
	float			m_flLaserBrightness[3];

private:
	CBeam			*m_pLaser;
	float			m_flPrevLaserBrightness;
};

IMPLEMENT_CLIENTCLASS_DT( C_TurretFloor, DT_FloorTurret, CNPC_FloorTurret )
	RecvPropInt( RECVINFO( m_iEyeAttachment ) ),
	RecvPropInt( RECVINFO( m_iLaserDotSprite ) ),
	RecvPropArray( RecvPropFloat( RECVINFO( m_flLaserColor[0] ) ), m_flLaserColor ),
	RecvPropArray( RecvPropFloat( RECVINFO( m_flLaserBrightness[0] ) ), m_flLaserBrightness ),
END_RECV_TABLE()
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TurretFloor::Spawn( void )
{
	SetNextClientThink( CLIENT_THINK_ALWAYS );

	m_pLaser = CBeam::BeamCreate(LASER_BEAM_SPRITE, LASER_BEAM_WIDTH);
	BaseClass::Spawn();
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TurretFloor::ClientThink( void )
{
	if ( engine->IsPaused() )
		return;

	float r, g, b, a;
	GetLaserColors(r, g, b, a);
	if ( ( r + g + b ) <= 0 )
		return;

	if ( a == 0 )
		return;

	Vector vecStart, vecEnd;
	GetLaserVectors(vecStart, vecEnd);

	// Laser Beam
	UpdateLaserBeam( vecStart, vecEnd, r, g, b, a );
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TurretFloor::GetLaserColors( float &r, float &g, float &b, float &a )
{
	// Handle brightness:
	float flTransitionTime = m_flLaserBrightness[1];
	float flTransitionStart = m_flLaserBrightness[2];

	bool bInTransition = flTransitionStart + flTransitionTime > gpGlobals->curtime;
	if ( bInTransition )
	{
		float prevA = m_flPrevLaserBrightness;
		float newA = m_flLaserBrightness[0];

		float deltaTime = (float)( gpGlobals->curtime - flTransitionStart ) / flTransitionTime;
		a = SimpleSplineRemapValClamped( deltaTime, 0.0f, 1.0f, prevA, newA );
	}
	else
	{
		m_flPrevLaserBrightness = m_flLaserBrightness[0];
		a = m_flLaserBrightness[0];
	}

	// Color
	r = m_flLaserColor[0];
	g = m_flLaserColor[1];
	b = m_flLaserColor[2];
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TurretFloor::GetLaserVectors( Vector &start, Vector &end)
{
	// Get eye position / angles
	matrix3x4_t eyeToWorld;
	GetAttachment( m_iEyeAttachment, eyeToWorld );

	Vector	vecEye, vecEyeDir;

	MatrixGetColumn( eyeToWorld, 3, vecEye );
	MatrixGetColumn( eyeToWorld, 0, vecEyeDir );

	// Do a traceline for the laser's endpoint
	Vector vecEndPos = vecEye + (vecEyeDir * LASER_BEAM_RANGE);

	trace_t tr;
	UTIL_TraceLine( vecEye, vecEndPos, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

	start = vecEye;
	end = tr.endpos;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TurretFloor::UpdateLaserBeam( Vector start, Vector end, float r, float g, float b, float a)
{
	float flDist = (start - end).Length();
	float fraction = flDist / LASER_BEAM_RANGE;

	m_pLaser->PointsInit(end, start);
	m_pLaser->SetColor(r, g, b);
	m_pLaser->SetBrightness(a * LASER_BEAM_BRIGHTNESS);
	m_pLaser->SetNoise(0);
	m_pLaser->SetWidth(LASER_BEAM_WIDTH * (1.0f - fraction));
	m_pLaser->SetEndWidth(LASER_BEAM_WIDTH);
	m_pLaser->SetScrollRate(0);
	m_pLaser->SetFadeLength(0);
//	m_pLaser->SetHaloTexture(m_iLaserDotSprite);
//	m_pLaser->SetHaloScale(LASER_DOT_SCALE * (1.0f - fraction));
	m_pLaser->SetHaloScale(0);
	m_pLaser->SetCollisionGroup(COLLISION_GROUP_NONE);
	m_pLaser->SetStartEntity(this);

	Vector vecDir = (end - start);
	VectorNormalize(vecDir);

	dlight_t *dl = effects->CL_AllocDlight ( index );
	dl->origin = start + (vecDir * MIN(32, flDist));
	dl->color.r = r * a;
	dl->color.g = g * a;
	dl->color.b = b * a;
	dl->color.exponent = -3;
	dl->radius = 128;
	dl->die = gpGlobals->curtime + 0.01;
	dl->decay = 512;
}