#ifndef hl2r_campaign_database
#define hl2r_campaign_database
#ifdef _WIN32
#pragma once
#endif

#include "tier0/icommandline.h"

#define CAMPAIGN_HANDLE short int
#define CAMPAIGN_HANDLE_INVALID -1

// Campaign accessor macros:
#define GetCampaign(index) CampaignList()->Element(index)
#define GetCampaignFromID(id) CampaignList()->Element( GetCampaignDatabase()->GetCampaignHandleFromID(id) )
#define GetMountedCampaign() CampaignList()->Element( GetCampaignDatabase()->GetMountedCampaignHandle() )

#define IsWorkshopCampaignMounted() GetCampaignDatabase()->ValidCampaign( GetCampaignDatabase()->GetMountedCampaignHandle() )

// Directories:
#define CAMPAIGN_MOUNT_DIR						"campaign_launcher\\mounted"

#define CAMPAIGN_LAUCHER_SAVE_STORE_DIR			"save\\campaign_launcher"
#define DEFAULT_GAME_SAVE_STORE_DIR				"save\\default"

#define CONTENT_PARENT_FOLDER		"campaign_launcher"
#define CONTENT_FOLDER				"content"
#define CONTENT_DISABLED_FOLDER		"content_disabled"
#define CONTENT_ENABLED_FOLDER		"content_enabled"

// Filepaths:
#define CAMPAIGN_LAUCHER_HLEXTRACT_DIR	"campaign_launcher\\hllib\\bin\\x64\\HLExtract.exe"
#define CAMPAIGN_SCRIPT_FILE			"campaign_launcher\\campaigns.txt"
#define CAMPAIGN_BLACKLIST_FILE			"campaign_launcher\\file_blacklist.txt"

// Sound files:
#define CAMPAIGN_DEFAULT_SOUNDSCRIPTS_FILE	"scripts\\default_soundscripts.txt"		// Contains a list of all non-custom soundscripts in hl2 and its episodes.
#define DEFAULT_SOUNDSCRIPT_FILE			"scripts\\default_sounds.txt"			// Our own mod's soundscript to use as a template to output to our override file.

// A file that overrides our mod's soundscript to support both custom-campaign sounds and mods that replace game_sounds_manifest.
#define CAMPAIGN_SOUND_OVERRIDE_FILE	"campaign_launcher\\content_enabled\\content\\scripts\\level_sounds_e3_bugbait.txt"	

// Global string sizes:
#define CAMPAIGN_INDEX_LENGTH		5	// Allows up to 99999 listed Campaigns.
#define CAMPAIGN_FILESIZE_LENGTH	5	// Allows a filesize up to 99999 MB.
#define CAMPAIGN_MAP_INDEX_LENGTH	3	// Allows up to 999 maps in a single campaign.
#define CAMPAIGN_MAX_MAP_NAME		128

#define MAX_SOUNDSCRIPT_LENGTH		262144 // 2^18 Characters

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
#ifdef CLIENT_DLL
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
#endif

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

#ifdef CLIENT_DLL
struct CampaignSort_t
{
	ESortType	eType;
	ESortDirection	eDir;
};
#endif
class CCampaignDatabase
{
public:
	CCampaignDatabase();

#ifdef CLIENT_DLL

	EMountReturnCode	MountCampaign(const char *pCampaignID);
	void				UnmountMountedCampaign( void );

	void				DoCampaignScan( void );
	bool				HLExtractInstalled( void );
#endif

	// List accessor functions:
	CUtlVector<CampaignData_t *>	*CampaignList( void ) { return m_Campaigns; };

	CAMPAIGN_HANDLE		GetCampaignHandleFromID( const char *id );
	CAMPAIGN_HANDLE		GetMountedCampaignHandle( void );
	KeyValues*			GetKeyValuesFromCampaign( CAMPAIGN_HANDLE campaign );

	bool				ValidCampaign( CAMPAIGN_HANDLE campaign ) { return campaign >= 0; }
	int					GetCampaignCount( void )	{ return CampaignList()->Count(); }
	
	// Campaign script file:
	void	WriteScriptToList( void );
#ifdef CLIENT_DLL
	void	WriteListToScript( void );

	// Sorting:
	void			SortCampaignList( ESortType sorttype, ESortDirection sortdir );
	ESortType		GetSortType() { return m_SortMethod.eType; }
	ESortDirection	GetSortDir() { return m_SortMethod.eDir; }

	// Nodegraph hotfix:
	void	FlushCampaignGraphs( const char *pCampaignID );
#endif
	void	RunSoundScriptMount( void );
	void	RunBackgroundValidate( void );
#ifdef CLIENT_DLL
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

#endif
// Sounds:
	void		HandleCustomSoundScripts( const char *pCampaignID );
	void		MountSoundScripts( CUtlVector< const char *> *pFilePaths );


// Misc content:
#ifdef CLIENT_DLL
	void	ValidateBackgrounds( const char *pCampaignID );
	void	MountLauncherContent( bool bMount );
#endif
private:
	CUtlVector<CampaignData_t *>	*m_Campaigns;
	KeyValues *pCampaignScript;

#ifdef CLIENT_DLL
	CampaignSort_t m_SortMethod;
#endif
};

CCampaignDatabase *GetCampaignDatabase();

#endif