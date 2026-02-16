//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Weapons Resource implementation
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "history_resource.h"
#include <vgui_controls/Controls.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include "c_baseplayer.h"
#include "hud.h"
#include "ammodef.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

WeaponsResource gWR;
extern ConVar hl2r_ammo_labels;

void FreeHudTextureList( CUtlDict< CHudTexture *, int >& list );

static CHudTexture *FindHudTextureInDict( CUtlDict< CHudTexture *, int >& list, const char *psz )
{
	int idx = list.Find( psz );
	if ( idx == list.InvalidIndex() )
		return NULL;

	return list[ idx ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
WeaponsResource::WeaponsResource( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
WeaponsResource::~WeaponsResource( void )
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void WeaponsResource::Init( void )
{
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WeaponsResource::Reset( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Load all the sprites needed for all registered weapons
//-----------------------------------------------------------------------------
void WeaponsResource::LoadAllWeaponSprites( void )
{
	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if ( !player )
		return;

	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		if ( player->GetWeapon(i) )
		{
			LoadWeaponSprites( player->GetWeapon(i)->GetWeaponFileInfoHandle() );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WeaponsResource::LoadWeaponSprites( WEAPON_FILE_INFO_HANDLE hWeaponFileInfo )
{
	// WeaponsResource is a friend of C_BaseCombatWeapon
	FileWeaponInfo_t *pWeaponInfo = GetFileWeaponInfoFromHandle( hWeaponFileInfo );

	if ( !pWeaponInfo )
		return;

	// Already parsed the hud elements?
	if ( pWeaponInfo->bLoadedHudElements )
		return;

	pWeaponInfo->bLoadedHudElements = true;

	pWeaponInfo->iconActive = NULL;
	pWeaponInfo->iconInactive = NULL;
	pWeaponInfo->iconAmmo = NULL;
	pWeaponInfo->iconAmmo2 = NULL;
	pWeaponInfo->iconCrosshair = NULL;
	pWeaponInfo->iconAutoaim = NULL;
	pWeaponInfo->iconSmall = NULL;

	char sz[128];
	Q_snprintf(sz, sizeof( sz ), "scripts/%s", pWeaponInfo->szClassName);

	CUtlDict< CHudTexture *, int > tempList;
	LoadHudTextures( tempList, sz, g_pGameRules->GetEncryptionKey() );
	
	if ( tempList.Count() )
	{
		// Check for override scripts.
		Q_strncat(sz, "_override", sizeof(sz), COPY_ALL_CHARACTERS);
		
		CUtlDict< CHudTexture *, int > tempList2;
		LoadHudTextures( tempList2, sz, g_pGameRules->GetEncryptionKey() );
		
		if ( tempList2.Count() )
		{
			int numElements = tempList.Count();
			for ( int i = 0; i < numElements; i++ )
			{
				int iOverrideElementIndex = tempList2.Find( tempList.GetElementName(i) );
				if ( iOverrideElementIndex != tempList.InvalidIndex() )
				{
					tempList[i] = tempList2[iOverrideElementIndex];
		//			DevMsg("Overriden variable is: %s\n",  tempList.GetElementName(i) );
				}
			}
		}
	}

	if ( !tempList.Count() )
	{
		// no sprite description file for weapon, use default small blocks
		pWeaponInfo->iconActive = gHUD.GetIcon( "selection" );
		pWeaponInfo->iconInactive = gHUD.GetIcon( "selection" );
		pWeaponInfo->iconAmmo = gHUD.GetIcon( "bucket1" );
		return;
	}

	CHudTexture *p;
	p = FindHudTextureInDict( tempList, "crosshair" );
	if ( p )
	{
		pWeaponInfo->iconCrosshair = gHUD.AddUnsearchableHudIconToList( *p );
	}

	p = FindHudTextureInDict( tempList, "autoaim" );
	if ( p )
	{
		pWeaponInfo->iconAutoaim = gHUD.AddUnsearchableHudIconToList( *p );
	}

	p = FindHudTextureInDict( tempList, "zoom" );
	if ( p )
	{
		pWeaponInfo->iconZoomedCrosshair = gHUD.AddUnsearchableHudIconToList( *p );
	}
	else
	{
		pWeaponInfo->iconZoomedCrosshair = pWeaponInfo->iconCrosshair; //default to non-zoomed crosshair
	}

	p = FindHudTextureInDict( tempList, "zoom_autoaim" );
	if ( p )
	{
		pWeaponInfo->iconZoomedAutoaim = gHUD.AddUnsearchableHudIconToList( *p );
	}
	else
	{
		pWeaponInfo->iconZoomedAutoaim = pWeaponInfo->iconZoomedCrosshair;  //default to zoomed crosshair
	}

	CHudHistoryResource *pHudHR = GET_HUDELEMENT( CHudHistoryResource );	
	if( pHudHR )
	{
		p = FindHudTextureInDict( tempList, "weapon" );
		if ( p )
		{
			pWeaponInfo->iconInactive = gHUD.AddUnsearchableHudIconToList( *p );
			if ( pWeaponInfo->iconInactive )
			{
				pWeaponInfo->iconInactive->Precache();
				pHudHR->SetHistoryGap( pWeaponInfo->iconInactive->Height() );
			}
		}

		p = FindHudTextureInDict( tempList, "weapon_s" );
		if ( p )
		{
			pWeaponInfo->iconActive = gHUD.AddUnsearchableHudIconToList( *p );
			if ( pWeaponInfo->iconActive )
			{
				pWeaponInfo->iconActive->Precache();
			}
		}

		p = FindHudTextureInDict( tempList, "weapon_small" );
		if ( p )
		{
			pWeaponInfo->iconSmall = gHUD.AddUnsearchableHudIconToList( *p );
			if ( pWeaponInfo->iconSmall )
			{
				pWeaponInfo->iconSmall->Precache();
			}
		}

		p = FindHudTextureInDict( tempList, "ammo" );
		if ( p )
		{
			pWeaponInfo->iconAmmo = gHUD.AddUnsearchableHudIconToList( *p );
			if ( pWeaponInfo->iconAmmo )
			{
				pWeaponInfo->iconAmmo->Precache();
				pHudHR->SetHistoryGap( pWeaponInfo->iconAmmo->Height() );
				CacheWeaponAmmoIcon( pWeaponInfo->iAmmoType, pWeaponInfo->iconAmmo);
			}
		}

		p = FindHudTextureInDict( tempList, "ammo2" );
		if ( p )
		{
			pWeaponInfo->iconAmmo2 = gHUD.AddUnsearchableHudIconToList( *p );
			if ( pWeaponInfo->iconAmmo2 )
			{
				pWeaponInfo->iconAmmo2->Precache();
				pHudHR->SetHistoryGap( pWeaponInfo->iconAmmo2->Height() );
				CacheWeaponAmmoIcon( pWeaponInfo->iAmmo2Type, pWeaponInfo->iconAmmo2);
			}
		}
	}

	FreeHudTextureList( tempList );
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void WeaponsResource::CacheWeaponAmmoIcon(int iAmmoType, CHudTexture *pIcon)
{
	if ( m_cachedIcons.Count() )
	{
		for ( int i = 0; i < m_cachedIcons.Count(); i++ )
		{
			if ( m_cachedIcons[i]->ammotype == iAmmoType )
				return;
		}
	}

	cachedIcon_t *icon = new cachedIcon_t(iAmmoType, pIcon);
	m_cachedIcons.AddToTail(icon);
}
//-----------------------------------------------------------------------------
// Purpose: Helper function to return a Ammo pointer from id
//-----------------------------------------------------------------------------
CHudTexture *WeaponsResource::GetAmmoIconFromWeapon( int iAmmoId )
{
	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if ( !player )
		return NULL;

	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		C_BaseCombatWeapon *weapon = player->GetWeapon( i );
		if ( !weapon )
			continue;

		if ( weapon->GetPrimaryAmmoType(CBaseCombatWeapon::INDEX_CARRY) == iAmmoId )
		{
			return weapon->GetWpnData().iconAmmo;
		}
		else if ( weapon->GetSecondaryAmmoType() == iAmmoId )
		{
			return weapon->GetWpnData().iconAmmo2;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Helper function to return a Ammo pointer from id
//-----------------------------------------------------------------------------
CHudTexture *WeaponsResource::GetAmmoIconFromWeapon_Cached( int iAmmoId )
{
	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if ( !player )
		return NULL;

	if ( m_cachedIcons.Count() )
	{
		for ( int i = 0; i < m_cachedIcons.Count(); i++ )
		{
			if ( m_cachedIcons[i]->ammotype == iAmmoId )
				return m_cachedIcons[i]->icon;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Get a pointer to a weapon using this ammo
//-----------------------------------------------------------------------------
const FileWeaponInfo_t *WeaponsResource::GetWeaponFromAmmo( int iAmmoId )
{
	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if ( !player )
		return NULL;

	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		C_BaseCombatWeapon *weapon = player->GetWeapon( i );
		if ( !weapon )
			continue;

		if ( weapon->GetPrimaryAmmoType(CBaseCombatWeapon::INDEX_CARRY) == iAmmoId )
		{
			return &weapon->GetWpnData();
		}
		else if ( weapon->GetSecondaryAmmoType() == iAmmoId )
		{
			return &weapon->GetWpnData();
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Return a label string from an ammo id
//-----------------------------------------------------------------------------
char const *WeaponsResource::GetAmmoLabelFromID( int iAmmoId )
{
	// Setup our default label.
	char const *label = "#Valve_Hud_AMMO";
	
	if ( !hl2r_ammo_labels.GetBool() )
		return label;
	
	if ( iAmmoId == GetAmmoDef()->Index("pistol") )
		label = "#Valve_Hud_AMMO_pistol";
	
	if ( iAmmoId == GetAmmoDef()->Index("357") )
		label = "#Valve_Hud_AMMO_357";
	
	if ( iAmmoId == GetAmmoDef()->Index("smg1") )
		label = "#Valve_Hud_AMMO_smg1";
	
	if ( iAmmoId == GetAmmoDef()->Index("ar2") )
		label = "#Valve_Hud_AMMO_ar2";
	
	if ( iAmmoId == GetAmmoDef()->Index("buckshot") )
		label = "#Valve_Hud_AMMO_shotgun";
	
	if ( iAmmoId == GetAmmoDef()->Index("xbowbolt") )
		label = "#Valve_Hud_AMMO_crossbow";
	
	if ( iAmmoId == GetAmmoDef()->Index("rpg_round") )
		label = "#Valve_Hud_AMMO_rpg";
	
	if ( iAmmoId == GetAmmoDef()->Index("grenade") )
		label = "#Valve_Hud_AMMO_grenade";
	
	if ( iAmmoId == GetAmmoDef()->Index("slam") )
		label = "#Valve_Hud_AMMO_slam";

	if ( iAmmoId == GetAmmoDef()->Index("bugbait") )
		label = "#Valve_Hud_AMMO_bugbait";

	if ( iAmmoId == GetAmmoDef()->Index("hmg") )
		label = "#Valve_Hud_AMMO_hmg";

	if ( iAmmoId == GetAmmoDef()->Index("smg2") )
		label = "#Valve_Hud_AMMO_smg2";
	
	return label;
}

//-----------------------------------------------------------------------------
// Purpose: Helper function for getting an xoffset value from ammoIDs for history_resource
//-----------------------------------------------------------------------------
float WeaponsResource::GetAmmoDrawOffesetFromID( int iAmmoId )
{
	// Setup our default offset.
	float offset = 1.25f;
	
	if ( iAmmoId == GetAmmoDef()->Index("pistol") )
		offset = 1.25f;
	
	if ( iAmmoId == GetAmmoDef()->Index("357") )
		offset = 1.25f;
	
	if ( iAmmoId == GetAmmoDef()->Index("smg1") )
		offset = 1.25f;
	
	if ( iAmmoId == GetAmmoDef()->Index("ar2") )
		offset = 1.5f;
	
	if ( iAmmoId == GetAmmoDef()->Index("buckshot") )
		offset = 1.25f;
	
	if ( iAmmoId == GetAmmoDef()->Index("xbowbolt") )
		offset = 0.75f;
	
	if ( iAmmoId == GetAmmoDef()->Index("rpg_round") )
		offset = 0.75f;
	
	if ( iAmmoId == GetAmmoDef()->Index("grenade") )
		offset = 1.5f;
	
	if ( iAmmoId == GetAmmoDef()->Index("slam") )
		offset = 0.75f;
	
	return offset;
}

