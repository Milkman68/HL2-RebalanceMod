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
	pCampaignScript->LoadFromFile( filesystem, "scripts/campaigns.txt", "MOD" );

	// Initilize our internal list:
	WriteScriptToList();
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
		int i = m_Campaigns.AddToTail();
		CampaignData_t* pNewCampaign = &m_Campaigns[i];

		V_strcpy_safe(pNewCampaign->id, dirName);
		V_strcpy_safe(pNewCampaign->name, "undefined");
		pNewCampaign->game = GAME_INVALID;
		pNewCampaign->mounted = false;
		pNewCampaign->installed = true;
		pNewCampaign->maplist = list;
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
	char szVpkPath[512];
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
	static char output[2048];
	V_sprintf_safe(output, GetOutputFromHLE(pAddonID, "cd maps\r\ndir") );

	// This only appears in hllib's console if the cd command was successful.
	if ( !V_stristr(output, "Directory of root\\maps:") )
		return false;
	 
	// Remove all space characters.
	char substr[2048];
	V_StrSubst(output, " ", "", substr, 2048 );
	V_sprintf_safe(output, substr);

	// Remove all carriage return characters.
	V_StrSubst(output, "\r", "", substr, 2048 );
	V_sprintf_safe(output, substr);

	// Split the rest of the string into tokens separated by linebreaks.
	char *pszToken = strtok( output, "\n" );
	while ( pszToken != NULL )
	{
		// Throw this away if this is determined to just be 
		// a map replacement addon.
		if ( IsMapReplacement(pszToken) )
			return false;

		// This token has a map file.
		if ( V_stristr(pszToken, ".bsp") )
		{
			char mapname[256];
			V_sprintf_safe(mapname, pszToken);

			// Remove the .bsp part and output it to our passed list.
			V_StrSubst(mapname, ".bsp", "", substr, 2048 );
			V_sprintf_safe(mapname, substr);

			int len = V_strlen( mapname );
			char *out = new char[ len + 1 ];
			V_memcpy( out, mapname, len );
			out[ len ] = 0;

			list->AddToTail(out);
		}

		pszToken = strtok( NULL, "\n" );
	}

	return true;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CCampaignDatabase::IsMapReplacement( const char *pMap )
{
	static char *configBuffer = NULL;

	char szMaplistPath[ 1024 ];
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
		int iCampaign = m_Campaigns.AddToTail();
		CampaignData_t *pCampaignData = &m_Campaigns[iCampaign];

		V_strcpy_safe(				pCampaignData->id, pCampaign->GetString("id") );
		V_strcpy_safe(				pCampaignData->name, pCampaign->GetString("name") );
		pCampaignData->game =		pCampaign->GetInt("game");
		pCampaignData->mounted =	pCampaign->GetBool("mounted");
		pCampaignData->installed =	pCampaign->GetBool("installed");
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
	return &( m_Campaigns[index] );
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CampaignData_t *CCampaignDatabase::GetCampaignDataFromID(const char *id)
{
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
KeyValues* CCampaignDatabase::GetKeyValuesFromCampaign( CampaignData_t *campaign )
{
	int campaignIndex = GetCampaignIndex(campaign);

	char keyname[512];
	V_sprintf_safe(keyname, "%d", campaignIndex);
	KeyValues *pData = new KeyValues(keyname);

	if ( !pData )
		return NULL;

	KeyValues *pMaplist = new KeyValues("maplist");

	if ( !pMaplist )
		return NULL;

	CampaignData_t *pCampaign = GetCampaignDatabase()->GetCampaignData(campaignIndex);
	
	for (int i = 0; i < pCampaign->maplist.Count(); i++ )
	{
		char mapindex[512];
		V_sprintf_safe(mapindex, "%d", i);
		pMaplist->SetString(mapindex, pCampaign->maplist[i]);
	}

	pData->SetString("id",		pCampaign->id);
	pData->SetString("name",	pCampaign->name);
	pData->SetInt("game",		pCampaign->game);
	pData->SetBool("mounted",	pCampaign->mounted);
	pData->SetBool("installed",	pCampaign->installed);
	pData->AddSubKey(pMaplist);

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
const char *CCampaignDatabase::GetSteamAppsDir(void)
{
	static char steamappsdir[512];

	V_strcpy(steamappsdir, engine->GetGameDirectory() );

	int iSliceChar = 512;
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

	V_StrSlice(steamappsdir, 0, iSliceChar, steamappsdir, 512 );
	return steamappsdir;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CCampaignDatabase::HLExtractInstalled(void)
{
	char szHLExtractDir[256];
	V_sprintf_safe( szHLExtractDir, "%s\\hllib\\bin\\x64\\HLExtract.exe", engine->GetGameDirectory() );

	return g_pFullFileSystem->FileExists( szHLExtractDir );
}

//-----------------------------------------------------------------------------
// Purpose: Helper function to setup a string that can be sent to CreateProcessA
// to start hlextract.
//-----------------------------------------------------------------------------
const char *CCampaignDatabase::ParseCMD( const char *pCampaignID, const char *pAppend )
{
	char szHLExtractPath[512];
	V_sprintf_safe( szHLExtractPath, "%s\\hllib\\bin\\x64\\HLExtract.exe", engine->GetGameDirectory() );

	char szVpkPath[512];
	V_sprintf_safe( szVpkPath, "%s\\workshop\\content\\220\\%s/workshop_dir.vpk", GetSteamAppsDir(), pCampaignID);

	static char szCommandLine[1024];
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
	static char szOutputBuffer[2048];
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

	char cmd[1024];
	V_sprintf_safe(cmd, ParseCMD(pAddonID, " -c"));

	if (CreateProcessA(NULL, cmd, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
	{
		CloseHandle(hPipeReadIn);
		CloseHandle(hPipeWriteOut);
		
		char input[1024];
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
#define NUM_PATHS 8
const char *szExtractPathList[NUM_PATHS] =
{
	"cfg",
	"maps",
	"materials",
	"models",
	"particles",
	"resource",
	"scripts",
	"sound"
};

bool CCampaignDatabase::ExtractVPK(const char *pAddonID)
{
	// Iterate through our list of files to extract.
	char szExtractList[256];
	for (int i = 0; i < NUM_PATHS; i++)
	{
		char szPathName[32];
		V_sprintf_safe(szPathName, " -e \"%s\"", szExtractPathList[i]);
		V_strncat(szExtractList, szPathName, sizeof(szExtractList));
	}

	STARTUPINFOA si = {};
    PROCESS_INFORMATION pi = {};
    si.cb = sizeof(si);

	char cmd[1024];
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
EMountReturnCode CCampaignDatabase::MountCampaign(const char *pCampaignID)
{
	if (!ExtractVPK(pCampaignID))
		return FAILED_TO_EXTRACT_VPK;

	return SUCESSFULLY_MOUNTED;
}