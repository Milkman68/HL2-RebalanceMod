#include "cbase.h"

using namespace vgui;

#include "tier0/vprof.h"
#include "KeyValues.h"
#include "filesystem.h"
#include "utlbuffer.h"
#include <vgui_controls/QueryBox.h>
#include <hl2r/vgui_controls/GameSwitchQueryBox.h>
#include "ienginevgui.h"
#include <hl2r\hl2r_utils.h>
#include <hl2r\hl2r_campaign_manager.h>
#include "hl2r_shared_manager.h"
#include "hl2r_game_manager.h"

CGameManager *g_pCGameManager = NULL;
CGameManager *GetGameManager( void )
{
	if ( !g_pCGameManager )
		static CGameManager StaticGameManager;

	return g_pCGameManager;
}

//------------------------------------------------------------------------------x
// Purpose:
//------------------------------------------------------------------------------
CGameManager::CGameManager()
{
	Assert( g_pCGameManager == NULL );
	g_pCGameManager = this;

	KeyValues *pGameFile = new KeyValues( "GameSwitch.txt" );
	pGameFile->LoadFromFile( filesystem, "games/gameswitch.txt", "MOD" );

	// TURN THIS INTO A FUNCTION!!!!

	// Get all listed games:
	KeyValues *pGamesKey = pGameFile->FindKey("GameTypes");
	if ( pGamesKey != NULL )
	{
		int i = 0;
		for ( char index[GM_MAX_GAMETYPES] = "0"; pGamesKey->FindKey(index); V_sprintf_safe(index, "%d", i) )
		{
			const char *pGameDirectory = pGamesKey->GetString(index);

			int len = V_strlen( pGameDirectory );
			char *out = new char[ len + 1 ];
			V_memcpy( out, pGameDirectory, len );
			out[ len ] = 0;

			Game_t *game = new Game_t;
			game->directory = out;
			game->type = V_atoi( index );

			m_GameList.AddToTail(game);
			i++;
		}
	}

	m_FileBlacklist = new CUtlVector<const char *>;

	// Get all globalfiles:
	KeyValues *pGlobalFilesKey = pGameFile->FindKey("Blacklist");
	if ( pGlobalFilesKey != NULL )
	{
		int i = 0;
		for ( char index[GM_MAX_GLOBALFILES] = "0"; pGlobalFilesKey->FindKey(index); V_sprintf_safe(index, "%d", i) )
		{
			const char *pFileName = pGlobalFilesKey->GetString(index);

			int len = V_strlen( pFileName );
			char *out = new char[ len + 1 ];
			V_memcpy( out, pFileName, len );
			out[ len ] = 0;

			m_FileBlacklist->AddToTail( out );
			i++;
		}
	}

	pGameFile->deleteThis();
}
//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
int CGameManager::GetGameInfoGameType( void )
{
	KeyValues *pGameinfoFile = new KeyValues( "gameinfo.txt" );

	pGameinfoFile->LoadFromFile( filesystem, "gameinfo.txt", "MOD" );
	for ( KeyValues *pKey = pGameinfoFile->GetFirstSubKey(); pKey; pKey = pKey->GetNextKey() )
	{
		if (!Q_stricmp(pKey->GetName(),"gametype"))
		{
			pGameinfoFile->deleteThis();
			return pKey->GetInt();
		}
	}

	pGameinfoFile->deleteThis();
	return -1;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CGameManager::SetGameType( int gametype )
{
	if ( gametype == -1 )
		return false;

	if ( gametype == GetCurrentGameType()->type )
		return false;

	CSharedManager *sharedManager = GetSharedManager();
	sharedManager->TransferSharedFiles(gametype);

	if ( !TransferGameFiles(gametype) )
		return false;

	if ( !DoGameinfoFilesTransfer(gametype) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
#define NUM_GAME_FOLDERS 4
const char *szGameFolders[NUM_GAME_FOLDERS] =
{
	"cfg",
	"scripts",
	"resource",
};

bool CGameManager::TransferGameFiles( int gametype )
{
	bool bSucess = false;

	// Iterate through the directories we should transfer from our gameswitch.txt script.
	for ( int i = 0; i < NUM_GAME_FOLDERS; i++ )
	{
		// First move the current game's folders into our storage folder.
		if ( MoveDirectory( NULL, szGameFolders[i], GetCurrentGameType()->directory, m_FileBlacklist ) )
			bSucess = true;

		// Get the folder from storage and move it into our main game directory.
		if ( MoveDirectory( GetGameType(gametype)->directory, szGameFolders[i], NULL, m_FileBlacklist ) )
			bSucess = true;
	}

	return bSucess;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
#define MoveFile( start, dest ) g_pFullFileSystem->RenameFile(start, dest)

bool CGameManager::DoGameinfoFilesTransfer( int gametype )
{
	if ( gametype == -1 )
		return false;

	char szGameinfoPath[MAX_PATH];
	V_sprintf_safe(szGameinfoPath, "%s\\gameinfo.txt", engine->GetGameDirectory());

	char szGameinfoStoragePath[MAX_PATH];
	V_sprintf_safe(szGameinfoStoragePath, "%s\\%s\\gameinfo.txt", engine->GetGameDirectory(), GetCurrentGameType()->directory);

	if (!MoveFile(szGameinfoPath, szGameinfoStoragePath))
		return false;

	char szNewGameinfoStoragePath[MAX_PATH];
	V_sprintf_safe(szNewGameinfoStoragePath, "%s\\%s\\gameinfo.txt", engine->GetGameDirectory(), GetGameType(gametype)->directory );

	if (!MoveFile(szNewGameinfoStoragePath, szGameinfoPath))
		return false;

	return true;
}

//------------------------------------------------------------------------------
// Purpose : Code for hooking into the base GameUI panel. Credit goes to: 
// https://github.com/HL2RP/HL2RP
//------------------------------------------------------------------------------
static VPANEL FindChildPanel(VPANEL parent, CUtlStringList& pathNames, int index = 0)
{
	if (index >= pathNames.Size())
	{
		return parent;
	}

	for (auto child : g_pVGuiPanel->GetChildren(parent))
	{
		if (Q_strcmp(g_pVGuiPanel->GetName(child), pathNames[index]) == 0)
		{
			return FindChildPanel(child, pathNames, index + 1);
		}
	}

	return 0;
}

static Panel* FindGameUIChildPanel(const char* pPath)
{
	CUtlStringList pathNames;
	Q_SplitString(pPath, "/", pathNames);
	return g_pVGuiPanel->GetPanel(FindChildPanel(enginevgui->GetPanel(PANEL_GAMEUIDLL), pathNames), "GameUI");
}

static void CC_SwitchGame( const CCommand &args )
{
	CGameManager *manager = GetGameManager();
	if ( !manager )
		return;

	if ( args.ArgC() > 1 )
	{
		int iGame = atoi(args[1]);
		if (iGame < 0)
			return;

		// Allow game switching to the same game if we have a campaign mounted.
		if ( iGame == GetCurrentGameType()->type && !IsWorkshopCampaignMounted() )
			return;

		GameSwitchQueryBox *pBox = new GameSwitchQueryBox("#GameManager_SwitchGame_Disclaimer", "#GameManager_SwitchGame_Text", FindGameUIChildPanel("BaseGameUIPanel"));
		pBox->SetOKButtonText("#GameManager_SwitchGame_Confirm");
		pBox->SetSelectedGameType( iGame );
		pBox->DoModal();
	}
}
static ConCommand gm_switchgame("gm_switchgame", CC_SwitchGame, "switches gametype to a specified index" );
