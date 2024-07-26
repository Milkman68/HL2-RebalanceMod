#include "cbase.h"
using namespace vgui;
#include <vgui/IVGui.h>
#include <vgui_controls/PropertyDialog.h>
#include <vgui_controls/PropertySheet.h>
#include <vgui_controls/ScrollableEditablePanel.h>
#include "ienginevgui.h"
#include "hl2r_options.h"

//------------------------------------------------------------------------------
// Purpose : HL2R's challenge options parent panel
//------------------------------------------------------------------------------
CChallengesHL2RBasePanel::CChallengesHL2RBasePanel(vgui::Panel* parent, vgui::EditablePanel* child) : ScrollableEditablePanel(parent, child, NULL){}

//------------------------------------------------------------------------------
// Purpose : We need this because the main panel sends the apply message to here 
// instead of the panel parented to this one. So we need to send it manually to the child panel.
//------------------------------------------------------------------------------
void CChallengesHL2RBasePanel::OnApplyChanges()
{
	ipanel()->SendMessage(GetChild(0)->GetVPanel(), new KeyValues("ApplyChanges"), GetVPanel());
}
//------------------------------------------------------------------------------
// Purpose : HL2R's challenge options page
//------------------------------------------------------------------------------
CSubOptionsChallengesHL2R::CSubOptionsChallengesHL2R(vgui::Panel* parent) : EditablePanel(parent, NULL)
{
 	CheckButtonRef.InitCheckButtons( this, C_CheckButtons, ARRAYSIZE(C_CheckButtons) );
	SetBounds(0, 0, 422, 775);
	
	toggleAllButton = new Button(this, "toggleAllButton", "");
	toggleAllButton->SetCommand("toggleall");

	LoadControlSettings("resource/ui/hl2r_optionssubchallenge.res");
}

void CSubOptionsChallengesHL2R::OnApplyChanges()
{
	CheckButtonRef.UpdateConVars( this, C_CheckButtons, ARRAYSIZE(C_CheckButtons) );
}

void CSubOptionsChallengesHL2R::OnCheckButtonChecked(Panel* panel)
{
	PostActionSignal(new KeyValues("ApplyButtonEnable"));
}

void CSubOptionsChallengesHL2R::OnCommand(const char* pcCommand)
{
	BaseClass::OnCommand(pcCommand);
	if (!Q_stricmp(pcCommand, "toggleall"))
	{
		bool bState = true;
		
		for ( int i = 0; i < ARRAYSIZE(C_CheckButtons); i++ )
		{
			if ( C_CheckButtons[i].Button->IsSelected() )
			{
				bState = false;
				break;
			}
		}
		
		for ( int i = 0; i < ARRAYSIZE(C_CheckButtons); i++ )
		{
			C_CheckButtons[i].Button->SetSelected(bState);
		}
	}
}
//------------------------------------------------------------------------------
// Purpose : HL2R's Game options page
//------------------------------------------------------------------------------
CSubGameOptionsHL2R::CSubGameOptionsHL2R(vgui::Panel* parent) : PropertyPage(parent, NULL)
{
	BoxButtonRef.InitBoxButtons( this, G_BoxButtons, ARRAYSIZE(G_BoxButtons) );
	TickSliderRef.InitTickSliders( this, G_TickSliders, ARRAYSIZE(G_TickSliders) );
	
	LoadControlSettings("resource/ui/hl2r_optionssubgame.res");
} 

void CSubGameOptionsHL2R::OnApplyChanges()
{
	BoxButtonRef.UpdateConVars( this, G_BoxButtons, ARRAYSIZE(G_BoxButtons) );
	TickSliderRef.UpdateConVars( this, G_TickSliders, ARRAYSIZE(G_TickSliders) );
}

void CSubGameOptionsHL2R::OnControlModified()
{
	PostActionSignal(new KeyValues("ApplyButtonEnable"));
}
//------------------------------------------------------------------------------
// Purpose : HL2R's Visuals options page
//------------------------------------------------------------------------------
CSubVisualOptionsHL2R::CSubVisualOptionsHL2R(vgui::Panel* parent) : PropertyPage(parent, NULL)
{
	BoxButtonRef.InitBoxButtons( this, V_BoxButtons, ARRAYSIZE(V_BoxButtons) );
	
	LoadControlSettings("resource/ui/hl2r_optionssubvisuals.res");
} 

void CSubVisualOptionsHL2R::OnApplyChanges()
{
	BoxButtonRef.UpdateConVars( this, V_BoxButtons, ARRAYSIZE(V_BoxButtons) );
}

void CSubVisualOptionsHL2R::OnControlModified()
{
	PostActionSignal(new KeyValues("ApplyButtonEnable"));
}
//------------------------------------------------------------------------------
// Purpose : HL2R's Options panel
//------------------------------------------------------------------------------
CHL2RMenu::CHL2RMenu(vgui::VPANEL parent) : BaseClass(NULL, "HL2RMenu")
{
	SetDeleteSelfOnClose(true);
	SetBounds(0, 0, 422, 590);
	SetSizeable( false );
	MoveToCenterOfScreen();

	SetTitle("#hl2r_options_title", true);
	
	m_pSubOptionsChallengesHL2R = new CSubOptionsChallengesHL2R(this);
	
	// Parent the challenge page to a separate scrollable-panel.
	m_pChallengesHL2RBasePanel = new CChallengesHL2RBasePanel(this, m_pSubOptionsChallengesHL2R);
	AddPage(m_pChallengesHL2RBasePanel, "#hl2r_options_game");
	
	m_pSubVisualOptionsHL2R = new CSubVisualOptionsHL2R(this);
	AddPage(m_pSubVisualOptionsHL2R, "#hl2r_options_hud");

 	m_pSubGameOptionsHL2R = new CSubGameOptionsHL2R(this);
	AddPage(m_pSubGameOptionsHL2R, "#hl2r_options_misc");
	
	SetApplyButtonVisible(true);
	GetPropertySheet()->SetTabWidth(168);
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
	BaseClass::OnClose();
	isHL2RActive = false;
}

CHL2RMenu* HL2RMenu = NULL;

CON_COMMAND(EnableHL2RPanel, "Turns on the HL2R options Panel")
{
	VPANEL gameToolParent = enginevgui->GetPanel(PANEL_CLIENTDLL_TOOLS);
	if (!isHL2RActive)
	{
		HL2RMenu = new CHL2RMenu(gameToolParent);
		isHL2RActive = true;
	}

	HL2RMenu->Activate();
}
