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
	FAILED_TO_EXTRACT_VPK,
};

struct CampaignData_t
{
	char	id[16];		// A 10-digit string of numbers that a campaign's folder is named with. 
	char	name[256];	// What this campaign is labeled as.
	int		game;		// What game is this campaign for?
	bool	mounted;	// Is this the currently mounted campaign?
	bool	installed;	// Left as 0 if this campaign is no longer installed.
	CUtlVector<const char*> maplist;
};

class CCampaignDatabase
{
public:
	CCampaignDatabase();

	bool IsCampaignLoaderMod();
	bool HLExtractInstalled( void );

	void WriteListToScript( void );
	void WriteScriptToList( void );

	CampaignData_t*	GetCampaignData( int index );
	CampaignData_t*	GetCampaignDataFromID(const char *id);
	int				GetCampaignIndex( CampaignData_t *campaign );

	KeyValues*		GetKeyValuesFromCampaign( CampaignData_t *campaign );
	int				GetCampaignCount( void );

	EMountReturnCode MountCampaign(const char *pFileName);
	void			DoCampaignScan( void );

private:
	bool		PotentialCampaignVPK( const char *pAddonID);

	bool		ScanForMapsInVPK( const char *pAddonID, CUtlVector<const char *> *list );
	bool		IsMapReplacement( const char *pMap );

	const char	*GetSteamAppsDir(void);

	const char	*ParseCMD( const char *pCampaignID, const char *pAppend = NULL );
	const char	*GetOutputFromHLE( const char *pCampaignID, const char *pCommand );
	bool		ExtractVPK( const char *pCampaignID );

private:
	KeyValues *pCampaignScript;
	CUtlVector<CampaignData_t>	m_Campaigns;
};

CCampaignDatabase *GetCampaignDatabase();

#endif