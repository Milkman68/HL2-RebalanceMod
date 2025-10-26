#ifndef hl2r_campaign_database
#define hl2r_campaign_database
#ifdef _WIN32
#pragma once
#endif

#include "tier0/icommandline.h"

#define CAMPAIGN_HANDLE short int

// Campaign accessor macros:
#define GetCampaign(index) CampaignList()->Element(index)
#define GetCampaignFromID(id) CampaignList()->Element( GetCampaignDatabase()->GetCampaignHandleFromID(id) )
#define GetMountedCampaign() CampaignList()->Element( GetCampaignDatabase()->GetMountedCampaignHandle() )

#define IsWorkshopCampaignMounted() GetCampaignDatabase()->ValidCampaign( GetCampaignDatabase()->GetMountedCampaignHandle() )

// Directories:
#define CAMPAIGN_MOUNT_DIR						"campaign_launcher\\mounted"

#define CAMPAIGN_LAUCHER_SAVE_STORE_DIR			"save\\campaign_launcher"
#define DEFAULT_GAME_SAVE_STORE_DIR				"save\\default"

#define CAMPAIGN_LAUCHER_CONTENT_DIR			"campaign_launcher\\content"
#define CAMPAIGN_LAUCHER_CONTENT_DISABLED_DIR	"campaign_launcher\\content_disabled"

// Filepaths:
#define CAMPAIGN_LAUCHER_HLEXTRACT_DIR	"campaign_launcher\\hllib\\bin\\x64\\HLExtract.exe"
#define CAMPAIGN_SCRIPT_FILE			"campaign_launcher\\campaigns.txt"
#define CAMPAIGN_BLACKLIST_FILE			"campaign_launcher\\file_blacklist.txt"

// Global string sizes:
#define CAMPAIGN_INDEX_LENGTH		5	// Allows up to 99999 listed Campaigns.
#define CAMPAIGN_FILESIZE_LENGTH	5	// Allows a filesize up to 99999 MB.
#define CAMPAIGN_MAP_INDEX_LENGTH	3	// Allows up to 999 maps in a single campaign.
#define CAMPAIGN_MAX_MAP_NAME		128

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
	FAILED_TO_MOUNT_FILES,
	FAILED_TO_TRANSFER_GAMEINFO,
	FAILED_TO_STORE_SAVE_FILES,
	FAILED_TO_RETRIEVE_SAVE_FILES,
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

enum EMoveSaveFileType
{
	STORE_TO_DEFAULT = 0,
	STORE_TO_CAMPAIGN,
	RETRIEVE_FROM_DEFAULT,
	RETRIEVE_FROM_CAMPAIGN
};

struct CampaignDateTable_t
{
	char	minute[8];
	char	hour[8];
	char	period[4];
	char	day[8];
	char	month[8];
	char	year[8];

	void CopyTable( CampaignDateTable_t *newTable )
	{
		if ( newTable == NULL )
			return;

		V_strcpy_safe(minute,	newTable->minute);
		V_strcpy_safe(hour,		newTable->hour);
		V_strcpy_safe(period,	newTable->period);
		V_strcpy_safe(day,		newTable->day);
		V_strcpy_safe(month,	newTable->month);
		V_strcpy_safe(year,		newTable->year);
	}
};

#define CAMPAIGN_NAME_LENGTH 128
#define CAMPAIGN_ID_LENGTH 11
struct CampaignData_t
{
	char	id[CAMPAIGN_ID_LENGTH];		// A 10-digit string of numbers that a campaign's folder is named with. 
	char	name[CAMPAIGN_NAME_LENGTH];	// What this campaign is labeled as.
	int		game;		// What game is this campaign for?
	bool	mounted;	// Is this the currently mounted campaign?
	bool	installed;	// Left as 0 if this campaign is no longer installed.
	CUtlVector<const char*> maplist; // A list containing the names of all maps in this campaign.
	int		startingmap;
	int		filesize;
	CampaignDateTable_t	*datetable;

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

	EMountReturnCode	MountCampaign(const char *pCampaignID);
	void				UnmountMountedCampaign( void );

	void				DoCampaignScan( void );
	bool				HLExtractInstalled( void );

	// List accessor functions:
	CUtlVector<CampaignData_t *>	*CampaignList( void ) { return m_Campaigns; };

	CAMPAIGN_HANDLE		GetCampaignHandleFromID( const char *id );
	CAMPAIGN_HANDLE		GetMountedCampaignHandle( void );
	KeyValues*			GetKeyValuesFromCampaign( CAMPAIGN_HANDLE campaign );

	bool				ValidCampaign( CAMPAIGN_HANDLE campaign ) { return campaign >= 0; }
	int					GetCampaignCount( void )	{ return CampaignList()->Count(); }
	
	// Campaign script file:
	void	WriteListToScript( void );
	void	WriteScriptToList( void );

	// Sorting:
	void			SortCampaignList( ESortType sorttype, ESortDirection sortdir );
	ESortType		GetSortType() { return m_SortMethod.eType; }
	ESortDirection	GetSortDir() { return m_SortMethod.eDir; }

	// Nodegraph hotfix:
	void	FlushMountedCampaignGraphs( void );

private:

	// File dating:
	int						GetVPKSize( const char *pWorkshopModID );
	CampaignDateTable_t		*GetVPKDate( const char *pWorkshopModID );

	// VPK Scanning:
	bool		PotentialCampaignVPK( const char *pWorkshopModID );
	bool		ScanForMapsInVPK( const char *pWorkshopModID, CUtlVector<const char *> *list );
	bool		IsMapReplacement( const char *pMap );

	const char	*ParseCMD( const char *pWorkshopModID, const char *pAppend = NULL );
	const char	*GetOutputFromHLE( const char *pWorkshopModID, const char *pCommand );


// VPK Extraction:
	bool	ExtractVPK( const char *pWorkshopModID );
	bool	MountExtractedFiles( const char *pWorkshopModID );


// File Cleanup:
	void	ClearCampaignFolder( void );


	void	SetCampaignAsMounted( const char *pCampaignID );
	void	FixupMountedCampaignFiles( const char *pWorkshopModID );

// Save files:
	void	MoveSaveFiles( EMoveSaveFileType movetype, const char *pCampaignID = NULL );

private:
	CUtlVector<CampaignData_t *>	*m_Campaigns;

	KeyValues *pCampaignScript;

	CampaignSort_t m_SortMethod;
};

CCampaignDatabase *GetCampaignDatabase();

#endif