//-----------------------------------------------------------------------------
// Purpose: A class for managing files shared between the campaign and game manager
//-----------------------------------------------------------------------------
#include "cbase.h"

using namespace vgui;

#include "tier0/vprof.h"
#include "KeyValues.h"
#include "filesystem.h"
#include "utlbuffer.h"
#include <vgui_controls/QueryBox.h>
#include "ienginevgui.h"
#include <hl2r\hl2r_utils.h>
#include <hl2r\hl2r_campaign_manager.h>
#include "hl2r_shared_manager.h"
#include "hl2r_game_manager.h"

CSharedManager *g_pCSharedManager = NULL;
CSharedManager *GetSharedManager( void )
{
	if ( !g_pCSharedManager )
		static CSharedManager StaticSharedManager;

	return g_pCSharedManager;
}

//------------------------------------------------------------------------------x
// Purpose:
//------------------------------------------------------------------------------
CSharedManager::CSharedManager()
{
	Assert( g_pCSharedManager == NULL );
	g_pCSharedManager = this;
}


#define NUM_Shared_FOLDERS 2
const char *szSharedFolders[NUM_Shared_FOLDERS] =
{
	"maps\\graphs",
	"save",
};
//-----------------------------------------------------------------------------
// Purpose: A more robust version of the TransferGameFiles function that 
//			can additionaly handle retreiving files from campaigns aswell.
// 
//			Unlike TransferGameFiles this does not move subdirectories, as 
//			for organization/asthetic reasons, campaign file storage is setup to be contained 
//			inside the shared folders themselve instead of a separate directory.
// 
// Input  : gametype - The index of the game folder we should send our files to.
//-----------------------------------------------------------------------------
bool CSharedManager::TransferSharedFiles( int gametype )
{
	bool bSuccess = false;
	for ( int i = 0; i < NUM_Shared_FOLDERS; i++ )
	{
		char szFolderDir[MAX_PATH];
		if ( IsWorkshopCampaignMounted() )
		{
			V_strcpy_safe(szFolderDir, VarArgs("%s\\default", szSharedFolders[i]) );
		}
		else
		{
			V_strcpy_safe(szFolderDir, szSharedFolders[i]);
		}

		char szGameFolderDir[MAX_PATH];
		V_sprintf_safe(szGameFolderDir, "%s\\%s", GetCurrentGameType()->directory, szSharedFolders[i] );

		// Move files from our Folder directory to our storage's Folder directory.
		if ( MoveFilesInDirectory( szFolderDir, szGameFolderDir ) )
			bSuccess = true;

		char szNewFolderDir[MAX_PATH];
		V_sprintf_safe(szNewFolderDir, "%s\\%s", GetGameType(gametype)->directory, szSharedFolders[i] );

		// Move files from our new  storage's Folder directory to our Folder directory.
		if ( MoveFilesInDirectory( szNewFolderDir, szFolderDir ) )
			bSuccess = true;
	}

	return bSuccess;
}

//-----------------------------------------------------------------------------
// Purpose: Use this function to move files to and from campaign storage when
//			needed.
// 
// Input  : pCampaignID - The ID of the campaign that we're pulling/pushing to.
// Input  : movetype - The kind of file movement we're doing.
//-----------------------------------------------------------------------------
bool CSharedManager::MoveSharedFiles( ESharedFolderMoveType movetype, const char *pCampaignID )
{
	bool bSuccess = false;
	for ( int i = 0; i < NUM_Shared_FOLDERS; i++ )
	{
		if ( movetype == STORE_TO_CAMPAIGN || movetype == RETRIEVE_FROM_CAMPAIGN )
		{
			if ( pCampaignID == NULL )
				return false;
		}

		switch( movetype )
		{
		case STORE_TO_DEFAULT:
			{
				if ( MoveFilesInDirectory(szSharedFolders[i], VarArgs("%s\\default", szSharedFolders[i]) ) )
					bSuccess = true;
			}
			break;

		case STORE_TO_CAMPAIGN:
			{
				if ( MoveFilesInDirectory(szSharedFolders[i], VarArgs("%s\\%s", VarArgs("%s\\campaign_launcher", szSharedFolders[i]), pCampaignID) ) )
					bSuccess = true;
			}
			break;

		case RETRIEVE_FROM_DEFAULT:
			{
				if ( MoveFilesInDirectory( VarArgs("%s\\default", szSharedFolders[i]), szSharedFolders[i] ) )
					bSuccess = true;
			}
			break;

		case RETRIEVE_FROM_CAMPAIGN:
			{
				if ( MoveFilesInDirectory( VarArgs("%s\\%s", VarArgs("%s\\campaign_launcher", szSharedFolders[i]), pCampaignID), szSharedFolders[i] ) )
					bSuccess = true;
			}
			break;
		}
	}

	return bSuccess;
}