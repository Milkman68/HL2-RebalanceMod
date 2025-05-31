#include <windows.h>
#include "cbase.h"

using namespace vgui;

#include "tier0/vprof.h"
#include "KeyValues.h"
#include "filesystem.h"
#include "utlbuffer.h"
#include "hl2r_campaign_database.h"
#include <vgui_controls/PropertyDialog.h>
#include "vgui/ISystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CCampaignDatabase *g_pCampaignDatabase = NULL;
CCampaignDatabase *GetCampaignDatabase( void )
{
	if ( !g_pCampaignDatabase )
		static CCampaignDatabase StaticCampaignDatabase;

	return g_pCampaignDatabase;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CCampaignDatabase::CCampaignDatabase()
{
	Assert( g_pCampaignDatabase == NULL );
	g_pCampaignDatabase = this;

	pCampaignScript = new KeyValues( "campaignscript" );
	if ( pCampaignScript->LoadFromFile( filesystem, "scripts/campaigns.txt", "MOD" ) )
	{
		// Initilize our internal list:
		WriteScriptToList();
		SortCampaignList(BY_NAME, DECENDING_ORDER);
		WriteListToScript();
	}

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CCampaignDatabase::IsCampaignLoaderMod()
{
	KeyValues *pGameinfoFile = new KeyValues( "gameinfo.txt" );

	pGameinfoFile->LoadFromFile( filesystem, "gameinfo.txt", "MOD" );
	KeyValues* pFileSystemInfo = pGameinfoFile->FindKey( "FileSystem" );

	for ( KeyValues *pKey = pFileSystemInfo->GetFirstSubKey(); pKey; pKey = pKey->GetNextKey() )
	{
		if (!Q_stricmp(pKey->GetName(),"CampaignLauncher"))
		{
			pGameinfoFile->deleteThis();
			return true;
		}
	}

	pGameinfoFile->deleteThis();
	return false;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCampaignDatabase::DoCampaignScan( void )
{
	// We need to find campaigns through manual directory searching as their id's are not listed anywhere.
	FileFindHandle_t fh;
	for ( const char *dirName = g_pFullFileSystem->FindFirst( "../../workshop/content/220\\*.*", &fh ); dirName; dirName = g_pFullFileSystem->FindNext( fh ))
	{
		// Check if this directory is worth scanning for maps.
		// We do this to minimize the amount of time hlextract.exe has to run then close.
		if ( !PotentialCampaignVPK(dirName) )
			continue;

		CUtlVector<const char *> list;
		if ( !ScanForMapsInVPK(dirName, &list) )
			continue;

		// We found a campaign directory that isn't listed in the script. 
		// Create a blank entry for it in our script.
		int i = m_Campaigns.AddToTail(new CampaignData_t);
		CampaignData_t* pNewCampaign = m_Campaigns[i];

		V_strcpy_safe(pNewCampaign->id, dirName);
		V_strcpy_safe(pNewCampaign->name, "undefined");
		pNewCampaign->game = GAME_INVALID;
		pNewCampaign->mounted = false;
		pNewCampaign->installed = true;
		pNewCampaign->maplist = list;
		pNewCampaign->startingmap = -1;
		pNewCampaign->filesize = GetVPKSize(dirName);
		pNewCampaign->datetable = GetVPKDate(dirName);
	}

	g_pFullFileSystem->FindClose( fh );
	GetCampaignDatabase()->WriteListToScript();
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CCampaignDatabase::PotentialCampaignVPK( const char *pAddonID)
{
	// Don't accept any invalid directories.
	if ( !Q_stricmp( pAddonID, ".." ) || !Q_stricmp( pAddonID, "." ) || !Q_stricmp( pAddonID, "sound" ) )
	{
		return false;
	}

	// Filter out ID's that're listed in the workshop file, as that would make them mods.
	KeyValues *pWorkshopFile = new KeyValues( "workshopaddons" );		
	if ( pWorkshopFile->LoadFromFile( filesystem, "../../common/Half-Life 2/hl2_complete/cfg/workshop.txt", "GAME" ) && pWorkshopFile->FindKey(pAddonID) )
	{
		if ( pWorkshopFile->FindKey(pAddonID) )
		{
			pWorkshopFile->deleteThis();
			return false;
		}
	}
	pWorkshopFile->deleteThis();

	// Check that the workshop folder this ID points to even currently contains a VPK.
	char szVpkPath[MAX_PATH];
	V_sprintf_safe( szVpkPath, "%s\\workshop\\content\\220\\%s/workshop_dir.vpk", GetSteamAppsDir(), pAddonID);

	bool bContainsVPK = g_pFullFileSystem->FileExists( szVpkPath );

	// Is this ID already part of our campaign list?
	CampaignData_t *pListedCampaign = GetCampaignDatabase()->GetCampaignDataFromID(pAddonID);
	if ( pListedCampaign != NULL )
	{
		// Don't list this campaign as visible if it is no longer a campaign.
		// This can occur if it was unsubscribed from and the directory no longer contains a vpk.
		if ( pListedCampaign->installed != bContainsVPK )
		{
			pListedCampaign->installed = bContainsVPK;
			GetCampaignDatabase()->WriteListToScript();
		}

		return false;
	}

	return true;
}
//-----------------------------------------------------------------------------
// Purpose: Scan the vpk in this directory for any maps, if there are any
// output it to a CUtlVector
//-----------------------------------------------------------------------------
bool CCampaignDatabase::ScanForMapsInVPK( const char *pAddonID, CUtlVector<const char *> *list )
{
	// Start up hlextract and get the output of the console window from running the
	// "dir" command.
	static char output[HLE_MAX_OUTPUT_LENGTH];
	V_sprintf_safe(output, GetOutputFromHLE(pAddonID, "cd maps\r\ndir") );

	// This only appears in hllib's console if the cd command was successful.
	if ( !V_stristr(output, "Directory of root\\maps:") )
		return false;
	 
	// Remove all space characters.
	char substr[HLE_MAX_OUTPUT_LENGTH];
	V_StrSubst(output, " ", "", substr, sizeof(substr) );
	V_sprintf_safe(output, substr);

	// Remove all carriage return characters.
	V_StrSubst(output, "\r", "", substr, sizeof(substr) );
	V_sprintf_safe(output, substr);

	bool bHasCustomMaps = false;

	// Split the rest of the string into tokens separated by linebreaks.
	for ( char *pszToken = strtok( output, "\n" ); pszToken != NULL; pszToken = strtok( NULL, "\n" ))
	{
		// This token has a map file.
		if ( !V_stristr(pszToken, ".bsp") )
			continue;

		if ( !IsMapReplacement(pszToken) )
			bHasCustomMaps = true;

		char mapname[CAMPAIGN_MAX_MAP_NAME];
		V_sprintf_safe(mapname, pszToken);

		// Remove the .bsp part and output it to our passed list.
		V_StrSubst(mapname, ".bsp", "", substr, sizeof(substr) );
		V_sprintf_safe(mapname, substr);

		int len = V_strlen( mapname );
		char *out = new char[ len + 1 ];
		V_memcpy( out, mapname, len );
		out[ len ] = 0;

		list->AddToTail(out);
	}

	return bHasCustomMaps;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CCampaignDatabase::IsMapReplacement( const char *pMap )
{
	static char *configBuffer = NULL;

	char szMaplistPath[ MAX_PATH ];
	V_sprintf_safe(szMaplistPath, "%s\\default_maplist.txt", engine->GetGameDirectory() );

	if ( configBuffer == NULL )
	{
		FileHandle_t fh = filesystem->Open( szMaplistPath, "rb" );
		if ( fh == FILESYSTEM_INVALID_HANDLE )
			return false;

		// read file into memory
		int size = filesystem->Size(fh);
		char *buf = new char[size + 1];

		filesystem->Read(buf, size, fh);
		buf[size] = 0;
		filesystem->Close(fh);

		configBuffer = buf;
	}

	const char *search = Q_stristr(configBuffer, pMap );
	if ( search )
		return true;

	return false;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCampaignDatabase::WriteScriptToList( void )
{
	m_Campaigns.RemoveAll();
	for ( KeyValues *pCampaign = pCampaignScript->GetFirstSubKey(); pCampaign; pCampaign = pCampaign->GetNextKey() )
	{
		int iCampaign = m_Campaigns.AddToTail(new CampaignData_t);
		CampaignData_t *pCampaignData = m_Campaigns[iCampaign];

		V_strcpy_safe(				pCampaignData->id, pCampaign->GetString("id") );
		V_strcpy_safe(				pCampaignData->name, pCampaign->GetString("name") );
		pCampaignData->game =		pCampaign->GetInt("game");
		pCampaignData->mounted =	pCampaign->GetBool("mounted");
		pCampaignData->installed =	pCampaign->GetBool("installed");

		// Handle the maplist:
		KeyValues *pMaplist = pCampaign->FindKey("maplist");
		if ( pMaplist != NULL )
		{
			int i = 0;

			for ( char index[CAMPAIGN_INDEX_LENGTH] = "0"; pMaplist->FindKey(index); V_sprintf_safe(index, "%d", i) )
			{
				char *map = new char[CAMPAIGN_MAX_MAP_NAME];
				V_memcpy( map, pMaplist->GetString(index), CAMPAIGN_MAX_MAP_NAME );
				pCampaignData->maplist.AddToTail(map);

				i++;
			}
		}

		// Get the datetable:
		KeyValues *pDateTable = pCampaign->FindKey("datetable");
		if ( pDateTable != NULL )
		{
			pCampaignData->datetable = new CampaignDateTable_t;

			V_strcpy_safe(pCampaignData->datetable->minute,	pDateTable->GetString("minute"));
			V_strcpy_safe(pCampaignData->datetable->hour,	pDateTable->GetString("hour"));
			V_strcpy_safe(pCampaignData->datetable->period,	pDateTable->GetString("period"));

			V_strcpy_safe(pCampaignData->datetable->day,	pDateTable->GetString("day"));
			V_strcpy_safe(pCampaignData->datetable->month,	pDateTable->GetString("month"));
			V_strcpy_safe(pCampaignData->datetable->year,	pDateTable->GetString("year"));
		}

		pCampaignData->startingmap = pCampaign->GetInt("startingmap", -1 );
		pCampaignData->filesize =	pCampaign->GetInt("filesize");
	}
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCampaignDatabase::WriteListToScript( void )
{
	pCampaignScript->Clear();
	for ( int i = 0; i < GetCampaignCount(); i++ )
		pCampaignScript->AddSubKey( GetKeyValuesFromCampaign(GetCampaignData(i)) );

	CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );
	pCampaignScript->RecursiveSaveToFile( buf, 0 );

	FileHandle_t fh;
	fh = g_pFullFileSystem->Open( "scripts/campaigns.txt", "wb" );

	if ( fh == FILESYSTEM_INVALID_HANDLE )
		return;

	g_pFullFileSystem->Write( buf.Base(), buf.TellPut(), fh );
	g_pFullFileSystem->Close( fh );
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CampaignData_t *CCampaignDatabase::GetCampaignData(int index )
{
	return m_Campaigns[index];
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CampaignData_t *CCampaignDatabase::GetCampaignDataFromID(const char *id)
{
	if ( !id )
		return NULL;

	for (int i = 0; i < GetCampaignDatabase()->GetCampaignCount(); i++ )
	{
		CampaignData_t *pCampaign = GetCampaignDatabase()->GetCampaignData(i);
		if ( pCampaign && !Q_strcmp(pCampaign->id, id) )
			return pCampaign;
	}

	return NULL;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CCampaignDatabase::GetCampaignIndex( CampaignData_t *campaign )
{
	for (int i = 0; i < GetCampaignDatabase()->GetCampaignCount(); i++ )
	{
		CampaignData_t *pCampaign = GetCampaignDatabase()->GetCampaignData(i);
		if ( pCampaign && pCampaign == campaign )
			return i;
	}

	return NULL;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CampaignData_t *CCampaignDatabase::GetMountedCampaign( void )
{
	for (int i = 0; i < GetCampaignDatabase()->GetCampaignCount(); i++ )
	{
		CampaignData_t *pCampaign = GetCampaignDatabase()->GetCampaignData(i);
		if ( pCampaign->mounted )
			return pCampaign;
	}

	return NULL;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
KeyValues* CCampaignDatabase::GetKeyValuesFromCampaign( CampaignData_t *campaign )
{
	int campaignIndex = GetCampaignIndex(campaign);
	CampaignData_t *pCampaign = GetCampaignDatabase()->GetCampaignData(campaignIndex);


	char keyname[CAMPAIGN_INDEX_LENGTH];
	V_sprintf_safe(keyname, "%d", campaignIndex);
	KeyValues *pData = new KeyValues(keyname);

	if ( !pData )
		return NULL;

	pData->SetString("id",		pCampaign->id);
	pData->SetString("name",	pCampaign->name);
	pData->SetInt("game",		pCampaign->game);
	pData->SetBool("mounted",	pCampaign->mounted);
	pData->SetBool("installed",	pCampaign->installed);
	pData->SetInt("filesize",	pCampaign->filesize);
	pData->SetInt("startingmap",pCampaign->startingmap);

	KeyValues *pMaplist = new KeyValues("maplist");
	if ( pMaplist != NULL )
	{
		for (int i = 0; i < pCampaign->maplist.Count(); i++ )
		{
			char mapindex[CAMPAIGN_MAP_INDEX_LENGTH];
			V_sprintf_safe(mapindex, "%d", i);
			pMaplist->SetString(mapindex, pCampaign->maplist[i]);
		}

		pData->AddSubKey(pMaplist);
	}
	else
	{
		return NULL;
	}


	KeyValues *pDateTable = new KeyValues("datetable");
	if ( pDateTable != NULL )
	{
		pDateTable->SetString("minute",		pCampaign->datetable->minute);
		pDateTable->SetString("hour",		pCampaign->datetable->hour);
		pDateTable->SetString("period",		pCampaign->datetable->period);
		pDateTable->SetString("day",		pCampaign->datetable->day);
		pDateTable->SetString("month",		pCampaign->datetable->month);
		pDateTable->SetString("year",		pCampaign->datetable->year);

		pData->AddSubKey(pDateTable);
	}
	else
	{
		return NULL;
	}

	return pData;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CCampaignDatabase::GetCampaignCount( void )
{
	return m_Campaigns.Count();
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
static int __cdecl CampaignSortFunc( CampaignData_t * const *ppLeft, CampaignData_t * const *ppRight )
{
	const CampaignData_t *pLeft = *ppLeft;
	const CampaignData_t *pRight = *ppRight;

	int higher = GetCampaignDatabase()->GetSortDir() == ASCENDING_ORDER ? 1 : -1;
	int lower = GetCampaignDatabase()->GetSortDir() == ASCENDING_ORDER ? -1 : 1;

	if ( !pLeft->mounted && pRight->mounted )
		return 1;

	if ( !pRight->mounted && pLeft->mounted )
		return -1;

	if ( !Q_stricmp(pLeft->name, "undefined" ) && Q_stricmp(pRight->name, "undefined" ) )
		return 1;

	if ( !Q_stricmp(pRight->name, "undefined" ) && Q_stricmp(pLeft->name, "undefined" ))
		return -1;

switch (GetCampaignDatabase()->GetSortType())
	{
	case BY_SIZE:
		{
			if ( pLeft->filesize < pRight->filesize )
				return higher;

			if ( pLeft->filesize > pRight->filesize )
				return lower;

			return 0;
		}

	case BY_NAME:
		{
			char szLeftName[CAMPAIGN_NAME_LENGTH];
			V_strcpy(szLeftName, pLeft->name);
			strlwr(szLeftName);

			char szRightName[CAMPAIGN_NAME_LENGTH];
			V_strcpy(szRightName, pRight->name);
			strlwr(szRightName);

			int iMax = 0;
			for( int i = 0; szLeftName[i] != '\0' && szRightName[i] != '\0'; i++ )
				iMax = i;

			for( int i = 0; i <= iMax; i++ )
			{
				if ( szLeftName[i] < szRightName[i] )
					return higher;

				if ( szLeftName[i] > szRightName[i] )
					return lower;
			}

			return 0;
		}

	case BY_DATE:
		{
			CampaignDateTable_t *pTableLeft = pLeft->datetable;
			CampaignDateTable_t *pTableRight = pRight->datetable;

			int iHourLeft  = V_atoi(pTableLeft->hour);
			iHourLeft	== 12 ? 0 : iHourLeft;

			int iHourRight = V_atoi(pTableRight->hour);
			iHourRight	== 12 ? 0 : iHourRight;

			// Year:
			if ( V_atoi(pTableLeft->year) < V_atoi(pTableRight->year) )
				return higher;

			if ( V_atoi(pTableLeft->year) > V_atoi(pTableRight->year) )
				return lower;

			// Month:
			if ( V_atoi(pTableLeft->month) < V_atoi(pTableRight->month) )
				return higher;

			if ( V_atoi(pTableLeft->month) > V_atoi(pTableRight->month) )
				return lower;

			// Day:
			if ( V_atoi(pTableLeft->day) < V_atoi(pTableRight->day) )
				return higher;

			if ( V_atoi(pTableLeft->day) > V_atoi(pTableRight->day) )
				return lower;

			// Period:
			if ( !Q_stricmp(pTableLeft->period, "PM" ) && !Q_stricmp(pTableRight->period, "AM" ) )
				return higher;

			if ( !Q_stricmp(pTableLeft->period, "AM" ) && !Q_stricmp(pTableRight->period, "PM" ) )
				return lower;

			// Hour:
			if ( iHourLeft < iHourRight )
				return higher;

			if ( iHourRight < iHourLeft )
				return lower;

			// Minute:
			if ( V_atoi(pTableLeft->minute) < V_atoi(pTableRight->minute) )
				return higher;

			if ( V_atoi(pTableLeft->minute) > V_atoi(pTableRight->minute) )
				return lower;

			return 0;
		}
	}

	return 0;
}

void CCampaignDatabase::SortCampaignList( ESortType sorttype, ESortDirection sortdir )
{
	m_SortMethod.eType = sorttype;
	m_SortMethod.eDir = sortdir;

	m_Campaigns.Sort(CampaignSortFunc);
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CCampaignDatabase::GetVPKSize( const char *pAddonID)
{
	float total = 0;

	char szVpkPath[MAX_PATH];
	V_sprintf_safe( szVpkPath, "../../workshop/content/220/%s\\*.*", pAddonID);

	FileFindHandle_t fh;
	for ( const char *dirName = g_pFullFileSystem->FindFirst( szVpkPath, &fh ); dirName; dirName = g_pFullFileSystem->FindNext( fh ))
	{
		char szFilePath[MAX_PATH];
		V_sprintf_safe( szFilePath, "../../workshop/content/220/%s/%s\n", pAddonID, dirName);

		total += filesystem->Size(szFilePath, "MOD");
	}
	g_pFullFileSystem->FindClose( fh );

	// Convert bytes to mb:
	total = RoundFloatToInt(total * 0.000001);
	return total;
}
//-----------------------------------------------------------------------------
// Purpose: This mountain of code converts a string given by the GetFileTime
// function and returns it in a way more usable and structured format.
//-----------------------------------------------------------------------------
const char *szMonths[12] =
{
	"January",
	"Febuary",
	"March",
	"April",
	"May",
	"June",
	"July",
	"August",
	"September"
	"October"
	"November"
	"December"
};

enum
{
	MONTH_TOKEN = 1,
	DAY_TOKEN = 2,
	TIME_TOKEN = 3,
	YEAR_TOKEN = 4,
};

enum
{
	MINUTE_TOKEN = 1,
	HOUR_TOKEN = 0
};

CampaignDateTable_t *CCampaignDatabase::GetVPKDate( const char *pAddonID)
{
	char szVpkPath[MAX_PATH];
	V_sprintf_safe( szVpkPath, "%s\\workshop\\content\\220\\%s/workshop_dir.vpk", GetSteamAppsDir(), pAddonID);

	char szFileTime[64];
	long filetime = g_pFullFileSystem->GetFileTime(szVpkPath);

	g_pFullFileSystem->FileTimeToString( szFileTime, sizeof( szFileTime ), filetime );

	// Calender stuff:
	char szDay[24] = "-1\0", szMonth[24] = "-1\0", szYear[24] = "-1\0";

	// Clock stuff:
	char szMinute[24] = "-1\0", szHour[24] = "-1\0", szPeriod[4] = "-1\0";

	char *strtokptr_date;

	int iDateTokenIndex = 0;
	for ( char *pszDateToken = strtok_s( szFileTime, " ", &strtokptr_date ); pszDateToken != NULL; pszDateToken = strtok_s( NULL, " ", &strtokptr_date ))
	{

	switch(iDateTokenIndex)
		{
		case TIME_TOKEN:
			{
				char szTimeString[24];
				V_sprintf_safe( szTimeString, pszDateToken);

				char *strtokptr_time;

				// Convert 24 hour time to 12 hour time.
				int iTimeTokenIndex = 0;
				for ( char *pszTimeToken = strtok_s( szTimeString, ":", &strtokptr_time ); pszTimeToken != NULL; pszTimeToken = strtok_s( NULL, ":", &strtokptr_time ))
				{
					if ( iTimeTokenIndex == MINUTE_TOKEN )
					{
						V_sprintf_safe( szMinute, "%s", pszTimeToken);
					}
					else if ( iTimeTokenIndex == HOUR_TOKEN )
					{
						int hour = V_atoi(pszTimeToken);

						V_sprintf_safe( szPeriod, "AM");

						if ( hour >= 12 )
						{
							V_sprintf_safe( szPeriod, "PM");

							if ( hour > 12 )
								hour -= 12;
						}
						else if (hour == 0)
						{
							hour = 12;
						}

						V_sprintf_safe( szHour, "%d", hour);
					}


					iTimeTokenIndex++;
				}
				break;
			}

		case DAY_TOKEN:
			{
				V_sprintf_safe(szDay, "%s", pszDateToken);
				break;
			}

		case MONTH_TOKEN:
			{
				for (int i = 0; i < 12; i++ )
				{
					if ( Q_strcmp(pszDateToken, szMonths[i]) )
						continue;

					V_sprintf_safe(szMonth, "%d", i);
					break;
				}

				break;
			}

		case YEAR_TOKEN:
			{
				V_sprintf_safe(szYear, "%s", pszDateToken);
				break;
			}
		}

		iDateTokenIndex++;
	}

	CampaignDateTable_t *out = new CampaignDateTable_t;

	// HACK: We need to add an exclamation point to integer strings denote them as strings,
	// or else character sequences like "05" will be converted into just "5" by the keyvalue system.
	V_sprintf_safe( out->minute, "%s!", szMinute );
	V_sprintf_safe( out->hour, "%s!", szHour );
	V_sprintf_safe( out->period, szPeriod );

	V_sprintf_safe( out->day, "%s!", szDay );
	V_sprintf_safe( out->month, "%s!", szMonth );
	V_sprintf_safe( out->year, "%s!", szYear );

	return out;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CCampaignDatabase::GetSteamAppsDir(void)
{
	static char steamappsdir[MAX_PATH];

	V_strcpy(steamappsdir, engine->GetGameDirectory() );

	int iSliceChar = MAX_PATH;
	int iSlashCount = 0;

	// Iterate backwards from the end of our GetGameDirectory() string to the start and look for any slashes.
	// As soon as we've found 2 (moved up 2 directories) stop and return that as the steamapps directory.
	while ( iSliceChar > 0 )
	{
		if ( steamappsdir[iSliceChar] && steamappsdir[iSliceChar] == '\\' )
			iSlashCount++;

		if (iSlashCount == 2)
			break;

		iSliceChar--;
	}

	if (iSliceChar == 0)
		return NULL;

	V_StrSlice(steamappsdir, 0, iSliceChar, steamappsdir, MAX_PATH );
	return steamappsdir;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CCampaignDatabase::HLExtractInstalled(void)
{
	char szHLExtractDir[MAX_PATH];
	V_sprintf_safe( szHLExtractDir, "%s\\hllib\\bin\\x64\\HLExtract.exe", engine->GetGameDirectory() );

	return g_pFullFileSystem->FileExists( szHLExtractDir );
}

//-----------------------------------------------------------------------------
// Purpose: Helper function to setup a string that can be sent to CreateProcessA
// to start hlextract.
//-----------------------------------------------------------------------------
const char *CCampaignDatabase::ParseCMD( const char *pAddonID, const char *pAppend )
{
	char szHLExtractPath[MAX_PATH];
	V_sprintf_safe( szHLExtractPath, "%s\\hllib\\bin\\x64\\HLExtract.exe", engine->GetGameDirectory() );

	char szVpkPath[MAX_PATH];
	V_sprintf_safe( szVpkPath, "%s\\workshop\\content\\220\\%s/workshop_dir.vpk", GetSteamAppsDir(), pAddonID);

	static char szCommandLine[MAX_PATH*2];
	V_sprintf_safe(szCommandLine, "\"%s\" -p \"%s\" -v", szHLExtractPath, szVpkPath);

	if ( pAppend != NULL )
		V_strncat(szCommandLine, pAppend, sizeof(szCommandLine));

	return szCommandLine;
}
//-----------------------------------------------------------------------------
// Purpose: Run hlextract to get the output of the CLI.
// This can be used to get information about a specific .vpk.
//-----------------------------------------------------------------------------

const char *CCampaignDatabase::GetOutputFromHLE( const char *pAddonID, const char *pCommand )
{
	static char szOutputBuffer[HLE_MAX_OUTPUT_LENGTH];
	memset(szOutputBuffer, 0, sizeof(szOutputBuffer));

	SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };

	HANDLE hPipeReadOut, hPipeWriteOut;
	if (!CreatePipe(&hPipeReadOut, &hPipeWriteOut, &sa, 0)) return NULL;
	SetHandleInformation(hPipeReadOut, HANDLE_FLAG_INHERIT, 0);

	HANDLE hPipeReadIn, hPipeWriteIn;
	if (!CreatePipe(&hPipeReadIn, &hPipeWriteIn, &sa, 0)) return NULL;
	SetHandleInformation(hPipeWriteIn, HANDLE_FLAG_INHERIT, 0);

	STARTUPINFOA si = { sizeof(STARTUPINFOA) };
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdInput = hPipeReadIn;
	si.hStdOutput = hPipeWriteOut;
	si.hStdError = hPipeWriteOut;

	PROCESS_INFORMATION pi = {};

	char cmd[HLE_MAX_CMD_LENGTH];
	V_sprintf_safe(cmd, ParseCMD(pAddonID, " -c"));

	if (CreateProcessA(NULL, cmd, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
	{
		CloseHandle(hPipeReadIn);
		CloseHandle(hPipeWriteOut);
		
		char input[32];
		V_sprintf_safe( input, "%s\r\nexit", pCommand );

		DWORD bytesWritten;
		WriteFile(hPipeWriteIn, input, 1024, &bytesWritten, NULL);
		CloseHandle(hPipeWriteIn);

		DWORD totalBytes = 0, bytesRead = 0;
		while (totalBytes < sizeof(szOutputBuffer) - 1)
		{
			if (!ReadFile(hPipeReadOut, szOutputBuffer + totalBytes, sizeof(szOutputBuffer) - totalBytes - 1, &bytesRead, NULL) || bytesRead == 0)
				break;

			totalBytes += bytesRead;
		}
		szOutputBuffer[totalBytes] = '\0';

		CloseHandle(hPipeReadOut);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

	//	DevMsg("HLExtract output:\n%s\n", szOutputBuffer);
		return szOutputBuffer;
	}

	CloseHandle(hPipeReadIn);
	CloseHandle(hPipeWriteIn);
	CloseHandle(hPipeReadOut);
	CloseHandle(hPipeWriteOut);

	return NULL;
}
//-----------------------------------------------------------------------------
// Purpose: Extract the contents of a 
//-----------------------------------------------------------------------------
#define NUM_VPK_PATHS 9
const char *szVpkFilePaths[NUM_VPK_PATHS] =
{
	"cfg",
	"maps",
	"materials",
	"models",
	"particles",
	"resource",
	"scripts",
	"scenes",
	"sound"
};

bool CCampaignDatabase::ExtractVPK(const char *pAddonID)
{
	// Iterate through our list of files to extract.
	char szExtractList[256];
	V_sprintf_safe(szExtractList, "");

	for (int i = 0; i < NUM_VPK_PATHS; i++)
	{
		char szPathName[24];
		V_sprintf_safe(szPathName, " -e \"%s\"", szVpkFilePaths[i]);
		V_strncat(szExtractList, szPathName, sizeof(szExtractList));
	}

	STARTUPINFOA si = {};
    PROCESS_INFORMATION pi = {};
    si.cb = sizeof(si);

	char cmd[HLE_MAX_CMD_LENGTH];
	V_sprintf_safe(cmd, ParseCMD( pAddonID, szExtractList ) );

	// Create an instance of HLExtract with our campaign as the loaded vpk.
	if (CreateProcessA(NULL, cmd, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi) != NULL )
	{
		WaitForSingleObjectEx(pi.hProcess, INFINITE, TRUE);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		return true;
	}
	
	return false;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CCampaignDatabase::MountExtractedFiles( const char *pCampaignID )
{
	bool bMountedFile = false;

	for (int i = 0; i < NUM_VPK_PATHS; i++)
	{
		char szFilePath[MAX_PATH];
		V_sprintf_safe( szFilePath, "%s\\workshop\\content\\220\\%s/%s", GetSteamAppsDir(), pCampaignID, szVpkFilePaths[i]);

		char szSearchPath[MAX_PATH];
		V_sprintf_safe( szSearchPath, "%s\\*.*", szFilePath);

		FileFindHandle_t fh;
		if ( g_pFullFileSystem->FindFirst( szSearchPath, &fh ) )
		{
			char szNewFilePath[MAX_PATH];
			V_sprintf_safe( szNewFilePath, "%s\\mounted\\%s/%s", engine->GetGameDirectory(), pCampaignID, szVpkFilePaths[i]);

			if ( g_pFullFileSystem->RenameFile(szFilePath, szNewFilePath) )
				bMountedFile = true; 
		}
		g_pFullFileSystem->FindClose( fh );
	}

	return bMountedFile;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *szGameinfoPaths[3] =
{
	"gameinfo\\hl2",
	"gameinfo\\ep1",
	"gameinfo\\ep2",
};

bool CCampaignDatabase::TransferGameinfoFiles( int currentgameinfo, int newgameinfo )
{
	if ( currentgameinfo == -1 && newgameinfo == -1 )
		return false;

	char szGameinfoPath[MAX_PATH];
	V_sprintf_safe(szGameinfoPath, "%s\\gameinfo.txt", engine->GetGameDirectory());

	int gameindex = currentgameinfo != -1 ? currentgameinfo : 0;

	char szGameinfoStoragePath[MAX_PATH];
	V_sprintf_safe(szGameinfoStoragePath, "%s\\%s\\gameinfo.txt", engine->GetGameDirectory(), szGameinfoPaths[gameindex]);

	if (!g_pFullFileSystem->RenameFile(szGameinfoPath, szGameinfoStoragePath))
		return false;

	int newgameindex = newgameinfo != -1 ? newgameinfo : 0;

	char szNewGameinfoStoragePath[MAX_PATH];
	V_sprintf_safe(szNewGameinfoStoragePath, "%s\\%s\\gameinfo.txt", engine->GetGameDirectory(), szGameinfoPaths[newgameindex] );

	char szNewGameinfoPath[MAX_PATH];
	V_sprintf_safe(szNewGameinfoPath, "%s\\gameinfo.txt", engine->GetGameDirectory() );

	if (!g_pFullFileSystem->RenameFile(szNewGameinfoStoragePath, szNewGameinfoPath))
		return false;

	return true;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CCampaignDatabase::MountNewGameinfo( const char *pCampaignID )
{
	CampaignData_t *pPrevMounted = GetMountedCampaign();
	CampaignData_t *pCampaign = GetCampaignDataFromID(pCampaignID);

	int iCurrentGameType = pPrevMounted ? pPrevMounted->game : -1;
	int iNewGametype = pCampaign ? pCampaign->game : -1;

	if ( iCurrentGameType == iNewGametype )
		return true;

	return TransferGameinfoFiles(iCurrentGameType, iNewGametype);
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CCampaignDatabase::StoreSaveFiles( const char *pCampaignID )
{
	char szSaveFolderPath[MAX_PATH];
	V_sprintf_safe( szSaveFolderPath, "%s\\save", engine->GetGameDirectory() );

	char szSearchPath[MAX_PATH];
	V_sprintf_safe( szSearchPath, "%s\\*.*", szSaveFolderPath);

	FileFindHandle_t fh;
	for ( const char *dirName = g_pFullFileSystem->FindFirst( szSearchPath, &fh ); dirName; dirName = g_pFullFileSystem->FindNext( fh ))
	{
		if ( !Q_stricmp( dirName, "root" ) || !Q_stricmp( dirName, ".." ) || !Q_stricmp( dirName, "." ) )
			continue;

		char szFilePath[MAX_PATH];
		V_sprintf_safe( szFilePath, "%s\\%s", szSaveFolderPath, dirName );

		if ( g_pFullFileSystem->IsDirectory(szFilePath) )
			continue;

		char szSaveStoragePath[MAX_PATH];
		V_sprintf_safe( szSaveStoragePath, "%s\\%s\\%s", szSaveFolderPath, pCampaignID, dirName );

		if (!g_pFullFileSystem->RenameFile(szFilePath, szSaveStoragePath))
			return false;
	}

	g_pFullFileSystem->FindClose( fh );
	return true;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CCampaignDatabase::RetrieveSaveFiles( const char *pCampaignID )
{
	char szCampaignSavePath[MAX_PATH];
	V_sprintf_safe( szCampaignSavePath, "%s\\save\\%s", engine->GetGameDirectory(), pCampaignID );

	char szSearchPath[MAX_PATH];
	V_sprintf_safe( szSearchPath, "%s\\*.*", szCampaignSavePath);

	FileFindHandle_t fh;
	for ( const char *dirName = g_pFullFileSystem->FindFirst( szSearchPath, &fh ); dirName; dirName = g_pFullFileSystem->FindNext( fh ))
	{
		if ( !Q_stricmp( dirName, "root" ) || !Q_stricmp( dirName, ".." ) || !Q_stricmp( dirName, "." ) )
			continue;

		char szFilePath[MAX_PATH];
		V_sprintf_safe( szFilePath, "%s\\%s", szCampaignSavePath, dirName );

		char szSaveFolderPath[MAX_PATH];
		V_sprintf_safe( szSaveFolderPath, "%s\\save\\%s", engine->GetGameDirectory(), dirName );

		if (!g_pFullFileSystem->RenameFile(szFilePath, szSaveFolderPath))
			return false;
	}

	g_pFullFileSystem->FindClose( fh );

	return true;
}
//-----------------------------------------------------------------------------
// Purpose: This sounds bad I know, but it's just being used exclusively for 
// cleaning up previously mounted campaign folders. The reason this is here is because this kind of function 
// doesn't exist in the filesystem class, so I had to make one myself.
//-----------------------------------------------------------------------------
void CCampaignDatabase::RemoveFilesInDirectory( const char *pDir )
{
	char szSearchPath[MAX_PATH];
	V_sprintf_safe( szSearchPath, "%s\\*.*", pDir);

	FileFindHandle_t fh;
	for ( const char *dirName = g_pFullFileSystem->FindFirst( szSearchPath, &fh ); dirName; dirName = g_pFullFileSystem->FindNext( fh ))
	{
		if ( !Q_stricmp( dirName, "root" ) || !Q_stricmp( dirName, ".." ) || !Q_stricmp( dirName, "." ) )
			continue;

		char szFilePath[MAX_PATH];
		V_sprintf_safe( szFilePath, "%s\\%s", pDir, dirName );

		if ( g_pFullFileSystem->IsDirectory(szFilePath) )
		{
			RemoveFilesInDirectory(szFilePath);
			RemoveDirectory(szFilePath);
		}
		else
		{
			g_pFullFileSystem->RemoveFile(szFilePath);
		}
	}

	g_pFullFileSystem->FindClose( fh );
	RemoveDirectory(pDir);
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCampaignDatabase::ClearCampaignFolder( void )
{
	char szMountedFolderPath[MAX_PATH];
	V_sprintf_safe( szMountedFolderPath, "%s\\mounted", engine->GetGameDirectory());

	char szSearchPath[MAX_PATH];
	V_sprintf_safe( szSearchPath, "%s\\*.*", szMountedFolderPath);

	FileFindHandle_t fh;
	for ( const char *dirName = g_pFullFileSystem->FindFirst( szSearchPath, &fh ); dirName; dirName = g_pFullFileSystem->FindNext( fh ))
	{
		if ( !Q_stricmp( dirName, "root" ) || !Q_stricmp( dirName, ".." ) || !Q_stricmp( dirName, "." ) )
			continue;

		char szDirPath[MAX_PATH];
		V_sprintf_safe( szDirPath, "%s\\mounted\\%s", engine->GetGameDirectory(), dirName );

		RemoveFilesInDirectory(szDirPath);
	}

	g_pFullFileSystem->FindClose( fh );
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCampaignDatabase::SetCampaignAsMounted( const char *pCampaignID )
{
	CampaignData_t *pPrevMounted = GetMountedCampaign();
	CampaignData_t *pCampaign = GetCampaignDataFromID(pCampaignID);

	if ( pPrevMounted )
	{
		pPrevMounted->mounted = false;
		StoreSaveFiles(pPrevMounted->id);
	}

	RetrieveSaveFiles(pCampaignID);
	FixupMountedCampaign(pCampaignID);

	pCampaign->mounted = true;
	WriteListToScript();
}
//-----------------------------------------------------------------------------
// This is unfinished but its not really needed atm so whatever.
//-----------------------------------------------------------------------------
/*
void CCampaignDatabase::UnmountMountedCampaign()
{
	CampaignData_t *pMounted = GetMountedCampaign();
	if ( !pMounted )
		return;

	TransferGameinfoFile(NULL);

	ClearCampaignFolder();
	StoreSaveFiles(pMounted->id);

	pMounted->mounted = false;
	WriteListToScript();
}
*/
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCampaignDatabase::FixupMountedCampaign( const char *pCampaignID )
{
	KeyValues *pBlacklistScript = new KeyValues("blacklist");
	if ( !pBlacklistScript->LoadFromFile( filesystem, "scripts/file_blacklist.txt", "MOD" ) )
		return;

	for ( KeyValues *pFile = pBlacklistScript->GetFirstSubKey(); pFile; pFile = pFile->GetNextKey() )
	{
		char szFilePath[MAX_PATH];
		V_sprintf_safe( szFilePath, "%s\\mounted\\%s\\%s", engine->GetGameDirectory(), pCampaignID, pFile->GetString() );

		if ( g_pFullFileSystem->IsDirectory(szFilePath) )
		{
			RemoveFilesInDirectory(szFilePath);
		}
		else
		{
			if ( g_pFullFileSystem->FileExists(szFilePath) )
				g_pFullFileSystem->RemoveFile(szFilePath);
		}
	}
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
EMountReturnCode CCampaignDatabase::MountCampaign(const char *pCampaignID)
{
	ClearCampaignFolder();

	if (!ExtractVPK(pCampaignID))
		return FAILED_TO_EXTRACT_VPK;

	if (!MountExtractedFiles(pCampaignID))
		return FAILED_TO_MOUNT_FILES;

	if (!MountNewGameinfo(pCampaignID))
		return FAILED_TO_TRANSFER_GAMEINFO;

	SetCampaignAsMounted(pCampaignID);
	return SUCESSFULLY_MOUNTED;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCampaignDatabase::FlushMountedCampaignGraphs( void )
{
	// Remove the graphs folder.
	char szCampaignNodeGraphPath[MAX_PATH];
	V_sprintf_safe( szCampaignNodeGraphPath, "%s\\mounted\\%s\\maps\\graphs", engine->GetGameDirectory(), GetMountedCampaign()->id );

	RemoveFilesInDirectory(szCampaignNodeGraphPath);
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CON_COMMAND(campaign_flush_nodegraph, "Clears all .ain  files from our mounted campaign's maps folder.")
{
	CCampaignDatabase *database = GetCampaignDatabase();
	if ( !database )
		return;

	database->FlushMountedCampaignGraphs();
}