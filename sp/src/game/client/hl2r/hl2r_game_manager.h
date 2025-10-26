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

	int		GetCurrentGameType(void);
	bool	SetGameType( int gametype );
		
private:

	bool	TransferGameFiles( int gametype );
	void	HandleSaveFiles( int gametype );
	bool	DoGameinfoFilesTransfer( int gametype );

private:
	CUtlVector<Game_t *>		m_GameList;
	CUtlVector<const char *>	*m_GlobalFilesList;
};

CGameManager *GetGameManager();

#endif