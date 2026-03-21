#include "cbase.h"
#include "model_types.h"
#include "clienteffectprecachesystem.h"
#include "fx.h"
#include "c_te_effect_dispatch.h"
#include "beamdraw.h"
#include "c_weapon__stubs.h"
#include "functionproxy.h"
#include "c_basehlcombatweapon.h"
#include "iefx.h"
#include "beam_shared.h"

#define	LASER_BEAM_SPRITE	"effects/laser1.vmt"

#define	LASER_BEAM_RANGE		512
#define	LASER_BEAM_BRIGHTNESS	50
#define	LASER_BEAM_WIDTH		5.0f

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

class C_WeaponSniperRifle : public C_BaseHLCombatWeapon
{
	DECLARE_CLASS( C_WeaponSniperRifle, C_BaseHLCombatWeapon );
public:
	C_WeaponSniperRifle() {}

	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

private:
	C_WeaponSniperRifle( const C_WeaponSniperRifle &other ) {}

private:
	void			GetLaserVectors( Vector &start, Vector &end);
	void			GetLaserColors( float &r, float &g, float &b, float &a );

	void			UpdateLaserBeam( Vector start, Vector end, float r, float g, float b, float a);

private:

	// CNetworkVars
	int				m_iLaserAttachment;
	int				m_iLaserDotSprite;

	float			m_flLaserColor[3];
	float			m_flLaserBrightness[3];

private:
	CBeam			*m_pLaser;
	float			m_flPrevLaserBrightness;
};


STUB_WEAPON_CLASS_IMPLEMENT( weapon_sniperrifle, C_WeaponSniperRifle );

IMPLEMENT_CLIENTCLASS_DT( C_WeaponSniperRifle, DT_WeaponSniperRifle, CWeaponSniperRifle )
	RecvPropInt( RECVINFO( m_iLaserAttachment ) ),
	RecvPropInt( RECVINFO( m_iLaserDotSprite ) ),
	RecvPropArray( RecvPropFloat( RECVINFO( m_flLaserColor[0] ) ), m_flLaserColor ),
	RecvPropArray( RecvPropFloat( RECVINFO( m_flLaserBrightness[0] ) ), m_flLaserBrightness ),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_WeaponSniperRifle::GetLaserColors( float &r, float &g, float &b, float &a )
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
void C_WeaponSniperRifle::GetLaserVectors( Vector &start, Vector &end)
{
	// Get eye position / angles
	matrix3x4_t eyeToWorld;
	GetAttachment( m_iLaserAttachment, eyeToWorld );

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
void C_WeaponSniperRifle::UpdateLaserBeam( Vector start, Vector end, float r, float g, float b, float a)
{
//	float flDist = (start - end).Length();
//	float fraction = flDist / LASER_BEAM_RANGE;

	m_pLaser->PointsInit(end, start);
	m_pLaser->SetColor(r, g, b);
	m_pLaser->SetBrightness(a * LASER_BEAM_BRIGHTNESS);
	m_pLaser->SetNoise(0);
	m_pLaser->SetWidth(LASER_BEAM_WIDTH/* * (1.0f - fraction)*/);
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
}