#ifndef GAMESWITCHQUERYBOX
#define GAMESWITCHQUERYBOX
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/QueryBox.h>

//-----------------------------------------------------------------------------
// Purpose: An extension of SectionedListPanelHeader that allows individual columns to be
// selected via togglebuttons that sends the currently selected column's index to a target panel.
//-----------------------------------------------------------------------------
class GameSwitchQueryBox : public QueryBox
{
	DECLARE_CLASS_SIMPLE( GameSwitchQueryBox, QueryBox );

public:
	GameSwitchQueryBox(const char *title, const char *queryText, Panel *parent = NULL );
	GameSwitchQueryBox(const wchar_t *wszTitle, const wchar_t *wszQueryText, Panel *parent = NULL);

	void SetSelectedGameType( int type );
	int GetSelectedGameType( void );

public: 
	void OnCommand(const char *command);

private:
	int m_iSelectedGameType;
};
#endif