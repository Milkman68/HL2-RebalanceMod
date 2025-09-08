#include "cbase.h"

using namespace vgui;

#include "tier0/vprof.h"
#include "KeyValues.h"
#include "filesystem.h"
#include "utlbuffer.h"
#include <vgui_controls/QueryBox.h>
#include "ienginevgui.h"
#include "hl2r_game_manager.h"

//-----------------------------------------------------------------------------
// Purpose: Creates A Message box with a question in it and yes/no buttons
//-----------------------------------------------------------------------------
class GameSwitchQueryBox : public QueryBox
{
	DECLARE_CLASS_SIMPLE( GameSwitchQueryBox, QueryBox );

public:
	GameSwitchQueryBox(const char *title, const char *queryText, Panel *parent = NULL ) : QueryBox(title, queryText, parent) {
		SetSelectedGameType(-1);}

	GameSwitchQueryBox(const wchar_t *wszTitle, const wchar_t *wszQueryText, Panel *parent = NULL) : QueryBox(wszTitle, wszQueryText, parent){
		SetSelectedGameType(-1);}

	void SetSelectedGameType( int type ) { m_iSelectedGameType = type; }
	int GetSelectedGameType( void ) { return m_iSelectedGameType; }

public: 
	void OnCommand(const char *command)
	{
		if (!stricmp(command, "OK") && GetSelectedGameType() != -1)
		{
			CGameManager *manager = GetGameManager();

			engine->ClientCmd("disconnect");
			manager->SetGameType( GetSelectedGameType() );

			engine->ClientCmd("_restart");
		}
	
		BaseClass::OnCommand(command);
	}

private:
	int m_iSelectedGameType;
};

