#ifndef hl2r_shared_manager
#define hl2r_shared_manager
#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
// Shared Manager:
//-----------------------------------------------------------------------------
enum ESharedFolderMoveType
{
	STORE_TO_DEFAULT = 0,
	STORE_TO_CAMPAIGN,
	RETRIEVE_FROM_DEFAULT,
	RETRIEVE_FROM_CAMPAIGN
};

class CSharedManager
{
public:
			CSharedManager();
public:
	bool	TransferSharedFiles( int gametype );
	bool	MoveSharedFiles( ESharedFolderMoveType movetype, const char *pCampaignID = NULL );
};

CSharedManager *GetSharedManager();

#endif