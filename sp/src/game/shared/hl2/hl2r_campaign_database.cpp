#ifdef CLIENT_DLL
#include <windows.h>
#endif

#include "cbase.h"


#ifdef CLIENT_DLL
using namespace vgui;
#endif

#include "tier0/vprof.h"
#include "KeyValues.h"
#include "filesystem.h"
#include "utlbuffer.h"
#include "hl2r_campaign_database.h"

#ifdef CLIENT_DLL
#include <vgui_controls/PropertyDialog.h>
#include "vgui/ISystem.h"
#endif

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
	BuildCampaignScript();
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
// Purpose: Checks if a given directory string exists in /workshop/content/220/ and
// isn't listed in the players loadorder. (Which contains exclusively non-campaign mods.)
//-----------------------------------------------------------------------------
bool CCampaignDatabase::IsCampaignDirectory( const char *directoryname)
{
	// Don't accept any invalid directories.
	if ( !Q_stricmp( directoryname, ".." ) || !Q_stricmp( directoryname, "." ) || !Q_stricmp( directoryname, "sound" ) )
	{
		return false;
	}

	// Find our workshop file.
	KeyValues *pWorkshopFile = new KeyValues( "workshopaddons" );		
	if ( !pWorkshopFile->LoadFromFile( filesystem, "../../common/Half-Life 2/hl2_complete/cfg/workshop.txt", "GAME" ) )
	{
		pWorkshopFile->deleteThis();
		return false;
	}
		
	// We need to check workshop.txt as it lists all non-campaign addon's id's,
	// making it possible to check if this directory contains a campaign.
	if ( pWorkshopFile->FindKey(directoryname) )
	{
		pWorkshopFile->deleteThis();
		return false;
	}
		
	char dir[512];
	V_sprintf_safe(dir, "../../workshop/content/220/%s\\*.*", directoryname );

	bool bFound = false;

	// Make sure this folder actually has a .vpk file, if not throw it away as a dud.
	FileFindHandle_t fh;
	for( const char *filename = g_pFullFileSystem->FindFirst( dir, &fh ); filename; filename = g_pFullFileSystem->FindNext( fh ) )
	{
		if ( filename != NULL && !Q_stricmp( filename, "workshop_dir.vpk" ) )
			bFound = true;
	}
		
	g_pFullFileSystem->FindClose( fh );
	pWorkshopFile->deleteThis();

	return bFound;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCampaignDatabase::BuildCampaignScript( void )
{
	bool bChanged = false;

	// We need to find campaigns through manual directory searching as their id's are not listed anywhere.
	FileFindHandle_t fh;
	for ( const char *dirName = g_pFullFileSystem->FindFirst( "../../workshop/content/220\\*.*", &fh ); dirName; dirName = g_pFullFileSystem->FindNext( fh ))
	{
		bool bIsCampaign = IsCampaignDirectory(dirName);

		CampaignData_t *pListedCampaign = GetCampaignDatabase()->GetCampaignDataFromID(dirName);
		if ( pListedCampaign != NULL )
		{
			// Don't list this campaign as visible if it is no longer a campaign.
			// This can occur if it was unsubscribed from and the directory no longer contains a vpk.
			if ( pListedCampaign->visible != bIsCampaign )
			{
				pListedCampaign->visible = bIsCampaign;
				bChanged = true;
			}
			continue;
		}

		// We found a campaign directory that isn't "listed" in the script. 
		// Create a blank entry for it in our script.
		if ( bIsCampaign )
		{
			int i = m_Campaigns.AddToTail();
			CampaignData_t *pNewCampaign = &m_Campaigns[i];

			V_strcpy_safe(pNewCampaign->id, dirName);
			V_strcpy_safe(pNewCampaign->name, "undefined");
			pNewCampaign->game = GAME_INVALID;
			pNewCampaign->mounted = false;
			pNewCampaign->visible = true;
			pNewCampaign->invalid = false;

			bChanged = true;
		}
	}

	g_pFullFileSystem->FindClose( fh );

	if ( !bChanged )
		GetCampaignDatabase()->WriteListToScript();
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
		pCampaignData->visible =	pCampaign->GetBool("visible");
		pCampaignData->invalid =	pCampaign->GetBool("invalid");
	}
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCampaignDatabase::WriteListToScript( void )
{
	pCampaignScript->Clear();
	for ( int i = 0; i < GetCampaignCount(); i++ )
		pCampaignScript->AddSubKey( GetKeyValuesFromData(i) );

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
KeyValues* CCampaignDatabase::GetKeyValuesFromData( int index )
{
	char keyname[512];
	V_sprintf_safe(keyname, "%d", index);
	KeyValues *pData = new KeyValues(keyname);

	if ( !pData )
		return NULL;

	CampaignData_t *pCampaign = GetCampaignDatabase()->GetCampaignData(index);
	pData->SetString("id",		pCampaign->id);
	pData->SetString("name",	pCampaign->name);
	pData->SetInt("game",		pCampaign->game);
	pData->SetBool("mounted",	pCampaign->mounted);
	pData->SetBool("visible",	pCampaign->visible);
	pData->SetBool("invalid",	pCampaign->invalid);

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
KeyValues *CCampaignDatabase::GetCampaignScript( void )
{
	return pCampaignScript;
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CCampaignDatabase::GetSteamAppsDir(void)
{
	char *steamappsdir = new char[512];

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

#define EXTRACT_LISTSIZE 8
const char *szExtractList[EXTRACT_LISTSIZE] =
{
	"extract cfg",
	"extract maps",
	"extract materials",
	"extract models",
	"extract particles",
	"extract resource",
	"extract scripts",
	"extract sound",
};

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CCampaignDatabase::UnpackCampaignVPK(const char *pFileName)
{
	// CreatePipe
	SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };

	HANDLE hStdInRead = NULL;
	HANDLE hStdInWrite = NULL;

	CreatePipe(&hStdInRead, &hStdInWrite, &sa, 0);
	SetHandleInformation(hStdInWrite, HANDLE_FLAG_INHERIT, 0);

	// CreateProcess
	PROCESS_INFORMATION pi = {};
	STARTUPINFOA si = {};
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdInput = hStdInRead;
	si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	si.hStdError = GetStdHandle(STD_ERROR_HANDLE);

	char szHLExtractDir[256];
	V_sprintf_safe( szHLExtractDir, "%s\\hllib\\bin\\x64\\HLExtract.exe", engine->GetGameDirectory() );

	char szVpkDir[256];
	V_sprintf_safe( szVpkDir, "%s\\workshop\\content\\220\\%s/workshop_dir.vpk", GetSteamAppsDir(), pFileName);
	
	char szCommand[256];
	V_sprintf_safe(szCommand, "\"%s\" -p \"%s\" -c -v", szHLExtractDir, szVpkDir);

	bool bRet = false;
	if (CreateProcessA(NULL, szCommand, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) 
	{
		DWORD bytesWritten;
		for (int i = 0; i < EXTRACT_LISTSIZE; i++)
		{
			DevMsg("Command index is: \"%i\" String is: \"%s\"\n", i, szExtractList[i] );

			char szFileName[32];
			V_sprintf_safe(szFileName, "%s", szExtractList[i]);
			WriteFile(hStdInWrite, szFileName, sizeof(szFileName), &bytesWritten, NULL);
		}

		WriteFile(hStdInWrite, "exit", 4, &bytesWritten, NULL);
		bRet = true;
	}

	CloseHandle(hStdInWrite);
	CloseHandle(hStdInRead);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return bRet;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
EMountReturnCode CCampaignDatabase::MountCampaign(const char *pFileName)
{
	if (!UnpackCampaignVPK(pFileName))
		return FAILED_TO_UNPACK_VPK;

	return SUCESSFULLY_MOUNTED;
}
#endif