//------------------------------------------------------------------------------x
// Purpose:
//------------------------------------------------------------------------------
/*const char *szGameFilePaths[3] =
{
	"games\\hl2",
	"games\\ep1",
	"games\\ep2",
};
*/
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

	// Get all listed folders we have to transfer:
	KeyValues *pFoldersKey = pGameFile->FindKey("FolderTransfer");
	if ( pFoldersKey != NULL )
	{
		int i = 0;
		for ( char index[GM_MAX_FOLDERS] =  "0"; pFoldersKey->FindKey(index); V_sprintf_safe(index, "%d", i) )
		{
			const char *pFolderName = pFoldersKey->GetString(index);

			int len = V_strlen( pFolderName );
			char *out = new char[ len + 1 ];
			V_memcpy( out, pFolderName, len );
			out[ len ] = 0;

			m_FolderTransferList.AddToTail( out );
			i++;
		}
	}

	// Get all globalfiles:
	KeyValues *pGlobalFilesKey = pGameFile->FindKey("GlobalFiles");
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

			m_GlobalFilesList.AddToTail( out );
			i++;
		}
	}

	pGameFile->deleteThis();
}
//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
int CGameManager::GetCurrentGameType( void )
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
	if ( GetCurrentGameType() == gametype )
		return true;

	return TransferGameFiles(gametype);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CGameManager::TransferGameFiles( int gametype )
{
	Assert(gametype != -1);
	Assert(GetCurrentGameType() != -1);

	if ( gametype == -1 )
		return false;

	// Iterate through the directories we should transfer from our gameswitch.txt script.
	for ( int i = 0; i < m_FolderTransferList.Count(); i++ )
	{
		// First move the current game's folder into our storage folder.
		if ( !MoveDirectory( "none", m_FolderTransferList[i], m_GameList[ GetCurrentGameType() ]->directory ) )
			return false;

		// Get the folder from storage and move it into our main game directory.
		if ( !MoveDirectory( m_GameList[ gametype ]->directory, m_FolderTransferList[i], "none") )
			return false;
	}

	// Finally transfer the gameinfo.txt files.
	if ( !DoGameinfoFilesTransfer(gametype) )
		return false;

	return true;
}
//-----------------------------------------------------------------------------
// Purpose: Moves a directory from one location to another
// Input  : pDirFolder - The directory that contains the directory we want to move.
//			pDir - The directory we want to move.
//			pTargetDir - the directory we want to move the input directory to.
// Output : The sucess of this process.
//-----------------------------------------------------------------------------
bool CGameManager::MoveDirectory( const char *pDirFolder, const char *pDir, const char *pTargetDir )
{
	// The directory the file transfer is happening in.
	char szSearchPath[MAX_PATH];

	if ( !Q_stricmp(pDirFolder, "none") )
	{
		V_sprintf_safe( szSearchPath, "%s\\%s\\*.*", engine->GetGameDirectory(), pDir );
	}
	else
	{
		V_sprintf_safe( szSearchPath, "%s\\%s\\%s\\*.*", engine->GetGameDirectory(), pDirFolder, pDir );
	}

	//DevMsg("Directory folder is: [%s]		Directory is: [%s]		TargetDirectory is: [%s]\n\n\n", pDirFolder, pDir, pTargetDir);

	// Iterate through all files in this directory
	FileFindHandle_t fh;
	for ( const char *pFileName = g_pFullFileSystem->FindFirst( szSearchPath, &fh ); pFileName; pFileName = g_pFullFileSystem->FindNext( fh ))
	{
		// Handle invalid directories
		if ( !Q_stricmp( pFileName, "root" ) || !Q_stricmp( pFileName, ".." ) || !Q_stricmp( pFileName, "." ) )
			continue;

		char szFilePath[MAX_PATH];
		if ( !Q_stricmp(pDirFolder, "none") )
		{
			V_sprintf_safe( szFilePath, "%s\\%s", pDir, pFileName );
		}
		else
		{
			V_sprintf_safe( szFilePath, "%s\\%s\\%s", pDirFolder, pDir, pFileName );
		}

		// If we hit a directory, transfer the files in it too.
		if ( g_pFullFileSystem->IsDirectory(szFilePath) )
		{
			char szRelativeFilePath[MAX_PATH];
			V_sprintf_safe( szRelativeFilePath, "%s\\%s", pDir, pFileName );

		//	DevMsg("\n\n");
			MoveDirectory(pDirFolder, szRelativeFilePath, pTargetDir);
		}
		else
		{
			bool bIsGlobalFile = false;
			for (int i = 0; i < m_GlobalFilesList.Count(); i++ )
			{
				if ( !Q_stricmp(pFileName, m_GlobalFilesList[i] ) )
				{
					bIsGlobalFile = true;
					break;
				}
			}

			if ( bIsGlobalFile )
				continue;

			char szNewFilePath[MAX_PATH];
			if ( !Q_stricmp(pTargetDir, "none") )
			{
				V_sprintf_safe( szNewFilePath, "%s\\%s", pDir, pFileName );
			}
			else
			{
				V_sprintf_safe( szNewFilePath, "%s\\%s\\%s", pTargetDir, pDir, pFileName );
			}

	//		DevMsg("	Filepath is: [%s]		RenameFile Command is: [%s]", szFilePath, szNewFilePath);
	//		DevMsg("\n");

			g_pFullFileSystem->RenameFile(szFilePath, szNewFilePath);
		}

	}

	DevMsg("\n\n");

	g_pFullFileSystem->FindClose( fh );
	return true;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

bool CGameManager::DoGameinfoFilesTransfer( int gametype )
{
	if ( gametype == -1 )
		return false;

	char szGameinfoPath[MAX_PATH];
	V_sprintf_safe(szGameinfoPath, "%s\\gameinfo.txt", engine->GetGameDirectory());

	char szGameinfoStoragePath[MAX_PATH];
	V_sprintf_safe(szGameinfoStoragePath, "%s\\%s\\gameinfo.txt", engine->GetGameDirectory(), m_GameList[ GetCurrentGameType() ]->directory);

	if (!g_pFullFileSystem->RenameFile(szGameinfoPath, szGameinfoStoragePath))
		return false;

	char szNewGameinfoStoragePath[MAX_PATH];
	V_sprintf_safe(szNewGameinfoStoragePath, "%s\\%s\\gameinfo.txt", engine->GetGameDirectory(), m_GameList[ gametype ]->directory );

	if (!g_pFullFileSystem->RenameFile(szNewGameinfoStoragePath, szGameinfoPath))
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

		if ( iGame == GetGameManager()->GetCurrentGameType() )
			return;

		GameSwitchQueryBox *pBox = new GameSwitchQueryBox("#GameManager_SwitchGame_Disclaimer", "#GameManager_SwitchGame_Text", FindGameUIChildPanel("BaseGameUIPanel"));
		pBox->SetOKButtonText("#GameManager_SwitchGame_Confirm");
		pBox->SetSelectedGameType( iGame );
		pBox->DoModal();
	}
}
static ConCommand gm_switchgame("gm_switchgame", CC_SwitchGame, "switches gametype to a specified index" );
