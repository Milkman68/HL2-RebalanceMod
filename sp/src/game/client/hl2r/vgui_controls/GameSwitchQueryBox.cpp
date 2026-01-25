#include "cbase.h"
using namespace vgui;
#include "GameSwitchQueryBox.h"
#include "hl2r\hl2r_campaign_manager.h"
#include "hl2r\hl2r_game_manager.h"

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
GameSwitchQueryBox::GameSwitchQueryBox(const char *title, const char *queryText, Panel *parent ) : QueryBox(title, queryText, parent) 
{
	SetSelectedGameType(-1);
}

GameSwitchQueryBox::GameSwitchQueryBox(const wchar_t *wszTitle, const wchar_t *wszQueryText, Panel *parent ) : QueryBox(wszTitle, wszQueryText, parent)
{
	SetSelectedGameType(-1);
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void GameSwitchQueryBox::SetSelectedGameType(int type) 
{ 
	m_iSelectedGameType = type; 
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
int GameSwitchQueryBox::GetSelectedGameType(void) 
{ 
	return m_iSelectedGameType; 
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void GameSwitchQueryBox::OnCommand(const char* command)
{
	if (!stricmp(command, "OK") && GetSelectedGameType() != -1)
	{
		CGameManager* gamemanager = GetGameManager();
		CCampaignManager* campaignmanager = GetCampaignManager();

		engine->ClientCmd("disconnect");

		// If we're switching games through the main-menu then its because the user
		// wants to switch to the base-game. Unmount any mounted campaigns.
		campaignmanager->UnmountMountedCampaign();
		gamemanager->SetGameType(GetSelectedGameType());

		engine->ClientCmd("_restart");
	}

	BaseClass::OnCommand(command);
}