#include <windows.h>
#include "cbase.h"

#ifdef CLIENT_DLL
using namespace vgui;
#endif

#include "tier0/vprof.h"
#include "KeyValues.h"
#include "filesystem.h"
#include "utlbuffer.h"
#include "hl2r_utils.h"
#include <vgui_controls/PropertyDialog.h>
#include "vgui/ISystem.h"
#ifdef CLIENT_DLL
#include <hl2r\hl2r_game_manager.h>
#endif
#include "hl2r_campaign_database.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define HLE_MAX_OUTPUT_LENGTH 2048
#define HLE_MAX_CMD_LENGTH 1024

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
	if ( pCampaignScript->LoadFromFile( filesystem, CAMPAIGN_SCRIPT_FILE, "MOD" ) )
	{
		m_Campaigns = new CUtlVector<CampaignData_t *>;

		// Initilize our internal list: 
		WriteScriptToList();
#ifdef CLIENT_DLL
		SortCampaignList(BY_DATE, DECENDING_ORDER);
		WriteListToScript();
#endif
	}
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL
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

		CampaignData_t *pNewCampaign = new CampaignData_t;

		V_strcpy_safe(pNewCampaign->id, dirName);
		V_strcpy_safe(pNewCampaign->name, "undefined");
		pNewCampaign->game = GAME_INVALID;
		pNewCampaign->mounted = false;
		pNewCampaign->installed = true;
		pNewCampaign->maplist = list;
		pNewCampaign->startingmap = -1;
		pNewCampaign->filesize = GetVPKSize(dirName);
		pNewCampaign->datetable = GetVPKDate(dirName);

		CampaignList()->AddToTail(pNewCampaign);
	}

	g_pFullFileSystem->FindClose( fh );
	GetCampaignDatabase()->WriteListToScript();
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CCampaignDatabase::PotentialCampaignVPK( const char *pWorkshopModID)
{
	// Don't accept any invalid directories.
	if ( !Q_stricmp( pWorkshopModID, ".." ) || !Q_stricmp( pWorkshopModID, "." ) || !Q_stricmp( pWorkshopModID, "sound" ) )
	{
		return false;
	}

	// Filter out ID's that're listed in the workshop file, as that would make them mods.
	KeyValues *pWorkshopFile = new KeyValues( "workshopaddons" );		
	if ( pWorkshopFile->LoadFromFile( filesystem, "../../common/Half-Life 2/hl2_complete/cfg/workshop.txt", "GAME" ) && pWorkshopFile->FindKey(pWorkshopModID) )
	{
		if ( pWorkshopFile->FindKey(pWorkshopModID) )
		{
			pWorkshopFile->deleteThis();
			return false;
		}
	}
	pWorkshopFile->deleteThis();

	// Check that the workshop folder this ID points to even currently contains a VPK.
	char szVpkPath[MAX_PATH];
	V_sprintf_safe( szVpkPath, "%s\\workshop\\content\\220\\%s/workshop_dir.vpk", GetSteamAppsDir(), pWorkshopModID);

	bool bContainsVPK = g_pFullFileSystem->FileExists( szVpkPath );

	// Is this ID already part of our campaign list?
	CAMPAIGN_HANDLE hListedCampaign = GetCampaignHandleFromID(pWorkshopModID);
	if ( ValidCampaign(hListedCampaign) )
	{
		// Don't list this campaign as visible if it is no longer a campaign.
		// This can occur if it was unsubscribed from and the directory no longer contains a vpk.
		if ( GetCampaign(hListedCampaign)->installed != bContainsVPK )
		{
			GetCampaign(hListedCampaign)->installed = bContainsVPK;
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
bool CCampaignDatabase::ScanForMapsInVPK( const char *pWorkshopModID, CUtlVector<const char *> *list )
{
	// Start up hlextract and get the output of the console window from running the
	// "dir" command.
	static char output[HLE_MAX_OUTPUT_LENGTH];
	V_sprintf_safe(output, GetOutputFromHLE(pWorkshopModID, "cd maps\r\ndir") );

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
#endif
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCampaignDatabase::WriteScriptToList( void )
{
	CampaignList()->RemoveAll();
	for ( KeyValues *pCampaign = pCampaignScript->GetFirstSubKey(); pCampaign; pCampaign = pCampaign->GetNextKey() )
	{
		CampaignData_t *pCampaignData = new CampaignData_t;

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

		CampaignList()->AddToTail(pCampaignData);
	}
}
#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCampaignDatabase::WriteListToScript( void )
{
	pCampaignScript->Clear();
	for ( CAMPAIGN_HANDLE i = 0; i < GetCampaignCount(); i++ )
		pCampaignScript->AddSubKey( GetKeyValuesFromCampaign(i) );

	CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );
	pCampaignScript->RecursiveSaveToFile( buf, 0 );

	FileHandle_t fh;
	fh = g_pFullFileSystem->Open( CAMPAIGN_SCRIPT_FILE, "wb" );

	if ( fh == FILESYSTEM_INVALID_HANDLE )
		return;

	g_pFullFileSystem->Write( buf.Base(), buf.TellPut(), fh );
	g_pFullFileSystem->Close( fh );
}
#endif
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CAMPAIGN_HANDLE CCampaignDatabase::GetCampaignHandleFromID(const char *id)
{
	if ( !id )
		return NULL;

	for (CAMPAIGN_HANDLE i = 0; i < GetCampaignCount(); i++ )
	{
		if ( GetCampaign(i) && !Q_strcmp(GetCampaign(i)->id, id) )
			return i;
	}

	return -1;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CAMPAIGN_HANDLE CCampaignDatabase::GetMountedCampaignHandle( void )
{
	for (CAMPAIGN_HANDLE i = 0; i < GetCampaignCount(); i++ )
	{
		if ( GetCampaign(i)->mounted )
			return i;
	}

	return -1;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
KeyValues* CCampaignDatabase::GetKeyValuesFromCampaign( CAMPAIGN_HANDLE campaign )
{
	char keyname[CAMPAIGN_INDEX_LENGTH];
	V_sprintf_safe(keyname, "%d", campaign);
	KeyValues *pData = new KeyValues(keyname);

	if ( !pData )
		return NULL;

	pData->SetString("id",		GetCampaign(campaign)->id);
	pData->SetString("name",	GetCampaign(campaign)->name);
	pData->SetInt("game",		GetCampaign(campaign)->game);
	pData->SetBool("mounted",	GetCampaign(campaign)->mounted);
	pData->SetBool("installed",	GetCampaign(campaign)->installed);
	pData->SetInt("filesize",	GetCampaign(campaign)->filesize);
	pData->SetInt("startingmap",GetCampaign(campaign)->startingmap);

	KeyValues *pMaplist = new KeyValues("maplist");
	if ( pMaplist != NULL )
	{
		for (int i = 0; i < GetCampaign(campaign)->maplist.Count(); i++ )
		{
			char mapindex[CAMPAIGN_MAP_INDEX_LENGTH];
			V_sprintf_safe(mapindex, "%d", i);
			pMaplist->SetString(mapindex, GetCampaign(campaign)->maplist[i]);
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
		pDateTable->SetString("minute",	GetCampaign(campaign)->datetable->minute);
		pDateTable->SetString("hour",	GetCampaign(campaign)->datetable->hour);
		pDateTable->SetString("period",	GetCampaign(campaign)->datetable->period);
		pDateTable->SetString("day",	GetCampaign(campaign)->datetable->day);
		pDateTable->SetString("month",	GetCampaign(campaign)->datetable->month);
		pDateTable->SetString("year",	GetCampaign(campaign)->datetable->year);

		pData->AddSubKey(pDateTable);
	}
	else
	{
		return NULL;
	}

	return pData;
}
#ifdef CLIENT_DLL
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
				return lower;

			if ( V_atoi(pTableLeft->year) > V_atoi(pTableRight->year) )
				return higher;

			// Month:
			if ( V_atoi(pTableLeft->month) < V_atoi(pTableRight->month) )
				return lower;

			if ( V_atoi(pTableLeft->month) > V_atoi(pTableRight->month) )
				return higher;

			// Day:
			if ( V_atoi(pTableLeft->day) < V_atoi(pTableRight->day) )
				return lower;

			if ( V_atoi(pTableLeft->day) > V_atoi(pTableRight->day) )
				return higher;

			// Period:
			if ( !Q_stricmp(pTableLeft->period, "PM" ) && !Q_stricmp(pTableRight->period, "AM" ) )
				return lower;

			if ( !Q_stricmp(pTableLeft->period, "AM" ) && !Q_stricmp(pTableRight->period, "PM" ) )
				return higher;

			// Hour:
			if ( iHourLeft < iHourRight )
				return lower;

			if ( iHourRight < iHourLeft )
				return higher;

			// Minute:
			if ( V_atoi(pTableLeft->minute) < V_atoi(pTableRight->minute) )
				return lower;

			if ( V_atoi(pTableLeft->minute) > V_atoi(pTableRight->minute) )
				return higher;

			return 0;
		}
	}

	return 0;
}

void CCampaignDatabase::SortCampaignList( ESortType sorttype, ESortDirection sortdir )
{
	m_SortMethod.eType = sorttype;
	m_SortMethod.eDir = sortdir;

	CampaignList()->Sort(CampaignSortFunc);
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CCampaignDatabase::GetVPKSize( const char *pWorkshopModID)
{
	float total = 0;

	char szVpkPath[MAX_PATH];
	V_sprintf_safe( szVpkPath, "../../workshop/content/220/%s\\*.*", pWorkshopModID);

	FileFindHandle_t fh;
	for ( const char *dirName = g_pFullFileSystem->FindFirst( szVpkPath, &fh ); dirName; dirName = g_pFullFileSystem->FindNext( fh ))
	{
		char szFilePath[MAX_PATH];
		V_sprintf_safe( szFilePath, "../../workshop/content/220/%s/%s\n", pWorkshopModID, dirName);

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
	"Jan",
	"Feb",
	"Mar",
	"Apr",
	"May",
	"Jun",
	"Jul",
	"Aug",
	"Sep",
	"Oct",
	"Nov",
	"Dec"
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

CampaignDateTable_t *CCampaignDatabase::GetVPKDate( const char *pWorkshopModID )
{
	char szVpkPath[MAX_PATH];
	V_sprintf_safe( szVpkPath, "%s\\workshop\\content\\220\\%s/workshop_dir.vpk", GetSteamAppsDir(), pWorkshopModID);

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
bool CCampaignDatabase::HLExtractInstalled(void)
{
	char szHLExtractDir[MAX_PATH];
	V_sprintf_safe( szHLExtractDir, "%s\\%s", engine->GetGameDirectory(), CAMPAIGN_LAUCHER_HLEXTRACT_DIR );

	return g_pFullFileSystem->FileExists( szHLExtractDir );
}

//-----------------------------------------------------------------------------
// Purpose: Helper function to setup a string that can be sent to CreateProcessA
// to start hlextract.
//-----------------------------------------------------------------------------
const char *CCampaignDatabase::ParseCMD( const char *pAddonID, const char *pAppend )
{
	char szHLExtractPath[MAX_PATH];
	V_sprintf_safe( szHLExtractPath, "%s\\%s", engine->GetGameDirectory(), CAMPAIGN_LAUCHER_HLEXTRACT_DIR );

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
// 
// 
// NOTE: This function is AI generated by ChatGPT. At the time I was honestly getting pretty fed up with the windows library
// so I just generated this out of convenience to get it done. Looking back its something I probably really shouldn't have done that due to
// the many ethical/moral problems with using AI generation. 
//
// It's not something I'm planning on doing in the future, but I just wanted to put this here just so everyone can know.
//-----------------------------------------------------------------------------
const char *CCampaignDatabase::GetOutputFromHLE( const char *pWorkshopModID, const char *pCommand )
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
	V_sprintf_safe(cmd, ParseCMD(pWorkshopModID, " -c"));

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

bool CCampaignDatabase::ExtractVPK(const char *pWorkshopModID)
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
	V_sprintf_safe(cmd, ParseCMD( pWorkshopModID, szExtractList ) );

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
bool CCampaignDatabase::MountExtractedFiles( const char *pWorkshopModID )
{
	bool bMountedFile = false;

	for (int i = 0; i < NUM_VPK_PATHS; i++)
	{
		char szFilePath[MAX_PATH];
		V_sprintf_safe( szFilePath, "%s\\workshop\\content\\220\\%s/%s", GetSteamAppsDir(), pWorkshopModID, szVpkFilePaths[i]);

		char szSearchPath[MAX_PATH];
		V_sprintf_safe( szSearchPath, "%s\\*.*", szFilePath);

		FileFindHandle_t fh;
		if ( g_pFullFileSystem->FindFirst( szSearchPath, &fh ) )
		{
			char szNewFilePath[MAX_PATH];
			V_sprintf_safe( szNewFilePath, "%s\\%s\\%s/%s", engine->GetGameDirectory(), CAMPAIGN_MOUNT_DIR, pWorkshopModID, szVpkFilePaths[i]);

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
void CCampaignDatabase::ClearCampaignFolder( void )
{
	char szMountedFolderPath[MAX_PATH];
	V_sprintf_safe( szMountedFolderPath, "%s\\%s", engine->GetGameDirectory(), CAMPAIGN_MOUNT_DIR);

	char szSearchPath[MAX_PATH];
	V_sprintf_safe( szSearchPath, "%s\\*.*", szMountedFolderPath);

	FileFindHandle_t fh;
	for ( const char *dirName = g_pFullFileSystem->FindFirst( szSearchPath, &fh ); dirName; dirName = g_pFullFileSystem->FindNext( fh ))
	{
		if ( !Q_stricmp( dirName, "root" ) || !Q_stricmp( dirName, ".." ) || !Q_stricmp( dirName, "." ) )
			continue;

		char szDirPath[MAX_PATH];
		V_sprintf_safe( szDirPath, "%s\\%s\\%s", engine->GetGameDirectory(), CAMPAIGN_MOUNT_DIR, dirName );

		RemoveFilesInDirectory(szDirPath, NULL);
	}

	g_pFullFileSystem->FindClose( fh );
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCampaignDatabase::SetCampaignAsMounted( const char *pCampaignID )
{
	FixupMountedCampaignFiles(pCampaignID);

	// Was a campaign already mounted?
	if ( IsWorkshopCampaignMounted() )
	{
		// If true, store the previous campaign's save files in a dedicated location
		MoveSaveFiles( STORE_TO_CAMPAIGN, GetMountedCampaign()->id );
		GetMountedCampaign()->mounted = false;
	}
	else
	{
		// We're going from the basegame to a custom campaign.
		// Store the savefiles in a separate location.
		MoveSaveFiles( STORE_TO_DEFAULT, GetMountedCampaign()->id );
	}

	// Import any previously stored savefiles from the campaign we're mounting if we have any.
	MoveSaveFiles( RETRIEVE_FROM_CAMPAIGN, pCampaignID );

	// Mount our launcher content.
	MountLauncherContent(true);

	// Mount any custom sounds this mod has.

	// NOTE: This is now done through the server in gameinterface.cpp on startup.
	//HandleCustomSoundScripts(pCampaignID);

	// Prevent invalid background maps from loading.
	ValidateBackgrounds(pCampaignID);

	GetCampaignFromID(pCampaignID)->mounted = true;
	WriteListToScript();
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCampaignDatabase::FixupMountedCampaignFiles( const char *pCampaignID )
{
	KeyValues *pBlacklistScript = new KeyValues("blacklist");
	if ( !pBlacklistScript->LoadFromFile( filesystem, CAMPAIGN_BLACKLIST_FILE, "MOD" ) )
		return;

	for ( KeyValues *pFile = pBlacklistScript->GetFirstSubKey(); pFile; pFile = pFile->GetNextKey() )
	{
		char szFilePath[MAX_PATH];
		V_sprintf_safe( szFilePath, "%s\\%s\\%s\\%s", engine->GetGameDirectory(), CAMPAIGN_MOUNT_DIR, pCampaignID, pFile->GetString() );

		if ( g_pFullFileSystem->FileExists(szFilePath) )
			g_pFullFileSystem->RemoveFile(szFilePath);
	}
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCampaignDatabase::MoveSaveFiles( EMoveSaveFileType movetype, const char *pCampaignID )
{
	if ( movetype == STORE_TO_CAMPAIGN || movetype == RETRIEVE_FROM_CAMPAIGN )
	{
		if ( pCampaignID == NULL )
			return;
	}

	switch( movetype )
	{
	case STORE_TO_DEFAULT:
		MoveFilesInDirectory("save", DEFAULT_GAME_SAVE_STORE_DIR );
		break;

	case STORE_TO_CAMPAIGN:
		MoveFilesInDirectory("save", VarArgs("%s\\%s", CAMPAIGN_LAUCHER_SAVE_STORE_DIR, pCampaignID) );
		break;

	case RETRIEVE_FROM_DEFAULT:
		MoveFilesInDirectory(DEFAULT_GAME_SAVE_STORE_DIR, "save" );

	case RETRIEVE_FROM_CAMPAIGN:
		MoveFilesInDirectory( VarArgs("%s\\%s", CAMPAIGN_LAUCHER_SAVE_STORE_DIR, pCampaignID), "save" );
		break;
	}
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCampaignDatabase::MountLauncherContent( bool bMount )
{
	char szContentDisabledDir[MAX_PATH];
	V_sprintf_safe(szContentDisabledDir, "%s\\%s", CONTENT_PARENT_FOLDER, CONTENT_DISABLED_FOLDER);

	char szContentEnabledDir[MAX_PATH];
	V_sprintf_safe(szContentEnabledDir, "%s\\%s", CONTENT_PARENT_FOLDER, CONTENT_ENABLED_FOLDER);

	if ( bMount )
	{
		// Mount our launcher content.
		MoveDirectory( szContentDisabledDir, CONTENT_FOLDER, szContentEnabledDir );
	}
	else
	{
		// Unmount our launcher content.
		MoveDirectory( szContentEnabledDir, CONTENT_FOLDER, szContentDisabledDir );
	}
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCampaignDatabase::ValidateBackgrounds( const char *pCampaignID )
{
	char szChapterBackgroundsDir[MAX_PATH];
	V_sprintf_safe(szChapterBackgroundsDir, "%s\\%s\\scripts\\chapterbackgrounds.txt", CAMPAIGN_MOUNT_DIR, pCampaignID );

	// Get the chapterbackgrounds.txt file.
	KeyValues *pChapterBackgroundsFile = new KeyValues("defaultmanifest");
	if ( !pChapterBackgroundsFile->LoadFromFile( filesystem, CAMPAIGN_DEFAULT_SOUNDSCRIPTS_FILE, "MOD" ) )
		return;

	for ( KeyValues *pBackgroundMap = pChapterBackgroundsFile->GetFirstSubKey(); pBackgroundMap; pBackgroundMap = pBackgroundMap->GetNextKey() )
	{
		// If this campaign's background list contains a missing map get rid of the file.
		if ( !g_pFullFileSystem->FileExists( VarArgs("maps\\%s.bsp", pBackgroundMap->GetString() )) )
		{
			g_pFullFileSystem->RemoveFile(szChapterBackgroundsDir);
			return;
		}
	}
}
#endif
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCampaignDatabase::HandleCustomSoundScripts( const char *pCampaignID )
{
	// Get a list of all of hl2's normal soundscripts.
	KeyValues *pDefaultSoundManifestFile = new KeyValues("defaultmanifest");
	if ( !pDefaultSoundManifestFile->LoadFromFile( filesystem, CAMPAIGN_DEFAULT_SOUNDSCRIPTS_FILE, "MOD" ) )
		return;

	// Get our campaign's sound_manifest file.
	char szCampaignSoundManifest[MAX_PATH];
	V_sprintf_safe( szCampaignSoundManifest, "%s\\%s\\scripts\\game_sounds_manifest.txt", CAMPAIGN_MOUNT_DIR, pCampaignID );

	KeyValues *pCampaignSoundManifest = new KeyValues("campaignmanifest");
	if ( !pCampaignSoundManifest->LoadFromFile( filesystem, szCampaignSoundManifest, "MOD" ) )
		return;

	// A list of all new soundscript files our campaign's manifest contains.
	CUtlVector<const char *> *pNewSoundScriptList = new CUtlVector<const char *>;
	bool bFoundScripts = false;

	for ( KeyValues *pCampaignSoundScript = pCampaignSoundManifest->GetFirstSubKey(); pCampaignSoundScript; pCampaignSoundScript = pCampaignSoundScript->GetNextKey() )
	{
		char szFilePath[64];
		V_strcpy_safe(szFilePath, pCampaignSoundScript->GetString());

		// Check if this file is a soundscript file already in hl2.
		bool bInsideList = false;
		for ( KeyValues *pFile = pDefaultSoundManifestFile->GetFirstSubKey(); pFile; pFile = pFile->GetNextKey() )
		{
			if ( !Q_stricmp(pFile->GetString(), szFilePath ) )
				bInsideList = true;
		}

		// If it has the same name as a default file, don't mount it.
		if ( bInsideList )
			continue;

		bFoundScripts = true;

		int len = V_strlen( szFilePath );
		char *out = new char[ len + 1 ];
		V_memcpy( out, szFilePath, len );
		out[ len ] = 0;

		pNewSoundScriptList->AddToTail(out);
	}

	if ( bFoundScripts )
		MountSoundScripts( pNewSoundScriptList );

	pDefaultSoundManifestFile->deleteThis();
	pCampaignSoundManifest->deleteThis();
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCampaignDatabase::MountSoundScripts( CUtlVector< const char *> *pSoundScripts )
{
	char szOutput[MAX_SOUNDSCRIPT_LENGTH];
	int len = 0;

	char *defaultscript = ReadFileIntoBuffer( DEFAULT_SOUNDSCRIPT_FILE, len );
	Q_strcpy(szOutput, defaultscript);

	// Iterate through all this campaign's custom-soundscripts.
	for ( int i = 0; i < pSoundScripts->Count(); i++ )
	{
		char *soundscript = ReadFileIntoBuffer( pSoundScripts->Element(i), len );
		Q_snprintf( szOutput, sizeof(szOutput), "%s\n%s", szOutput, soundscript);
	}

	FileHandle_t fh;
	fh = g_pFullFileSystem->Open( CAMPAIGN_SOUND_OVERRIDE_FILE, "wb" );

	if ( fh == FILESYSTEM_INVALID_HANDLE )
		return;

	g_pFullFileSystem->Write( szOutput, len, fh );
	g_pFullFileSystem->Close( fh );
}
#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
EMountReturnCode CCampaignDatabase::MountCampaign( const char *pCampaignID )
{
	CGameManager *manager = GetGameManager();
	manager->SetGameType( GetCampaignFromID(pCampaignID)->game );

	ClearCampaignFolder();

	if (!ExtractVPK( pCampaignID ))
		return FAILED_TO_EXTRACT_VPK;

	if (!MountExtractedFiles( pCampaignID ))
		return FAILED_TO_MOUNT_FILES;

	SetCampaignAsMounted( pCampaignID );
	return SUCESSFULLY_MOUNTED;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCampaignDatabase::UnmountMountedCampaign( void )
{
	if ( !IsWorkshopCampaignMounted() )
		return;

	// Store our campaign's save files and retrieve the saves
	// from our basegame.
	MoveSaveFiles( STORE_TO_CAMPAIGN, GetMountedCampaign()->id );
	MoveSaveFiles( RETRIEVE_FROM_DEFAULT );

	// Unmount our launcher content.
	MountLauncherContent(false);

	// Clear out the mounted campaigns folder.
	ClearCampaignFolder();

	GetMountedCampaign()->mounted = false;
	WriteListToScript();
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCampaignDatabase::FlushMountedCampaignGraphs( void )
{
	// Remove the graphs folder.
	char szCampaignNodeGraphPath[MAX_PATH];
	V_sprintf_safe( szCampaignNodeGraphPath, "%s\\%s\\%s\\maps\\graphs", engine->GetGameDirectory(), CAMPAIGN_MOUNT_DIR, GetMountedCampaign()->id );

	RemoveFilesInDirectory(szCampaignNodeGraphPath, NULL);
}

CON_COMMAND(campaign_flush_nodegraph, "Clears all .ain  files from our mounted campaign's maps folder.")
{
	CCampaignDatabase *database = GetCampaignDatabase();
	if ( !database )
		return;

	database->FlushMountedCampaignGraphs();
}
#endif
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL
void CCampaignDatabase::RunBackgroundValidate( void )
{
	if ( !IsWorkshopCampaignMounted() )
		return;

	ValidateBackgrounds(GetMountedCampaign()->id);
}
CON_COMMAND(campaign_run_background_validate, "")
{
	CCampaignDatabase *database = GetCampaignDatabase();
	if ( !database )
		return;

	database->RunBackgroundValidate();
}
#endif
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCampaignDatabase::RunSoundScriptMount( void )
{
	if ( !IsWorkshopCampaignMounted() )
		return;

	HandleCustomSoundScripts(GetMountedCampaign()->id);
}
#ifdef CLIENT_DLL
CON_COMMAND(campaign_run_soundscript_mount, "")
{
	CCampaignDatabase *database = GetCampaignDatabase();
	if ( !database )
		return;

	database->RunSoundScriptMount();
}
#endif