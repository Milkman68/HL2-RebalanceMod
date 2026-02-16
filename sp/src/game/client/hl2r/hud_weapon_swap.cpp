#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "iclientmode.h"
#include "engine/IEngineSound.h"
#include "vgui_controls/AnimationController.h"
#include "vgui_controls/Controls.h"
#include "vgui_controls/Panel.h"
#include "vgui/ISurface.h"
#include "../hud_crosshair.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "ammodef.h"
#include "in_buttons.h"
#include "c_basehlplayer.h"

#include "vgui/ILocalize.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

/*
==================================================
CHUDWeaponSwap 
==================================================
*/

using namespace vgui;

class CHUDWeaponSwap : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHUDWeaponSwap, vgui::Panel );
public:
	CHUDWeaponSwap( const char *pElementName );

	bool ShouldDraw( void );
	virtual void OnThink();
	virtual void Paint();
	virtual void ApplySchemeSettings( IScheme *scheme );

private:

	enum eDrawPosition
	{
		POSITION_TOP = 0,
		POSITION_BOTTOM,
	};
	void	DrawWeaponBox( const CHudTexture *pIcon, int x, int y, int wide, int tall, bool bWarningClr, Color clr, eDrawPosition pos, const char *pText = NULL );
	void	UpdateWeaponIcons( void );
	void	DrawTextAtPos( int x, int y, int wide, int tall, const char *pText, Color col );

private:
	CPanelAnimationVar( float, m_flPrimaryBoxWide, "PrimaryBoxWide", "1.25" );
	CPanelAnimationVar( float, m_flPrimaryBoxTall, "PrimaryBoxTall", "1.25" );

	CPanelAnimationVar( float, m_flSecondaryBoxWide, "SecondaryBoxWide", "2.0" );
	CPanelAnimationVar( float, m_flSecondaryBoxTall, "SecondaryBoxTall", "2.0" );;

	CPanelAnimationVarAliasType( float, m_flBoxOffsetX, "BoxOffsetX", "300", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flBoxOffsetY, "BoxOffsetY", "0", "proportional_float" );

	CPanelAnimationVarAliasType( float, m_flBoxGapX, "BoxGapX", "0", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flBoxGapY, "BoxGapY", "100", "proportional_float" );

	CPanelAnimationVar( Color, m_BoxColor, "BoxColor", "SelectionBoxBg" );
	CPanelAnimationVar( Color, m_PrimaryBoxColor, "PrimaryBoxColor", "SelectionBoxBg" );
	CPanelAnimationVar( Color, m_SecondaryBoxColor, "SecondaryBoxColor", "SelectionBoxBg" );

	CPanelAnimationVarAliasType( float, m_flTextInsetY, "TextInsetY", "5", "proportional_float" );
	CPanelAnimationVar( vgui::HFont, m_hTextFont, "TextFont", "HudSelectionText" );

	const CHudTexture *m_pOldWeaponTexture;
	const CHudTexture *m_pNewWeaponTexture;

	bool bOldWeaponEmpty;
	bool bNewWeaponEmpty;

	char m_szNewWeaponPrintName[80];
	char m_szOldWeaponPrintName[80];

	CBaseEntity	*m_hReplacementWeapon;
};

DECLARE_HUDELEMENT( CHUDWeaponSwap );

CHUDWeaponSwap::CHUDWeaponSwap( const char *pElementName ) :CHudElement( pElementName ), BaseClass( NULL, "HUDWeaponSwap" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MISCSTATUS );
}

//-----------------------------------------------------------------------------
// Purpose: Save CPU cycles by letting the HUD system early cull
// costly traversal.  Called per frame, return true if thinking and 
// painting need to occur.
//-----------------------------------------------------------------------------
bool CHUDWeaponSwap::ShouldDraw( void )
{
	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if ( player == NULL )
		return false;

	return ( CHudElement::ShouldDraw() && !engine->IsDrawingLoadingImage() );
}

//-----------------------------------------------------------------------------
// Purpose: hud scheme settings
//-----------------------------------------------------------------------------
void CHUDWeaponSwap::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	SetPaintBackgroundEnabled(false);

	// set our size
	int screenWide, screenTall;
	int x, y;
	GetPos(x, y);
	GetHudSize(screenWide, screenTall);
	SetBounds( x, y, screenWide - x, screenTall - y );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHUDWeaponSwap::OnThink()
{
	C_BaseHLPlayer *player = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if ( player == NULL )
		return;

	if ( m_hReplacementWeapon != player->GetClosestReplacementWeapon() )
	{
		if ( player->GetClosestReplacementWeapon() != NULL )
		{
			// We've found a new weapon, fade in and update our hud
			UpdateWeaponIcons();

			SetAlpha(0);
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "FadeInWeaponSwapHUD" );
		}
		else
		{
			if ( m_hReplacementWeapon == NULL )
			{
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "CloseWeaponSwapHUD" );
			}
			else
			{
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "FadeOutWeaponSwapHUD" );
			}
		}
	}

	m_hReplacementWeapon = player->GetClosestReplacementWeapon();
	BaseClass::OnThink();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
