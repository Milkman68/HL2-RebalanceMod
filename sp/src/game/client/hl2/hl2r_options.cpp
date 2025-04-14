#include "cbase.h"
using namespace vgui;
#include <vgui/IVGui.h>
#include <vgui_controls/PropertyDialog.h>
#include <vgui_controls/PropertySheet.h>
#include <vgui_controls/ScrollableEditablePanel.h>
#include "ienginevgui.h"
#include <filesystem.h>
#include <vgui/ISurface.h>
#include "hl2r_options.h"

#define PANEL_WIDTH 422
#define PANEL_HEIGHT 590

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

//------------------------------------------------------------------------------
// Purpose: Helper function for determining screen proportion scaling values
//------------------------------------------------------------------------------
static int GetAdjustedSize( int iValue )
{
	int screenW, screenH;
	surface()->GetScreenSize( screenW, screenH );
	
	float flRatio = MAX( MAX( 1.0f, (float)screenW / 1920.0f ), MAX( 1.0f, (float)screenH / 1080.0f ) );
	iValue *= (float)flRatio;
	
	return (int)iValue;
}
//------------------------------------------------------------------------------
// Purpose: Challenge panel
//------------------------------------------------------------------------------
void CSubChallengePanel::SubPanelInit( void )
{
	toggleAllButton = new Button(this, "toggleAllButton", "");
	toggleAllButton->SetCommand("toggleall");
		
	BaseClass::SubPanelInit();
}

void CSubChallengePanel::OnCommand(const char* pcCommand)
{
	BaseClass::OnCommand(pcCommand);
	if (!Q_stricmp(pcCommand, "toggleall"))
	{
		bool bState = true;
		
		for ( int i = 0; i < ARRAYSIZE(challenges_CheckButtons); i++ )
		{
			if ( challenges_CheckButtons[i].Button->IsSelected() )
			{
				bState = false;
				break;
			}
		}
			
		for ( int i = 0; i < ARRAYSIZE(challenges_CheckButtons); i++ )
		{
				challenges_CheckButtons[i].Button->SetSelected(bState);
		}
	}
}
//------------------------------------------------------------------------------
// Purpose: Parent panel
//------------------------------------------------------------------------------
CHL2RMenu::CHL2RMenu() : PropertyDialog(FindGameUIChildPanel("BaseGameUIPanel"), "OtherSettings")
{
	SetDeleteSelfOnClose(true);
	SetApplyButtonVisible(true);
	SetCloseButtonVisible(false);
	SetSizeable( false );
		
	SetBounds(0, 0, GetAdjustedSize(PANEL_WIDTH), GetAdjustedSize(PANEL_HEIGHT) );
	MoveToCenterOfScreen();

	SetTitle("#hl2r_options_title", true);

	hl2r_OptionsTabs[2].Panel = new CSubChallengePanel(this);
	hl2r_OptionsTabs->ParseOptionsPanels( this, hl2r_OptionsTabs );
}

void CHL2RMenu::Activate()
{
	BaseClass::Activate();
	EnableApplyButton(false);
}

void CHL2RMenu::OnThink()
{
	BaseClass::OnThink();
	if (engine->IsPaused() || engine->IsLevelMainMenuBackground() || !engine->IsInGame())
		SetVisible(true);
	else
		SetVisible(false);
}

bool isHL2RActive = false;
void CHL2RMenu::OnClose()
{
	hl2r_OptionsTabs->KillOptionsPanels(hl2r_OptionsTabs);

	BaseClass::OnClose();
	isHL2RActive = false;
}

void CHL2RMenu::OnScreenSizeChanged(int iOldWide, int iOldTall)
{
	BaseClass::OnScreenSizeChanged(iOldWide, iOldTall);
	
	BaseClass::OnClose();
	isHL2RActive = false;
}
//------------------------------------------------------------------------------
// Purpose: Console Command
//------------------------------------------------------------------------------
CHL2RMenu* HL2RMenu = NULL;
CON_COMMAND(OpenHL2ROptionsDialog, "Turns on the HL2R options Panel")
{
	if (!isHL2RActive)
	{
		HL2RMenu = new CHL2RMenu();
		isHL2RActive = true;
	}

	HL2RMenu->Activate();
}
