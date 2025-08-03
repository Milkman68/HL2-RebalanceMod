#ifndef hl2r_game_manager
#define hl2r_game_manager
#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
// Game Manager:
//-----------------------------------------------------------------------------
#define GM_MAX_GAMETYPES	10
#define GM_MAX_FOLDERS		15
#define GM_MAX_GLOBALFILES	50

struct Game_t
{
	const char *directory;
	int			type;
};

class CGameManager
{
public:
			CGameManager();
public:

	int	GetCurrentGameType(void);
	bool	SetGameType( int gametype );
		
private:

	bool	TransferGameFiles( int gametype );

	bool	MoveDirectory( const char *pDirFolder, const char *pDir, const char *pTargetDir );
	bool	DoGameinfoFilesTransfer( int gametype );

private:
	CUtlVector<Game_t *>		m_GameList;

	CUtlVector<const char *>	m_FolderTransferList;
	CUtlVector<const char *>	m_GlobalFilesList;
};

CGameManager *g_pCGameManager = NULL;
CGameManager *GetGameManager( void )
{
	if ( !g_pCGameManager )
		static CGameManager StaticGameManager;

	return g_pCGameManager;
}
#endif