// ADD TEXT TELLING HOW TO SWITCH!!!!
void CHUDWeaponSwap::Paint()
{
	C_BaseHLPlayer *player = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if ( player == NULL )
		return;

	if ( GetAlpha() > 0 && m_pOldWeaponTexture != NULL && m_pNewWeaponTexture != NULL )
	{
		int wide = MAX( m_pOldWeaponTexture->Width(), m_pNewWeaponTexture->Width() );
		int tall = MAX( m_pOldWeaponTexture->Height(), m_pNewWeaponTexture->Height() );

		// Draw the weapon boxes:
		DrawWeaponBox(m_pOldWeaponTexture, m_flBoxOffsetX, m_flBoxOffsetY, wide * m_flPrimaryBoxWide, tall * m_flPrimaryBoxTall, bOldWeaponEmpty, m_PrimaryBoxColor, POSITION_TOP, m_szOldWeaponPrintName);
		DrawWeaponBox(m_pNewWeaponTexture, m_flBoxOffsetX, m_flBoxOffsetY, wide * m_flSecondaryBoxWide, tall * m_flSecondaryBoxTall, bNewWeaponEmpty, m_SecondaryBoxColor, POSITION_BOTTOM, m_szNewWeaponPrintName );

		const CHudTexture *arrowIcon = gHUD.GetIcon("swap_arrow");
		if ( arrowIcon )
		{
			// Default sizes:
			float WidthScale  = ScreenWidth() / 2560.0;
			float HeightScale = ScreenHeight() / 1440.0;
					
			// Using only one seems to preserve shape better.
			float flScale = MAX(WidthScale, HeightScale) * 0.4f;

			// Get the center of the screen
			float fX, fY;
			bool bBehindCamera = false;
			CHudCrosshair::GetDrawPosition( &fX, &fY, &bBehindCamera );

			int yCenter = (int)fY - ( arrowIcon->Height()  *flScale ) / 2;
			int xCenter = (int)fX - ( arrowIcon->Width()  *flScale ) / 2;

			arrowIcon->DrawSelf(m_flBoxOffsetX + xCenter, m_flBoxOffsetY + yCenter, arrowIcon->Width() * flScale, arrowIcon->Height() * flScale, gHUD.m_clrNormal);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHUDWeaponSwap::DrawWeaponBox(const CHudTexture *pIcon, int xoffset, int yoffset, int wide, int tall, bool bWarningClr, Color clr, eDrawPosition pos, const char *pText )
{
	if ( !pIcon )
		return;

	int iSeparationOffsetX = 0;
	int iSeparationOffsetY = 0;

	if ( pos == POSITION_TOP )
	{
		if ( m_flBoxGapX > 0 )
		{
			iSeparationOffsetX = ( -MAX(pIcon->Width(), wide) ) / 2;
			iSeparationOffsetX -= m_flBoxGapX;
		}

		if ( m_flBoxGapY > 0 )
		{
			iSeparationOffsetY = ( -MAX(pIcon->Height(), tall) ) / 2;
			iSeparationOffsetY -= m_flBoxGapY;
		}
	}
	else
	{
		if ( m_flBoxGapX > 0 )
		{
			iSeparationOffsetX = ( MAX(pIcon->Width(), wide) ) / 2;
			iSeparationOffsetX += m_flBoxGapX;
		}

		if ( m_flBoxGapY > 0 )
		{
			iSeparationOffsetY = ( MAX(pIcon->Height(), tall) ) / 2;
			iSeparationOffsetY += m_flBoxGapY;
		}
	}

	clr = bWarningClr ? gHUD.m_clrCaution : clr;

	// Get the center of the screen
	float fX, fY;
	bool bBehindCamera = false;
	CHudCrosshair::GetDrawPosition( &fX, &fY, &bBehindCamera );

	// Icon:
	float flWidth = pIcon->Width();
	float flHeight = pIcon->Height();

	int xCenter = fX - flWidth / 2;
	int yCenter = fY - flHeight / 2;
	pIcon->DrawSelf(xCenter + xoffset + iSeparationOffsetX, yCenter + yoffset + iSeparationOffsetY, flWidth, flHeight, clr);

	// Selection box:
	flWidth = MAX( pIcon->Width(), wide);
	flHeight = MAX( pIcon->Height(), tall);

	xCenter = fX - flWidth / 2;
	yCenter = fY - flHeight / 2;
	DrawBox(xCenter + xoffset + iSeparationOffsetX, yCenter + yoffset + iSeparationOffsetY, flWidth, flHeight, m_BoxColor, 255); // HACK

	if ( pText != NULL ) 
		DrawTextAtPos(xCenter + xoffset + iSeparationOffsetX, yCenter + yoffset + iSeparationOffsetY, flWidth, flHeight, pText, clr );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHUDWeaponSwap::UpdateWeaponIcons( void )
{
	C_BaseHLPlayer *player = (C_BaseHLPlayer*)C_BasePlayer::GetLocalPlayer();

	C_BaseCombatWeapon *pNewWeapon = dynamic_cast<C_BaseCombatWeapon*>(player->GetClosestReplacementWeapon());
	C_BaseCombatWeapon *pOldWeapon = player->Weapon_GetSlot(pNewWeapon->GetSlot(), pNewWeapon->GetPosition(true));

	m_pNewWeaponTexture = pNewWeapon->GetWpnData().iconSmall;
	m_pOldWeaponTexture = pOldWeapon->GetWpnData().iconSmall;

	bool bHasPrimaryAmmoForWeapon = player->GetAmmoCount(pNewWeapon->GetPrimaryAmmoType(CBaseCombatWeapon::INDEX_CARRY));
	bool bHasSecondaryAmmoForWeapon = player->GetAmmoCount(pNewWeapon->GetSecondaryAmmoCount());

	// Check ammo for new weapon
	bNewWeaponEmpty = !pNewWeapon->HasAnyAmmo() && !bHasPrimaryAmmoForWeapon && !bHasSecondaryAmmoForWeapon;

	bHasPrimaryAmmoForWeapon = player->GetAmmoCount(pOldWeapon->GetPrimaryAmmoType(CBaseCombatWeapon::INDEX_CARRY));
	bHasSecondaryAmmoForWeapon = player->GetAmmoCount(pOldWeapon->GetSecondaryAmmoCount());

	// Check ammo for old weapon
	bOldWeaponEmpty = !pOldWeapon->HasAnyAmmo() && bHasPrimaryAmmoForWeapon && bHasSecondaryAmmoForWeapon;

	V_strcpy(m_szNewWeaponPrintName, pNewWeapon->GetWpnData().szPrintName );
	V_strcpy(m_szOldWeaponPrintName, pOldWeapon->GetWpnData().szPrintName );
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHUDWeaponSwap::DrawTextAtPos( int x, int y, int wide, int tall, const char *pText, Color col )
{
	wchar_t text[128];
	wchar_t *tempString = g_pVGuiLocalize->Find(pText);

	// setup our localized string
	if (tempString)
	{
#ifdef WIN32
		_snwprintf(text, sizeof(text) / sizeof(wchar_t) - 1, L"%s", tempString);
#else
		_snwprintf(text, sizeof(text) / sizeof(wchar_t) - 1, L"%S", tempString);
#endif
		text[sizeof(text) / sizeof(wchar_t) - 1] = 0;
	}
	else
	{
		// string wasn't found by g_pVGuiLocalize->Find()
		g_pVGuiLocalize->ConvertANSIToUnicode(pText, text, sizeof(text));
	}

	surface()->DrawSetTextColor(col);
	surface()->DrawSetTextFont(m_hTextFont);

	// count the position
	int slen = 0, charCount = 0, maxslen = 0;
	int firstslen = 0;
	{
		for (wchar_t *pch = text; *pch != 0; pch++)
		{
			if (*pch == '\n')
			{
				// newline character, drop to the next line
				if (slen > maxslen)
				{
					maxslen = slen;
				}
				if (!firstslen)
				{
					firstslen = slen;
				}

				slen = 0;
			}
			else if (*pch == '\r')
			{
				// do nothing
			}
			else
			{
				slen += surface()->GetCharacterWidth(m_hTextFont, *pch);
				charCount++;
			}
		}
	}
	if (slen > maxslen)
	{
		maxslen = slen;
	}
	if (!firstslen)
	{
		firstslen = maxslen;
	}

	int tx = x + ((wide - firstslen) / 2);
	int ty = y + tall - surface()->GetFontTall(m_hTextFont) - m_flTextInsetY;

	int charcount_newline_scan = charCount;
	for (wchar_t *pch = text; charcount_newline_scan > 0; pch++)
	{
		if (*pch == '\n')
			ty -= surface()->GetFontTall(m_hTextFont);

		charcount_newline_scan--;
	}

	surface()->DrawSetTextPos(tx, ty);

	// adjust the charCount by the scan amount
	charCount *= 1.0f; // ORIGNIAL: m_flTextScan
	for (wchar_t *pch = text; charCount > 0; pch++)
	{
		if (*pch == '\n')
		{
			// newline character, move to the next line
			surface()->DrawSetTextPos(x + ((wide - slen) / 2), ty + (surface()->GetFontTall(m_hTextFont)  *1.1f));
		}
		else if (*pch == '\r')
		{
			// do nothing
		}
		else
		{
			surface()->DrawUnicodeChar(*pch);
			charCount--;
		}
	}
}
