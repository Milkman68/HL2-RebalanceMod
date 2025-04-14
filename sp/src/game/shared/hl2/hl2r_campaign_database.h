#ifndef hl2r_campaign_database
#define hl2r_campaign_database
#ifdef _WIN32
#pragma once
#endif

#include "tier0/icommandline.h"

//-----------------------------------------------------------------------------
// Campaign Database:
//-----------------------------------------------------------------------------
enum EGameType
{
	GAME_INVALID = -1,

	GAME_HL2,
	GAME_EPISODE_1,
	GAME_EPISODE_2,
};

enum EMountReturnCode
{
	SUCESSFULLY_MOUNTED = 0,
	FAILED_TO_UNPACK_VPK,
};

struct CampaignData_t
{
	char	id[16];		// A 10-digit string of numbers that a campaign's folder is named with. 
	char	name[256];	// What this campaign is labeled as.
	int		game;		// What game is this campaign for?
	bool	mounted;	// Is this the currently mounted campaign?
	bool	visible;	// Left as 0 if this campaign is no longer installed.
	bool	invalid;	// Is 1 if this campaign is actually a mod.
};

class CCampaignDatabase
{
public:
	CCampaignDatabase();

	bool IsCampaignLoaderMod();

	void WriteListToScript( void );
	void WriteScriptToList( void );

	CampaignData_t*	GetCampaignData( int index );
	CampaignData_t*	GetCampaignDataFromID(const char *id);

	KeyValues*		GetKeyValuesFromData( int index );
	int				GetCampaignCount( void );

#ifdef CLIENT_DLL
	// Mounting functions:
	EMountReturnCode MountCampaign(const char *pFileName);
#endif

private:

	KeyValues *pCampaignScript;
	CUtlVector<CampaignData_t>	m_Campaigns;

	bool		IsCampaignDirectory( const char *filename);
	void		BuildCampaignScript( void );
	KeyValues	*GetCampaignScript( void );

#ifdef CLIENT_DLL
	// Mounting functions:
	const char	*GetSteamAppsDir(void);
	bool		UnpackCampaignVPK(const char *pFileName);
#endif
};

CCampaignDatabase *GetCampaignDatabase();

#endif