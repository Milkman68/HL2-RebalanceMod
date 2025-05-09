#ifndef hl2r_campaign_database
#define hl2r_campaign_database
#ifdef _WIN32
#pragma once
#endif

#include "tier0/icommandline.h"

#define CAMPAIGN_NAME_LENGTH 128
#define CAMPAIGN_ID_LENGTH 11

#define HLE_MAX_OUTPUT_LENGTH 2048
#define HLE_MAX_CMD_LENGTH 1024

#define CAMPAIGN_INDEX_LENGTH 5		// Allows up to 99999 listed Campaigns.
#define CAMPAIGN_FILESIZE_LENGTH 5	// Allows a filesize up to 99999 MB.
#define CAMPAIGN_MAP_INDEX_LENGTH 3	// Allows up to 999 maps in a single campaign.

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

enum ESortType
{
	BY_NAME = 0,
	BY_SIZE,
	BY_DATE,
};

enum ESortDirection
{
	ASCENDING_ORDER = 0,
	DECENDING_ORDER
};

struct CampaignData_t
{
	char	id[CAMPAIGN_ID_LENGTH];		// A 10-digit string of numbers that a campaign's folder is named with. 
	char	name[CAMPAIGN_NAME_LENGTH];	// What this campaign is labeled as.
	int		game;		// What game is this campaign for?
	bool	mounted;	// Is this the currently mounted campaign?
	bool	installed;	// Left as 0 if this campaign is no longer installed.

	CUtlVector<const char*> maplist; // A list containing the names of all maps in this campaign.
	int		filesize;
	char	date;

};

struct CampaignSort_t
{
	ESortType	eType;
	ESortDirection	eDir;
};

class CCampaignDatabase
{
public:
	CCampaignDatabase();

	bool IsCampaignLoaderMod();
	bool HLExtractInstalled( void );

	// List accessor functions:
	void WriteListToScript( void );
	void WriteScriptToList( void );

	CampaignData_t*	GetCampaignData( int index );
	CampaignData_t*	GetCampaignDataFromID(const char *id);
	int				GetCampaignIndex( CampaignData_t *campaign );

	KeyValues*		GetKeyValuesFromCampaign( CampaignData_t *campaign );
	int				GetCampaignCount( void );

	// Sorting functions:
	void			SortCampaignList( ESortType sorttype, ESortDirection sortdir );

	ESortType		GetSortType() { return m_SortMethod.eType; }
	ESortDirection	GetSortDir() { return m_SortMethod.eDir; }

	// VPK accessor functions:
	EMountReturnCode MountCampaign(const char *pFileName);
	void			DoCampaignScan( void );

private:
	bool		PotentialCampaignVPK( const char *pAddonID);

	bool		ScanForMapsInVPK( const char *pAddonID, CUtlVector<const char *> *list );
	bool		IsMapReplacement( const char *pMap );

	int			GetVPKSize( const char *pAddonID);
	int			GetVPKDate( const char *pAddonID);

	const char	*GetSteamAppsDir(void);

	const char	*ParseCMD( const char *pAddonID, const char *pAppend = NULL );
	const char	*GetOutputFromHLE( const char *pAddonID, const char *pCommand );
	bool		ExtractVPK( const char *pAddonID );
	bool		MountExtractedFiles( const char *pCampaignID, const char *pDirectory );

private:
	KeyValues *pCampaignScript;
	CUtlVector<CampaignData_t *>	m_Campaigns;

	CampaignSort_t m_SortMethod;
};

CCampaignDatabase *GetCampaignDatabase();

#endif