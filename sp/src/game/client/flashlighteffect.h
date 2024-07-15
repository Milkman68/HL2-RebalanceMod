//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef FLASHLIGHTEFFECT_H
#define FLASHLIGHTEFFECT_H
#ifdef _WIN32
#pragma once
#endif

struct dlight_t;


class CFlashlightEffect
{
public:

	CFlashlightEffect(int nEntIndex = 0);
	virtual ~CFlashlightEffect();

	virtual void UpdateLight(const Vector &vecPos, const Vector &vecDir, const Vector &vecRight, const Vector &vecUp, int nDistance);
	void TurnOn();
	void TurnOff();
	bool IsOn( void ) { return m_bIsOn;	}

	ClientShadowHandle_t GetFlashlightHandle( void ) { return m_FlashlightHandle; }
	void SetFlashlightHandle( ClientShadowHandle_t Handle ) { m_FlashlightHandle = Handle;	}
	
protected:

	void LightOff();
	void LightOffOld();
	void LightOffNew();

	void UpdateLightNew(const Vector &vecPos, const Vector &vecDir, const Vector &vecRight, const Vector &vecUp);
	void UpdateLightOld(const Vector &vecPos, const Vector &vecDir, int nDistance);

	bool m_bIsOn;
	int m_nEntIndex;
	ClientShadowHandle_t m_FlashlightHandle;

	// Vehicle headlight dynamic light pointer
	dlight_t *m_pPointLight;
	float m_flDistMod;

	// Texture for flashlight
	CTextureReference m_FlashlightTexture;
};

class CHeadlightEffect : public CFlashlightEffect
{
public:
	
	CHeadlightEffect();
	~CHeadlightEffect();

	virtual void UpdateLight(const Vector &vecPos, const Vector &vecDir, const Vector &vecRight, const Vector &vecUp, int nDistance);
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

class CProjMuzzleFlashEffect : public CFlashlightEffect
{
public:
	
	CProjMuzzleFlashEffect();
	~CProjMuzzleFlashEffect();

	void UpdateLight( const FlashlightState_t &state );
};

class C_ProjMuzzleFlash : public C_BaseEntity
{
public:
	
	C_ProjMuzzleFlash();
	~C_ProjMuzzleFlash();
	
	DECLARE_CLASS( C_ProjMuzzleFlash, C_BaseEntity );
	
	bool ShouldDraw();
	
public:

	virtual void Simulate( void );
	
	float 	die;
	float	holdtime;
	float 	fov;
	int 	range;
	float 	color[4];
	float	clq[3];
	
private:
	void Update( void );
	void Stop( void );
	
private:
	
	float 					m_flDuration;
	float 					m_flHoldDuration;
	CProjMuzzleFlashEffect 	*m_pMuzzleFlash;
};



#endif // FLASHLIGHTEFFECT_H
